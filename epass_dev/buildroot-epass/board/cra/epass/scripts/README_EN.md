# CRA Electric Pass Image Build Scripts

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the CRA Electric Pass board-level image-generation scripts, the device-tree packaging manifest, and the UBI volume configuration. They run at the end of the Buildroot process and assemble the generated U-Boot, Linux kernel, device trees, and root filesystem into images suitable for booting or flashing.

The EXE files in the `binary/` subdirectory are Windows host-side utilities. See the [Windows utilities documentation](binary/README_EN.md).

## Directory Structure

```text
scripts/
├── binary/
│   ├── epass_flasher.exe
│   ├── UsbTreeView.exe
│   ├── zadig-2.9.exe
│   └── README.md
├── buildimage.sh
├── gensdimage.py
├── kernel.its
├── mkdt.sh
├── mknanduboot.sh
├── ubinize-rootfs.cfg
└── README.md
```

| File | Type | Purpose |
| --- | --- | --- |
| `mknanduboot.sh` | Bash script | Rearrange the standard SUNXI SPL/U-Boot output for the current SPI-NAND page layout |
| `mkdt.sh` | Bash script | Preprocess and compile the base device tree and all device-tree overlays |
| `buildimage.sh` | POSIX shell script | Generate the UBIFS/UBI root filesystem, FIT boot bundle, and simplified SD image |
| `kernel.its` | FIT Image Tree Source | Define the kernel, base device tree, and overlays included in `boot.itb` |
| `ubinize-rootfs.cfg` | UBI configuration | Define the root-filesystem UBI volume |
| `gensdimage.py` | Python script | Generate a simplified SD image containing only the boot offset and SPL/U-Boot |
| `binary/` | Windows programs | Provide USB diagnostics, driver configuration, and device-flashing tools |

## Buildroot Entry Point

The board configuration invokes these scripts through:

```make
BR2_ROOTFS_POST_IMAGE_SCRIPT="board/cra/epass/scripts/mknanduboot.sh board/cra/epass/scripts/mkdt.sh board/cra/epass/scripts/buildimage.sh"
```

After the root filesystem and the main build artifacts are ready, Buildroot runs the scripts in the following order:

```text
mknanduboot.sh
        │
        ▼
     mkdt.sh
        │
        ▼
  buildimage.sh
```

1. `mknanduboot.sh` generates the U-Boot image for NAND.
2. `mkdt.sh` generates the DTB and DTBO files referenced by `kernel.its`.
3. `buildimage.sh` packages the kernel and device trees into `boot.itb` and generates the root-filesystem image.

The scripts depend on environment variables exported by Buildroot:

| Variable | Typical Location | Purpose |
| --- | --- | --- |
| `BINARIES_DIR` | `output/images` | Store final images and temporary packaging files |
| `BUILD_DIR` | `output/build` | Locate the Linux source tree and headers |
| `HOST_DIR` | `output/host` | Locate host-side tools built by Buildroot |

Running these scripts outside the Buildroot environment is therefore not recommended.

## Overall Inputs and Outputs

Primary inputs:

```text
output/images/
├── u-boot-sunxi-with-spl.bin
├── zImage
└── rootfs.tar

board/cra/epass/devicetree/linux/
├── base/
├── screen/
├── interface/
└── ext/
```

Expected outputs:

```text
output/images/
├── u-boot-sunxi-with-nand-spl.bin
├── boot.itb
├── rootfs_ubi.img
├── sd_image.img
└── dt/
    ├── base/
    ├── screen/
    ├── interface/
    └── ext/
```

These scripts only generate files. They do not automatically flash a physical device.

## `mknanduboot.sh`

This script converts the Buildroot/U-Boot output:

```text
u-boot-sunxi-with-spl.bin
```

into:

```text
u-boot-sunxi-with-nand-spl.bin
```

### Main Parameters

```sh
UBOOT_OFFSET=32
PAGESIZE=2048
BLOCKSIZE=128
SPLBLOCKS=25
```

- Treat the NAND page size as 2,048 bytes.
- Use 1,024-byte blocks for `dd` operations.
- Write the first 26 1-KiB SPL blocks at the beginning of successive 2-KiB pages.
- At offset 52 KiB in the output, append the input data beginning at offset 32 KiB.
- Flush only the generated output file at the end.
- Check whether the host `od` supports `--endian`.
- Read and print the SPL size from the SPL header.
- Read and print the U-Boot size from the expected main U-Boot header.
- Confirm that the NAND page size is aligned to 1 KiB.

### Maintenance Notes

- This script is tightly coupled to the F1C100S/SUNIV SPL layout and the current NAND page structure.
- `SPLSIZE` and `UBOOTSIZE` are currently displayed only and are not used for bounds checking.
- `BLOCKSIZE` is defined but currently unused.
- The old comment following `UBOOT_OFFSET=32` does not fully agree with the meaning of the value. Revalidate it against the current U-Boot binary layout before making changes.
- `od --endian` and `sync -d` are GNU/Linux-specific features. The script is not suitable for direct execution in a native Windows shell.

