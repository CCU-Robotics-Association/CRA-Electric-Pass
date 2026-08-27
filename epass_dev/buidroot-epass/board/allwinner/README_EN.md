# Shared Allwinner Board Support

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains platform support shared by several Allwinner boards in this Buildroot tree:

```text
board/allwinner/
```

## Directory Layout

```text
allwinner/
├── generic/                 Reusable boot and image support for Allwinner boards
│   ├── uboot.env            Shared default U-Boot environment
│   ├── kernel.its           Shared FIT image description
│   ├── splash.bmp           Shared boot splash
│   ├── genimage-*.cfg       SD, SPI-NOR, and SPI-NAND image layouts
│   ├── scripts/             Shared image-generation scripts
│   ├── rootfs/              Shared root filesystem overlay
│   └── legacy/              Legacy boot environment and flashing-image layout
├── suniv-f1c100s/           Shared SUNIV support for F1C100S/F1C200S devices
│   ├── linux.defconfig      Shared Linux configuration baseline
│   ├── uboot.defconfig      Shared U-Boot configuration baseline
│   ├── devicetree/          Shared Linux and U-Boot SoC device trees
│   ├── patch/               Shared SUNIV Linux and U-Boot patches
│   └── rootfs/              Shared SUNIV root filesystem overlay
└── sun8i-v3/                Shared Allwinner V3s/S3 platform support
    ├── linux.defconfig      V3s/S3 Linux configuration baseline
    └── devicetree/          Shared V3s and S3 SoC device trees
```

## Integration with CRA Electric Pass

The source configuration for the current CRA board target is:

```text
board/cra/epass/cra_epass_defconfig
```

It assembles the system in the following layers:

```text
Shared Allwinner layer
board/allwinner/generic/
        ↓
Shared SUNIV F1C100S layer
board/allwinner/suniv-f1c100s/
        ↓
CRA Electric Pass board layer
board/cra/epass/
```

The effective references are:

| Shared file or directory | Current role |
| --- | --- |
| `generic/rootfs/` | First rootfs overlay layer; installs the shared `preinit` script |
| `suniv-f1c100s/rootfs/` | Second rootfs overlay layer; installs SUNIV platform utilities |
| `suniv-f1c100s/patch/linux/` | Applies shared SUNIV kernel patches before the CRA Linux patches |
| `suniv-f1c100s/patch/u-boot/` | Applies shared SUNIV boot support before the CRA U-Boot patches |
| `suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi` | Supplies SoC controllers, clocks, interrupts, DMA, and peripheral base nodes to the CRA Linux DTS |
| `suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi` | Supplies SUNIV SoC base nodes to the CRA U-Boot DTS |

The following files exist in the shared tree, but the current CRA configuration provides its own replacements and does not reference them directly:

```text
generic/uboot.env
generic/kernel.its
generic/genimage-*.cfg
generic/scripts/
generic/splash.bmp
suniv-f1c100s/linux.defconfig
suniv-f1c100s/uboot.defconfig
sun8i-v3/
```

## `generic/`: Shared Allwinner Layer

### `uboot.env`

This is a shared U-Boot environment for several Allwinner boards. It provides the logic to:

- locate and boot `kernel.itb` from MMC0, MMC1, SPI-NOR, or SPI-NAND;
- scan available boot entries across multiple media;
- load and display `splash.bmp`;
- define DFU targets for MMC, SPI flash, and MTD devices;
- fall back to FEL or DFU if no normal boot medium is available;
- define shared image offsets and read lengths for SPI-NOR and SPI-NAND.

It follows the shared image layout and does not match the NAND partitions, FIT contents, or boot flow currently used by CRA Electric Pass. Changes to the pass's default environment belong in:

```text
board/cra/epass/uboot.env
```

Do not use this shared file as a substitute.

### `kernel.its`

This file is the shared FIT (Flattened Image Tree) description. It packages the following into `kernel.itb`:

