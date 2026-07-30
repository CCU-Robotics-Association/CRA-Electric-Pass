# CRA Electric Pass Board-Level Patches

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the board-level source patches applied to Linux 5.4.99 and U-Boot 2020.07 while Buildroot builds CRA Electric Pass.

These patches provide F1C100S/F1C200S support that is missing from the selected upstream releases and implement the display, panel initialization, ADC, USB, SPI-NAND, audio, keyboard, GPIO, and DFU functions required by the current hardware.

## Directory Structure

```text
patch/
├─ linux/
│  ├─ 0000-f1c100s-gpadc-regs.patch
│  ├─ 0001-epass-icon.patch
│  ├─ 0002-panel-simple.patch
│  ├─ 0003-f1c100s-defe-debe-fix.patch
│  ├─ 0004-swap_rb_as_config.patch
│  ├─ 0005-gpadc-low-freq.patch
│  ├─ 0006-initalize-st7701.patch
│  ├─ 0007-srgn-drm-atomic-ioctl.patch
│  ├─ 0008-force-usb-fs-dt-switch.patch
│  ├─ 0009-m5stack-cardkb-driver.patch
│  ├─ 0010-i2s-and-es-driver.patch
│  ├─ 0011-fbcon-cra-width-hack.patch
│  ├─ 0012-gpio-backport-pulls.patch
│  └─ README.md
├─ uboot/
│  ├─ 0001-uart-pull.patch
│  ├─ 0002-musb-force-fs.patch
│  ├─ 0003-spi-nand-mx35lf1g.patch
│  ├─ 0004-f1c-spi-fix.patch
│  ├─ 0005-dfu-verify-block.patch
│  └─ README.md
└─ README.md
```

| Subdirectory | Target source | Patch count | Details |
| --- | --- | ---: | --- |
| `linux/` | Linux 5.4.99 | 13 | [Linux patch documentation](linux/README_EN.md) |
| `uboot/` | U-Boot 2020.07 | 5 | [U-Boot patch documentation](uboot/README_EN.md) |

## Buildroot Configuration Entry Point

The patch paths are selected by:

```text
board/cra/epass/cra_epass_defconfig
```

through the following settings:

```make
BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE="5.4.99"
BR2_LINUX_KERNEL_PATCH="board/allwinner/suniv-f1c100s/patch/linux board/cra/epass/patch/linux"
BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE="board/cra/epass/linux.defconfig"

BR2_TARGET_UBOOT_CUSTOM_VERSION_VALUE="2020.07"
BR2_TARGET_UBOOT_PATCH="board/allwinner/suniv-f1c100s/patch/u-boot board/cra/epass/patch/uboot"
BR2_TARGET_UBOOT_CUSTOM_CONFIG_FILE="board/cra/epass/uboot.defconfig"
```

Both Linux and U-Boot use a two-level patch structure:

```text
Official upstream source
      │
      ▼
Shared SUNIV/F1C100S patches
      │
      ▼
CRA Electric Pass board-level patches
      │
      ▼
Board defconfig
      │
      ▼
Build outputs
```

The shared patches provide the SoC-level foundation. The patches in this directory add the board-specific functionality required by CRA Electric Pass and carry forward adaptations inherited from the original project. The two levels have contextual dependencies, so the CRA patches cannot be applied directly to source that has not first received the shared patch series.

## Patch Application Order

Buildroot applies patches in lexical filename order:

```text
0000
0001
0002
……
0012
```

Within one subdirectory, a later patch may modify a file created or changed by an earlier patch. For example:

```text
Linux 0006
  └─ Creates drivers/staging/cra/ and the base Kconfig

Linux 0009
  └─ Adds the CardKB driver and Kconfig option to the same directory
```

Therefore:

- The numbers define both ordering and dependencies between patches.
- Renaming or moving a patch may change its application order.
- When changing context introduced by an earlier patch, verify that every later patch still matches.
- Successfully parsing every patch does not prove that the complete series can be applied in sequence.

## Linux Patch Responsibilities

The Linux patches primarily cover:

| Function | Related patches |
| --- | --- |
| GPADC registers, sampling, and filtering | `0000`, `0005` |
| Kernel framebuffer boot logo | `0001` |
| 384×640 panel timing | `0002` |
| DEFE/DEBE, scaling, and YUV display | `0003` |
| Red/blue channel swap | `0004` |
| ST7701 GPIO initialization | `0006` |
| Private DRM interface used by `drm_app_neo` | `0007` |
| Linux MUSB Full-Speed/High-Speed selection | `0008` |
| M5Stack CardKB | `0009` |
| I²S and audio codecs such as ES8311 | `0010` |
| Framebuffer text-console width workaround | `0011` |
| GPIO pull-up, pull-down, and runtime configuration | `0012` |

Higher-risk areas in the Linux patch series include:

- Display code in `0003` that is tied to the current resolution and vendor BSP tables.
- The private interface in `0007`, which directly operates DRM registers and userspace memory mappings.
- The unresolved initialization defect in the MUSB `power` local variable in `0008`.
- The global modification to the generic framebuffer console in `0011`.
- The backport affecting the Linux GPIO core and UAPI in `0012`.

See the following document for implementation details, dependencies, and known issues:

```text
linux/README_EN.md
```

## U-Boot Patch Responsibilities

The U-Boot patches primarily cover:

| Function | Related patch |
| --- | --- |
| UART0 TX/RX pull-ups | `0001` |
| Force U-Boot MUSB Gadget to Full-Speed | `0002` |
| Identify Macronix MX35LF1G SPI-NAND devices in SPL | `0003` |
| SUNIV SPI parent-clock and divider calculation | `0004` |
| DFU post-write verification, bad-block marking, and skipping | `0005` |

They participate in the following boot chain:

```text
Device power-on
   │
   ▼
SPL initializes DRAM, UART0, and SPI0
   │
   ▼
Identify SPI-NAND and load main U-Boot
   │
   ▼
Main U-Boot reads the environment and boot.itb
   │
   ├─ Boot Linux normally
   │
   └─ Enter USB DFU
          └─ Write, read back, verify, and handle bad blocks
```

See the following document for implementation details, partition relationships, and NAND image post-processing:

```text
uboot/README_EN.md
```

## Relationship to Other Directories

This directory cannot be maintained independently of the following files.

### Kernel and U-Boot Configurations

```text
board/cra/epass/linux.defconfig
board/cra/epass/uboot.defconfig
```

When a patch adds a new Kconfig feature, the corresponding defconfig must select it. Otherwise, the code may exist in the source tree without being compiled.

### Device Trees

```text
board/cra/epass/devicetree/linux/
board/cra/epass/devicetree/uboot/
```

The following values must remain consistent between the device trees and patches:

- `cra,epass-panel`.
- `cra,st7701-initseq`.
- `cra,swap-b-r`.
- `cra,usb-hs-enabled`.
- The SPI0 and SPI-NAND nodes.
- `spi-max-frequency`.
- CardKB, ES8311, and other expansion-device nodes.

### Main Application

Linux patch `0007-srgn-drm-atomic-ioctl.patch` and the following files:

```text
drm_app_neo/src/driver/srgn_drm.h
drm_app_neo/src/driver/drm_warpper.c
drm_app_neo/src/render/
drm_app_neo/src/overlay/
```

together define the private DRM ABI between the kernel and userspace. When changing an IOCTL number, structure, field width, or command meaning, update and rebuild both sides together.

### Image Scripts

```text
board/cra/epass/scripts/mknanduboot.sh
board/cra/epass/scripts/mkdt.sh
board/cra/epass/scripts/buildimage.sh
```

The patches only modify source code. Device-tree compilation, NAND SPL layout conversion, and final image generation are separate build stages handled by these scripts.

## Correct Rebuild Procedure After a Patch Change

### After Changing a Linux Patch

From the Buildroot root directory in Linux or WSL, run:

```bash
make cra_epass_defconfig
make linux-dirclean
make linux
```

### After Changing a U-Boot Patch

```bash
make cra_epass_defconfig
make uboot-dirclean
make uboot
```

### Complete System Build

```bash
make
```

After a patch changes, run the corresponding `*-dirclean` target so Buildroot extracts a clean source tree and reapplies the patch series. Running only:

```bash
make linux-rebuild
make uboot-rebuild
```

will not normally repeat an already completed patch stage and may continue using stale source under `output/build/`.

These build commands only generate files. They do not automatically write anything to a physical device. Flashing a complete image, replacing U-Boot, updating the kernel, and uploading the main application are separate operations.

## Line Endings and Windows Checkouts

The project previously encountered the following problems after a Windows checkout:

- CRLF shell scripts produced `/bin/sh^M` errors.
- Git symbolic links became regular files containing only the textual target path.
- Different patches mixed CRLF and LF line endings.
- An earlier patch created an LF file that a later CRLF patch could not match.

All Linux and U-Boot patches in this directory now use LF line endings.

After adding or changing patches, check them before committing:

```bash
file board/cra/epass/patch/linux/*.patch
file board/cra/epass/patch/uboot/*.patch
```

Do not conceal checkout problems by disabling Buildroot hash verification, skipping failed patches, or editing `output/build/` directly.

## Secondary Development Principles

1. Record permanent changes in patches, configuration files, device trees, or traceable upstream commits.
2. Do not make long-term changes directly under `output/build/linux-5.4.99/` or `output/build/uboot-2020.07/`.
3. Give new patches four-digit numeric prefixes and document their dependencies and scope.
4. After changing an earlier patch, recheck every later patch.
5. Keep Kconfig, defconfig, device trees, and drivers consistent.
6. Keep the private UAPI ABI consistent between the kernel and userspace.
7. Use LF for every text patch.
8. Preserve original author attribution and license information.
9. Do not treat successful patch application as proof of successful compilation.
10. Do not treat successful compilation as proof of correct operation on physical hardware.
11. Build validation does not authorize flashing or modifying a physical device.

## Current Validation Status

The following checks have been completed for this directory:

- All 13 Linux patches can be parsed as valid unified diffs.
- All 5 U-Boot patches can be parsed as valid unified diffs.
- Every patch uses LF line endings.
- The shared SUNIV Linux patches and CRA Linux patches can be applied in order to a clean Linux 5.4.99 source tree.
- Linux Kconfig recognizes the `CONFIG_CRA_EP_*` options.
- The shared SUNIV U-Boot patches and CRA U-Boot patches can be applied in Buildroot order.
- The current U-Boot patch series has passed compilation verification.

These checks show that the current patch series, naming, and configuration relationships remain suitable for further builds. They do not replace a complete system build or physical testing of boot, display, USB, audio, SPI-NAND, and DFU behavior.
