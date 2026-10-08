# Buildroot Documentation Sources

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the AsciiDoc sources, build rules, and supporting resources for the Buildroot 2020.02.7 user manual:

```text
docs/
```

## Directory Structure

```text
docs/
├── README.md
├── conf/
│   └── asciidoc-text.conf
├── images/
│   └── github_hash_mongrel2.png
└── manual/
    ├── manual.txt
    ├── manual.mk
    ├── docbook-xsl.css
    ├── adding-board-support.txt
    ├── adding-packages*.txt
    ├── configure*.txt
    ├── customize*.txt
    ├── rebuilding-packages.txt
    ├── writing-rules.txt
    └── other manual chapter sources
```

The current directory retains:

| Directory | File count | Purpose |
| --- | ---: | --- |
| `conf/` | 1 | Configuration for plain-text output |
| `images/` | 1 | Example image referenced by a manual chapter |
| `manual/` | 71 | Manual entry point, build rules, styles, and chapter sources |

## `conf/`

### `asciidoc-text.conf`

This file is used only when generating the Buildroot manual in plain-text format. It primarily controls:

- how HTTP, HTTPS, FTP, file, IRC, and email links appear in text output;
- how links with and without display labels are expanded;
- how image macros are hidden in plain-text output to avoid meaningless image placeholders.

Buildroot's common AsciiDoc infrastructure automatically detects and loads this file for the relevant output format.

## `images/`

This directory contains external image resources used while generating the manual.

Only the following file is currently retained:

| File | Referenced by | Purpose |
| --- | --- | --- |
| `github_hash_mongrel2.png` | `manual/adding-packages-tips.txt` | Demonstrates an example involving hashes for GitHub source archives |

The original Buildroot tree used `docs/images -> website/images` as a symbolic link. A previous Windows checkout converted that link into an ordinary text file. This project now uses a real `docs/images/` directory so that documentation builds no longer depend on that Git symbolic link.

## `manual/`

`manual/` is the main part of this directory. Except for the `.mk` and CSS files, chapters are written as AsciiDoc-style `.txt` sources.

### `manual.txt`

`manual.txt` is the entry point for the complete manual. It defines the title, version information, license notice, and chapter inclusion order.

The manual is organized into four parts:

```text
Getting started
User guide
Developer guide
Appendix
```

Chapters are assembled with statements such as:

```text
include::introduction.txt[]
include::configure.txt[]
include::adding-packages.txt[]
```

There is therefore no need to copy edited chapter text back into `manual.txt`. When the manual is regenerated, AsciiDoc reads every referenced chapter in the order defined by the entry file.

### `manual.mk`

`manual.mk` connects the Buildroot manual to the top-level build system:

```make
MANUAL_SOURCES = $(sort $(wildcard docs/manual/*.txt) $(wildcard docs/images/*))
MANUAL_RESOURCES = $(TOPDIR)/docs/images

$(eval $(call asciidoc-document))
```

It is responsible for:

- collecting all manual chapters and images as build dependencies;
- selecting the image resource directory;
- invoking the common documentation macro provided by `package/doc-asciidoc.mk`;
- defining HTML, split HTML, PDF, plain-text, and ePub outputs;
- placing generated documents in `output/docs/manual/`.

The top-level `Makefile` directly includes:

```make
include docs/manual/manual.mk
```

Consequently, the entire `docs/` directory or `docs/manual/manual.mk` cannot be removed without also updating the top-level `Makefile`. GNU Make may otherwise fail during its parsing stage even when the intended operation is only a firmware build.

### `docbook-xsl.css`

This file provides styles for the generated HTML/DocBook manual.

## Manual Chapter Categories

The chapter sources can be grouped by purpose as follows.

### Getting Started and Basic Usage

| File | Contents |
| --- | --- |
| `introduction.txt` | Buildroot goals and fundamental concepts |
| `prerequisite.txt` | Build host prerequisites |
| `getting.txt` | Obtaining the Buildroot sources |
| `quickstart.txt` | Quick build workflow |
| `resources.txt` | Buildroot community and reference resources |
| `configure.txt` | Buildroot configuration interfaces and major options |
| `configure-other-components.txt` | Configuration entry points for Linux, U-Boot, and other components |
| `common-usage.txt` | Common workflows |
| `make-tips.txt` | Make usage tips |
| `rebuilding-packages.txt` | Differences among package rebuild, reconfiguration, and full cleanup |