- the ARM Linux `zImage`;
- one `devicetree.dtb`;
- CRC32 checks for the kernel and device tree;
- the default boot configuration, `conf@0`.

Both the shared kernel load address and entry address are `0x80000000`. The current CRA project supports multiple screen, interface, and extension DTBOs, so it uses its own `scripts/kernel.its` instead of this file.

### `genimage-*.cfg`

Three configuration files define the shared image layouts:

| File | Target medium | Main layout |
| --- | --- | --- |
| `genimage-sdcard.cfg` | SD card | Places U-Boot at `0x2000`, followed by a FAT boot partition and an ext4 rootfs partition |
| `genimage-nor.cfg` | 16 MiB SPI-NOR | Places U-Boot, the splash image, kernel, and read-only rootfs at fixed offsets |
| `genimage-nand.cfg` | 128 MiB SPI-NAND | Places U-Boot, the splash image, kernel, and read-only rootfs at fixed offsets |

The shared SPI flash layout uses these main offsets:

| Content | Offset | Reserved size |
| --- | --- | --- |
| U-Boot | `0x000000` | `0x080000` (512 KiB) |
| Boot splash | `0x080000` | `0x080000` (512 KiB) |
| Kernel/FIT | `0x100000` | `0x500000` (5 MiB) |
| rootfs | `0x600000` | Remaining space on the medium |

These values belong to the shared image scheme and are not the authoritative CRA Electric Pass partition definition. Any layout change must be checked against the U-Boot environment, Linux `bootargs`, device-tree partitions, image-generation scripts, and flashing configuration.

### `scripts/`

| File | Purpose |
| --- | --- |
| `mknanduboot.sh` | Rearranges SPL/U-Boot for a 2 KiB NAND page layout so that it can boot from SPI-NAND |
| `genimage.sh` | Builds the shared FIT, copies the splash image, converts NAND U-Boot, and invokes `genimage` to create SD, NOR, and NAND images |

These scripts run during Buildroot's post-image stage and depend on Buildroot variables such as `BINARIES_DIR` and `HOST_DIR`. They are not intended to be run as standalone scripts outside the Buildroot environment.

### `rootfs/preinit`

This script checks the kernel command line for an `overlayfsdev=` parameter. If present, it attempts to:

1. mount the specified MTD device as JFFS2 at `/overlay`;
2. use the current read-only root filesystem as the lower directory;
3. create the overlayfs upper and work directories;
4. mount the merged filesystem at `/tmp`;
5. `chroot` into the merged system and continue booting.

If `overlayfsdev=` is absent, the script invokes the normal `init` process directly. Including this file in the rootfs does not by itself prove that a device uses overlayfs; the effective behavior also depends on the kernel command line and init flow.

### `legacy/`

`legacy/` preserves an older compatibility path:

- `uboot.env` loads a separate `zImage` and DTB rather than the current shared FIT;
- `genimage-flasher.cfg` creates a FAT flashing medium that contains NOR and NAND system images.

Do not mix these files into the current CRA build unless you are specifically maintaining hardware that still uses this legacy boot scheme.

## `suniv-f1c100s/`: Shared SUNIV Layer

This directory provides common support for the F1C100S/F1C200S family. Most of the ARM926T, clock controller, DMA, USB, audio, SPI flash, and on-chip peripheral foundations used by CRA Electric Pass come from this layer.

### Shared Device Trees

| File | Consumer | Purpose |
| --- | --- | --- |
| `devicetree/linux/suniv-f1c100s.dtsi` | Linux 5.4 series | Defines SoC-level CPU, clock, interrupt, reset, DMA, GPIO, and on-chip controller nodes |
| `devicetree/uboot/suniv-f1c100s.dtsi` | U-Boot 2020.07 series | Defines the SUNIV SoC-level nodes needed during the U-Boot boot stage |

The shared `.dtsi` files describe SoC capabilities. They should not contain board-specific details such as the CRA LCD, buttons, external audio codec, or interface selection. CRA hardware connections belong in the `.dts` files and overlays under `board/cra/epass/devicetree/`.

