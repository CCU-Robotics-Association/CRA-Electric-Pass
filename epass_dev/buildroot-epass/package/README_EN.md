<div align="center">

# Buildroot Package System

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `package/` contains the package definitions and build infrastructure shipped with Buildroot 2020.02.7. It describes where third-party and project sources come from, their dependencies and cross-compilation steps, and the installation boundaries between the host tools, staging sysroot, and target root filesystem.

<p align="center">
  <a href="#overview">Overview</a> ·
  <a href="#package-lifecycle">Lifecycle</a> ·
  <a href="#root-infrastructure">Infrastructure</a> ·
  <a href="#anatomy-of-a-package">Package anatomy</a> ·
  <a href="#cra-electric-pass-packages">CRA packages</a> ·
  <a href="#package-build-targets">Build targets</a> ·
  <a href="#syntax-and-comments">Syntax</a>
</p>

---

## Overview

```text
package/
├── Config.in
├── Config.in.host
├── Makefile.in
├── pkg-generic.mk
├── pkg-download.mk
├── pkg-utils.mk
├── pkg-autotools.mk
├── pkg-cmake.mk
├── pkg-kconfig.mk
├── pkg-kernel-module.mk
├── pkg-python.mk
├── pkg-meson.mk
├── pkg-waf.mk
├── pkg-golang.mk
├── pkg-perl.mk
├── pkg-luarocks.mk
├── pkg-rebar.mk
├── pkg-virtual.mk
├── epass_drm_app/
├── epass_usb_responder/
├── epassctl/
├── srgn_config/
├── libcedarc/
├── libcedarx/
└── <Buildroot upstream packages>/
```

The tree contains thousands of upstream package files. Counts are descriptive snapshots, not stable interfaces.

## Package lifecycle

A package definition may:

1. decide whether a package is enabled through Kconfig;
2. resolve direct and transitive dependencies;
3. fetch sources from Git, HTTP, mirrors, or local directories;
4. verify downloaded hashes;
5. extract sources and apply ordered patches;
6. configure and cross-compile for the target;
7. install development files into the staging sysroot;
8. install runtime files into the target rootfs;
9. record license material for `legal-info`;
10. maintain stage stamps for incremental builds.

Buildroot generates a complete firmware root filesystem; it is not an online binary package repository.

## Root infrastructure

| File | Responsibility |
| --- | --- |
| `Config.in` | Target-package menus and package `source` statements |
| `Config.in.host` | Host-only utility menus |
| `Makefile.in` | Cross tools, flags, install tools, and global build environment |
| `pkg-generic.mk` | Package lifecycle, dependencies, stamps, install, and clean targets |
| `pkg-download.mk` | Download methods, cache, and mirror fallback |
| `pkg-utils.mk` | Kconfig helpers, package-name conversion, and common functions |
| `pkg-autotools.mk` | Autoconf/Automake infrastructure |
| `pkg-cmake.mk` | CMake cross-compilation infrastructure |
| `pkg-kconfig.mk` | Configuration management for Kconfig projects |
| `pkg-kernel-module.mk` | External Linux module interface |
| `pkg-python.mk`, `pkg-meson.mk`, `pkg-waf.mk` | Language/build-system helpers |
| `pkg-golang.mk`, `pkg-perl.mk`, `pkg-luarocks.mk`, `pkg-rebar.mk` | Language package helpers |
| `pkg-virtual.mk` | Virtual packages and provider selection |
| `doc-asciidoc.mk` | Host-side Buildroot documentation rules |

CRA entries are currently included in the shared package menus. Moving their menu category is safe when symbol names remain unchanged; renaming Kconfig symbols would break defconfig compatibility.

## Anatomy of a package

```text
package/example/
├── Config.in
├── example.mk
├── example.hash
├── 0001-fix-cross-build.patch
└── 0002-fix-runtime-path.patch
```

### `Config.in`

Defines `BR2_PACKAGE_<NAME>`, architecture and toolchain constraints, selected dependencies, optional features, help text, and the upstream URL. Kconfig `depends on` controls selection only; it does not replace build dependencies in the `.mk` file.

### `<package>.mk`

