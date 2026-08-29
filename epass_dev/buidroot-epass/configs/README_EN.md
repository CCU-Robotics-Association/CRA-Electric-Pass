# Buildroot Configuration Entry Points

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains board configuration entry points that Buildroot can load directly:

```text
configs/
```

## Current Entry Points

| Entry file | Buildroot command | Canonical configuration source | Purpose |
| --- | --- | --- | --- |
| `cra_epass_defconfig` | `make cra_epass_defconfig` | `board/cra/epass/cra_epass_defconfig` | Current CRA Electric Pass target |
| `hatlab_badge200_defconfig` | `make hatlab_badge200_defconfig` | `board/hatlab/badge200/hatlab_badge200_defconfig` | Legacy HatLab BADGE200 target |
| `sipeed_lichee_nano_defconfig` | `make sipeed_lichee_nano_defconfig` | `board/sipeed/lichee/nano/sipeed_lichee_nano_defconfig` | Legacy Sipeed Lichee Nano target |
| `widora_mangopi_r1_defconfig` | `make widora_mangopi_r1_defconfig` | `board/widora/mangopi/r1/widora_mangopi_r1_defconfig` | Legacy Widora MangoPi R1 target |
| `widora_mangopi_r2_defconfig` | `make widora_mangopi_r2_defconfig` | `board/widora/mangopi/r2/widora_mangopi_r2_defconfig` | Legacy Widora MangoPi R2 target |
| `widora_mangopi_r3_defconfig` | `make widora_mangopi_r3_defconfig` | `board/widora/mangopi/r3/widora_mangopi_r3_defconfig` | Legacy Widora MangoPi R3 target |

## Relationship Between Entry Points and Canonical Configurations

Using CRA Electric Pass as an example:

```text
configs/cra_epass_defconfig
        ↓ symbolic link
board/cra/epass/cra_epass_defconfig
        ↓ make cra_epass_defconfig
repository-root .config
        ↓ make
output/
```

These files serve different purposes:

| File | Nature |
| --- | --- |
| `configs/cra_epass_defconfig` | Buildroot command entry point, normally a symbolic link |
| `board/cra/epass/cra_epass_defconfig` | Canonical source file for the CRA board configuration |
| `.config` | Fully expanded configuration used by the current output directory; local build state that should not be committed |

## Git Symbolic Links

Git should store these entry points with mode:

```text
120000
```

This mode identifies a Git symbolic link. For example, `configs/hatlab_badge200_defconfig` contains the following link target:

```text
../board/hatlab/badge200/hatlab_badge200_defconfig
```

In a normal Linux checkout, `ls -l` should show that it points to the board configuration source rather than being a separate small text file.

### Windows Checkout Problem

When symbolic-link support is not enabled on Windows, Git may check out the link as a regular file containing only one line with the target path:

```text
../board/cra/epass/cra_epass_defconfig
```

Although such a file exists, Buildroot does not interpret that line as an instruction to load another configuration. It treats the file itself as the defconfig, so the intended configuration is not loaded correctly.

Use the following commands to identify the problem:

```sh
ls -l configs/cra_epass_defconfig
file configs/cra_epass_defconfig
```

For a valid link, `ls -l` displays `->`. If the entry is reported as a regular ASCII text file, the link was damaged during checkout.

The recommended approach is to check out and build the repository in Linux or within the Linux filesystem of WSL, with Git configured to preserve symbolic links. Do not merely copy Windows-created path placeholder files into WSL.

## Loading a Configuration

Before building CRA Electric Pass, start from a clean configuration:

```sh
make distclean
make cra_epass_defconfig
```

Then inspect the key architecture options:

```sh
grep -E 'BR2_(arm|arm926t|ARM_EABI|ARM_SOFT_FLOAT|ARM_INSTRUCTIONS_ARM)' .config
```

The current CRA target should use:

- the ARM architecture;
- ARM926T;
- ARM EABI;
- soft-float;
- the ARM instruction set.

After confirming these options, build the project:

```sh
make
```

Run `make distclean` before loading a different board target as well. Do not layer CRA, BADGE200, Lichee Nano, or MangoPi configurations in the same output directory.

## Updating a Defconfig

To change Buildroot packages or system options, use the following workflow:

```sh
make cra_epass_defconfig
make menuconfig
make update-defconfig
```

`make update-defconfig` reduces the current configuration to a minimal defconfig and writes it to the entry point selected by `BR2_DEFCONFIG`. If that entry is a valid symbolic link, the write reaches `board/cra/epass/cra_epass_defconfig`.

If Windows has replaced the link with a regular path text file, the update may overwrite `configs/cra_epass_defconfig` itself without updating `board/cra/epass/cra_epass_defconfig`. This creates two conflicting configurations. Always verify the link state before running `update-defconfig`.

After updating, at minimum run:

```sh
git diff -- board/cra/epass/cra_epass_defconfig
make cra_epass_defconfig
```

Then confirm that reloading the target preserves the required architecture, toolchain, Linux kernel, U-Boot, rootfs overlays, and post-image scripts.

## Adding a Board Target

Use the following structure for a new target:

```text
board/<vendor>/<board>/<target>_defconfig
configs/<target>_defconfig -> ../board/<vendor>/<board>/<target>_defconfig
```

The target name should:

- contain only lowercase letters, numbers, and underscores;
- end in `_defconfig`;
- match the `make <target>_defconfig` command;
- clearly distinguish the vendor, board, and hardware revision;
- avoid vague names such as `default`, `test`, or `new`.

After adding the link, run:

```sh
make list-defconfigs
make <target>_defconfig
```

Confirm that Buildroot finds the entry point and that all referenced U-Boot, Linux, Device Tree, rootfs, and image-generation scripts exist.

## Removing or Renaming a Target

When removing a board target, do not delete only the `board/` directory or only the `configs/` entry point. At minimum, check and update all of the following:

1. `configs/<target>_defconfig`;
2. the canonical configuration and related files under `board/<vendor>/<board>/`;
3. target lists and links in README files;
4. automated build scripts;
5. CI, release scripts, and flashing tools;
6. other defconfigs that may depend on a shared layer being removed.

Renaming a target likewise requires updating the entry-point name, canonical configuration name, internal paths, documentation, and invocation command together. Otherwise, stale links will remain.

## Boundary with Generated Files

`configs/` is a build-input directory, not a generated-output directory. Common generated files and local build state are found at:

```text
.config
.config.old
output/build/
output/host/
output/target/
output/images/
```

Cleaning `output/` must not remove the entry points under `configs/`. Conversely, changing files under `output/` is not a substitute for updating the board defconfig.
