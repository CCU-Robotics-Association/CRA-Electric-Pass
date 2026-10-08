# Buildroot Development Helper Scripts

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains three Buildroot helper scripts intended for use on a development computer:

```text
helper/
```

They rebuild the kernel, rebuild U-Boot, or temporarily enter a generated ARM root filesystem through QEMU/chroot. These scripts are not part of the standard upstream Buildroot directory layout and are not invoked by a normal `make` automatically.

## Directory Contents

```text
helper/
├── rebuild-kernel.sh   Rebuild Linux and refresh the complete Buildroot output
├── rebuild-uboot.sh    Rebuild U-Boot and refresh the complete Buildroot output
└── emulate-chroot.sh   Extract rootfs.tar and enter it through QEMU/chroot
```

All three scripts have executable mode in Git. They rely on Shell, relative paths, and Linux system calls, so they must run under Linux or a suitable WSL environment. They cannot be executed directly from Windows PowerShell.

## Common Prerequisites

Before running a script, ensure that:

- the current directory is the Buildroot repository root, not `helper/`;
- a valid `.config` has been generated from the intended target defconfig;
- Git symbolic links and Shell scripts are intact;
- the scripts use Unix LF line endings;
- the Buildroot host dependencies are installed;
- the current `output/` belongs to the board being maintained.

A typical CRA Electric Pass setup is:

```sh
cd /path/to/buildroot-epass
make cra_epass_defconfig
```

Do not run these scripts against an `output/` tree that was just used for a different board. When switching among CRA, BADGE200, Lichee Nano, MangoPi, or another target, use a clean output directory or run `make distclean` before loading the new configuration.

## `rebuild-kernel.sh`

The script is equivalent to:

```sh
rm ./output/images/*.dtb
make linux-rebuild -j8
make
```

### Execution Flow

1. Delete existing `*.dtb` files from `output/images/`;
2. run Buildroot's `linux-rebuild` target with eight parallel jobs;
3. run a complete `make` so that images and post-processing stages that depend on kernel output are refreshed.

### Appropriate Use Cases

- Rebuilding after changing the Linux kernel configuration;
- verifying source or patch changes that have already entered the kernel build tree;
- refreshing images after changes to Linux DTS/DTB content;
- rerunning later image-generation stages that depend on kernel artifacts.

### Notes

- The script deletes the current DTB outputs first and does not create a backup;
- it does not enable `set -e`, so later build commands may still run if the deletion command fails;
- when the `*.dtb` wildcard matches no files, `rm` can report an error in some Shell environments;
- `-j8` is a fixed job count and does not adapt to available CPU or memory;
- `linux-rebuild` is not equivalent to a full `linux-dirclean`; it does not redownload or completely clear the kernel build directory;
- after changing the patch series, the existing kernel build directory may already contain old patches and require a more thorough cleanup.

Run it as follows:

```sh
./helper/rebuild-kernel.sh
```

The script only changes build artifacts under `output/`. It does not flash a physical device automatically.

## `rebuild-uboot.sh`

The script is equivalent to:

```sh
make uboot-rebuild -j8
make
```

### Execution Flow

1. Rebuild U-Boot with eight parallel jobs;
2. run a complete `make` to refresh final images that depend on U-Boot.

### Appropriate Use Cases

- Rebuilding after changing `uboot.defconfig`;
- rebuilding after changing the U-Boot device tree;
- verifying source changes that are already present in the U-Boot build tree;
- regenerating NAND or SD images that contain U-Boot/SPL.

### Notes

- `uboot-rebuild` does not necessarily re-extract the source or reapply every patch;
- after changing the U-Boot patch series, the old build directory may no longer be suitable;
- the script always uses `-j8`; hosts with limited resources may need to use the standard Buildroot commands manually instead;
- U-Boot, SPL, device trees, the default environment, and the image layout are interdependent, so a successful compilation alone is not sufficient validation.

Run it as follows:

```sh
./helper/rebuild-uboot.sh
```

The script produces new U-Boot and image files, but it does not write to SPI NAND, SPI NOR, an SD card, or a physical device automatically.

## `emulate-chroot.sh`

This script temporarily extracts the generated ARM root filesystem into:

```text
output/chroot/
```

It then starts `/bin/sh` from the target filesystem through `qemu-arm-static` and `chroot`.

### Execution Flow

The script:

1. runs with root privileges through `#!/usr/bin/sudo bash`;
2. checks that `output/` exists in the current directory;
3. if `output/chroot/` already exists, attempts to unmount its `proc/` and deletes the entire old directory;
4. creates a new `output/chroot/`;
5. extracts `output/images/rootfs.tar` into it;
6. mounts the host procfs at `output/chroot/proc/`;
7. copies the host's `/usr/bin/qemu-arm-static` into the target root filesystem;
8. executes `chroot . /bin/sh`;
9. after the user leaves the Shell, unmounts procfs and deletes the entire `output/chroot/` directory.

### Host Dependencies

The host requires at least:

- `sudo`;
- `bash`;
- `tar`;
- `mount`, `umount`, and `chroot`;
- `/usr/bin/qemu-arm-static`;
- binfmt_misc/QEMU configuration capable of executing ARM user-space programs;
- a generated `output/images/rootfs.tar`.

If QEMU or binfmt_misc is not configured correctly, `chroot . /bin/sh` may fail with `Exec format error`.

### High-Risk Behavior

This script presents greater host-side risk than the two rebuild scripts:

- it acquires root privileges through `sudo`;
- it exposes the host procfs to the target root filesystem;
- it deletes an existing `output/chroot/` without prompting;
- `cleanup()` uses relative paths, making both the launch directory and current directory state important;
- if the script is interrupted, the terminal closes unexpectedly, or a command fails, procfs may remain mounted;
- entering an untrusted root filesystem is equivalent to running programs from it with root privileges.

Use this script only with a root filesystem that you built yourself and trust. Do not extract and enter an unknown image or third-party root filesystem directly.

### Running and Exiting

After confirming the dependencies and root filesystem, run the script from the repository root:

```sh
./helper/emulate-chroot.sh
```

Inside the target Shell, leave normally with:

```sh
exit
```

This gives the script an opportunity to unmount procfs and remove the temporary directory. Do not simply close the terminal.

### Checking After an Interrupted Run

If the script terminates unexpectedly, first check the mount state:

```sh
mountpoint output/chroot/proc
```

If it remains mounted, unmount it correctly:

```sh
sudo umount output/chroot/proc
```

Only handle `output/chroot/` after confirming that procfs is no longer mounted. Do not delete the directory while procfs is still mounted.

## Inputs and Outputs

| Script | Primary inputs | Main modification scope | Writes to a physical device? |
| --- | --- | --- | --- |
| `rebuild-kernel.sh` | `.config`, Linux sources/patches/device tree | `output/build/linux-*`, `output/images/`, and later images | No |
| `rebuild-uboot.sh` | `.config`, U-Boot sources/patches/device tree | `output/build/uboot-*`, `output/images/`, and later images | No |
| `emulate-chroot.sh` | `output/images/rootfs.tar` | Temporarily creates and removes `output/chroot/` and mounts procfs | No |
