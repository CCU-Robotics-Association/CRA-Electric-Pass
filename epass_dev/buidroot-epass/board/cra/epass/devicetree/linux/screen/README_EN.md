# CRA Electric Pass Display Device Tree Overlays

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the Linux LCD device tree overlays for CRA Electric Pass. They select the ST7701 initialization sequence that matches the physical display during boot. The current project targets the Shirogane v0.6 hardware revision only.

## Files

| File | Initialization sequence | Additional handling | Boot setting |
| --- | --- | --- | --- |
| `boe.dts` | ST7701 initialization sequence for the BOE display | None | `screen=boe` |
| `hsd.dts` | ST7701 initialization sequence for the HSD display | None | `screen=hsd` |
| `laowu.dts` | Same ST7701 initialization sequence as `hsd.dts` | Swaps the red and blue channels in TCON0 | `screen=laowu` |

## Role in the Display Pipeline

The complete display pipeline is:

```text
Linux DRM
   │
   ▼
Allwinner DE
   │
   ▼
TCON0
   │
   ▼
RGB565 parallel bus
   │
   ▼
ST7701 LCD panel
```

The related configuration is distributed across several locations:

```text
base/epass.dtsi
├─ Declares the panel node
├─ Declares the PWM backlight
├─ Declares the RGB565 pin group
├─ Enables TCON0
└─ Declares the st7701initseq node, disabled by default

base/devicetree.dts
└─ Assigns the SDA, SCL, and CS GPIOs to st7701initseq

screen/*.dts
├─ Enables st7701initseq
├─ Supplies the initialization sequence for the selected display
├─ Specifies the shared panel compatible string
└─ Enables red/blue channel swapping when required

Kernel patches
├─ 0002-panel-simple.patch: shared resolution, timings, and RGB565 format
├─ 0004-swap_rb_as_config.patch: red/blue channel swapping in TCON0
└─ 0006-initalize-st7701.patch: GPIO-bit-banged ST7701 initialization driver
```

## Shared Base Configuration

### RGB Display Bus

The `lcd_rgb565_no_de_pins` group in `base/epass.dtsi` uses the following pins:

```text
PD1–PD11
PD13–PD18
PD20
PD21
```

These pins carry the RGB565 data, pixel clock, and synchronization signals. The `no_de` suffix indicates that this pin group does not use a dedicated Data Enable signal.

The shared panel description uses:

```dts
compatible = "lattland,mostima", "simple-panel";
```

`lattland,mostima` is a custom compatible string registered by a kernel patch from the original project. It is not a standard ST7701 panel model provided by mainline Linux.

### ST7701 Initialization Pins

`base/devicetree.dts` configures the initialization driver as follows:

| Signal | GPIO | Purpose |
| --- | --- | --- |
| SDA | PE4 | Serial command or data bit |
| SCL | PD19 | Serial clock |
| CS | PE11 | Chip select |
| RST | Not declared | The driver supports an optional reset pin, but the current board device tree does not use one |

Communication is implemented by a custom kernel driver that toggles the GPIOs directly. It does not use the F1C200S hardware SPI controller.

Each transfer sends one command/data indicator bit followed by eight payload bits:

- An indicator bit of `0` denotes a command.
- An indicator bit of `1` denotes data.
- Each byte is transmitted most significant bit first.
- Pulling `CS` low begins a write group, and pulling it high ends the group.

This GPIO-bit-banged initialization method is specific to the current display wiring. These pins cannot simply be treated as an ordinary SPI overlay.

### Backlight

The backlight is not configured in this directory. It is managed by the `pwm-backlight` node in `base/epass.dtsi`:

- PWM controller: PWM0
- Period: `10000 ns`
- Frequency: approximately `100 kHz`
- Brightness table: `0 4 8 16 32 64 128 196 220 255`
- Default brightness index: `6`
- Default brightness value: `128`

## Shared Display Mode

All three display overlays ultimately use the same `lattland,mostima` panel description. The shared display mode is added to the Linux `panel-simple` driver by:

```text
board/cra/epass/patch/linux/0002-panel-simple.patch
```

The current parameters are:

| Parameter | Value |
| --- | ---: |
| Pixel clock | 24000 kHz |
| Horizontal active pixels | 384 |
| Horizontal sync start | 444 |
| Horizontal sync end | 450 |
| Horizontal total | 528 |
| Vertical active pixels | 640 |
| Vertical sync start | 656 |
| Vertical sync end | 660 |
| Vertical total | 669 |
| Declared refresh rate | 60 Hz |
| Bus format | `MEDIA_BUS_FMT_RGB565_1X16` |
| Bits per color component | 6 bpc |

Note that the refresh rate calculated from the pixel clock and totals is:

```text
24,000,000 ÷ (528 × 669) ≈ 67.9 Hz
```

## Basic Overlay Structure

All three files are Device Tree Overlays:

```dts
#include <dt-bindings/display/st7701initseq.h>

/dts-v1/;
/plugin/;

/ {
    fragment@1 {
        target = <&st7701initseq>;
        __overlay__ {
            status = "okay";
            init-sequence = <...>;
        };
    };

    fragment@2 {
        target = <&panel>;
        __overlay__ {
            compatible = "lattland,mostima", "simple-panel";
        };
    };
};
```

### `fragment@1`

This fragment targets `&st7701initseq` in the base device tree:

- It changes the default `status = "disabled"` to `status = "okay"`.
- It supplies the `init-sequence` for the selected display.
- It allows the custom ST7701 initialization driver to match and run during Linux startup.

### `fragment@2`

This fragment targets `&panel` in the base device tree and specifies the shared panel-driver compatible string.

All three files currently set the same `compatible` value, so they also use the same resolution, synchronization timings, and RGB565 format. Their primary difference is the ST7701 register initialization data rather than the DRM display mode.

`base/epass.dtsi` already declares the exact same `compatible` value. As a result, this fragment does not alter the final value in the current configuration and appears to be a redundant declaration retained from the original overlay structure. If separate panel timings are added for different displays in the future, this fragment can select a distinct compatible string for each display while the kernel panel driver supplies the corresponding description.

Starting the numbering at `fragment@1` without a `fragment@0` is not a device tree syntax error. Fragment numbers only need to be unique within the same overlay.

### `fragment@3`

Only `laowu.dts` contains:

```dts
fragment@3 {
    target = <&tcon0>;
    __overlay__ {
        srgn,swap-b-r;
    };
};
```

`srgn,swap-b-r` is a private boolean property defined by a kernel patch from the original project. `0004-swap_rb_as_config.patch` reads this property and sets the color-channel swap bit in the TCON0 control register.

If the display otherwise works correctly but red and blue are reversed, first check whether the wrong `hsd`/`laowu` configuration was selected instead of swapping every pixel in the application.

The property name is tightly coupled to the kernel patch. If it is renamed to `cra,swap-b-r` in the future, both of the following must be updated:

```text
screen/laowu.dts
board/cra/epass/patch/linux/0004-swap_rb_as_config.patch
```

Changing only one side will disable red/blue channel swapping.

## Initialization Sequence Instructions

`init-sequence` is not an ordinary byte array. It is encoded with macros from:

```text
include/dt-bindings/display/st7701initseq.h
```

The custom kernel driver then interprets the array one entry at a time.

| Macro | Arguments | Purpose |
| --- | --- | --- |
| `ST7701INIT_BEGIN_WRITE` | None | Pulls `CS` low to begin a transfer group |
| `ST7701INIT_WRITE_COMMAND_8` | One command | Writes one 8-bit command |
| `ST7701INIT_WRITE_C8_D8` | One command and one data value | Writes a command followed by one data byte |
| `ST7701INIT_WRITE_C8_D16` | One command and two data values | Writes a command followed by two data bytes |
| `ST7701INIT_WRITE_BYTES` | Length and the corresponding data values | Continues by writing the specified number of data bytes |
| `ST7701INIT_END_WRITE` | None | Pulls `CS` high to end the transfer |
| `ST7701INIT_DELAY` | Duration in milliseconds | Sleeps for the specified duration |

For example:

```dts
ST7701INIT_BEGIN_WRITE

ST7701INIT_WRITE_COMMAND_8 0xFF
ST7701INIT_WRITE_BYTES 5
0x77 0x01 0x00 0x00 0x10

ST7701INIT_WRITE_C8_D16 0xC1 0x07 0x02

ST7701INIT_END_WRITE
ST7701INIT_DELAY 100
```

This means:

1. Pull chip select low.
2. Send command `0xFF`.
3. Immediately send five data bytes.
4. Send command `0xC1` followed by two data bytes.
5. Pull chip select high.
6. Wait for 100 ms.

The driver does not validate register values against the ST7701 datasheet, nor does it fully verify that enough arguments remain after each macro instruction. An incorrect length, a missing argument, or disrupted instruction boundaries can cause an invalid initialization and may even make the driver read beyond the intended array boundary.

## Scope of the Initialization Sequences

These long sequences primarily contain:

- ST7701 extended-command page selection.
- Power, voltage, and analog parameters.
- Positive and negative Gamma curves.
- Source/Gate output and GIP mapping.
- Scan-direction and display-orientation settings.
- Pixel-format configuration.
- Sleep exit.
- Display enable.
- Delays required between initialization stages.

Some of these registers belong to ST7701 vendor-specific extension pages, so their purpose cannot be inferred solely from generic MIPI DCS command names. Obtain the datasheets for the corresponding panel and controller before making changes, and preserve the initialization table that has already been verified on the physical display.

All three sequences contain:

- `0x11`: exits sleep mode.
- `0x29`: turns the display on.
- `0x3A 0x50`: selects the pixel format currently in use.

The BOE sequence differs from the HSD/Laowu sequence in its Gamma, power, timing-control, GIP-mapping, and delay values. The configurations must not be mixed merely by copying and renaming a file.

## Differences Between the Three Configurations

### `boe.dts`

`boe.dts` uses an independent initialization table. Compared with HSD/Laowu, it contains different Gamma, power, and extended-register values.

Its primary delay stages are:

```text
120 ms
10 ms
20 ms
```

After enabling the display, it also sends:

```text
0x35 0x00
```

This step is not present in the HSD/Laowu sequence.

### `hsd.dts`

`hsd.dts` uses another ST7701 initialization table and does not enable red/blue channel swapping in TCON0.

Its primary delay stages are:

```text
150 ms
100 ms
20 ms
```

It is the default argument passed by the current `flash.py` entry point:

```python
flash("hsd", {...})
```

Here, “default” refers only to the current flashing-script invocation. It does not mean that every physical unit is fitted with an HSD display.

### `laowu.dts`

The initialization sequence in `laowu.dts` is identical to the one in `hsd.dts`. The only source-level difference is the following addition in `laowu.dts`:

```dts
srgn,swap-b-r;
```

Therefore:

```text
hsd    = HSD initialization table
laowu = HSD initialization table + TCON0 red/blue swap
```

If the HSD initialization sequence is changed in the future, check whether `laowu.dts` must be updated as well. This prevents two register tables that are intended to remain identical from diverging accidentally.

## Kernel Initialization Driver

ST7701 initialization support is not provided by the original Linux 5.4.99 source. It is added by:

```text
board/cra/epass/patch/linux/0006-initalize-st7701.patch
```

The patch adds:

```text
include/dt-bindings/display/st7701initseq.h
drivers/staging/shirogane/Kconfig
drivers/staging/shirogane/Makefile
drivers/staging/shirogane/st7701init.c
```

The driver is built directly into the kernel through:

```text
CONFIG_SHIROGANE_SIMPLE_ST7701_INIT=y
```

The driver matches:

```dts
compatible = "lattland,st7701-initseq";
```

After a successful match, it:

1. Requests the SDA, SCL, CS, and optional RST GPIOs.
2. Reads `init-sequence` from the device tree as an array of 32-bit cells.
3. Creates a workqueue task.
4. Executes the GPIO initialization sequence asynchronously from the workqueue.

The current device tree does not declare an RST GPIO, so the driver continues without a dedicated reset pin.

`lattland,*`, `srgn,*`, and `SHIROGANE_*` are all internal names inherited from the original project. They may be migrated gradually to CRA naming during further development, but the device tree, kernel patches, configuration symbols, and every reference must be updated together. A simple string replacement is not sufficient.

## Compilation and Packaging

The Buildroot post-image process invokes:

```text
board/cra/epass/scripts/mkdt.sh
```

The script processes every `.dts` file in this directory. It first runs the C preprocessor with the kernel headers:

```sh
cpp -nostdinc \
    -I "${BUILD_DIR}/linux-5.4.99/include/" \
    -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
    -P -undef -x assembler-with-cpp
```

It then runs:

```sh
dtc -@ -I dts -O dtb
```

