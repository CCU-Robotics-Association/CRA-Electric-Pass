<div align="center">

# Buildroot Host Support Tools

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `support/` contains the host-side dependency checks, download backends, Kconfig implementation, patching and license helpers, SDK utilities, statistics, and automated tests shipped with Buildroot 2020.02.7.

<p align="center">
  <a href="#directory-layout">Layout</a> ·
  <a href="#relationship-to-normal-builds">Build relation</a> ·
  <a href="#dependencies">Dependencies</a> ·
  <a href="#download">Downloads</a> ·
  <a href="#kconfig">Kconfig</a> ·
  <a href="#scripts">Scripts</a> ·
  <a href="#testing">Tests</a> ·
  <a href="#syntax-and-comments">Syntax</a>
</p>

---

## Directory layout

```text
support/
├── config-fragments/
├── dependencies/
├── docker/
├── download/
├── gnuconfig/
├── kconfig/
├── legal-info/
├── libtool/
├── misc/
├── scripts/
└── testing/
```

The directory mixes Shell, Python, C, Make, Kconfig, configuration fragments, patches, and test assets.

## Relationship to normal builds

| Directory | Used by a normal build | Role |
| --- | --- | --- |
| `dependencies/` | Yes | Validates host tools and environment |
| `download/` | Yes | Downloads, exports, and verifies package sources |
| `gnuconfig/` | Yes | Refreshes `config.guess` and `config.sub` for Autotools packages |
| `kconfig/` | Yes | Implements `defconfig`, `menuconfig`, `nconfig`, and related frontends |
| `libtool/` | Yes | Fixes package-supplied Libtool scripts for cross compilation |
| `misc/` | Depending on configuration | CMake toolchain, SDK relocation, and target warnings |
| `scripts/` | Yes or target-specific | Patching, users, RPATH, graphs, and size analysis |
| `legal-info/` | `legal-info` only | License report templates and Buildroot hashes |
| `config-fragments/` | Normally no | Autobuilder and minimal test configurations |
| `docker/` | No | Historical upstream CI container definition |
| `testing/` | No | QEMU/runtime test framework |

The CRA configuration is `board/cra/epass/cra_epass_defconfig`; the files in `config-fragments/` are upstream test material and are not merged into it automatically.

## `dependencies/`

`dependencies.mk` and `dependencies.sh` check the host rather than the physical pass device. They validate environment variables, core commands, GNU Make/GCC/patch/archive utilities, download tools, filesystem behavior, and version compatibility. Specialized `check-host-*` files cover CMake, Python, tar, compression tools, coreutils, Bison/Flex, and documentation tooling.

These checks target Linux/Unix hosts, not native PowerShell or CMD environments.

## `download/`

`package/pkg-download.mk` calls the unified download layer. `dl-wrapper` downloads into a temporary directory, tries the primary source and mirrors, invokes the appropriate backend, checks the package hash, atomically moves successful results into `dl/`, and removes incomplete files on failure.

| Backend | Source type |
| --- | --- |
| `wget` | HTTP, HTTPS, FTP, and ordinary files |
| `git`, `svn`, `hg`, `cvs`, `bzr` | Version-control exports |
| `scp` | SSH/SCP transfer |
| `file` | Local files or directories |

`check-hash` distinguishes matching hashes, mismatches, missing entries, unsupported algorithms, and packages without a hash file. Downloading affects the network and `dl/`; it does not connect to or flash the target device.

## `gnuconfig/`

Contains `config.guess` and `config.sub`, used to identify build and target triplets. Buildroot may replace stale copies in Autotools packages. `README.buildroot` records the upstream GNU config revision used by this snapshot.

## `kconfig/`

This is the Linux kernel Kconfig implementation adapted for Buildroot. Its base is documented in `README.buildroot`.

| File | Function |
| --- | --- |
| `conf.c` | Non-interactive config, oldconfig, and defconfig handling |
| `mconf.c` + `lxdialog/` | `make menuconfig` |
| `nconf.c` | `make nconfig` |
| `qconf.cc` | Qt `make xconfig` |
| `gconf.c`, `gconf.glade` | GTK `make gconfig` |
| `confdata.c`, `symbol.c`, `expr.c`, `menu.c` | Parsing and symbol evaluation core |
| `merge_config.sh` | Configuration-fragment merging |

The patches in `kconfig/patches/` adapt the kernel tool for Buildroot's `BR2_` namespace, external trees, output directory, and interface behavior.

## `legal-info/`, `libtool/`, and `misc/`

- `legal-info/` provides templates and hashes for `make legal-info`, which gathers package lists, versions, licenses, redistributable sources, patches, license texts, and warnings.
- `libtool/` contains Buildroot fixes for several historical Libtool branches, mainly for cross-compilation, sysroot, static linking, and install paths.
- `misc/` includes CMake support, a generated toolchain-file template, SDK relocation, target-directory warnings, shared Make helpers, and a historical Vagrant setup.

`output/target/` is not a deployable root filesystem by itself: its final ownership, device nodes, and fakeroot-stage permissions are not yet represented. Deploy images from `output/images/`.

## `scripts/`

Common build helpers include:

| Script | Purpose |
| --- | --- |
| `apply-patches.sh` | Applies ordered package patch series |
| `check-bin-arch` | Detects host binaries accidentally installed into the target |
| `check-host-rpath` / `fix-rpath` | Validates and sanitizes host/staging/target RPATHs |
| `check-kernel-headers.sh` | Checks toolchain kernel-header versions |
| `check-merged-usr.sh` | Validates skeletons and overlays for merged `/usr` |
| `mkmakefile` | Creates an output-tree forwarding Makefile |
| `mkusers` | Converts users tables into accounts and fakeroot actions |
| `pycompile.py` | Generates target Python bytecode during cross builds |
| `setlocalversion` | Derives local version suffixes from VCS state |

Maintenance and analysis tools include `br2-external`, `generate-gitlab-ci-yml`, `genimage.sh`, `graph-build-time`, `graph-depends`, `pkg-stats`, and `size-stats`.

## `testing/`

The Python test framework builds configurations and runs them in QEMU. `run-tests` is the entry point; `infra/` supplies builder, emulator, and base-test classes; `tests/` covers core behavior, filesystems, init systems, downloads, packages, and toolchains. It is upstream Buildroot test infrastructure, not an on-device CRA test suite.

## Syntax and comments

| Type | Language | Convention |
| --- | --- | --- |
| `.sh` or extensionless scripts | POSIX Shell/Bash | `#` comments; preserve LF and shebang |
| `.py` | Python | `#` comments or docstrings |
| `.mk` | GNU Make | `#` comments; recipe lines require tabs |
| `.c/.h` | C/C++ | Follow the existing `/* ... */` or `//` style |
| `.config` fragments | Kconfig | `#` comments and `# CONFIG_x is not set` |
| `.patch` | Unified diff | Preserve headers, context, and original line endings |

---

<div align="center">

<sub><b>support/</b> · host-side helpers, infrastructure, and tests for Buildroot</sub>

</div>