## `mkdt.sh`

This script compiles the Linux base device tree and device-tree overlays.

### Input Directories

```text
board/cra/epass/devicetree/linux/
├── base/
├── screen/
├── interface/
└── ext/
```

### Output Directories

```text
${BINARIES_DIR}/dt/
├── base/*.dtb
├── screen/*.dtbo
├── interface/*.dtbo
└── ext/*.dtbo
```

Each DTS file passes through two stages:

1. The C preprocessor, `cpp`, expands `#include` directives, macros, and Linux device-tree headers.
2. `dtc -@` compiles the result while preserving the symbol information required to apply overlays.

The base device tree produces a `.dtb` file. Display, interface, and extension configurations produce `.dtbo` files.

The script deletes the entire `${BINARIES_DIR}/dt` directory before recreating it. Do not manually store files there if they need to be retained.

### Linux-Version Coupling

The script directly references:

```text
${BUILD_DIR}/linux-5.4.99/
```

If the Linux version changes, update this directory name as well; otherwise, the preprocessor will be unable to find the kernel device-tree headers.

### Maintenance Notes

- `set -o pipefail` only affects pipeline return values and is not equivalent to `set -e`.
- The current script does not guarantee immediate termination when an individual command fails.
- If an input directory contains no matching `.dts` files, the shell glob may enter the loop as an unexpanded literal string.
- Output filenames must remain consistent with `kernel.its` and the `screen`, `interface`, and `ext` values in the U-Boot environment.

## `kernel.its`

`kernel.its` is the U-Boot FIT image description read by `mkimage` to generate:

```text
boot.itb
```

The current FIT contains the following components.

### Linux Kernel

| FIT Node | Input File | Load Address | Entry Address |
| --- | --- | --- | --- |
| `kernel` | `zImage` | `0x80008000` | `0x80008000` |

The kernel architecture is ARM, and no compression is used.

### Base Device Tree

| FIT Node | Input File |
| --- | --- |
| `fdt-base` | `dt/base/devicetree.dtb` |

### Display Overlays

| FIT Node | Input File | Boot-Environment Value |
| --- | --- | --- |
| `fdt-screen-hsd` | `dt/screen/hsd.dtbo` | `screen=hsd` |
| `fdt-screen-boe` | `dt/screen/boe.dtbo` | `screen=boe` |
| `fdt-screen-laowu` | `dt/screen/laowu.dtbo` | `screen=laowu` |

### Interface Overlays

```text
adc_pa1
adc_pa123
i2c0
i2s0_pa
i2s0_pe
spi1
uart1
uart2
usbhost
usbhs
```

The corresponding FIT nodes use the `fdt-iface-` prefix.

### Extension Overlays

```text
cardkb
es8311_sound
lsm6ds3_pre0.4
```

The corresponding FIT nodes use the `fdt-ext-` prefix.

U-Boot does not rely on a default FIT `configurations` node. Instead, `uboot.env` uses `imxtract` to extract the kernel and device trees by node name, then applies the display, interface, and extension overlays in sequence.

The current ITS does not define hash or digital-signature nodes. Although the Buildroot configuration enables host-side U-Boot tools with FIT-signature support, the generated `boot.itb` does not currently use this verification mechanism.

## `buildimage.sh`

This is the final image-assembly script.

### Stage 1: Prepare Packaging Files

The script copies the following files into `${BINARIES_DIR}`:

```text
ubinize-rootfs.cfg
gensdimage.py
kernel.its
```

It uses:

```text
${HOST_DIR}/bin/mkimage
```

as the FIT image-generation tool.

### Stage 2: Generate UBIFS and UBI

The script:

1. Creates a temporary `rootfs/` directory.
2. Extracts `rootfs.tar`.
3. Generates `rootfs_ubifs.img` with `mkfs.ubifs`.
4. Generates the final `rootfs_ubi.img` with `ubinize`.

Main NAND/UBI parameters:

| Parameter | Value | Meaning |
| --- | ---: | --- |
| Minimum I/O unit | 2048 | NAND page size |
| Physical eraseblock size | 131072 | 128 KiB |
| UBIFS logical eraseblock size | 126976 | Usable space after subtracting UBI headers |
| Maximum logical eraseblock count | 922 | `mkfs.ubifs -c` |
| Compression algorithm | LZO | `mkfs.ubifs -x lzo` |

### Stage 3: Generate the FIT Image

The script runs:

```sh
mkimage -f kernel.its boot.itb
```

This packages `zImage`, the base device tree, and all overlays into `boot.itb`.

### Stage 4: Generate the SD Image

The script runs:

```sh
python3 gensdimage.py
```

to generate `sd_image.img`.

### Stage 5: Cleanup

The script removes:

- the copied `ubinize-rootfs.cfg`;
- the intermediate `rootfs_ubifs.img`;
- the extracted temporary `rootfs/` directory.

The copied `kernel.its` and `gensdimage.py` are not currently removed at the end of the script.

### Maintenance Notes

- The script does not enable `set -e`, so it may continue after some commands fail.
- It initially removes `ubi.img`, while the actual output is named `rootfs_ubi.img`; the two names do not match.
- On the first build, missing `rootfs/` or `ubi.img` paths produce removal errors, but these normally do not stop subsequent steps.
- If the process fails partway through, stale or incomplete images may remain in the output directory. File existence alone does not prove a successful build.

## `ubinize-rootfs.cfg`

This file defines one dynamic UBI volume:

```ini
[rootfs]
mode=ubi
image=rootfs_ubifs.img
vol_id=0
vol_type=dynamic
vol_name=rootfs
vol_size=117071872
vol_alignment=1
```

Field definitions:

| Field | Purpose |
| --- | --- |
| `mode=ubi` | Create a UBI volume |
| `image=rootfs_ubifs.img` | Use the temporary UBIFS image as the volume contents |
| `vol_id=0` | Assign volume ID 0 |
| `vol_type=dynamic` | Create an updatable dynamic volume |
| `vol_name=rootfs` | Match the kernel argument `root=ubi0:rootfs` |
| `vol_size=117071872` | Allocate 117,071,872 bytes to the root filesystem |
| `vol_alignment=1` | Use the default alignment of one logical eraseblock |

The volume size is exactly:

```text
126976 × 922 = 117071872
```

It therefore matches the logical eraseblock size and maximum block count passed to `mkfs.ubifs` by `buildimage.sh`. These values must remain consistent when modified.

## `gensdimage.py`

This script currently performs only two operations:

1. Create `sd_image.img`.
2. Write 4,096 zero bytes, then append the complete `u-boot-sunxi-with-spl.bin`.

The resulting layout is equivalent to:

```text
0x00000000 ─ 0x00000FFF    4 KiB of zero padding
0x00001000 ─ end of file   u-boot-sunxi-with-spl.bin
```

It does not write:

- `boot.itb`;
- `rootfs_ubi.img`;
- a partition table;
- a filesystem;
- the runtime boot environment.

The current `sd_image.img` is therefore a simplified U-Boot boot image, not a complete SD-card system image containing the kernel and root filesystem.

The imported `os` module is currently unused.

## Build Dependencies

The scripts use the following host-side tools:

```text
bash
sh
cpp
dtc
mkimage
fakeroot
tar
mkfs.ubifs
ubinize
od
grep
xargs
dd
sync
python3
```

The Buildroot configuration enables several of the corresponding host packages, but the build environment must still be Linux or a compatible WSL environment. Do not execute the shell scripts directly in native PowerShell.

## Standard Build Procedure

After configuring a valid board defconfig entry, run the following commands from the Buildroot root directory:

```sh
make cra_epass_defconfig
make
```

Buildroot prepares the environment variables and automatically runs all three post-image scripts. They normally do not need to be invoked manually.

## Cross-File Dependencies

When changing any of the following items, inspect the related files as well:

| Change | Files and Areas to Review |
| --- | --- |
| Linux version | `mkdt.sh`, Buildroot defconfig, and patch directories |
| Display name | Display DTS, `kernel.its`, `uboot.env`, and flashing environment |
| Interface or extension name | Corresponding DTS, `kernel.its`, and `uboot.env` |
| NAND page/block parameters | `mknanduboot.sh`, `buildimage.sh`, and `ubinize-rootfs.cfg` |
| NAND partition layout | `uboot.defconfig`, `uboot.env`, Linux DTS, and flashing tools |
| UBI volume name | `ubinize-rootfs.cfg` and Linux `bootargs` |
| Kernel load address | `kernel.its`, `uboot.env`, and the kernel boot layout |

## Current File Formats

In the current checkout:

- The three shell scripts use LF line endings.
- `gensdimage.py` and `ubinize-rootfs.cfg` use CRLF line endings.
- `kernel.its` contains both CRLF and LF, making it a mixed-line-ending file.

These differences do not necessarily prevent the current build, but future edits should standardize the files on UTF-8 with LF line endings to avoid additional Linux-build and Git-diff problems.

## Safety and Maintenance Guidelines

- Do not point `BINARIES_DIR` at a mounted physical device.
- Before building, confirm that `rootfs.tar`, `zImage`, and the U-Boot output all come from the same valid build.
- Do not reuse images from another hardware revision without first verifying the NAND parameters.
- After renaming a device-tree file, update the FIT node and boot environment as well.
- After a failed build, inspect the logs and regenerate the images instead of flashing potentially stale outputs.
- Before flashing, verify the image name, target partition, and device model again.
- Keep the Windows binary utilities separate from the Buildroot scripts; do not include them in the target rootfs.

