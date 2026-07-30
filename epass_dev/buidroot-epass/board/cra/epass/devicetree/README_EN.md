# CRA Electric Pass Device Trees

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the board-level device tree sources for CRA Electric Pass, divided into two independent configurations for U-Boot and Linux. The current project targets the Shirogane v0.6 board revision only.

Device trees describe the processor's internal controllers, mainboard pins, boot storage, display system, interfaces, and external devices. Linux and U-Boot drivers match the physical hardware through device tree nodes and `compatible` strings.

## Directory Structure

```text
devicetree/
├─ uboot/
│  ├─ suniv-f1c100s-generic.dts
│  └─ README.md
└─ linux/
   ├─ base/
   ├─ screen/
   ├─ interface/
   ├─ ext/
   └─ README.md
```

| Directory | Stage | Main responsibility | Details |
| --- | --- | --- | --- |
| `uboot/` | SPL and U-Boot | Boot flash, boot console, USB DFU, boot-stage MMC, and the required SoC resources | [uboot/README.md](uboot/README.md) |
| `linux/` | Linux | Complete mainboard hardware, display pipeline, screens, interfaces, external devices, and Linux driver parameters | [linux/README.md](linux/README.md) |

The device passes through several software stages between power-on and application startup:

```text
Allwinner BROM
        │
        ▼
SPL
        │
        ▼
U-Boot
        │
        ├─ Reads the boot environment
        ├─ Reads boot.itb
        ├─ Extracts the Linux base DTB
        └─ Applies the Linux DTBO files
        │
        ▼
Linux
        │
        ▼
CRA Electric Pass application
```

SPL and U-Boot must access the SPI-NAND device, serial console, and USB DFU before Linux starts, so they require a device tree for the bootloader's own drivers.

After Linux starts, it reinitializes the hardware and uses a separate, more complete device tree. This tree describes the LCD, backlight, keys, storage, interfaces, external devices, and other information required by Linux drivers.

The two device trees are not synchronized automatically:

- Changes under `uboot/` do not alter the hardware state after Linux starts.
- Changes under `linux/` do not alter hardware initialization in SPL or U-Boot.
- The same controller may have different enable states, pin configurations, and purposes in the two trees.
- Hardware changes that affect both the bootloader and Linux must be checked in both device trees.

## Responsibilities of the Two Device Trees

| Hardware or function | U-Boot device tree | Linux device tree |
| --- | --- | --- |
| Reading the SPI-NAND boot medium | Required | Handles MTD, partitions, and runtime access |
| Boot serial console | Required | Handles the Linux serial console and general-purpose UARTs |
| USB DFU | Required | Handles Linux USB Gadget, Host, or other modes |
| Boot-stage SD/MMC access | Configured as required by the boot process | Handles Linux SD-card and MMC drivers |
| LCD and backlight | Not currently managed by U-Boot | Fully configured |
| ST7701 initialization | Not handled | Configured by the Linux device tree and screen overlays |
| LRADC keys | Not handled | Configured by the Linux device tree |
| I²C, I²S, SPI1, and UART expansion | Usually unnecessary | Configured by Linux interface overlays |
| CardKB, ES8311, and LSM6DS3 | Unnecessary | Configured by Linux external-device overlays |

The current U-Boot configuration disables:

```text
# CONFIG_VIDEO_SUNXI is not set
```

## Shared SoC Definitions

The CRA board-level DTS files in this directory do not redefine every F1C100S/F1C200S register and controller. The U-Boot and Linux trees each include shared SUNIV definitions supplied by Buildroot.

Shared U-Boot file:

```text
board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
```

Shared Linux file:

```text
board/allwinner/suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi
```

They are composed as follows:

```text
Shared SUNIV SoC definitions
        │
        ▼
CRA board-level base definitions
        │
        ▼
Current board entry point or overlay
```

The shared `.dtsi` files provide register addresses, clocks, resets, interrupts, DMA, GPIO, and basic controller nodes. The CRA files select the controllers, pins, and parameters used by the current PCB.

