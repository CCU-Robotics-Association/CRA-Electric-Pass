# CRA Electric Pass Linux Device Tree

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the base device tree and device tree overlays used by CRA Electric Pass during the Linux stage. The current project targets the Shirogane v0.6 board revision only.

These files describe the mainboard hardware, display, SoC interfaces, and external devices available after Linux starts. The device tree used by SPL and U-Boot themselves is located at:

```text
board/cra/epass/devicetree/uboot/
```

The two device trees are independent. Changes made in this directory do not automatically alter the hardware configuration used during SPL or U-Boot.

## Directory Structure

```text
devicetree/linux/
├─ base/
│  ├─ epass.dtsi
│  └─ devicetree.dts
├─ screen/
│  ├─ boe.dts
│  ├─ hsd.dts
│  └─ laowu.dts
├─ interface/
│  ├─ adc_pa1.dts
│  ├─ adc_pa123.dts
│  ├─ i2c0.dts
│  ├─ i2s0_pa.dts
│  ├─ i2s0_pe.dts
│  ├─ spi1.dts
│  ├─ uart1.dts
│  ├─ uart2.dts
│  ├─ usbhost.dts
│  └─ usbhs.dts
└─ ext/
   ├─ cardkb.dts
   ├─ es8311_sound.dts
   └─ lsm6ds3_pre0.4.dts
```

The four subdirectories have the following responsibilities:

| Directory | Output type | Responsibility | Details |
| --- | --- | --- | --- |
| `base/` | DTB | Describes hardware that is always present on the mainboard and provides nodes and labels referenced by overlays | [base/README.md](base/README.md) |
| `screen/` | DTBO | Selects the ST7701 initialization sequence and display-specific behavior for the installed LCD | [screen/README.md](screen/README.md) |
| `interface/` | DTBO | Enables SoC controllers, selects pin multiplexing, or changes the USB operating mode | [interface/README.md](interface/README.md) |
| `ext/` | DTBO | Declares specific external devices connected through the corresponding interfaces | [ext/README.md](ext/README.md) |

## Device Tree Composition

The final device tree used by Linux is not produced by compiling a single `.dts` file. U-Boot assembles it dynamically from a base DTB and one or more DTBO files:

```text
Allwinner SUNIV SoC definitions
board/allwinner/suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi
        │
        ▼
Shared CRA mainboard definition
base/epass.dtsi
        │
        ▼
Shirogane v0.6 base entry point
base/devicetree.dts
        │
        ▼
screen overlay
        │
        ▼
interface overlays (zero or more)
        │
        ▼
ext overlays (zero or more)
        │
        ▼
U-Boot passes the assembled DTB to Linux
```

The application order is fixed:

```text
base → screen → interface → ext
```

If several overlays modify the same property, a later overlay may replace a value set earlier. If different overlays enable controllers that use the same pins, the device tree may still compile successfully, but the Linux drivers can encounter resource conflicts at runtime.

## Base Device Tree

The Buildroot configuration provides the shared SoC definition, the shared CRA mainboard definition, and the current board entry point to the Linux build system through:

```text
BR2_LINUX_KERNEL_DTS_SUPPORT=y
BR2_LINUX_KERNEL_CUSTOM_DTS_PATH="
    board/allwinner/suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi
    board/cra/epass/devicetree/linux/base/epass.dtsi
    board/cra/epass/devicetree/linux/base/devicetree.dts"
```

The base files are included in this order:

```text
suniv-f1c100s.dtsi
        ↓
epass.dtsi
        ↓
devicetree.dts
```

Their roles are:

- `epass.dtsi` describes the device identity, display pipeline, power supplies, backlight, GPIO multiplexing, SPI-NAND, serial ports, SD interface, USB, video engine, and default peripheral states.
- `devicetree.dts` is the only base entry point currently used for Shirogane v0.6. It adds the power-off GPIO, ST7701 initialization pins, and LRADC key parameters.

The generated file is:

```text
output/images/dt/base/devicetree.dtb
```

## Screen Overlays

One screen overlay matching the physical LCD must be selected at boot:

| Boot value | Source file | Main difference |
| --- | --- | --- |
| `screen=boe` | `screen/boe.dts` | ST7701 initialization sequence for the BOE display |
| `screen=hsd` | `screen/hsd.dts` | ST7701 initialization sequence for the HSD display |
| `screen=laowu` | `screen/laowu.dts` | Uses the HSD initialization sequence and swaps the TCON0 red and blue channels |

The generated files are:

```text
output/images/dt/screen/boe.dtbo
output/images/dt/screen/hsd.dtbo
output/images/dt/screen/laowu.dtbo
```

If `screen` is empty or does not match an available name, U-Boot cannot extract the correct `fdt-screen-*` node from the FIT image. Display initialization will be unavailable, and the boot process may also fail.

## Interface Overlays

`interface/` enables internal SoC controllers and selects pin layouts. It does not describe specific devices connected to those buses.

The available names are:

| Boot value | Purpose |
| --- | --- |
| `adc_pa1` | Adds the PA1 ADC input to the default PA0 input |
| `adc_pa123` | Enables all four ADC inputs on PA0-PA3 |
| `i2c0` | Enables the hardware I²C0 controller |
| `i2s0_pa` | Enables I²S0 with the PA pin layout |
| `i2s0_pe` | Enables I²S0 with the PE pin layout |
| `spi1` | Enables SPI1 and the predefined Spidev child device |
| `uart1` | Enables UART1 |
| `uart2` | Enables UART2 |
| `usbhost` | Forces the USB OTG controller into host mode |
| `usbhs` | Requests the project-specific USB High-Speed mode |

Several names can be separated by spaces in the boot environment:

```text
interface=i2c0 uart1
```

U-Boot applies them in the order in which they are written. Every interface name must exactly match both the source filename and the corresponding FIT-node suffix.

## External-Device Overlays

`ext/` describes specific devices connected to the mainboard interfaces. An external-device overlay usually depends on an interface overlay that enables the required controller first.

| Boot value | External device | Main dependency |
| --- | --- | --- |
| `cardkb` | M5Stack Unit CardKB | `interface=i2c0` |
| `es8311_sound` | Everest ES8311 audio codec | Select an appropriate I²S0 interface layout |
| `lsm6ds3_pre0.4` | ST LSM6DS3 six-axis inertial sensor | `interface=i2c0`, with the PE2 conflict resolved |

For example:

```text
interface=i2c0
ext=cardkb
```

Several external-device names can also be separated by spaces:

```text
ext=cardkb lsm6ds3_pre0.4
```

Before enabling an external device, verify the power supply, logic levels, bus address, physical wiring, and pin multiplexing. A device tree that compiles successfully is not necessarily suitable for the connected hardware.

## Compilation

The device trees are generated by the Buildroot image post-processing script:

```text
board/cra/epass/scripts/mkdt.sh
```

The script iterates over the `.dts` files in `base/`, `interface/`, `ext/`, and `screen/`.

Each source file is first expanded by the C preprocessor:

```sh
cpp -nostdinc \
    -I "${BUILD_DIR}/linux-5.4.99/include/" \
    -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
    -P -undef -x assembler-with-cpp
```

It is then compiled into a DTB or DTBO with:

```sh
dtc -@ -I dts -O dtb
```

`-@` preserves the symbols and fixup information required by overlays, allowing each DTBO to reference labels such as `&i2c0`, `&i2s0`, `&pio`, `&uart1`, and `&usb_otg` from the base device tree.

The generated directory structure is:

```text
output/images/dt/
├─ base/
│  └─ devicetree.dtb
├─ screen/
│  ├─ boe.dtbo
│  ├─ hsd.dtbo
│  └─ laowu.dtbo
├─ interface/
│  └─ *.dtbo
└─ ext/
   └─ *.dtbo
```

`mkdt.sh` currently hard-codes the Linux source directory as `linux-5.4.99`. If the kernel is upgraded, both header search paths in the script must be updated accordingly.

## Packaging into `boot.itb`

After the device trees have been generated:

```text
board/cra/epass/scripts/kernel.its
```

packages the Linux kernel, the base DTB, and every DTBO into the FIT image:

```text
output/images/boot.itb
```

The FIT nodes follow these naming rules:

| File type | FIT node |
| --- | --- |
| Linux kernel | `kernel` |
| Base device tree | `fdt-base` |
| Screen overlay | `fdt-screen-<name>` |
| Interface overlay | `fdt-iface-<name>` |
| External-device overlay | `fdt-ext-<name>` |

For example:

```text
screen/boe.dtbo
        ↓
fdt-screen-boe

interface/i2c0.dtbo
        ↓
fdt-iface-i2c0

ext/cardkb.dtbo
        ↓
fdt-ext-cardkb
```

`mkdt.sh` automatically compiles every `.dts` file in these directories, but `kernel.its` does not discover new files automatically. Whenever an overlay is added, removed, or renamed, `kernel.its` must be updated as well. Otherwise, the generated file will not be included in `boot.itb`.

## Assembly During U-Boot

The default U-Boot environment is stored in:

```text
board/cra/epass/uboot.env
```

At boot, U-Boot first reads the text environment from offset `0xFA000` in SPI-NAND, then extracts the base device tree from `boot.itb`:

```text
imxtract $fitaddr fdt-base $dtbaddr
```

It then extracts and applies the selected screen overlay:

```text
imxtract $fitaddr fdt-screen-${screen} $dtboaddr
fdt apply $dtboaddr
```

The interface and external-device overlays are processed sequentially in loops:

```text
for ov in ${interface}
    imxtract $fitaddr fdt-iface-${ov} $dtboaddr
    fdt apply $dtboaddr

for ov in ${ext}
    imxtract $fitaddr fdt-ext-${ov} $dtboaddr
    fdt apply $dtboaddr
```

The complete sequence is:

```text
Read fdt-base
        ↓
Apply fdt-screen-${screen}
        ↓
Apply each fdt-iface-${interface}
        ↓
Apply each fdt-ext-${ext}
        ↓
Append bootargs
        ↓
bootz
```

## Boot Environment

The template stored in the repository is:

```text
board/cra/epass/uEnv.txt
```

It currently contains:

```text
interface=
ext=
```

Optional interface and external-device overlays are therefore disabled by default.

The template itself does not define `screen=`. The existing `flash.py` generates `.bootenv.txt` at runtime and writes the screen type from the first argument passed to `flash()` or `flash2()`:

```text
screen=hsd
```

When creating or editing the boot environment manually, a valid `screen` value must be supplied. A complete example is:

```text
screen=hsd
interface=i2c0
ext=cardkb
```

Every name in the boot environment must exactly match the suffix of the corresponding FIT node in `kernel.its`, including letter case, underscores, and periods.

## Common Dependencies and Conflicts

The following table is only an overview. Refer to the README in each subdirectory for exact pins and restrictions:

| Combination | Notes |
| --- | --- |
| `cardkb` + `i2c0` | CardKB requires the hardware I²C0 controller |
| `lsm6ds3_pre0.4` + `i2c0` | LSM6DS3 requires the hardware I²C0 controller |
| `es8311_sound` + `i2s0_pa` or `i2s0_pe` | ES8311 audio data requires I²S0 |
| `i2s0_pa` and `i2s0_pe` | Only one I²S0 pin layout should be selected |
| `adc_pa1` and `adc_pa123` | Only one of the two ADC pin ranges should normally be selected |
| `usbhost` and `usbhs` | Both modify the same USB controller and must not be enabled together without verification |
| `lsm6ds3_pre0.4` and power-off control | Both involve PE2 and must be checked against the board revision |
| ADC, UART1, and the I²S0 PA layout | Some configurations reuse PA1-PA3 and must be checked individually |
| UART2 and SPI1 | Some pins overlap on PE7 and PE8 and must be checked before combining them |

U-Boot does not detect these hardware conflicts automatically. Successfully applying an overlay only proves that the device tree structures can be merged; it does not prove that the PCB routing, electrical connections, and Linux drivers can operate together.

## Adding a New Device Tree Configuration

Use the following sequence when adding a configuration:

1. Decide whether it belongs in `base`, `screen`, `interface`, or `ext`.
2. Create the `.dts` file in the appropriate directory and reference labels already defined by the base device tree. If a required label does not exist, first add a stable definition under `base/`.
3. Check pin multiplexing, power, logic levels, bus addresses, interrupts, and conflicts with other overlays.
4. Run the build and confirm that `mkdt.sh` generates the corresponding DTB or DTBO.
5. Add a FIT node to `kernel.its` that follows the established naming rules.
6. Use a name in the boot environment that exactly matches the FIT-node suffix.
7. Confirm that `boot.itb` actually contains the new node.
8. Retain a recoverable image and serial logs before testing on the physical device.

Do not edit:

```text
output/images/dt/
```

This directory contains generated build output. It is deleted and recreated the next time `mkdt.sh` runs. All persistent changes must be made in the `.dts` or `.dtsi` sources in this directory, or in the corresponding build scripts.

## Building and Inspection

For a complete build, run:

```sh
make cra_epass_defconfig
make
```

If only the related images need to be regenerated, the project's rebuild workflow may be used, but confirm that Linux, the device trees, and the FIT image are not being taken from stale build output.

After building, inspect:

```text
output/images/dt/base/devicetree.dtb
output/images/dt/screen/*.dtbo
output/images/dt/interface/*.dtbo
output/images/dt/ext/*.dtbo
output/images/boot.itb
```

List the nodes in the FIT image with:

```sh
output/host/bin/mkimage -l output/images/boot.itb
```

The generated base device tree can be decompiled for inspection:

```sh
dtc -I dtb -O dts \
    -o devicetree.decoded.dts \
    output/images/dt/base/devicetree.dtb
```

Verify the following:

- The base DTB contains the symbols required by the overlays.
- Every file referenced by `kernel.its` exists.
- The FIT node selected by `screen` exists.
- Every name in `interface` and `ext` maps to the correct node.
- Controllers that use the same physical pins have not been enabled accidentally.
- The total size of `boot.itb` does not exceed the 5 MiB limit enforced by U-Boot `checkfit`.

## Secondary-Development Principles

- Before changing a base-node label, search for every overlay that references it.
- When changing a `compatible` string, also inspect the corresponding Linux driver or kernel patch.
- Whenever a `.dts` file is added, update `kernel.its` as well.
- When renaming a file, update the FIT node, boot environment, and documentation together.
- Do not place a specific external device directly under `interface/`; keep interfaces and devices in separate layers.
- Do not resolve one overlay conflict by silently overriding an unrelated property in another overlay.
- Do not assume that a hardware combination works merely because DTC compiled it successfully.
- All scripts executed by Linux or Buildroot must use LF line endings.
- After making changes, complete the build, inspect the FIT nodes, and decompile the resulting device tree before testing on physical hardware.
