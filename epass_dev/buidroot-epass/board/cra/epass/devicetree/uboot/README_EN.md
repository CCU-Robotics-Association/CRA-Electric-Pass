# CRA Electric Pass U-Boot Device Tree

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the board-level device tree used by CRA Electric Pass during the SPL and U-Boot stages. The current project targets the Shirogane v0.6 board revision only.

## Files

| File | Purpose |
| --- | --- |
| `suniv-f1c100s-generic.dts` | Board-level U-Boot device tree entry point currently used by CRA Electric Pass |

This file contains only board-specific selections and peripheral enable states. Register addresses, clocks, resets, interrupts, and basic controller nodes for the F1C100S/F1C200S are provided by the shared file:

```text
board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
```

## Difference from the Linux Device Tree

The boot process uses two separate device trees:

```text
Power on
 │
 ▼
SPL / U-Boot
 │
 └─ Uses devicetree/uboot/suniv-f1c100s-generic.dts
 │
 ▼
U-Boot reads boot.itb
 │
 ├─ Extracts the Linux base device tree
 ├─ Applies the screen overlay
 ├─ Applies the interface overlay
 └─ Applies the ext overlay
 │
 ▼
Linux
    └─ Uses the combined device tree from devicetree/linux/
```

The two device trees serve different purposes:

| U-Boot device tree | Linux device tree |
| --- | --- |
| Used by SPL and U-Boot drivers | Used by Linux kernel drivers |
| Describes only the hardware accessed before Linux starts | Describes the complete system hardware |
| Covers boot resources such as SPI-NAND, DFU, and UART | Covers the LCD, backlight, keys, and application peripherals |
| Compiled into U-Boot itself | Packaged into `boot.itb` as DTB and DTBO files |

The current U-Boot configuration explicitly disables:

```text
# CONFIG_VIDEO_SUNXI is not set
```

## Device Tree Composition

The Buildroot configuration supplies two custom device tree source files to U-Boot through:

```text
BR2_TARGET_UBOOT_CUSTOM_DTS_PATH=
    board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
    board/cra/epass/devicetree/uboot/suniv-f1c100s-generic.dts
```

Before U-Boot is built, Buildroot copies them to:

```text
output/build/uboot-2020.07/arch/arm/dts/
```

The board-level DTS then includes the shared SUNIV SoC definitions with:

```dts
#include "suniv-f1c100s.dtsi"
```

The resulting relationship is:

```text
Shared SUNIV SoC description
board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
        │
        ▼
CRA Electric Pass board-level selection
board/cra/epass/devicetree/uboot/suniv-f1c100s-generic.dts
        │
        ▼
suniv-f1c100s-generic.dtb
        │
        ▼
Compiled into U-Boot
```

`uboot.defconfig` selects this DTS as the default device tree with:

```text
CONFIG_DEFAULT_DEVICE_TREE="suniv-f1c100s-generic"
```

## Shared SUNIV Device Tree

The shared `suniv-f1c100s.dtsi` defines internal SoC resources, including:

| Node | Address or parameter | Purpose |
| --- | --- | --- |
| `osc24M` | 24 MHz | Main oscillator |
| `osc32k` | 32768 Hz | Low-speed clock |
| `cpu` | ARM926EJ-S | CPU type |
| `sram-controller` | `0x01c00000` | SRAM controller |
| `spi0` | `0x01c05000` | SPI0 controller |
| `ccu` | `0x01c20000` | Clock and reset controller |
| `intc` | `0x01c20400` | Interrupt controller |
| `pio` | `0x01c20800` | GPIO and pin multiplexing |
| `timer` | `0x01c20c00` | Timer |
| `watchdog` | `0x01c20ca0` | Watchdog |
| `uart0` | `0x01c25000` | UART0 |
| `uart1` | `0x01c25400` | UART1 |
| `uart2` | `0x01c25800` | UART2 |
| `usb_otg` | `0x01c13000` | USB OTG controller |
| `usbphy` | `0x01c13400` | USB PHY |
| `mmc0` | `0x01c0f000` | First MMC controller |
| `mmc2` | `0x01c10000` | Second MMC controller resource |

