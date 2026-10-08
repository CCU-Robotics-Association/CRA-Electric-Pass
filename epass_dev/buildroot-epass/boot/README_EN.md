# Buildroot Bootloader Build Directory

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains Buildroot integration for bootloaders, first-stage boot firmware, and trusted execution environment firmware:

```text
boot/
```

## Directory Structure

```text
boot/
├── Config.in
├── common.mk
├── afboot-stm32/
├── arm-trusted-firmware/
├── at91bootstrap/
├── at91bootstrap3/
├── at91dataflashboot/
├── barebox/
├── binaries-marvell/
├── boot-wrapper-aarch64/
├── grub2/
├── gummiboot/
├── lpc32xxcdl/
├── mv-ddr-marvell/
├── mxs-bootlets/
├── opensbi/
├── optee-os/
├── s500-bootloader/
├── shim/
├── syslinux/
├── uboot/
└── vexpress-firmware/
```

## Top-Level Files

### `Config.in`

This file creates the `Bootloaders` menu in the Buildroot configuration interface and sources the `Config.in` file from each subdirectory.

It only organizes the available configuration entries; it does not cause every bootloader to be built. A package is selected only when its corresponding `BR2_TARGET_*` option is enabled and its architecture requirements are satisfied.

### `common.mk`

The file contains:

```make
include $(sort $(wildcard boot/*/*.mk))
```

It loads the `.mk` files for all bootloader packages in sorted order, making their download, configuration, build, and installation rules available to the main Buildroot build system.

## Subdirectories

| Subdirectory | Primary purpose |
| --- | --- |
| `afboot-stm32/` | Small bootloader for STM32 platforms |
| `arm-trusted-firmware/` | Trusted Firmware-A/ATF boot stages for ARMv7-A and ARMv8-A platforms |
| `at91bootstrap/` | Legacy first-stage bootloader for Atmel AT91 devices |
| `at91bootstrap3/` | Third-generation first-stage bootloader for Atmel/Microchip AT91 devices |
| `at91dataflashboot/` | AT91 DataFlash boot support |
| `barebox/` | Barebox bootloader and its auxiliary components |
| `binaries-marvell/` | SCP firmware required when building ATF for Marvell Armada platforms |
| `boot-wrapper-aarch64/` | Lightweight wrapper for booting a kernel in AArch64 software simulators |
| `grub2/` | GNU GRUB 2 for x86, EFI, and selected ARM platforms |
| `gummiboot/` | Simple boot manager for x86 UEFI systems |
| `lpc32xxcdl/` | Kickstart and S1L boot components for NXP LPC32xx devices |
| `mv-ddr-marvell/` | DDR training sources required by ATF on Marvell Armada platforms |
| `mxs-bootlets/` | First-stage bootlets for Freescale/NXP i.MX23 and i.MX28 devices |
| `opensbi/` | RISC-V SBI firmware implementation |
| `optee-os/` | ARM TrustZone secure-world images and TA development components |
| `s500-bootloader/` | First-stage bootloader for Actions Semiconductor S500 devices |
| `shim/` | Signed chain loader for UEFI Secure Boot environments |
| `syslinux/` | Bootloader collection for x86 BIOS, PXE, ISO, and EFI systems |
| `uboot/` | Generic U-Boot build integration |
| `vexpress-firmware/` | Firmware for ARM Versatile Express platforms |

## Common Files in Each Subdirectory

Each bootloader package typically contains the following types of files:

| Type | Purpose |
| --- | --- |
| `Config.in` | Defines enablement, version, platform, output format, and additional options |
| `<package>.mk` | Defines the source location, dependencies, build commands, and installation destinations |
| `<package>.hash` | Stores checksums for source archives and license files |
| `*.patch` | Fixes compatibility, build, or security issues in specific upstream versions |
| Configuration and resource files | Used by packages such as GRUB and Gummiboot to generate their final boot configuration |

Most of these files come from upstream Buildroot. They are build inputs rather than generated artifacts. Whether a patch is applied depends on the selected package version and Buildroot's version-specific patch rules.

## The `uboot/` Directory

`boot/uboot/` is the part of this directory that is directly used by CRA Electric Pass:

```text
boot/uboot/
├── Config.in
├── uboot.mk
├── uboot.hash
├── 2015.07/
├── 2016.07/
└── 2016.09.01/
```

