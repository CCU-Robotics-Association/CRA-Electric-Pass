# Buildroot Root Filesystem Image Infrastructure

Read this document in other languages: [English](README_EN.md), [中文](README.md).

## Directory Structure

```text
fs/
├── README.md
├── Config.in
├── common.mk
├── axfs/
├── btrfs/
├── cloop/
├── cpio/
├── cramfs/
├── ext2/
├── f2fs/
├── initramfs/
├── iso9660/
├── jffs2/
├── romfs/
├── squashfs/
├── tar/
├── ubi/
├── ubifs/
└── yaffs2/
```

Each format normally contains:

```text
Config.in       # Kconfig switches and format parameters
<format>.mk     # Required host tools, generation command, and output name
```

The directory currently supports 16 root filesystem or image-generation formats.

## Difference Between `fs/`, Rootfs Overlays, and `output/target/`

| Path | Role | Deploy directly? |
| --- | --- | --- |
| `fs/` | Sources for Buildroot image-generation rules | No |
| `board/cra/epass/rootfs/` | Hand-written CRA runtime file overlay | No; merged during the build |
| `output/target/` | Intermediate root directory after packages and overlays are merged | Not recommended |
| `output/build/buildroot-fs/` | Temporary fakeroot workspace for each image format | No |
| `output/images/` | Packaged root filesystems and board images | Final build output |

`output/target/` is created with ordinary user privileges and cannot fully represent device nodes, final ownership, or certain permissions. Buildroot applies this metadata under fakeroot, so `output/target/` should not be copied directly to a device as the production root filesystem.

## Overall Generation Flow

The root filesystem image is generated approximately as follows:

```text
Selected target packages
        ↓
Installed into output/target/
        ↓
Skeleton and BR2_ROOTFS_OVERLAY merged
        ↓
target-finalize / post-build scripts
        ↓
User and device tables generated
        ↓
Copied to a temporary target directory for each format
        ↓
fakeroot: ownership, users, device nodes, permissions
        ↓
Format generator such as mkfs, tar, or cpio
        ↓
Optional outer compression such as gzip, xz, or lzo
        ↓
output/images/rootfs.*
        ↓
Board post-image scripts assemble final images
```

Generating a root filesystem does not write anything to a physical device automatically. Image generation and device flashing are separate operations.

## Top-Level Files

### `Config.in`

This file defines the `Filesystem images` menu and includes the `Config.in` file for every supported format. It provides configuration entry points only; it does not generate images itself.

Multiple formats can be enabled in one build. For example, `rootfs.tar` and `rootfs.ext4` can be generated from the same target file set, while each receives its own fakeroot and formatting stage.

### `common.mk`

This is the shared core infrastructure for normal root filesystem formats. Its main responsibilities are:

- defining the `rootfs-common` dependency;
- combining package-provided and user-provided user, group, permission, and device tables;
- creating a separate temporary workspace for each format;
- copying files from the finalized target directory;
- generating and executing the fakeroot script;
- establishing `root:root` as the initial ownership for target files;
- invoking `mkusers` to create users and groups;
- invoking `makedevs` to apply device nodes and permission rules;
- running post-fakeroot scripts and format-specific hooks;
- invoking the selected format's image-generation command;
- optionally producing gzip, bzip2, lzma, lz4, lzo, or xz compressed copies;
- registering the `rootfs-<format>` build target.

Each format only needs to declare its dependencies and `ROOTFS_<FORMAT>_CMD`; `common.mk` supplies the common workflow.

## Format Directories

| Directory | Primary purpose | Used directly by the current CRA build? |
| --- | --- | --- |
| `axfs/` | Read-only AXFS image with XIP/on-demand access support | No |
| `btrfs/` | Btrfs root filesystem image | No |
| `cloop/` | Compressed Loop Block Device image | No |
| `cpio/` | CPIO archive, commonly used for initramfs | No |
| `cramfs/` | Traditional read-only compressed filesystem | No |
| `ext2/` | ext2/ext3/ext4 image generation | No |
| `f2fs/` | F2FS image for flash-based storage | No |
| `initramfs/` | Embeds a root filesystem archive into the Linux kernel | No |
| `iso9660/` | Bootable ISO image | No |
| `jffs2/` | JFFS2 image for raw flash | No |
| `romfs/` | Simple read-only ROMFS image | No |
| `squashfs/` | Highly compressed read-only SquashFS image | No |
| `tar/` | Tar archive preserving the directory layout and target metadata | Yes |
| `ubi/` | Uses `ubinize` to wrap UBIFS and other volumes in a UBI container | Not through this generic entry point |
| `ubifs/` | Generates a UBIFS filesystem for a UBI volume | Not through this generic entry point |
| `yaffs2/` | YAFFS2 image for raw NAND | No |