to generate device tree overlays with symbol-based relocation support.

The output files are:

```text
output/images/dt/screen/boe.dtbo
output/images/dt/screen/hsd.dtbo
output/images/dt/screen/laowu.dtbo
```

`kernel.its` packages them into the following FIT image nodes:

```text
fdt-screen-boe
fdt-screen-hsd
fdt-screen-laowu
```

Adding a `.dts` file to this directory is not, by itself, enough to make it available to U-Boot. After adding a new display type, also update:

```text
board/cra/epass/scripts/kernel.its
```

and add the corresponding `fdt-screen-*` node for the new `.dtbo`.

## Selecting a Display at Boot

The flashing script writes the following value to the boot-environment text:

```text
screen=hsd
```

During startup, U-Boot imports the environment from offset `0xFA000` in the SPI-NAND and runs:

```text
imxtract $fitaddr fdt-screen-${screen} $dtboaddr
```

For example:

```text
screen=boe
        ↓
fdt-screen-boe
        ↓
boe.dtbo
```

After extraction, U-Boot applies the display overlay to the base device tree with:

```text
fdt apply $dtboaddr
```

Valid values must exactly match the corresponding FIT node suffix:

```text
boe
hsd
laowu
```

The `uEnv.txt` template in the current repository does not contain `screen=`, while `flash.py` generates a `.bootenv.txt` file that includes it at runtime. Therefore:

- When using the existing `flash.py`, the display type comes from the first argument to `flash()` or `flash2()`.
- When creating or editing the boot environment manually, a valid `screen=` entry must be supplied.
- If `screen` is empty or misspelled, U-Boot cannot extract the correct `fdt-screen-*` node.

## Common Symptoms and Checks

| Symptom | Check first |
| --- | --- |
| Backlight is on, but no image appears | Whether `screen=` is valid, whether ST7701 initialization ran, and the RGB clock and synchronization signals |
| Display is completely dark | PWM backlight, power supplies, and panel connection; do not inspect only the initialization sequence |
| Red and blue are reversed | Whether `hsd` and `laowu` were confused, and whether `srgn,swap-b-r` took effect |
| Incorrect color gradation | RGB565 wiring, pixel format, Gamma table, and panel model |
| Image rolls or tears | Shared DRM timings, ST7701 scan configuration, and pixel clock |
| Incorrect image orientation | Scan-direction settings in the initialization table; do not change TCON before confirming the cause |
| Intermittent white screen during startup | Initialization order, delays, power stability, and asynchronous initialization timing |
| Macros cannot be found during compilation | Whether `0006-initalize-st7701.patch` has been applied to the Linux headers |
| `.dtbo` was generated but cannot be found at boot | Whether `kernel.its` contains the matching `fdt-screen-*` node |

Search the kernel log for messages from the original driver with:

```sh
dmesg | grep -i st7701
dmesg | grep -i srgn
```

When the original driver completes successfully, it logs the start and completion of the initialization task. Selecting `laowu` also produces messages related to red/blue channel swapping.

## Guidelines for Further Development

Before changing this directory, confirm the physical display model, flex-cable pinout, PCB connections, and a known-good boot configuration:

- Never change power, Gamma, or GIP registers at random without panel documentation and physical test hardware.
- Never assume that all ST7701 displays can share the same initialization sequence.
- Never attribute an application-level color problem directly to the panel. First use solid red, green, and blue test images to confirm the channel order.
- After changing `ST7701INIT_WRITE_BYTES`, verify both the declared length and the actual number of data values.
- Every `BEGIN_WRITE` should have a matching `END_WRITE`.
- Sleep-exit and display-enable commands, including their delays, should follow the panel documentation.
- To adjust the shared resolution or synchronization timings, modify and verify `0002-panel-simple.patch` rather than changing only this directory.
- If the red/blue swap property is renamed, update `0004-swap_rb_as_config.patch` at the same time.
- If the initialization driver's compatible string or configuration symbol changes, update `0006-initalize-st7701.patch`, the base device tree, and the display overlays together.
- After adding a display overlay, update both `kernel.its` and the set of display names accepted by the flashing tool.
- Begin physical testing from a known recoverable configuration, and preserve both the serial log and the original initialization table.
- Display testing does not require rewriting the entire system or the application resources. Prefer changing only the `screen` selection in the boot environment and observing the result.
