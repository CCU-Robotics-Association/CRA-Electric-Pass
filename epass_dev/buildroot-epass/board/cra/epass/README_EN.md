# CRA Electric Pass Board Support Package

Read this document in another language: [English](README_EN.md), [中文](README.md).

This directory contains the Buildroot board support package for CRA Electric Pass:

```text
board/cra/epass/
```

It combines the Buildroot system configuration, Linux and U-Boot configurations, device trees, patches, rootfs overlays, and post-image scripts into a complete, buildable firmware definition. The target processor is the ARM926T used by the Allwinner SUNIV F1C100S/F1C200S family, and the user space uses the ARM EABI soft-float ABI.

## Directory Structure

```text
epass/
├── cra_epass_defconfig   Buildroot board configuration
├── linux.defconfig       Linux 5.4.99 kernel configuration
├── uboot.defconfig       U-Boot 2020.07 configuration
├── uboot.env             Default environment compiled into U-Boot
├── uEnv.txt              Text environment template used during flashing
├── devicetree/           U-Boot and Linux device trees
├── patch/                Linux and U-Boot patch series
├── rootfs/               Root filesystem overlay
├── scripts/              Device-tree and firmware image generation scripts
└── tools/                Host-side resource generation tools
```

| Path | Primary responsibility | Details |
| --- | --- | --- |
| [`devicetree/`](devicetree/README_EN.md) | Describes boot hardware, the base board, display panels, interfaces, and external devices | [Device-tree documentation](devicetree/README_EN.md) |
| [`patch/`](patch/README_EN.md) | Adds SUNIV and Electric Pass hardware support to fixed Linux and U-Boot versions | [Patch documentation](patch/README_EN.md) |
| [`rootfs/`](rootfs/README_EN.md) | Overlays `/bin`, `/etc`, `/root`, `/assets`, and `/app` in the target system | [Rootfs documentation](rootfs/README_EN.md) |
| [`scripts/`](scripts/README_EN.md) | Generates NAND U-Boot, DTB/DTBO, FIT, UBI, and SD boot images | [Image-script documentation](scripts/README_EN.md) |
| [`tools/`](tools/README_EN.md) | Generates shutdown prompts and other resources on the development computer | [Tool documentation](tools/README_EN.md) |

## Buildroot Configuration Entry Points

The canonical source file for the board configuration is:

```text
board/cra/epass/cra_epass_defconfig
```

The user-facing Buildroot entry point is:

```text
configs/cra_epass_defconfig
```

This entry should point to the board configuration source so that the project configuration can be loaded with:

```sh
make cra_epass_defconfig
```

In a Linux checkout that preserves Git symbolic links, `configs/cra_epass_defconfig` is normally a symbolic link to:

```text
../board/cra/epass/cra_epass_defconfig
```

When symbolic-link support is not enabled on Windows, Git may check it out as a regular file containing only the target path. Such a file is not a valid Buildroot defconfig. Before building, verify under Linux or WSL that this entry is either a valid symbolic link or a complete copy of the configuration. Do not mistake a Windows-generated link placeholder for a working configuration file.

## `cra_epass_defconfig`

This file is the top-level assembly point for the board support package. It primarily determines the following:

- use of Buildroot's internal toolchain, glibc, and C++ support;
- the target hostname `epass` and login banner `Welcome to CRA Electric Pass`;
- dynamic device-node management through eudev;
- sequential application of the generic Allwinner, common SUNIV, and CRA rootfs overlays;
- Linux `5.4.99`, with both common SUNIV and CRA Linux patches;
- the `linux.defconfig` and Linux device trees in this directory;
- U-Boot `2020.07`, with both common SUNIV and CRA U-Boot patches;
- the `uboot.defconfig`, `uboot.env`, and U-Boot device trees in this directory;
- the CRA Electric Pass application, USB Responder, control tools, MTP, Cedar video decoding, and related runtime libraries;
- host-side tools required to generate UBI, FIT, and SD images and to provide the cross-compilation environment;
- execution of three post-image scripts after the main image artifacts have been produced.

The root login password is currently set in this configuration to:

```text
toor
```

This is acceptable for a controlled development device, but it must not be treated as a secure deployment configuration. If the device exposes a serial console, RNDIS, MTP, or another debugging interface, the risks associated with root login and writable filesystem access must be evaluated as well.

### Rootfs Overlay Order

The current configuration merges the overlays in this order:

```text
board/allwinner/generic/rootfs
        ↓
board/allwinner/suniv-f1c100s/rootfs
        ↓
board/cra/epass/rootfs
```