Most peripherals in the shared file default to:

```dts
status = "disabled";
```

The board-level DTS enables only the nodes required during the boot process.

The copyright notice at the beginning of the shared file is:

```text
Copyright 2018 Icenowy Zheng
```

The file was first imported into this repository by Aodzip in 2021. Its SoC register definitions were not rewritten during the CRA secondary-development work.

## Aliases and Console

### `aliases`

```dts
aliases {
    serial0 = &uart0;
    spi0 = &spi0;
};
```

| Alias | Target | Purpose |
| --- | --- | --- |
| `serial0` | `uart0` | Assigns UART0 as logical serial port 0 |
| `spi0` | `spi0` | Provides a stable alias for SPI0 |

Aliases do not add new hardware; they provide stable names for existing nodes.

### `chosen`

```dts
chosen {
    stdout-path = "serial0:115200n8";
};
```

The U-Boot console settings are:

| Parameter | Value |
| --- | --- |
| Port | UART0 |
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |

UART0 uses:

```text
PE0: UART0 TX
PE1: UART0 RX
```

The project patch `0001-uart-pull.patch` enables pull-ups on both PE0 and PE1.

## USB SRAM

```dts
&otg_sram {
    status = "okay";
};
```

The USB OTG controller requires the internal SRAM D region. The shared device tree disables this SRAM partition by default, so the board-level DTS enables it together with USB OTG.

If `usb_otg` is enabled without `otg_sram`, the MUSB driver may be unable to acquire the required SRAM.

## SPI0 and Boot Flash

### SPI0 Controller

```dts
&spi0 {
    pinctrl-names = "default";
    pinctrl-0 = <&spi0_pins_a>;
    status = "okay";
};
```

SPI0 uses:

```text
PC0
PC1
PC2
PC3
```

The exact signal assignments are determined by the SUNIV SPI0 pin multiplexing definition.

The current SPI-NAND node requests a maximum transfer frequency of:

```dts
spi-max-frequency = <80000000>;
```

`80 MHz` is an upper limit, not a guarantee that the hardware always operates at 80 MHz. `0004-f1c-spi-fix.patch` selects a divider based on the 200 MHz SUNIV SPI parent clock so that the resulting frequency does not exceed the requested limit.

### SPI-NAND Node

```dts
spi-nand@0 {
    reg = <0>;
    compatible = "spi-nand";
    spi-max-frequency = <80000000>;
};
```

This node describes the SPI-NAND device. The current Electric Pass hardware boots from SPI-NAND.

Project patches add support for:

| Chip ID | Model |
| --- | --- |
| `c2 12` | Macronix MX35LF1GE4AB |
| `c2 14` | Macronix MX35LF1G24AD |

The base SUNIV U-Boot patch also includes support for several Winbond and GigaDevice SPI-NAND devices.

The `@0` in the node name matches `reg = <0>`; both identify SPI0 chip select 0.

## Serial Ports

### UART0

```dts
&uart0 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart0_pins_a>;
    status = "okay";
};
```

UART0 uses PE0 and PE1 and serves as the main U-Boot console.

### UART1

```dts
&uart1 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart1_pins_a>;
    status = "okay";
};
```

UART1 uses:

```text
PA2
PA3
```

Pin usage in U-Boot and Linux is independent. After Linux starts, PA2 and PA3 are configured again according to the Linux device tree.

## USB OTG and DFU

```dts
&usb_otg {
    status = "okay";
};

&usbphy {
    status = "okay";
};
```

Together, these nodes enable:

- The SUNIV USB OTG controller.
- The USB PHY.
- U-Boot USB Gadget support.
- DFU download support.

`uboot.defconfig` enables:

```text
CONFIG_CMD_DFU=y
CONFIG_DFU_MTD=y
CONFIG_USB_MUSB_GADGET=y
CONFIG_USB_GADGET_DOWNLOAD=y
```

`0002-musb-force-fs.patch` clears the MUSB High-Speed Enable bit, forcing U-Boot USB to operate in Full-Speed mode.

`0005-dfu-verify-block.patch` adds the following behavior to MTD DFU writes:

- Splits writes by erase block.
- Reads data back after each write and verifies it.
- Marks a block as bad if writing or verification fails.
- Skips bad blocks and retries on the next usable block.
- Returns `-ENOSPC` when no usable space remains.

The current U-Boot DFU implementation therefore does more than transfer data: it also handles SPI-NAND bad blocks and verifies written data.

## MMC Controllers

### `mmc0`

```dts
&mmc0 {
    status = "okay";
};
```

It uses:

```text
PF0-PF5
```

This is the first MMC/SD controller. The shared device tree enables pull-ups and uses `broken-cd`, meaning that the controller does not rely on a standard card-detect signal.

### `mmc2`

```dts
&mmc2 {
    status = "disabled";
};
```

Despite its label, `mmc2` actually uses:

```text
Registers: 0x01c10000
Clock: MMC1
Reset: MMC1
Pins: PC0, PC1, PC2
Function: mmc1
```

The label is inherited from the shared device tree and does not mean that the chip contains a separate controller numbered 2.

The current board-level DTS explicitly keeps this node disabled, and `uboot.defconfig` no longer sets `CONFIG_MMC_SUNXI_SLOT_EXTRA=1`.

### Pin Conflict with SPI0

`mmc2` uses:

```text
PC0, PC1, PC2
```

SPI0 uses:

```text
PC0, PC1, PC2, PC3
```

The two controllers therefore cannot use their current pin groups at the same time. The current code disables `mmc2` and removes the extra MMC slot configuration so that PC0-PC3 remain dedicated to the SPI0 boot flash.

If a second MMC controller is required in the future, its selection logic must be redesigned around the PCB routing and boot medium. Merely changing `status` back to `"okay"` is not sufficient.

## U-Boot Configuration

The project uses:

```text
U-Boot 2020.07
```

The Buildroot configuration is stored in:

```text
board/cra/epass/cra_epass_defconfig
```

The U-Boot Kconfig configuration is stored in:

```text
board/cra/epass/uboot.defconfig
```

Key settings are:

| Setting | Current value | Purpose |
| --- | --- | --- |
| `CONFIG_ARCH_SUNXI` | `y` | Allwinner SUNXI platform |
| `CONFIG_MACH_SUNIV` | `y` | F1C100S/F1C200S SUNIV platform |
| `CONFIG_SYS_TEXT_BASE` | `0x81700000` | Main U-Boot runtime address |
| `CONFIG_SPL` | `y` | Enables SPL |
| `CONFIG_DRAM_CLK` | `204` | DRAM clock setting |
| `CONFIG_SYS_CLK_FREQ` | `604000000` | Target CPU/system clock |
| `CONFIG_SPL_SPI_SUNXI` | `y` | Boots SPL from SUNXI SPI |
| `CONFIG_BOOTDELAY` | `0` | Disables the normal countdown |
| `CONFIG_BOOTCOMMAND` | `run distro_bootcmd;` | Default boot command |
| `CONFIG_MTD_SPI_NAND` | `y` | Enables SPI-NAND support |
| `CONFIG_CMD_DFU` | `y` | Enables the DFU command |
| `CONFIG_DFU_MTD` | `y` | Enables MTD access through DFU |
| `CONFIG_NET` | Disabled | Leaves the U-Boot network stack disabled |

## U-Boot Patches

Buildroot applies two patch sets in order:

```text
board/allwinner/suniv-f1c100s/patch/u-boot
board/cra/epass/patch/uboot
```

### Base SUNIV Patch

```text
board/allwinner/suniv-f1c100s/patch/u-boot/0001-v2020.07.11.patch
```

This patch adds or extends the following support in U-Boot 2020.07:

- ARM926EJ-S SUNXI startup code.
- The SUNIV SPL linker script.
- F1C100S/F1C200S clock support.
- SUNIV DRAM initialization.
- GPIO, UART, MMC, SPI, and USB PHY support.
- SPI-NAND boot support in SPL.
- SUNIV CCU clock and reset bindings.
- Support for selected GigaDevice SPI-NAND devices.
- The SUNIV memory layout.