### `uboot/Config.in`

This file defines Buildroot's U-Boot configuration options, including:

- U-Boot version and source location;
- the Kconfig or legacy build system;
- an upstream defconfig or a custom configuration file;
- additional configuration fragments and patch directories;
- host dependencies such as DTC, OpenSSL, and Python;
- output formats such as `u-boot.bin`, `u-boot.img`, and `u-boot.itb`;
- SPL/TPL output files;
- U-Boot environment images;
- the `boot.scr` boot script;
- custom U-Boot DTS/DTSI files copied before the build;
- additional Make options.

### `uboot/uboot.mk`

This file is responsible for:

1. Determining the U-Boot version, source archive, and download location from the configuration;
2. declaring the cross-toolchain and host-tool dependencies;
3. downloading remote patches and applying local patch directories in order;
4. loading the custom U-Boot configuration;
5. copying custom DTS/DTSI files into the temporary U-Boot source tree;
6. invoking the U-Boot Makefile to compile the project;
7. copying the selected U-Boot binaries and SPL files to `output/images/`;
8. generating an environment image or `boot.scr` when requested;
9. providing targets for `uboot-menuconfig`, configuration saving, rebuilding, and cleaning.

### `uboot/uboot.hash`

This file stores SHA-256 checksums for Buildroot's default U-Boot source archive and license files. The current file records U-Boot 2020.01, which was the default in Buildroot 2020.02.7. CRA selects the official U-Boot 2020.07 release as a custom version, so this file must not be treated as the CRA U-Boot version list.

### Version-Specific Patch Directories

`2015.07/`, `2016.07/`, and `2016.09.01/` contain Buildroot compatibility patches for older U-Boot releases. They primarily address compatibility between those releases and ARC toolchains or legacy build configuration behavior.

## Actual U-Boot Configuration for CRA Electric Pass

`board/cra/epass/cra_epass_defconfig` currently selects:

| Item | Current value |
| --- | --- |
| Bootloader | U-Boot |
| U-Boot version | `2020.07` |
| Build system | Kconfig |
| U-Boot configuration | `board/cra/epass/uboot.defconfig` |
| Common patch directory | `board/allwinner/suniv-f1c100s/patch/u-boot` |
| CRA patch directory | `board/cra/epass/patch/uboot` |
| Device Tree Compiler | DTC required |
| Default U-Boot output | `u-boot.bin` |
| SPL | Enabled |
| SPL/combined image | `u-boot-sunxi-with-spl.bin` |
| Common U-Boot DTSI | `board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi` |
| CRA U-Boot DTS | `board/cra/epass/devicetree/uboot/suniv-f1c100s-generic.dts` |

`board/cra/epass/uboot.defconfig` additionally enables SUNIV, SPL, SPI, MTD, DFU, USB Mass Storage, USB Gadget, and related features. It also specifies:

```text
CONFIG_DEFAULT_DEVICE_TREE="suniv-f1c100s-generic"
CONFIG_DEFAULT_ENV_FILE="../../../board/cra/epass/uboot.env"
CONFIG_BOOTCOMMAND="run distro_bootcmd;"
```

CRA boot behavior therefore cannot be determined from `boot/uboot/` alone. The following files and directories must be considered together:

```text
board/cra/epass/cra_epass_defconfig
board/cra/epass/uboot.defconfig
board/cra/epass/uboot.env
board/cra/epass/uEnv.txt
board/cra/epass/devicetree/uboot/
board/cra/epass/patch/uboot/
board/allwinner/suniv-f1c100s/patch/u-boot/
```

## CRA Boot Chain

The simplified CRA Electric Pass boot sequence is:

```text
Allwinner SUNIV Boot ROM
          │
          ▼
     U-Boot SPL
 Initialize clocks and DRAM
          │
          ▼
      Full U-Boot
 Read the built-in and external environments
          │
          ▼
       boot.itb
 Extract Linux, the base DTB, and DTBOs
          │
          ▼
     Linux 5.4.99
          │
          ▼
       CRA rootfs
```

Buildroot first generates `u-boot-sunxi-with-spl.bin`. The CRA post-image script then generates the following image according to the SPI-NAND page and block layout:

```text
output/images/u-boot-sunxi-with-nand-spl.bin
```