| Variable | Meaning |
| --- | --- |
| `<PKG>_VERSION` | Source version, tag, or commit |
| `<PKG>_SITE` / `_SITE_METHOD` | Source location and fetch method |
| `<PKG>_SOURCE` | Non-default archive name |
| `<PKG>_LICENSE` / `_LICENSE_FILES` | License metadata and texts |
| `<PKG>_DEPENDENCIES` | Build order and sysroot dependencies |
| `<PKG>_CONF_OPTS` | Additional configuration options |
| `<PKG>_INSTALL_STAGING` | Whether development files enter staging |
| `<PKG>_INSTALL_TARGET` | Whether runtime files enter the rootfs |
| `<PKG>_*_CMDS` | Custom configure/build/install commands |

Register the package with the matching infrastructure macro:

```make
$(eval $(generic-package))
$(eval $(autotools-package))
$(eval $(cmake-package))
```

Hyphens in directory names become underscores in Make variables; for example, `fb-test-app` uses `FB_TEST_APP_*`.

### Hashes and patches

`.hash` files verify archives and license texts:

```text
sha256  <SHA-256>  <filename>
```

Numbered patches apply in filename order. Each patch should document its purpose, upstream status, and origin, and the entire series must be revalidated after a source-version change.

## Target, staging, and host

| Directory | Contents |
| --- | --- |
| `dl/` | Download cache |
| `output/build/<pkg>-<version>/` | Temporary extracted, patched, and compiled source |
| `output/host/` | Host tools and the cross toolchain |
| `output/staging/` | Development view of the target sysroot |
| `output/target/` | Unpacked target root filesystem |
| `output/images/` | Final kernel, rootfs, bootloader, and combined images |

Common variables include `$(@D)`, `$(TARGET_DIR)`, `$(STAGING_DIR)`, `$(HOST_DIR)`, `$(TARGET_CC)`, `$(TARGET_CROSS)`, and `$(TARGET_CONFIGURE_OPTS)`. Never use `output/build/` or `output/target/` as the canonical source of a lasting change.

## CRA Electric Pass packages

The active configuration is `board/cra/epass/cra_epass_defconfig`.

| Package | Purpose | Installed path | Build system |
| --- | --- | --- | --- |
| `epass_drm_app` | Main DRM/LVGL interface | `/root/epass_drm_app` | CMake |
| `epass_usb_responder` | USB request responder | `/usr/bin/usb_responder` | CMake |
| `epassctl` | Main-application CLI | `/usr/bin/epassctl` | CMake |
| `srgn_config` | Device-tree/low-level configuration entry | `/usr/bin/srgn_config` | CMake |

Main multimedia dependencies include `libcedarc`, `libcedarx`, `tinyalsa`, `libdrm`, `libpng`, `jpeg-turbo`, and FreeType. The older `sunxi-cedarx` package is not selected by the current CRA defconfig and is distinct from the selected `libcedarc + libcedarx` stack.

The application package currently fetches a pinned revision from `https://github.com/rhodesepass/drm_app_neo.git`. For local development, use an uncommitted top-level `local.mk` override:

```make
EPASS_DRM_APP_OVERRIDE_SRCDIR = $(TOPDIR)/../drm_app_neo
```

## Package build targets

```sh
make <package>
make <package>-source
make <package>-rebuild
make <package>-reconfigure
make <package>-dirclean
```

| Target | Effect |
| --- | --- |
| `<package>` | Builds the package and required dependencies |
| `<package>-source` | Fetches only the package source |
| `<package>-rebuild` | Repeats build and install stages |
| `<package>-reconfigure` | Reconfigures, rebuilds, and reinstalls |
| `<package>-dirclean` | Removes the package build directory |

These targets may change `output/`, but they do not deploy to a physical device.

## Syntax and comments

| File | Language | Requirements |
| --- | --- | --- |
| `Config.in` | Kconfig | `#` comments; preserve indentation below `help` |
| `.mk` | GNU Make | `#` comments; recipe lines require tabs |
| `.hash` | Whitespace-delimited text | `#` comments; hashes and names must exactly match downloads |
| `.patch` | Unified/mail patch | Preserve headers, context, and original line endings |

---

<div align="center">

<sub><b>package/</b> · package metadata and build infrastructure for Buildroot</sub>

</div>
