# CRA Electric Pass U-Boot Patches

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the board-level patches added to U-Boot 2020.07 for CRA Electric Pass. They primarily cover the boot console, USB DFU, SPI-NAND identification, the SUNIV SPI clock, and post-write NAND verification.

These patches affect both the SPL and main U-Boot stages. SPL handles the earliest DRAM, SPI, and NAND boot operations. Main U-Boot loads the boot environment, reads the FIT image, assembles the Linux device tree, and provides DFU when booting fails or the user requests it.

## Build Integration

The board-level Buildroot configuration specifies the patch directory as follows:

```make
BR2_TARGET_UBOOT_CUSTOM_VERSION_VALUE="2020.07"
BR2_TARGET_UBOOT_PATCH="board/allwinner/suniv-f1c100s/patch/u-boot board/cra/epass/patch/uboot"
BR2_TARGET_UBOOT_CUSTOM_CONFIG_FILE="board/cra/epass/uboot.defconfig"
```

The build sequence is:

```text
Official U-Boot 2020.07 source
        │
        ▼
board/allwinner/suniv-f1c100s/patch/u-boot
        │
        ▼
board/cra/epass/patch/uboot
        │
        ▼
board/cra/epass/uboot.defconfig
        │
        ▼
Build SPL and main U-Boot
        │
        ▼
board/cra/epass/scripts/mknanduboot.sh
        │
        ▼
u-boot-sunxi-with-nand-spl.bin
```

## Directory Overview

```text
patch/uboot/
├─ 0001-uart-pull.patch
├─ 0002-musb-force-fs.patch
├─ 0003-spi-nand-mx35lf1g.patch
├─ 0004-f1c-spi-fix.patch
├─ 0005-dfu-verify-block.patch
└─ README.md
```

| Number | Primary purpose | Affected stage |
| --- | --- | --- |
| `0001` | Configure pull-ups on UART0 TX/RX | SPL and early U-Boot console |
| `0002` | Force the U-Boot MUSB Gadget controller to Full-Speed | U-Boot USB and DFU |
| `0003` | Identify two Macronix SPI-NAND devices in SPL | SPL boot from NAND |
| `0004` | Correct the SUNIV SPI parent clock and divider calculation | SPL and main U-Boot SPI |
| `0005` | Add post-write verification and bad-block handling to MTD DFU | Main U-Boot DFU |

## Patch Details

### 0001: UART0 Pin Pull-Ups

Target file:

```text
arch/arm/mach-sunxi/board.c
```

SUNIV UART0 uses:

```text
PE0: UART0 TX
PE1: UART0 RX
```

The original code already configures a pull-up on PE1. This patch adds the same configuration for PE0:

```c
sunxi_gpio_set_pull(SUNXI_GPE(0), SUNXI_GPIO_PULL_UP);
```

This gives TX and RX consistent default levels during early boot, before the serial driver is fully stable, or while no external serial adapter is connected.

The patch changes only the pin bias. It does not change:

- The UART baud rate.
- The serial controller index.
- The `console=ttyS0,115200` kernel argument.
- The Linux UART driver used after the kernel starts.

### 0002: Force U-Boot MUSB to Full-Speed

Target files:

```text
drivers/usb/musb-new/musb_core.c
drivers/usb/musb-new/musb_gadget.c
```

This patch stops setting the following bit when MUSB starts:

```text
MUSB_POWER_HSENAB
```

It also clears the bit again during USB Gadget wakeup and resume, preventing the controller from returning to High-Speed mode.

It affects U-Boot-stage USB Gadget functions, including:

- DFU.
- U-Boot USB downloads.
- Other U-Boot features that depend on the MUSB Gadget controller.

It does not control USB speed after Linux starts. During the Linux stage, USB speed is determined by the Linux MUSB driver and the `cra,usb-hs-enabled` property introduced by the Linux patch series.

The current patch modifies only the implementation actually used by U-Boot 2020.07:

```text
drivers/usb/musb-new/
```

An earlier version also referenced an obsolete MUSB path that no longer exists. That invalid part has been removed.

### 0003: Macronix SPI-NAND Identification in SPL

Target file:

```text
arch/arm/mach-sunxi/spl_spi_sunxi.c
```

After DRAM initialization, SPL must determine whether the device attached to SPI0 is SPI-NOR or SPI-NAND before it can load main U-Boot using the correct method.

This patch adds two Macronix chip IDs:

| Manufacturer and device ID | Model |
| --- | --- |
| `c2 12` | MX35LF1GE4AB |
| `c2 14` | MX35LF1G24AD |

When either ID matches, SPL sets the flash type to:

```c
FLASHTYPE_NAND
```

This patch handles only flash-type identification during SPL. Complete SPI-NAND access, bad-block support, and MTD operation in main U-Boot also depend on:

- The shared SUNIV U-Boot patch series.
- `CONFIG_MTD_SPI_NAND=y`.
- The SPI0 device-tree node.
- The SPI-NAND driver for the relevant manufacturer.

### 0004: SUNIV SPI Clock Corrections

Target file:

```text
drivers/spi/spi-sunxi.c
```

The original U-Boot driver uses the same 24 MHz constant for both the SPI parent clock and the maximum bus rate, and defaults to 1 MHz when the device tree does not specify a frequency. This does not match the actual clock structure of the SUNIV/F1C100S/F1C200S.

This patch adds the following values to each SoC variant:

```c
u32 mod_clk_hz;
u32 max_speed_hz;
```

The current SUNIV assumptions are:

| Parameter | Value |
| --- | --- |
| SPI divider parent clock | 200 MHz |
| Maximum SPI bus frequency | 100 MHz |
| Current SPI-NAND device-tree limit | 80 MHz |

The 200 MHz parent clock comes from the SPL configuration:

```text
PLL_PERIPH / 3
```

The driver calculates the CDR1/CDR2 dividers with upward rounding so that the actual SPI clock does not exceed the requested rate. If the device tree does not specify a frequency, the driver now uses the maximum rate of the corresponding SoC variant instead of the old fixed 1 MHz fallback.

The current U-Boot device tree uses:

```dts
spi-max-frequency = <80000000>;
```

This patch therefore directly affects SPI-NAND boot-read performance and stability.

If the SPL clock configuration, AHB clock, or `PLL_PERIPH` is changed later, the hard-coded 200 MHz parent-clock assumption must be reviewed. An incorrect parent-clock value will produce an SPI rate different from the requested value and may cause intermittent SPL read failures.

### 0005: DFU Post-Write Verification and Bad-Block Handling

Target file:

```text
drivers/dfu/dfu_mtd.c
```

The original U-Boot MTD DFU write path does not immediately read back and compare data after a successful write call. With SPI-NAND, some write failures may therefore remain undetected until the next boot or read operation.

This patch adds the following behavior:

1. Allocate a verification buffer equal to one NAND erase block when writing.
2. Limit each write to the remaining space in the current erase block.
3. Read the data back through MTD immediately after each write.
4. Compare the returned length and data with the original write.
5. Mark the current block as bad if the write or verification fails.
6. Skip known bad blocks and search for the next usable block.
7. Erase the replacement block and retry the current data.
8. Return `-ENOSPC` when no usable space remains.

This patch improves the reliability of DFU writes to SPI-NAND, but introduces two behaviors that require attention:

- A single write or readback failure may permanently mark the current block as bad.
- The verification buffer is as large as one NAND erase block and consumes additional U-Boot heap memory.

If a failure is caused by unstable power, a USB interruption, an excessively high SPI clock, or a transient signal problem rather than permanent NAND damage, marking the block as bad may be too aggressive. DFU operations should therefore be performed with stable power, USB connectivity, and SPI timing.

## Role in the Boot Chain

```text
Device power-on
   │
   ▼
SPL initializes DRAM, UART0, and SPI0
   │        │
   │        ├─ 0001 Stabilizes the UART0 pin levels
   │        └─ 0004 Calculates the SUNIV SPI clock correctly
   │
   ▼
SPL reads the SPI chip ID
   │
   └─ 0003 Identifies the Macronix MX35LF1G family as SPI-NAND
   │
   ▼
Load main U-Boot from NAND
   │
   ▼
Main U-Boot reads the environment and boot.itb
   │
   ├─ Normal path: boot Linux
   │
   └─ Failure or user request: enter DFU
             │
             ├─ 0002 Forces USB Full-Speed
             └─ 0005 Verifies writes and skips bad blocks
```

## Relationship to Configuration, Device Trees, and Scripts

### U-Boot Configuration

The relevant configuration is:

```text
board/cra/epass/uboot.defconfig
```

The primary options include:

```text
CONFIG_SPL=y
CONFIG_SPL_SPI_SUNXI=y
CONFIG_CMD_DFU=y
CONFIG_DFU_MTD=y
CONFIG_MTD=y
CONFIG_DM_MTD=y
CONFIG_MTD_SPI_NAND=y
CONFIG_SPI=y
CONFIG_DM_SPI=y
CONFIG_USB_MUSB_GADGET=y
CONFIG_USB_GADGET_DOWNLOAD=y
```

If a relevant option is disabled, a patch may still apply successfully even though the affected code is no longer compiled or executed.

### U-Boot Device Tree

The relevant device tree is located under:

```text
board/cra/epass/devicetree/uboot/
```

Its SPI0 and `spi-nand@0` nodes define:

- The SPI0 pins.
- The SPI-NAND chip select.
- The 80 MHz maximum frequency.
- The NAND device status.

### Partition Names

The current U-Boot configuration uses:

```text
spi-nand0=cranand
mtdparts=cranand:1M(u-boot)ro,6M(boot),-(rootfs)
```

Patch `0005` performs writes, verification, and bad-block skipping within the partition ranges supplied by MTD and DFU. When changing the partition layout, review all of the following together:

```text
board/cra/epass/uboot.defconfig
board/cra/epass/uboot.env
board/cra/epass/scripts/
Linux bootargs
```

### NAND Boot-Image Post-Processing

Buildroot first produces:

```text
u-boot-sunxi-with-spl.bin
```

It then runs:

```text
board/cra/epass/scripts/mknanduboot.sh
```

The script rearranges SPL for the 2 KiB NAND page layout, places main U-Boot at `0xD000`, and produces:

```text
output/images/u-boot-sunxi-with-nand-spl.bin
```

The patches in this directory do not perform that binary rearrangement. A successful patch application and U-Boot build do not prove that NAND image post-processing completed successfully; the final output file must be checked separately.

## Rebuilding After a Patch Change

From the Buildroot root directory in Linux or WSL, run:

```bash
make cra_epass_defconfig
make uboot-dirclean
make uboot
```

After changing a patch, use `uboot-dirclean` so Buildroot extracts a fresh U-Boot 2020.07 source tree and reapplies both the shared and CRA patch series from the beginning.

Running only:

```bash
make uboot-rebuild
```

will not normally repeat an already completed patch stage and may continue building the old source under `output/build/uboot-2020.07/`.

To generate the final images, continue with:

```bash
make
```

Build commands do not automatically write anything to a physical device. Flashing `u-boot-sunxi-with-nand-spl.bin` modifies the earliest boot area of the device and carries substantially more risk than replacing the main application. It must be authorized as a separate operation.

## Secondary Development Principles

1. Do not treat `output/build/uboot-2020.07/` as a permanent source directory. `uboot-dirclean` removes any changes made there.
2. Record permanent modifications as patches in this directory, or reorganize them into traceable commits when upgrading U-Boot.
3. Give new patches four-digit numeric prefixes, and state whether they depend on SPL, main U-Boot, the shared SUNIV patches, or another CRA patch.
4. Keep every patch in LF format to prevent Windows CRLF line endings from breaking patch context matching.
5. When changing the SPI parent clock or maximum rate, validate both SPL boot reads and main U-Boot MTD access.
6. Before adding an SPI-NAND ID, verify the manufacturer ID, device ID, page size, erase-block size, OOB layout, and ECC requirements.
7. When changing the DFU bad-block policy, distinguish permanent media failures from transient communication failures.
8. When changing the partition layout, review the U-Boot configuration, environment, image scripts, and Linux boot arguments together.
9. Do not remove original author attribution or hardware-vendor names. Record CRA modifications through additional documentation and Git history.
10. Patch application, successful compilation, successful image boot, and reliable DFU writes are four separate validation stages.

## Current Validation Status

The following checks have been completed for this directory:

- All five patches can be parsed as valid unified diffs.
- All CRA Linux and U-Boot patches use LF line endings.
- The shared SUNIV U-Boot patches and all five patches in this directory can be applied in Buildroot order.
- The corrected `0002-musb-force-fs.patch` modifies only the `drivers/usb/musb-new/` implementation that exists in U-Boot 2020.07.
- The current U-Boot configuration recognizes the CRA identity, SPI-NAND, DFU, MTD, SPL, and MUSB options.
- The current set of five patches has passed a U-Boot compilation check.

These checks do not replace testing of physical-device boot, serial output, sustained SPI-NAND access, or DFU failure recovery.

