# Buildroot Board-Support Directory

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the shared SoC layers, board-specific configurations, and CRA Electric Pass firmware definitions supported by this Buildroot tree:

```text
board/
```

## Directory Layout

```text
board/
├── allwinner/              Shared Allwinner platform layer
│   ├── generic/            Shared boot, image, and rootfs support for several boards
│   ├── suniv-f1c100s/      Shared F1C100S/F1C200S support
│   └── sun8i-v3/           Shared V3s/S3 support
├── cra/
│   └── epass/              CRA Electric Pass board support
├── hatlab/
│   └── badge200/           HatLab BADGE200 support
├── hqembed/
│   └── hq050ips/           Incomplete placeholder directory
├── sipeed/
│   └── lichee/
│       ├── nano/           Sipeed Lichee Nano support
│       └── zero/           Incomplete historical Lichee Zero configuration
└── widora/
    └── mangopi/
        ├── r1/             Widora MangoPi R1 support
        ├── r2/             Widora MangoPi R2 support
        └── r3/             Widora MangoPi R3 support
```

## Support Overview

| Directory | Target |
| --- | --- |
| `allwinner/generic/` | Shared Allwinner boot and image layer |
| `allwinner/suniv-f1c100s/` | Shared F1C100S/F1C200S layer |
| `allwinner/sun8i-v3/` | Shared V3s/S3 layer |
| `cra/epass/` | CRA Electric Pass |
| `hatlab/badge200/` | HatLab BADGE200 |
| `hqembed/hq050ips/` | HQEmbed/HQ050IPS |
| `sipeed/lichee/nano/` | Sipeed Lichee Nano |
| `sipeed/lichee/zero/` | Sipeed Lichee Zero |
| `widora/mangopi/r1/` | Widora MangoPi R1 |
| `widora/mangopi/r2/` | Widora MangoPi R2 |
| `widora/mangopi/r3/` | Widora MangoPi R3 |

## Documentation for Subdirectories

| Directory | Documentation |
| --- | --- |
| `allwinner/` | [Shared Allwinner Board Support](allwinner/README_EN.md) |
| `cra/epass/` | [CRA Electric Pass Board-Support Package](cra/epass/README_EN.md) |
| `hatlab/` | [HatLab Board Support](hatlab/README_EN.md) |
| `hqembed/` | [HQEmbed Board Directory](hqembed/README_EN.md) |
| `sipeed/` | [Sipeed Board Support](sipeed/README_EN.md) |
| `widora/` | [Widora MangoPi Board Support](widora/README_EN.md) |

## Buildroot Configuration Entry Points

Buildroot normally loads board configurations through `configs/` at the repository root rather than requiring users to enter paths under `board/` directly.

The repository currently provides these entries:

| Buildroot command | Canonical configuration source |
| --- | --- |
| `make cra_epass_defconfig` | `board/cra/epass/cra_epass_defconfig` |
| `make hatlab_badge200_defconfig` | `board/hatlab/badge200/hatlab_badge200_defconfig` |
| `make sipeed_lichee_nano_defconfig` | `board/sipeed/lichee/nano/sipeed_lichee_nano_defconfig` |
| `make widora_mangopi_r1_defconfig` | `board/widora/mangopi/r1/widora_mangopi_r1_defconfig` |
| `make widora_mangopi_r2_defconfig` | `board/widora/mangopi/r2/widora_mangopi_r2_defconfig` |
| `make widora_mangopi_r3_defconfig` | `board/widora/mangopi/r3/widora_mangopi_r3_defconfig` |

HQEmbed and Lichee Zero currently have no usable entries under `configs/`.

### Windows Symbolic-Link Issue

The `configs/*_defconfig` files are normally Git symbolic links to the canonical configuration files under `board/`. If Git symbolic-link support is disabled on Windows, a link may be checked out as a regular file containing only a relative path, for example:

```text
../board/cra/epass/cra_epass_defconfig
```

Such a regular file is not a complete defconfig. Building with it may fail or produce an incorrect configuration. Verify the entry in Linux or WSL with:

```sh
ls -l configs/cra_epass_defconfig
```

Restore a valid symbolic link when necessary. Do not copy a link placeholder and treat it as a new canonical board configuration.

## Shared and Board-Specific Layers

### Shared Layers

Files under `board/allwinner/` serve multiple hardware targets and are appropriate for:

- SoC controller, clock, interrupt, and shared peripheral nodes;
- Linux or U-Boot patches used by multiple boards;
- shared rootfs initialization scripts;
- image layouts, boot environments, and splash images shared by legacy targets.

A change to a shared layer may affect several build targets. Always search the complete reference chain before modifying it.

For example:

```text
board/allwinner/generic/splash.bmp
```

is used by the shared image flows for targets such as HatLab BADGE200, Sipeed Lichee Nano, and Widora MangoPi. CRA currently uses its own image-generation scripts and does not package this BMP into the CRA image.

### Board-Specific Layers

A board-specific directory should contain details that apply only to that hardware, such as:

- LCD resolution and timing;
- backlight, button, and power GPIOs;
- flash type and partition layout;
- touch, camera, audio, and sensor nodes;
- USB product identity;
- board-specific rootfs services;
- the final image-generation flow.

CRA-specific hardware changes should normally go under `board/cra/epass/` rather than into a legacy development-board directory.

## Common File Responsibilities

| File or directory | Responsibility |
| --- | --- |
| `*_defconfig` | Selects the toolchain, kernel, U-Boot, packages, rootfs, and image scripts |
| `linux.defconfig` | Configures Linux kernel features and drivers |
| `uboot.defconfig` | Configures U-Boot, SPL, boot media, display, and download functions |
| `uboot.env` | Defines the default environment and boot commands compiled into U-Boot |
| `uEnv.txt` | Provides an external text-form U-Boot environment template |
| `devicetree/linux/` | Describes hardware used during Linux runtime |
| `devicetree/uboot/` | Describes hardware needed during the U-Boot stage |
| `patch/linux/` | Contains patches applied to a pinned Linux version |
| `patch/u-boot/` | Contains patches applied to a pinned U-Boot version |
| `rootfs/` | Contains files overlaid onto the target root filesystem during the build |
| `scripts/` | Generates device trees, FIT, UBI, SD-card, or NAND images |
| `tools/` | Contains development-host utilities that do not run directly on the target device |

## Secondary-Development Principles

1. Use defconfig and script references to confirm whether a file participates in the current target build.
2. Check every downstream board before changing a shared layer to avoid cross-target regressions.
3. Base device-tree parameters on schematics, chip documentation, and physical validation rather than directory similarity.
4. Keep the Linux DTS, U-Boot DTS, partition table, boot arguments, and image layout consistent.
5. Preserve Unix LF line endings, executable permissions, and Git symbolic links in scripts and configuration entries.
6. Do not perform text replacement or line-ending conversion on binary assets; record their source and hash when updating them.
7. Do not treat generated Buildroot output as the canonical source for board configuration changes.
8. Keep CRA-specific changes in the CRA layer and maintain third-party board directories only for their own hardware.
9. Keep build validation separate from physical-device writes; every write operation requires explicit confirmation of the target device.