### CRA Board-Level Patches

| Patch | Purpose |
| --- | --- |
| `0001-uart-pull.patch` | Enables pull-ups on UART0 pins PE0 and PE1 |
| `0002-musb-force-fs.patch` | Disables MUSB High-Speed mode and forces U-Boot USB to Full-Speed |
| `0003-spi-nand-mx35lf1g.patch` | Adds IDs for two Macronix SPI-NAND devices |
| `0004-f1c-spi-fix.patch` | Corrects SUNIV SPI parent-clock, divider, and maximum-frequency handling |
| `0005-dfu-verify-block.patch` | Adds bad-block skipping and post-write verification to MTD DFU |

## SPL and U-Boot Images

Booting takes place in two stages:

```text
Allwinner BROM
        │
        ▼
SPL
├─ Initializes clocks and DRAM
├─ Detects the boot medium
└─ Reads the main U-Boot image from SPI-NAND
        │
        ▼
Main U-Boot
├─ Initializes the driver model
├─ Reads the boot environment
├─ Reads boot.itb
├─ Assembles the Linux device tree
└─ Starts Linux
```

The base SUNIV patch places the main U-Boot image at the following SPI-NAND offset:

```text
CONFIG_SYS_SPI_NAND_U_BOOT_OFFS = 0xD000
```

This is 52 KiB from the beginning of the SPI-NAND device.

Buildroot first produces:

```text
output/images/u-boot-sunxi-with-spl.bin
```

`mknanduboot.sh` then rearranges the SPL for the 2 KiB NAND page layout, places the main U-Boot image at `0xD000`, and produces:

```text
output/images/u-boot-sunxi-with-nand-spl.bin
```

The physical device must be flashed with the final file carrying the `nand-spl` suffix, not the unprocessed `with-spl` image.

## Default Boot Environment

The default U-Boot environment is stored in:

```text
board/cra/epass/uboot.env
```

This file is used because `uboot.defconfig` enables:

```text
CONFIG_USE_DEFAULT_ENV_FILE=y
CONFIG_DEFAULT_ENV_FILE="../../../board/cra/epass/uboot.env"
```

### Memory Addresses

| Variable | Address | Purpose |
| --- | ---: | --- |
| `kernaddr` | `0x80008000` | Linux zImage |
| `dtbaddr` | `0x80C00000` | Linux base DTB |
| `dtboaddr` | `0x80D00000` | Temporary DTBO |
| `envtxtaddr` | `0x80E00000` | Text boot environment |
| `fitaddr` | `0x81000000` | `boot.itb` |

`checkfit` limits the total FIT size to:

```text
0x500000
```

This is 5 MiB. The limit prevents FIT data from growing upward into the main U-Boot image at `0x81700000`.

### SPI-NAND Layout

The current boot logic uses:

| Region | Offset | Size |
| --- | ---: | ---: |
| U-Boot partition | `0x000000` | 1 MiB |
| Text boot environment | `0x0FA000` | 24 KiB |
| Boot partition | `0x100000` | 6 MiB |
| Rootfs partition | `0x700000` | Remaining space |

The text boot environment is located at the end of the U-Boot partition:

```text
0xFA000 + 0x6000 = 0x100000
```

It ends exactly where the Boot partition begins.

### Linux Boot Flow

The default `distro_bootcmd` performs:

```text
Read the text environment from 0xFA000
        │
        ▼
Read and validate boot.itb from 0x100000
        │
        ▼
Extract the kernel
        │
        ▼
Extract the Linux base DTB
        │
        ▼
Extract and apply fdt-screen-${screen}
        │
        ▼
Apply each fdt-iface-${interface}
        │
        ▼
Apply each fdt-ext-${ext}
        │
        ▼
Append the default bootargs
        │
        ▼
bootz
```

If the FIT header is invalid, the FIT size is zero, or the loaded image fails the FDT check, the boot environment enters `rundfu`.

## Building

For the initial configuration, run:

```sh
make cra_epass_defconfig
```

The project provides:

```sh
./rebuild-uboot.sh
```