### Project Customization

The `customize*.txt` chapters cover:

- recommended project directory layouts;
- saving Buildroot, Linux, and U-Boot configurations;
- root filesystem overlays;
- user and device permission tables;
- post-build and post-image customization;
- project patches and custom packages;
- maintaining external projects through `BR2_EXTERNAL`.

### Board Support and Package Development

| File or group | Contents |
| --- | --- |
| `adding-board-support.txt` | Basic structure for adding a new board |
| `writing-rules.txt` | Rules for writing `Config.in`, Makefiles, and documentation |
| `adding-packages.txt` | Main entry point for adding Buildroot packages |
| `adding-packages-directory.txt` | Package directory layout |
| `adding-packages-generic.txt` | `generic-package` infrastructure |
| `adding-packages-autotools.txt` | Autotools packages |
| `adding-packages-cmake.txt` | CMake packages |
| `adding-packages-meson.txt` | Meson packages |
| `adding-packages-python.txt` | Python packages |
| `adding-packages-cargo.txt` | Cargo/Rust packages |
| `adding-packages-golang.txt` | Go packages |
| `adding-packages-kconfig.txt` | Packages that use Kconfig |
| `adding-packages-kernel-module.txt` | External Linux kernel modules |
| `adding-packages-linux-kernel-spec-infra.txt` | Linux-kernel-specific package infrastructure |
| `adding-packages-hooks.txt` | Hooks for individual build stages |
| `adding-packages-tips.txt` | Package maintenance tips |

The directory also retains documentation for Perl, LuaRocks, Rebar, Waf, virtual packages, gettext, AsciiDoc, and other package types.

### Debugging, Contributions, and Appendices

| File | Contents |
| --- | --- |
| `faq-troubleshooting.txt` | Frequently asked questions and troubleshooting |
| `known-issues.txt` | Known Buildroot limitations |
| `debugging-buildroot.txt` | Debugging the build system |
| `using-buildroot-debugger.txt` | Using the cross-debugger |
| `patch-policy.txt` | Patch formatting, ordering, and licensing rules |
| `contribute.txt` | Contributing changes upstream to Buildroot |
| `developers.txt` | The `DEVELOPERS` file and maintainer rules |
| `release-engineering.txt` | Buildroot release process |
| `legal-notice.txt` | Open-source licensing and compliance |
| `makedev-syntax.txt` | Device table syntax |
| `makeusers-syntax.txt` | User table syntax |
| `migrating.txt` | Migration notes for older releases |

## Generating the Manual

Run the following commands from the Buildroot repository root in Linux or a correctly configured WSL environment.

### Generate All Formats

```sh
make manual
```

### Generate Individual Formats

```sh
make manual-html
make manual-split-html
make manual-pdf
make manual-text
make manual-epub
```

Generated files are placed in:

```text
output/docs/manual/
```

Expected outputs include:

```text
manual.html
manual.pdf
manual.text
manual.epub
```

The split HTML target creates a multi-file page structure whose exact file names are determined by the AsciiDoc/DocBook toolchain.

### Clean Generated Documentation

```sh
make manual-clean
```

This removes generated files from the documentation build directory. It does not delete the sources under `docs/manual/` and does not affect firmware outputs.

## Documentation Build Dependencies

According to the current `package/doc-asciidoc.mk`, documentation generation requires at least:

| Tool | Purpose |
| --- | --- |
| AsciiDoc/a2x | Parses manual sources and generates the available formats |
| `w3m` | Assists with text output and document conversion |
| `rsync` | Copies manual sources into the temporary documentation build directory |
| `xsltproc` | Performs DocBook/XSL transformations |
| `dblatex` | Generates PDF output |

PDF generation has an additional `xsltproc` version requirement. The current build rules check for known issues in older releases and may disable PDF output or report a warning when the requirement is not met.
