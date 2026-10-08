<div align="center">

# Buildroot Developer Utilities

<sub>Configuration, package, analysis, and maintenance utilities for Buildroot</sub>

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `utils/` contains host-side tools for Buildroot 2020.02.7 developers and maintainers. They manipulate configurations, check package metadata, generate test configurations and package skeletons, locate maintainers, and compare image sizes. They are not installed into the CRA Electric Pass root filesystem.

<p align="center">
  <a href="#responsibilities">Responsibilities</a> ·
  <a href="#tool-groups">Tools</a> ·
  <a href="#common-usage">Usage</a> ·
  <a href="#dependencies-and-compatibility">Compatibility</a> ·
  <a href="#maintenance-and-safety-boundaries">Boundaries</a>
</p>

---

## Responsibilities

| Area | Purpose |
| :--- | :--- |
| Configuration | Read, edit, compare, or randomly generate Buildroot configurations |
| Quality checks | Check package metadata, patches, and style; test packages across toolchains |
| Package generation | Generate initial PyPI or CPAN package definitions for review |
| Maintenance analysis | Find maintainers, log builds, and compare rootfs size results |

```text
utils/
├── brmake
├── check-package
├── checkpackagelib/
├── config
├── diffconfig
├── genrandconfig
├── get-developers
├── getdeveloperlib.py
├── scancpan
├── scanpypi
├── size-stats-compare
├── test-pkg
└── readme.txt
```

The upstream `readme.txt` files remain authoritative for their scripts; this document is a project-level index.

## Tool groups

### Configuration and builds

| Tool | Purpose | Main effect |
| :--- | :--- | :--- |
| `brmake` | Wraps `make`, timestamps logs, writes `br.log`, and filters terminal output | Runs the requested build and writes a log |
| `config` | Enables, disables, sets, or queries Kconfig symbols | Directly edits the selected `.config`; does not resolve dependencies |
| `diffconfig` | Shows Kconfig differences between configurations | Read-only unless its output is redirected |
| `genrandconfig` | Generates randomized autobuilder configurations | May access the network and feed later builds |

### Package maintenance

| Tool | Purpose |
| :--- | :--- |
| `check-package` | Checks `Config.in`, `.mk`, `.hash`, and patch conventions |
| `checkpackagelib/` | Python rule modules used by `check-package` |
| `test-pkg` | Tests one package against multiple toolchain configurations |
| `scanpypi` | Generates an initial Python package skeleton from PyPI metadata |
| `scancpan` | Generates an initial Perl package skeleton from CPAN metadata |

### Analysis and collaboration

| Tool | Purpose |
| :--- | :--- |
| `get-developers` | Finds maintainers affected by a patch, path, package, or architecture |
| `getdeveloperlib.py` | Parsing and lookup library for `get-developers` |
| `size-stats-compare` | Compares two `file-size-stats.csv` reports |

## Common usage

Run these commands from the Buildroot root.

```sh
./utils/check-package package/<name>/Config.in \
  package/<name>/<name>.mk \
  package/<name>/<name>.hash

./utils/diffconfig .config.old .config
./utils/diffconfig -m .config.old .config

./utils/config --file .config --state BR2_PACKAGE_BUSYBOX
cp .config /tmp/cra-epass.config
./utils/config --file /tmp/cra-epass.config --enable BR2_PACKAGE_BUSYBOX

./utils/get-developers -f board/cra/epass/
./utils/get-developers -p busybox

./utils/size-stats-compare before/file-size-stats.csv after/file-size-stats.csv
```

> [!IMPORTANT]
> `utils/config` normalizes symbols to uppercase and normally adds the `BR2_` prefix. It does not enforce Kconfig dependencies; run `make olddefconfig` or the relevant configuration target afterward.

## Dependencies and compatibility

| Category | Notes |
| :--- | :--- |
| Shell tools | Require a Linux/Unix shell; `brmake` also uses `unbuffer` from Expect |
| Python tools | Some scripts retain Python 2/3 compatibility and may require modules such as `six` |
| Perl tools | `scancpan` requires Perl and includes FatPacker-bundled modules |
| Network access | `genrandconfig`, `scanpypi`, and `scancpan` may query external services |
| Checkout | Run in a Linux tree that preserves LF, executable bits, and symbolic links |

> [!WARNING]
> The Windows checkout is suitable for browsing and editing, but PowerShell compatibility is not implied. Do not carry CRLF damage, lost executable bits, or broken symbolic links into the production build tree.

## Maintenance and safety boundaries

- This directory is mostly upstream Buildroot infrastructure; keep it aligned with the pinned Buildroot version.
- Read each tool's `--help` or source before running it.
- Back up a configuration before automated edits and re-run Kconfig afterward.
- `test-pkg`, `genrandconfig`, and `brmake` can perform real builds and change output directories.
- `scanpypi` and `scancpan` produce starting points only; review licenses, sources, dependencies, hashes, and installation paths.
- `get-developers` provides collaboration metadata, not a substitute for review or build validation.
- These tools do not flash firmware. Keep any later device-writing operation explicit and separate.

---

<div align="center">

<sub><b>utils/</b> · developer utilities shipped with Buildroot 2020.02.7</sub>

</div>