Another post-image stage packages the Linux kernel, base DTB, and Device Tree overlays into `boot.itb`. This image-generation logic belongs to `board/cra/epass/scripts/`; it is not performed by the top-level `boot/` directory alone.

## Patch Application Order

CRA configures the U-Boot patch paths in the following order:

```text
board/allwinner/suniv-f1c100s/patch/u-boot
board/cra/epass/patch/uboot
```

`uboot.mk` processes local files and directories in list order. The `*.patch` files within each directory are applied in a stable sorted order. Therefore:

- common SUNIV/F1C100S/F1C200S support is applied first;
- CRA Electric Pass-specific fixes are applied afterward;
- CRA patches may build on functionality introduced by the common patches;
- new patches should use consecutive, sortable numbers;
- CRA-specific patches should not be placed in `boot/uboot/`.

## Boundary Between Source Files and Generated Artifacts

### Files That Belong in Version Control

| Content | Canonical location |
| --- | --- |
| Buildroot bootloader package rules | `boot/` |
| CRA Buildroot target configuration | `board/cra/epass/cra_epass_defconfig` |
| CRA U-Boot configuration | `board/cra/epass/uboot.defconfig` |
| CRA default U-Boot environment | `board/cra/epass/uboot.env` |
| CRA external environment template | `board/cra/epass/uEnv.txt` |
| CRA U-Boot Device Tree sources | `board/cra/epass/devicetree/uboot/` |
| CRA U-Boot patches | `board/cra/epass/patch/uboot/` |
| Common SUNIV patches | `board/allwinner/suniv-f1c100s/patch/u-boot/` |
| CRA image-generation scripts | `board/cra/epass/scripts/` |

### Generated During the Build and Not Meant for Direct Maintenance

| Content | Typical location |
| --- | --- |
| Extracted and patched U-Boot source tree | `output/build/uboot-2020.07/` |
| Temporary U-Boot `.config` | `output/build/uboot-2020.07/.config` |
| Compiled objects and intermediate files | `output/build/uboot-2020.07/` |
| Standard U-Boot binary | `output/images/u-boot.bin` |
| SUNIV SPL/U-Boot combined image | `output/images/u-boot-sunxi-with-spl.bin` |
| SPI-NAND boot image | `output/images/u-boot-sunxi-with-nand-spl.bin` |

Temporary diagnostics may be performed in `output/build/uboot-2020.07/`, but manual changes there will be lost when the tree is cleaned or extracted again. Permanent changes must be written back to the CRA configuration, environment, Device Tree, or patch directories.

## Common Build Commands

Run the following commands from the Buildroot repository root in Linux or a correctly configured WSL environment.

### Load the CRA Configuration and Perform a Full Build

```sh
make cra_epass_defconfig
make -j$(nproc)
```

A full build also runs the CRA post-image scripts and generates the final SPI-NAND and SD boot images. Building does not automatically flash a physical device.

### Open the U-Boot Configuration Interface

```sh
make uboot-menuconfig
```

This command modifies the U-Boot `.config` in the temporary build tree. After validation, save the result to the canonical project configuration file.

### Save the U-Boot Defconfig

```sh
make uboot-update-defconfig
```

The current CRA target uses `BR2_TARGET_UBOOT_CUSTOM_CONFIG_FILE`, so the destination is:

```text
board/cra/epass/uboot.defconfig
```

If a full `.config` is specifically required, use:

```sh
make uboot-update-config
```

This project should normally maintain the compact, reviewable `uboot.defconfig` instead.

### Recompile U-Boot Only

```sh
make uboot-rebuild -j$(nproc)
make
```

The first command restarts from the U-Boot build stage. The second allows downstream image and post-image stages to refresh their outputs from the new U-Boot artifacts.

`uboot-rebuild` does not download or extract the source again, nor does it reapply patches from a clean tree.

### Rebuild from a Clean U-Boot Source Tree

```sh
make uboot-dirclean
make -j$(nproc)
```

Use `uboot-dirclean` in the following situations:

- a U-Boot patch was added, removed, renamed, or reordered;
- an existing patch was changed and must be validated against a clean source tree;
- the U-Boot version or source location changed;
- the temporary build tree was modified manually;
- stale build state may be affecting the result.

This command only removes Buildroot's U-Boot build directory. It does not erase or overwrite a physical device.