When multiple overlays contain the same path, the later file overrides the earlier one. The final behavior of a CRA file must therefore be evaluated together with any files already supplied by the first two layers.

### Post-Image Processing Order

After Buildroot produces the main build artifacts, it runs these scripts in order:

```text
scripts/mknanduboot.sh
        ↓
scripts/mkdt.sh
        ↓
scripts/buildimage.sh
```

The three scripts generate, respectively, the U-Boot image for SPI-NAND, the Linux DTB/DTBO files, and the final FIT, UBI, and simplified SD images.

## `linux.defconfig`

This file is the kernel configuration baseline for Linux `5.4.99`. Its main enabled features include:

| Subsystem | Current purpose |
| --- | --- |
| ARM AEABI, SUNXI/SUNIV | F1C100S/F1C200S ARM926T platform |
| MTD, SPI-NAND, UBI, UBIFS | On-board NAND partitions and root filesystem |
| SUN4I DRM, panel, backlight, fbcon | 360×640 LCD display pipeline and console |
| evdev, LRADC, I²C, SPI, GPIO | Buttons and board-level interfaces |
| USB MUSB, ConfigFS, ACM, RNDIS, FunctionFS | USB Gadget operating modes |
| MMC, VFAT, UTF-8 NLS | SD card access |
| ALSA SoC, SUN4I I²S, ES8311 | Optional audio path |
| IIO, GPADC, LSM6DSX | ADC and optional inertial sensor |
| `CONFIG_CRA_EP_*` | CRA Electric Pass-specific kernel support |

This configuration depends on both `patch/linux/` and `devicetree/linux/`. Enabling a Kconfig option without the corresponding patch or device-tree node may leave the option unrecognized, omit the driver from the build, or prevent the device from matching at runtime.

## `uboot.defconfig`

This file configures U-Boot `2020.07`. Its main settings include:

- SUNIV SPL and SPI-NAND boot support;
- a 204 MHz DRAM clock and 604 MHz system clock;
- a boot delay of zero;
- MTD, DFU, USB Mass Storage, and MUSB Gadget support;
- `suniv-f1c100s-generic` as the default device tree;
- compilation of the default environment from `uboot.env`;
- disabled video output during the U-Boot stage.

The default SPI-NAND partition layout is:

```text
1 MiB       u-boot (read-only)
6 MiB       boot
remaining   rootfs
```

This partition layout is represented in the U-Boot environment, Linux boot arguments, device tree, image-generation scripts, and flashing tools. Any change must be reviewed across all of these components as a single coordinated update.

## `uboot.env` and `uEnv.txt`

These files serve different purposes:

| File | Type | Purpose |
| --- | --- | --- |
| `uboot.env` | U-Boot default-environment source | Compiled into U-Boot through `CONFIG_DEFAULT_ENV_FILE`; defines load addresses, FIT loading, device-tree overlay application, and the DFU fallback flow |
| `uEnv.txt` | Text environment template | Written to the SPI-NAND text-environment region during flashing; supplies hardware selections such as `bootargs`, `screen`, `interface`, and `ext` |

The principal memory addresses currently used by `uboot.env` are:

| Content | Address |
| --- | --- |
| Linux kernel | `0x80008000` |
| Base DTB | `0x80C00000` |
| Temporary DTBO | `0x80D00000` |
| Text-environment buffer | `0x80E00000` |
| FIT image | `0x81000000` |

During boot, U-Boot reads up to `0x6000` bytes of text environment from SPI-NAND offset `0xFA000`. The end of this region aligns exactly with the Boot partition start at `0x100000`. It then reads `boot.itb` from `0x100000`, extracts the kernel and base DTB, and applies overlays in the following order:

```text
base → screen → interface → ext
```

The `uEnv.txt` currently stored in the repository does not preset `screen=`, and both `interface=` and `ext=` are empty. The existing flashing workflow generates the actual environment according to the selected hardware. If this template is written manually, a valid display type must be supplied; otherwise, U-Boot cannot extract the corresponding `fdt-screen-${screen}` node.

## Build Process

Run the following commands from the Buildroot root directory under Linux or a correctly configured WSL environment:

```sh
make cra_epass_defconfig
make
```

The overall relationship is:

```text
cra_epass_defconfig
        │
        ├─ Linux 5.4.99 + linux.defconfig
        │       ├─ patch/linux/
        │       └─ devicetree/linux/
        │
        ├─ U-Boot 2020.07 + uboot.defconfig + uboot.env
        │       ├─ patch/uboot/
        │       └─ devicetree/uboot/
        │
        ├─ rootfs/
        │
        └─ post-image scripts/
                │
                ▼
          output/images/
```

