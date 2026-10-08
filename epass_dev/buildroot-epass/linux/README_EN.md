<div align="center">

# Buildroot Linux Kernel Infrastructure

<sub>Linux source, configuration, patching, and image integration</sub>

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `linux/` implements Buildroot's Linux kernel package. It selects, downloads, verifies, extracts, patches, configures, builds, and installs the kernel and its device trees. CRA-specific kernel configuration, device trees, and patches live under `board/cra/epass/` and the shared Allwinner board directories.

<p align="center">
  <a href="#directory-layout">Directory layout</a> ·
  <a href="#file-reference">File reference</a> ·
  <a href="#patch-order">Patch order</a> ·
  <a href="#source-and-generated-files">Maintenance boundaries</a> ·
  <a href="#common-build-commands">Build commands</a> ·
  <a href="#recommended-development-workflow">Development</a>
</p>

---

## Directory layout

```text
linux/
├── Config.in
├── Config.ext.in
├── linux.mk
├── linux.hash
├── 0001-timeconst.pl-Eliminate-Perl-warning.patch.conditional
├── linux-ext-aufs.mk
├── linux-ext-ev3dev-linux-drivers.mk
├── linux-ext-fbtft.mk
├── linux-ext-rtai.mk
└── linux-ext-xenomai.mk
```

## File reference

### `Config.in`

Defines the `Kernel` menu: whether Linux is built, the source and version, patches, defconfig or custom configuration, configuration fragments, boot logo, image and compression formats, device trees and overlays, target installation, and host-side dependencies. CRA's selected values come from `board/cra/epass/cra_epass_defconfig`.

### `Config.ext.in`

Defines optional kernel extensions:

| Extension | Purpose |
| --- | --- |
| Xenomai | Adds Adeos/I-pipe real-time support on compatible architectures |
| RTAI | Applies the RTAI real-time kernel layer |
| ev3dev drivers | Adds LEGO MINDSTORMS EV3 drivers |
| FBTFT | Adds small TFT framebuffer drivers to older kernels |
| AUFS | Adds AUFS patches and modules |

### `linux.mk`

This is the central build recipe. It resolves the source version and URL, declares licensing and dependencies, downloads and applies patches, prepares Kconfig, copies project DTS/DTSI files into the temporary kernel tree, builds images/DTBs/modules, installs results, connects optional extensions, and exposes configuration-save and rebuild targets.

### `linux.hash`

Stores SHA-256 checksums for supported Linux archives and license files. The current snapshot covers the kernel versions known to Buildroot 2020.02.7, including 5.4.70, several 4.x LTS releases, and CIP archives.

### Conditional Perl compatibility patch

`0001-timeconst.pl-Eliminate-Perl-warning.patch.conditional` fixes an obsolete construct in older kernels' `kernel/timeconst.pl`. Buildroot first tests whether the patch applies: affected kernels receive it, while already-fixed kernels skip it.

### `linux-ext-*.mk`

| File | Integration behavior |
| --- | --- |
| `linux-ext-aufs.mk` | Adds AUFS patches, sources, and UAPI headers |
| `linux-ext-ev3dev-linux-drivers.mk` | Copies ev3dev drivers into `drivers/lego/` |
| `linux-ext-fbtft.mk` | Adds FBTFT to older framebuffer trees |
| `linux-ext-rtai.mk` | Selects and applies an RTAI HAL patch by kernel/architecture |
| `linux-ext-xenomai.mk` | Runs Xenomai's `prepare-kernel.sh` |

These hooks run only when the corresponding `BR2_LINUX_KERNEL_EXT_*` option is enabled.

---

## Patch order

The CRA configuration lists patch directories in this order:

```text
board/allwinner/suniv-f1c100s/patch/linux
board/cra/epass/patch/linux
```

`linux.mk` walks them sequentially and applies each directory's `*.patch` files in filename order. Shared F1C100S/F1C200S fixes therefore apply first, followed by CRA-specific changes. Use stable numeric prefixes for new patches and validate the full series from a clean source tree.

---

## Source and generated files

Commit the following canonical sources:

| Content | Canonical location |
| --- | --- |
| Buildroot kernel package rules | `linux/` |
| CRA Buildroot target configuration | `board/cra/epass/cra_epass_defconfig` |
| CRA kernel configuration | `board/cra/epass/linux.defconfig` |
| CRA Linux patches | `board/cra/epass/patch/linux/` |
| Shared Allwinner patches | `board/allwinner/suniv-f1c100s/patch/linux/` |
| CRA Linux device trees | `board/cra/epass/devicetree/linux/` |
| Shared Allwinner DTSI files | `board/allwinner/suniv-f1c100s/devicetree/linux/` |

Do not treat copied or patched files below `output/build/linux-<version>/` as source. That directory is disposable build output.

---

## Common build commands

Run these commands from the Buildroot root in Linux or a suitable WSL environment.

### Full build

```sh
make cra_epass_defconfig
make -j$(nproc)
```

This builds the toolchain, Linux, U-Boot, root filesystem, and final images. It does not flash a physical device.

### Configure and save

```sh
make linux-menuconfig
make linux-update-defconfig
```

`linux-menuconfig` edits the temporary kernel `.config`. `linux-update-defconfig` saves a minimal configuration back to `board/cra/epass/linux.defconfig`. Use `make linux-update-config` only when a full `.config` is intentionally required.

### Incremental rebuild

```sh
make linux-rebuild -j$(nproc)
```

This reruns compilation and installation without downloading, extracting, or replaying the patch series. After rebuilding, run `make` once more to refresh final images that consume the kernel output.

### Clean kernel rebuild

```sh
make linux-dirclean
make -j$(nproc)
```

Use `linux-dirclean` after adding, removing, renaming, or changing patches; changing the kernel version/source; manually damaging the temporary tree; or suspecting stale build state. It removes the temporary kernel build tree but does not flash hardware.

---

## Recommended development workflow

### Kernel options

```sh
make linux-menuconfig
make linux-update-defconfig
git diff -- board/cra/epass/linux.defconfig
```

### Device tree

Choose the source location by scope:

- shared F1C100S/F1C200S definitions: `board/allwinner/suniv-f1c100s/devicetree/linux/`;
- CRA Electric Pass shared board definitions: `board/cra/epass/devicetree/linux/base/epass.dtsi`;
- active entry point: `board/cra/epass/devicetree/linux/base/devicetree.dts`.

Do not edit copied files inside `output/build/linux-5.4.99/arch/arm/boot/dts/` as permanent source.

### Kernel source or drivers

1. Validate the change in a temporary kernel tree.
2. Export the change as a standard Git patch.
3. Place CRA-specific patches in `board/cra/epass/patch/linux/`.
4. Use sequential numeric filenames.
5. Run `make linux-dirclean`.
6. Rebuild and verify that the series applies cleanly.

Move a change to the shared Allwinner patch directory only when it is valid for every supported suniv F1C100S/F1C200S board.

---

<div align="center">

<sub><b>linux/</b> · Linux kernel build infrastructure for Buildroot</sub>

</div>