All `.sh` files executed by Linux or Buildroot must use LF line endings. If a Windows checkout converts them to CRLF, Bash may report `$'\r': command not found`, invalid `set` arguments, or loop syntax errors.

The script runs:

```sh
make uboot-clean-for-rebuild
make uboot -j8
make -j8
```

The final `make` invokes the image post-processing scripts, producing the final SPI-NAND U-Boot image and the other system images.

After a complete build, check:

```text
output/images/u-boot-sunxi-with-spl.bin
output/images/u-boot-sunxi-with-nand-spl.bin
output/images/boot.itb
output/images/rootfs_ubi.img
```

The presence of `u-boot-sunxi-with-spl.bin` alone does not mean that the NAND boot image is complete. Confirm that `mknanduboot.sh` finished successfully.

## Troubleshooting

| Symptom | Check first |
| --- | --- |
| No U-Boot serial output | UART0 on PE0/PE1, 115200n8, the UART pull-up patch, and power |
| SPL starts but cannot find main U-Boot | SPI-NAND model, chip ID, the `0xD000` layout, and `mknanduboot.sh` output |
| U-Boot cannot find the SPI-NAND device | SPI0 pins, the `spi-nand` node, the SPI-NAND driver, and the chip-ID patch |
| DTC reports an SPI-NAND unit-address mismatch | Confirm that the node name and `reg` are both 0: `spi-nand@0` and `reg = <0>` |
| SPI boot fails after re-enabling the second MMC controller | `mmc2` and SPI0 share PC0-PC2; the current board configuration must keep `mmc2` disabled |
| USB DFU does not enumerate | `usb_otg`, `usbphy`, `otg_sram`, the Full-Speed patch, and the host driver |
| DFU skips blocks while writing | Check for write failures, verification failures, or existing NAND bad blocks |
| The device repeatedly enters DFU after an invalid `boot.itb` | FIT header, total size, Boot partition contents, and the `checkfit` limit |
| U-Boot DTS changes do not take effect | Clean and rebuild U-Boot, and confirm that Buildroot recopies the custom DTS |
| Linux DTS changes do not affect U-Boot | The two device trees are independent; edit the U-Boot DTS separately |
| U-Boot does not display a boot image | `CONFIG_VIDEO_SUNXI` is disabled; Linux takes over the display later |

## Secondary-Development Principles

Before modifying this directory or the related U-Boot configuration, confirm the boot medium, PCB pin routing, and available recovery method:

- Never test an unknown SPL or U-Boot image without a working FEL/XFEL recovery path.
- Never use the ordinary `u-boot-sunxi-with-spl.bin` directly as the final image for the current SPI-NAND layout.
- Never allow SPI0 and the second MMC controller to claim PC0-PC2 at the same time.
- When changing only the displayed branding, edit `model` first and retain the Allwinner compatible strings.
- When changing a serial-port node, also check `stdout-path`, console aliases, pin multiplexing, and the UART patch.
- When changing the SPI frequency, verify the flash specifications, PCB signal integrity, SPL reads, and the main U-Boot SPI driver.
- When changing the SPI-NAND model, confirm that both SPL and main U-Boot can identify the device.
- When changing the `0xD000` main U-Boot offset, update both the SUNIV SPL configuration and `mknanduboot.sh`.
- When changing SPI-NAND partitions, review the U-Boot default environment, Linux `bootargs`, Linux device tree, UBI configuration, image scripts, and flashing tools together.
- When changing the `0xFA000` text-environment offset or its `0x6000` length, ensure that it cannot overwrite SPL, main U-Boot, or the Boot partition.
- If SPI-NOR support is restored, create a separate hardware configuration; SPI-NOR and SPI-NAND cannot share chip select 0 in the same configuration.
- Before re-enabling `mmc2`, resolve its physical conflict with SPI0 on PC0-PC2.
- Before changing the USB mode, verify U-Boot DFU, Linux USB Gadget behavior, and USB signal integrity on the physical device.
- After changing a U-Boot patch, perform a complete clean build rather than relying only on an existing `output/build/uboot-*` directory.
- Before testing on hardware, retain a verified U-Boot image, a working serial connection, and an XFEL recovery method.