The shared files originate from the original project and their respective upstream authors. Their existing SPDX identifiers, copyright notices, and commit history must be preserved; shared SoC definitions must not be relabeled as original CRA work.

## U-Boot Device Tree

The U-Boot board entry point is:

```text
uboot/suniv-f1c100s-generic.dts
```

It includes the shared SUNIV definitions through:

```dts
#include "suniv-f1c100s.dtsi"
```

It then enables the resources required by the current boot process:

- The UART0 boot console.
- UART1.
- SPI0 and SPI-NAND.
- USB OTG, the USB PHY, and OTG SRAM.
- The first MMC controller.

The current code explicitly disables the second MMC controller because its pins conflict with SPI0.

The Buildroot configuration references:

```text
BR2_TARGET_UBOOT_CUSTOM_DTS_PATH="
    board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
    board/cra/epass/devicetree/uboot/suniv-f1c100s-generic.dts"
```

The U-Boot configuration file is:

```text
board/cra/epass/uboot.defconfig
```

It selects this board-level DTS with:

```text
CONFIG_DEFAULT_DEVICE_TREE="suniv-f1c100s-generic"
```

## Linux Device Tree

The Linux base device tree consists of:

```text
linux/base/epass.dtsi
linux/base/devicetree.dts
```

The Buildroot configuration references:

```text
BR2_LINUX_KERNEL_CUSTOM_DTS_PATH="
    board/allwinner/suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi
    board/cra/epass/devicetree/linux/base/epass.dtsi
    board/cra/epass/devicetree/linux/base/devicetree.dts"
```

The base files are included in this order:

```text
suniv-f1c100s.dtsi
        ↓
linux/base/epass.dtsi
        ↓
linux/base/devicetree.dts
```

In addition to the base DTB, the Linux device tree contains three types of overlays:

| Type | Directory | Required | Purpose |
| --- | --- | --- | --- |
| Screen | `linux/screen/` | One must be selected | Selects BOE, HSD, or Laowu screen initialization |
| Interface | `linux/interface/` | Optional; several may be selected | Enables I²C, I²S, SPI, UART, ADC, or a USB mode |
| External device | `linux/ext/` | Optional; several may be selected | Declares devices such as CardKB, ES8311, and LSM6DS3 |

U-Boot assembles the Linux device tree in this order:

```text
base → screen → interface → ext
```

## Buildroot Integration

The main component versions are:

| Component | Version |
| --- | --- |
| Linux | `5.4.99` |
| U-Boot | `2020.07` |

The Buildroot configuration file is:

```text
board/cra/epass/cra_epass_defconfig
```

It specifies:

- The Linux version, patches, kernel configuration, and Linux DTS files.
- The U-Boot version, patches, U-Boot configuration, and U-Boot DTS files.
- The image post-processing scripts.

The post-processing scripts run in this order:

```text
board/cra/epass/scripts/mknanduboot.sh
        ↓
board/cra/epass/scripts/mkdt.sh
        ↓
board/cra/epass/scripts/buildimage.sh
```

The scripts have the following responsibilities:

| Script | Purpose |
| --- | --- |
| `mknanduboot.sh` | Rearranges the ordinary SPL/U-Boot output for the current SPI-NAND page layout |
| `mkdt.sh` | Compiles the Linux base DTB and every DTBO |
| `buildimage.sh` | Creates the UBI root filesystem and packages `boot.itb` according to `kernel.its` |

## From Source to Boot

The complete build relationship is:

```text
Shared U-Boot SUNIV .dtsi
        +
CRA uboot/*.dts
        │
        ▼
U-Boot/SPL build
        │
        ▼
u-boot-sunxi-with-spl.bin
        │
        ▼
mknanduboot.sh
        │
        ▼
u-boot-sunxi-with-nand-spl.bin

Shared Linux SUNIV .dtsi
        +
linux/base/*.dts*
        +
linux/screen/*.dts
        +
linux/interface/*.dts
        +
linux/ext/*.dts
        │
        ▼
mkdt.sh
        │
        ├─ devicetree.dtb
        └─ *.dtbo
        │
        ▼
kernel.its + zImage
        │
        ▼
buildimage.sh
        │
        ▼
boot.itb
```