“Not through this generic entry point” does not mean that CRA does not use UBI/UBIFS. The CRA board post-image script generates its own UBI image.

## Read-Only, Block-Device, and Raw-Flash Formats

The formats target different storage layers and cannot be interchanged based on image size alone.

| Category | Examples | Typical characteristics |
| --- | --- | --- |
| Plain archives | tar, cpio | Preserve a file collection; not necessarily mountable block images |
| Block filesystems | ext4, Btrfs, F2FS | Usually written to an SD/eMMC partition or another block device |
| Read-only compressed filesystems | SquashFS, CramFS, ROMFS | Suitable for fixed system content and not directly writable at runtime |
| Raw-flash filesystems | JFFS2, YAFFS2 | Directly depend on specific raw-flash characteristics |
| UBI management layer | UBI + UBIFS | UBI manages eraseblocks and bad blocks; UBIFS resides in a UBI volume |

The Electric Pass uses SPI NAND. Its PEB size, LEB size, minimum I/O unit, VID header offset, and volume size must agree with the actual NAND, kernel, and U-Boot configuration. An ext4 image or arbitrary UBI parameters cannot be substituted directly.

## `tar/`

`BR2_TARGET_ROOTFS_TAR` is enabled by default. The CRA defconfig therefore produces the following uncompressed archive even though the option is not written explicitly:

```text
output/images/rootfs.tar
```

`tar.mk`:

- sorts file names deterministically;
- stores numeric UIDs and GIDs;
- preserves extended attributes;
- omits atime/ctime from PaxHeaders to improve reproducibility;
- optionally produces gzip, bzip2, lz4, lzma, lzo, or xz compressed copies.

## Generic `ubi/` and `ubifs/` Implementations

### `ubifs/`

`ubifs.mk` invokes the host `mkfs.ubifs` tool. Its main parameters are:

| Option | Kconfig item | Meaning |
| --- | --- | --- |
| `-e` | `BR2_TARGET_ROOTFS_UBIFS_LEBSIZE` | Logical eraseblock size |
| `-m` | `BR2_TARGET_ROOTFS_UBIFS_MINIOSIZE` | Minimum I/O unit size |
| `-c` | `BR2_TARGET_ROOTFS_UBIFS_MAXLEBCNT` | Maximum number of logical eraseblocks |
| `-x` | Runtime compression selection | none, zlib, or lzo |

Its default output file is named `rootfs.ubifs`.

### `ubi/`

`ubi.mk` depends on `rootfs-ubifs` and then invokes the host `ubinize` tool to generate a UBI container. Its common parameters include:

- the NAND physical eraseblock (PEB) size;
- the sub-page size;
- the minimum I/O unit;
- a custom or default `ubinize.cfg`;
- additional `ubinize` options.

The default [ubi/ubinize.cfg](ubi/ubinize.cfg) creates a dynamic volume named `rootfs` and enables automatic resizing. Buildroot replaces `BR2_ROOTFS_UBIFS_PATH` in that configuration with the path to the generated UBIFS image.

## Syntax and Comments

| File type | Language | Comment and formatting requirements |
| --- | --- | --- |
| `Config.in` | Kconfig | Use `# comments`; preserve correct indentation under `help` |
| `.mk` | GNU Make | Use `# comments`; recipe commands must begin with a Tab |
| `.cfg` | ubinize/bootloader configuration | Usually uses `#` or `;` comments; preserve the key-value format |
| Shell scripts | POSIX Shell | Use `# comments`; preserve LF line endings, the shebang, and executable mode |
