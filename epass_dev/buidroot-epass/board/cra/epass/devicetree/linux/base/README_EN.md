# CRA Electric Pass Base Device Tree

Read this in other languages: [English](README.md), [中文](README_CN.md).

This directory contains the Linux base device tree for CRA Electric Pass. The current project targets the Shirogane v0.6 hardware revision only.

## Files

| File | Purpose |
| --- | --- |
| `epass.dtsi` | Describes the shared hardware of the electronic pass, including the model identifier, LCD display pipeline, power supplies, backlight, GPIO pin multiplexing, SPI-NAND, UART, SD, USB, video engine, and the default state of the core peripherals. |
| `devicetree.dts` | Serves as the final base device tree entry point for the physical board. It includes `epass.dtsi` and adds the power-off GPIO, ST7701 initialization pins, and LRADC key parameters. |

The generated base device tree is:

```text
output/images/dt/base/devicetree.dtb
```

## Composition

The final device tree is assembled in the following order:

```text
Linux suniv-f1c100s.dtsi
        ↓
base/epass.dtsi
        ↓
base/devicetree.dts
        ↓
screen, interface, and ext overlays
        ↓
U-Boot applies the overlays and boots Linux
```

`suniv-f1c100s.dtsi` provides the internal controller definitions shared by the F1C100S and F1C200S SoCs.

## `epass.dtsi`

### Device Identity

```dts
model = "CRA Electric Pass";
compatible = "cra,electric-pass",
             "allwinner,suniv-f1c200s",
             "allwinner,suniv-f1c100s";
```

`model` is the human-readable device name. The entries in `compatible` are ordered from the board-specific identifier to the most general SoC fallback.

### Boot Arguments

`chosen/bootargs` is a placeholder only. The actual kernel command line is supplied by U-Boot and includes the serial console, UBI/UBIFS root filesystem, and NAND partition parameters.

### Display System

The base display pipeline is:

```text
DE/FE/BE → TCON0 → RGB565 → LCD panel
```

The panel uses `lattland,mostima` to match the custom `panel-simple` kernel patch included with the project. The low-level display timing is 384×640 at 60 Hz, while the visible application interface should be designed for an area of approximately 360×640.

`st7701initseq` is disabled in the base device tree. The screen overlay selected at boot enables it and supplies the initialization sequence for the BOE, HSD, or Laowu panel.

### Power Supplies and Backlight

- `vcc3v3`: fixed 3.3 V supply used by the SD interface and other peripherals.
- `lradc_vref`: 3.0 V reference supply for the LRADC.
- `pwm-backlight`: controls the display backlight through PWM0.
- PWM period: 10000 ns, or approximately 100 kHz.
- Default brightness index: 6, corresponding to a brightness value of 128.

### GPIO Pin Multiplexing

This file defines the following reusable pin groups:

- `spi1_pins`: PE7, PE8, PE9, and PE10.
- `rtp_pins_0`: PA0.
- `rtp_pins_01`: PA0 and PA1.
- `lcd_rgb565_no_de_pins`: LCD RGB565 data, clock, and synchronization signals.
- `i2s_pins_pe` and `i2s_pins_pa`: two alternative I²S pin layouts.

These assignments must remain consistent with the PCB routing.

### SPI-NAND Partition Layout

The onboard SPI-NAND is arranged as a 128 MiB device:

| Partition | Start Address | Size | Purpose |
| --- | ---: | ---: | --- |
| `u-boot` | `0x000000` | 1 MiB | SPL, U-Boot, and the boot environment. |
| `boot` | `0x100000` | 6 MiB | Linux kernel, base device tree, and overlays. |
| `rootfs` | `0x700000` | 121 MiB | UBIFS root filesystem, applications, and resources. |

When changing the partition layout, update and verify the device tree, U-Boot command line, image-generation scripts, and flashing addresses together.

### Default Peripheral States