When the physical device boots:

```text
U-Boot stored in SPI-NAND
        │
        ├─ Uses its own device tree to initialize boot hardware
        ├─ Reads the text boot environment from 0xFA000
        ├─ Reads boot.itb from 0x100000
        ├─ Extracts the Linux base DTB
        ├─ Applies the screen/interface/ext DTBO files
        └─ Passes the assembled DTB to Linux
```

## Generated Files

Common build outputs include:

| File | Source | Purpose |
| --- | --- | --- |
| `output/images/u-boot-sunxi-with-spl.bin` | U-Boot build | U-Boot image before NAND page-layout processing |
| `output/images/u-boot-sunxi-with-nand-spl.bin` | `mknanduboot.sh` | Final U-Boot image for the physical SPI-NAND device |
| `output/images/dt/base/devicetree.dtb` | `mkdt.sh` | Linux base device tree |
| `output/images/dt/screen/*.dtbo` | `mkdt.sh` | Screen overlays |
| `output/images/dt/interface/*.dtbo` | `mkdt.sh` | Interface overlays |
| `output/images/dt/ext/*.dtbo` | `mkdt.sh` | External-device overlays |
| `output/images/boot.itb` | `buildimage.sh` | FIT image containing the Linux kernel, base DTB, and every DTBO |

Do not edit these directories directly:

```text
output/build/
output/images/dt/
```

They contain generated build output and will be overwritten after a clean or rebuild. Persistent changes must be made under `board/cra/epass/devicetree/`, in the related configuration, or in the build scripts.

## File Types

| Suffix | Meaning |
| --- | --- |
| `.dtsi` | Shared source file included by other device trees |
| `.dts` | Directly compilable device tree entry point or device tree overlay source |
| `.dtb` | Compiled base device tree binary |
| `.dtbo` | Compiled device tree overlay binary |
| `.its` | Text description of a FIT image |
| `.itb` | FIT binary image generated from an ITS file |

Device tree sources use DTS syntax and support C-style comments:

```dts
/* Block comment */
```

The current project also contains:

```dts
// Line comment
```

Prefer `/* ... */` for compatibility with device tree toolchains.

## Modification Guide

| Requirement | Files to modify |
| --- | --- |
| Change U-Boot serial output | `uboot/`, `uboot.defconfig`, or the U-Boot patches |
| Change a Linux serial port or enable a UART | `linux/base/` or `linux/interface/` |
| Change the SPI-NAND device accessed by SPL/U-Boot | `uboot/`, the U-Boot configuration, and the U-Boot patches |
| Change Linux MTD partitions | `linux/base/`, together with the boot arguments and image layout |
| Change a screen initialization sequence | `linux/screen/` |
| Change LCD timing or the display pipeline | `linux/base/`, the screen overlays, and the corresponding kernel patches |
| Change the backlight or keys | `linux/base/` |
| Enable I²C, I²S, SPI1, or UART | `linux/interface/` |
| Add a specific external device | `linux/ext/`, together with its required interface |
| Change USB DFU | `uboot/`, the U-Boot patches, and the U-Boot configuration |
| Change the Linux USB mode | `linux/interface/` and the corresponding Linux patches |
| Change the application UI | Outside the device tree; modify `drm_app_neo` |

If a change involves boot storage, UART, USB, clocks, or a shared group of physical pins, do not inspect only the single location listed in the table. Search every reference across both device trees, their configurations, patches, and scripts.

## File and Label Dependencies

When renaming a DTS file, also inspect:

- `CUSTOM_DTS_PATH` in `cra_epass_defconfig`.
- `CONFIG_DEFAULT_DEVICE_TREE` in `uboot.defconfig`.
- Directory and output rules in `mkdt.sh`.
- DTB/DTBO paths and FIT nodes in `kernel.its`.
- FIT-node extraction names in `uboot.env`.
- Selection values in `uEnv.txt`, `flash.py`, or the actual boot environment.
- README files and build commands at every level.