Common final artifacts include:

| File | Purpose |
| --- | --- |
| `u-boot-sunxi-with-nand-spl.bin` | SPL/U-Boot image for SPI-NAND |
| `boot.itb` | FIT package containing the Linux kernel, base DTB, and all DTBO files |
| `rootfs_ubi.img` | UBI/UBIFS root filesystem image |
| `sd_image.img` | Simplified SD boot image containing only the boot offset and SPL/U-Boot |
| `dt/base/*.dtb` | Linux base device trees |
| `dt/screen/*.dtbo` | Display-panel overlays |
| `dt/interface/*.dtbo` | Interface overlays |
| `dt/ext/*.dtbo` | External-device overlays |

These commands only generate build artifacts. They do not automatically flash a physical device.

## Device Boot Chain

The primary boot chain on the physical device is:

```text
Allwinner BROM
        ↓
SPL
        ↓
U-Boot
        ├─ Import the text environment
        ├─ Read boot.itb
        ├─ Extract the Linux kernel and base DTB
        ├─ Apply screen/interface/ext DTBO files
        └─ Start Linux
        ↓
Linux 5.4.99
        ├─ Mount the UBI/UBIFS rootfs
        └─ BusyBox init → autologin → /root/.profile
        ↓
/root/epass_drm_app
```

## Cross-Component Dependencies

| Change | Components that must also be checked |
| --- | --- |
| Linux version | `cra_epass_defconfig`, `linux.defconfig`, `patch/linux/`, `scripts/mkdt.sh` |
| U-Boot version | `cra_epass_defconfig`, `uboot.defconfig`, `uboot.env`, `patch/uboot/` |
| Display name or initialization sequence | `devicetree/linux/screen/`, `scripts/kernel.its`, `uboot.env`, and the active boot environment |
| Interface or peripheral name | The corresponding DTS, `kernel.its`, `uboot.env`, `uEnv.txt`, or flashing configuration |
| NAND page and erase-block parameters | `mknanduboot.sh`, `buildimage.sh`, `ubinize-rootfs.cfg`, and U-Boot NAND support |
| NAND partition layout | `uboot.defconfig`, `uboot.env`, Linux boot arguments, device trees, image tools, and flashing tools |
| UBI volume name or size | `ubinize-rootfs.cfg`, `buildimage.sh`, and Linux boot arguments |
| Rootfs files | `rootfs/`, the installation paths of related packages, and paths exposed through MTP |
| Main application exit codes | `drm_app_neo/src/config.h` and `rootfs/root/.profile` |

## Windows Checkouts and File Formats

This board directory contains Shell and Python scripts, DTS files, patches, configuration files, binaries, and Git symbolic links. Pay particular attention to the following when checking out the repository on Windows:

- Shell scripts, DTS files, configuration files, and patches should use UTF-8 with LF line endings;
- executable permissions on Shell scripts must be preserved;
- symbolic links such as `configs/cra_epass_defconfig` must not become regular files containing only a path string;
- do not rewrite ARM ELF files, PNG files, MP4 files, or Windows EXE files with a text editor;
- do not run Linux scripts that depend on Buildroot environment variables directly from PowerShell.

If a build fails unexpectedly, check line endings, symbolic links, file permissions, and the active Buildroot output tree first. Do not conceal the underlying problem by skipping patches, disabling validation, or editing generated directories directly.

## Secondary Development and Safety Principles

1. Before making a change, determine whether it belongs to the Buildroot configuration, kernel, U-Boot, device tree, rootfs, image-generation scripts, or main application. Do not mix unrelated layers into a single fix.
2. Keep configuration files, patches, device trees, and scripts consistent in both versioning and naming.
3. Modify tracked source files only; do not treat temporary edits under `output/build/` as a final implementation.
4. A successful build only proves that the software can be generated. It does not prove that the LCD, buttons, USB, audio, SPI-NAND, or power control has been validated on physical hardware.
5. Before flashing U-Boot, Boot, the rootfs, or a complete image, verify the device model, display type, NAND parameters, target partition, and backup state.
6. High-privilege entry points such as `format_sd`, DFU, flashing tools, and MTP can be destructive and must not be used with unknown devices or untrusted computers.
7. Building, documenting, and performing static checks do not authorize automatically flashing, replacing, or deleting files on a physical device.