The shared Linux device tree carries:

```text
SPDX-License-Identifier: (GPL-2.0+ OR X11)
```

The shared U-Boot device tree also retains the original author's copyright notice. Do not remove these notices when making changes.

### Shared Linux Patches

`patch/linux/` currently contains 16 numbered patches:

| Number | Main change |
| --- | --- |
| `0001` | Adds SUNIV USB support |
| `0002` | Corrects CCU definitions |
| `0003` | Adds DMA controller support |
| `0004` | Adds support for the on-chip audio codec |
| `0005` | Adds packed-format support to sun4i CSI |
| `0006` | Adds device-tree support to `rfkill-gpio` |
| `0007` | Corrects the SPI-NAND bad-block marker size |
| `0008` | Adds CedarX driver support |
| `0009` | Adds compatibility for early GDF5 A-series NAND devices |
| `0010` | Adds AW9523B GPIO expander support |
| `0011` | Allows `phy-sun4i-usb` to use nested interrupts |
| `0012` | Adds AXP199 support |
| `0013` | Adds XT25F128 SPI-NOR support |
| `0014` | Adds GD5F1GQ5UExxG SPI-NAND support |
| `0015` | Adds compatibility handling for older W25N01G devices |
| `0016` | Adds an on-die ECC quirk switch for SPI-NAND |

The CRA build applies these shared patches first and then applies `board/cra/epass/patch/linux/`. Later patches may depend on Kconfig options, drivers, or device-tree bindings introduced by earlier ones. Do not select patches only by filename, renumber them casually, or reorder them without dependency analysis.

### Shared U-Boot Patch

`patch/u-boot/0001-v2020.07.11.patch` is a large SUNIV platform patch against the U-Boot `2020.07` baseline. It adds ARM926EJ-S/SUNIV boot support, SPL, DRAM, clocks, GPIO, PWM, SPI, NAND, and the associated device-tree support.

The patch is tightly coupled to a specific U-Boot version. When upgrading U-Boot, re-evaluate whether the patch still applies and whether upstream U-Boot has incorporated equivalent functionality. Do not continue a build by ignoring failed hunks.

### `linux.defconfig`

This file is the shared SUNIV Linux configuration baseline. Its header identifies it as a full configuration generated by the Linux configuration system for Linux `5.4.92` with an ARM EABI toolchain. It is not the source of the current CRA Linux `5.4.99` configuration and must not be confused with `board/cra/epass/linux.defconfig`.

Because it is a complete generated configuration rather than a minimal `savedefconfig`, changes should be revalidated through the target Linux source tree's Kconfig tools. This avoids retaining options that have been renamed, disabled by unmet dependencies, or removed from the target kernel version.

### `uboot.defconfig`

This is the shared SUNIV U-Boot configuration baseline. It mainly enables:

- the ARM architecture and `CONFIG_MACH_SUNIV`;
- SPL and SUNXI SPI boot support;
- the shared `suniv-f1c100s-generic` device tree;
- a 408 MHz system clock and 168 MHz DRAM clock;
- MMC, SPI-NOR, SPI-NAND, and MTD;
- USB Mass Storage, DFU, and the MUSB gadget stack;
- `generic/uboot.env` as the default environment;
- shared 800×480 LCD parameters.

The current CRA project uses its own U-Boot configuration and environment. Changing this file does not change the electronic pass firmware.

### `rootfs/usr/sbin/mtd`

This file is a precompiled 32-bit ARM ELF user-space program, not a shell script or source file. Because the current CRA configuration includes `suniv-f1c100s/rootfs/`, the program is copied to:

```text
/usr/sbin/mtd
```

The directory does not contain corresponding source code for this binary. Keep the following constraints in mind:

- do not open and rewrite it in a text editor or convert its line endings;
- it cannot run directly on a Windows host;
- before replacing it, verify the target ABI, dynamic loader, dependencies, license, and provenance;
- for long-term maintenance, trace its build source and preserve a reproducible source-based build procedure.

## `sun8i-v3/`: Shared V3s/S3 Layer

This directory targets the Allwinner V3s/S3 platform and contains:

- `linux.defconfig`: a full generated Linux `5.4.35` configuration whose header identifies an ARM hard-float toolchain;
- `devicetree/linux/sun8i-v3s.dtsi`: the shared V3s SoC device tree;
- `devicetree/linux/sun8i-s3.dtsi`: the shared S3 SoC device tree.

V3s/S3 and the SUNIV F1C100S/F1C200S used by CRA Electric Pass are different configuration layers. The current `cra_epass_defconfig` does not reference this directory. Do not interchange their defconfigs or `.dtsi` files merely because both platforms are made by Allwinner.

## Source and Generated-File Boundaries

The configuration files, DTS sources, patches, scripts, environment files, and image layouts in this directory are build inputs and belong under version control. Linux, U-Boot, DTB, and firmware images generated by a build are normally written to:

```text
output/build/
output/host/
output/images/
```

Generated output is not a substitute for the source files in this directory. Direct edits under `output/build/linux-*` or `output/build/uboot-*` are lost when the build directory is cleaned or rebuilt.

Preserve the boundaries between file types:

| Type | Examples | Requirements |
| --- | --- | --- |
| Text configuration | `*.defconfig`, `*.env`, `*.cfg` | Use UTF-8/LF and validate changes with the relevant configuration or build tool |
| Device-tree source | `*.dts`, `*.dtsi` | Validate through the matching DTC, kernel, or U-Boot device-tree flow |
| Patch | `*.patch` | Line endings, path prefixes, order, and target source version all affect whether it applies |
| Shell script | `*.sh`, `preinit` | Preserve LF endings, the shebang, and executable permissions |
| Binary asset | `splash.bmp`, `usr/sbin/mtd` | Do not perform text replacement or line-ending conversion |

## Coupled Changes

| Modified location | Also verify |
| --- | --- |
| `generic/uboot.env` | Every board-level U-Boot configuration that references it, along with its boot media and image layout |
| `generic/kernel.its` | Kernel/DTB filenames, load addresses, generation scripts, and U-Boot commands |
| `generic/genimage-*.cfg` | U-Boot offsets, MTD partitions, image sizes, and flashing tools |
| `generic/rootfs/preinit` | Kernel `bootargs`, JFFS2/overlayfs support, and the init flow |
| Shared SUNIV `.dtsi` | Every board DTS that includes it, not only CRA Electric Pass |
| Shared Linux patches | Linux version, shared defconfig, later CRA patches, and every SUNIV board that consumes them |
| Shared U-Boot patch | U-Boot version, SPL/DRAM/SPI boot support, and later CRA patches |
| SUNIV `rootfs/` | Every target that overlays this directory and its target ABI |

Changes that apply only to CRA hardware or its user interface should go in `board/cra/epass/` or the relevant application directory. A change belongs in this shared directory only when it genuinely applies to multiple Allwinner or SUNIV boards.

## Build and Validation

The current CRA Electric Pass build entry point remains:

```sh
make cra_epass_defconfig
make
```

When validating a change to shared support, check at least the following:

1. every patch still applies cleanly, in numbered order, to the pinned Linux or U-Boot version;
2. Linux and U-Boot device trees compile and CRA board-level DTS references remain valid;
3. ARM binaries in the rootfs remain compatible with the current EABI soft-float user space;
4. U-Boot, FIT, DTB/DTBO, UBI, and SD images are generated successfully;
5. other board configurations that consume the modified shared files have not been broken unintentionally.

Builds and static checks only generate and validate software artifacts; they do not flash a physical device. Before any operation involving boot media, NAND page layout, partitions, or U-Boot, separately confirm the hardware model, target medium, backup, and recovery procedure.