When renaming a node label in the base device tree, search every overlay that references it. For example, renaming `i2c0`, `i2s0`, `pio`, `st7701initseq`, or `usb_otg` can prevent the corresponding DTBO files from being generated or applied correctly.

## Troubleshooting

| Symptom | Check first |
| --- | --- |
| U-Boot does not change after modifying a Linux DTS | The two trees are independent; edit `uboot/` |
| Linux drivers do not change after modifying a U-Boot DTS | Linux uses the separate tree under `linux/` |
| A newly generated overlay cannot be found at boot | Confirm that it was also added to `kernel.its` |
| The physical device still uses an old configuration after an overlay change | Regenerate `boot.itb` and check whether the Boot partition still contains the old image |
| U-Boot DTS changes do not take effect | Clean and rebuild U-Boot |
| A DTBO compiles but cannot be applied | Confirm that the base DTB preserves symbols and that the label and `target` exist |
| Several interfaces work separately but fail when combined | Check for GPIO, clock, DMA, interrupt, or bus-resource conflicts |
| The screen is blank or has incorrect colors | Check the `screen` selection, initialization sequence, RGB channel mapping, and kernel display patches |
| USB behaves differently during boot and under Linux | Inspect the U-Boot USB configuration and Linux USB overlays separately |
| Script builds fail after editing on Windows | Check whether `.sh` files were converted to CRLF |
| Output remains unchanged after source edits | Check for stale files under `output/build` or `output/images` |

## Building and Verification

Configure and build the complete project with:

```sh
make cra_epass_defconfig
make
```

Inspect the FIT contents after building:

```sh
output/host/bin/mkimage -l output/images/boot.itb
```

Decompile the Linux base device tree:

```sh
dtc -I dtb -O dts \
    -o devicetree-linux.decoded.dts \
    output/images/dt/base/devicetree.dtb
```

Decompile the U-Boot device tree:

```sh
dtc -I dtb -O dts \
    -o devicetree-uboot.decoded.dts \
    output/build/uboot-2020.07/u-boot.dtb
```

Verify the following:

- The U-Boot DTB enables only the controllers required during the boot stage.
- The Linux base DTB contains the symbols required by its DTBO files.
- Every DTB and DTBO referenced by `kernel.its` exists.
- FIT node names exactly match `screen`, `interface`, and `ext` in the boot environment.
- Controllers that use the same physical pins have not been enabled simultaneously.
- The total size of `boot.itb` does not exceed the 5 MiB limit enforced by U-Boot `checkfit`.
- The final U-Boot file is the NAND-layout image, `u-boot-sunxi-with-nand-spl.bin`.

A successful device tree build proves only that its syntax, references, and structure satisfy the toolchain. It does not prove that electrical levels, PCB wiring, timing, driver dependencies, and every overlay combination have been validated.

## Secondary-Development Principles

- Before making a change, determine whether the problem occurs during SPL/U-Boot or after Linux starts.
- Do not force the two device trees to contain identical nodes merely for visual consistency.
- Before modifying a shared SUNIV `.dtsi`, confirm that the change is valid for every board configuration that includes it.
- When changing a `compatible` string, inspect the matching U-Boot or Linux driver as well.
- When changing a pin group, search both device trees and every overlay for physical pin usage.
- When changing the boot-storage layout, inspect U-Boot, Linux MTD, UBI, the boot environment, image scripts, and flashing tools together.
- After adding a Linux overlay, update both `kernel.its` and the boot environment.
- Do not edit generated files under `output/build` or `output/images/dt` directly.
- All scripts executed by Linux or Buildroot must use LF line endings.
- Before testing on physical hardware, complete a clean build, decompile the DTB/DTBO files, inspect the FIT nodes, and retain a recoverable image and serial recovery method.