| Peripheral | Default State | Description |
| --- | --- | --- |
| PWM0 | Enabled | Controls the display backlight. |
| SPI0 | Enabled | Connects to the onboard SPI-NAND. |
| SPI1 | Disabled | Enabled by an interface overlay when required. |
| UART0 | Enabled | System debug console. |
| UART1/UART2 | Disabled | Enabled by interface overlays when required. |
| MMC0 | Enabled | 4-bit SD/MMC interface at 3.3 V. |
| USB OTG/PHY | Enabled | Supports USB device mode, RNDIS, and optional host mode. |
| Cedar/ION/DE/FE/BE | Enabled | Provides video decoding, display memory management, and display-engine support. |
| TVE0 | Disabled | Analog TV output is not used. |
| LRADC | Enabled | Reads the resistor-ladder keys. |
| I²C0 | Disabled | Enabled by an interface or extension overlay when required. |

## `devicetree.dts`

### Power-Off Control

```dts
gpios = <&pio 4 2 GPIO_ACTIVE_HIGH>;
timeout-ms = <3000>;
```

Driving PE2 high triggers the hardware power-off circuit. The driver waits for 3000 ms after asserting the signal.

An incorrect pin assignment can leave the device powered even after Linux has completed its shutdown sequence.

### ST7701 Initialization Interface

| Signal | GPIO |
| --- | --- |
| SDA | PE4 |
| SCL | PD19 |
| CS | PE11 |

These pins are used only to send initialization commands to the ST7701. LCD pixel data is transferred separately over the RGB565 bus.

### LRADC Keys

All five keys share LRADC channel 0. A resistor ladder produces a different target voltage for each key:

| Linux Key Code | Target Voltage |
| --- | ---: |
| `KEY_0` | 0 V |
| `KEY_1` | 1.396826 V |
| `KEY_2` | 1.111111 V |
| `KEY_3` | 0.825396 V |
| `KEY_4` | 0.444444 V |

The `voltage` property is expressed in microvolts.

Do not change these values without recalibrating the resistor ladder. Incorrect values can cause missed key presses, incorrect key detection, or unstable behavior near a voltage threshold.

## Overlays

The base device tree defines the hardware framework. The following features are still selected through overlays:

- `screen/`: initialization for BOE, HSD, and Laowu panels.
- `interface/`: ADC, I²C, I²S, SPI, UART, USB, and other interfaces.
- `ext/`: extension devices such as CardKB, ES8311, and LSM6DS3.

## Guidance for Further Development

Low-risk changes:

- Backlight brightness table and default brightness.
- Key `label` values.

Changes that require corresponding updates in the application or other configuration files:

- Linux key codes.
- UART, SPI, I²C, and I²S enablement.
- USB operating mode.
- `screen`, `interface`, and `ext` overlays.

High-risk settings:

- Power-off GPIO.
- ST7701 initialization GPIOs.
- LCD RGB pins and panel `compatible` string.
- LRADC key voltages.
- SPI-NAND partition layout.
- Voltage and clock parameters.

`lattland,mostima` and `lattland,st7701-initseq` are kernel driver match strings, not text displayed by the user interface. Renaming either one requires the corresponding Linux kernel patch to be updated at the same time.

## Comments and Formatting

Use C-style comments:

```dts
/* Brief explanation */

/*
 * Longer explanation.
 * Document the hardware rationale, units, and modification risks.
 */
```

Keep all files encoded as UTF-8.

## Building and Validation

Run the following commands from the Buildroot root directory in WSL:

```sh
make cra_epass_defconfig
make -j$(nproc)
```

Verify the model stored in the generated device tree:

```sh
output/host/bin/fdtget \
    output/images/dt/base/devicetree.dtb \
    / model
```

Expected output:

```text
CRA Electric Pass
```

Inspect the FIT image:

```sh
output/host/bin/dumpimage -l output/images/boot.itb
```

The base device tree should appear as `fdt-base`. Building and validating the image does not write anything to a physical device.
