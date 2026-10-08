<div align="center">

# Buildroot Cross-Toolchain Infrastructure

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `toolchain/` contains Buildroot 2020.02.7's cross-toolchain configuration, generation rules, compiler wrapper, and external-toolchain adapters. It determines the C library, compiler capabilities, target ABI, and the sysroot used to build target software.

<p align="center">
  <a href="#directory-layout">Layout</a> ·
  <a href="#responsibilities">Responsibilities</a> ·
  <a href="#current-cra-electric-pass-toolchain">Current toolchain</a> ·
  <a href="#internal-toolchain-build-flow">Build flow</a> ·
  <a href="#generated-output">Output</a> ·
  <a href="#recommended-application-workflow">Usage</a> ·
  <a href="#verification">Verification</a> ·
  <a href="#development-guidelines">Development</a>
</p>

---

## Directory layout

```text
toolchain/
├── Config.in
├── helpers.mk
├── toolchain.mk
├── toolchain-wrapper.c
├── toolchain-wrapper.mk
├── toolchain/toolchain.mk
├── toolchain-buildroot/
│   ├── Config.in
│   └── toolchain-buildroot.mk
└── toolchain-external/
    ├── Config.in
    ├── pkg-toolchain-external.mk
    ├── toolchain-external.mk
    ├── custom/
    └── <vendor and architecture definitions>/
```

## Responsibilities

The toolchain layer:

1. selects an internally built or imported external toolchain;
2. selects glibc, uClibc-ng, or musl;
3. determines CPU, instruction set, endianness, ABI, and floating-point ABI;
4. builds or imports Binutils, GCC, Linux headers, and the C runtime;
5. creates the target sysroot;
6. injects consistent Buildroot compiler flags through a wrapper;
7. publishes capabilities such as threads, C++, SSP, OpenMP, and locale to Kconfig;
8. validates external-toolchain declarations;
9. exposes `TARGET_CC`, `TARGET_CXX`, `TARGET_LD`, and related variables to packages.

It does not define the UI, device tree, kernel drivers, rootfs content, or flashing procedure.

## Current CRA Electric Pass toolchain

`board/cra/epass/cra_epass_defconfig` selects a Buildroot-built internal toolchain.

| Property | Current value | Meaning |
| --- | --- | --- |
| Architecture | 32-bit little-endian ARM | `BR2_arm=y` |
| CPU | ARM926EJ-S | Buildroot `arm926t` variant |
| ISA | ARMv5TEJ | No ARMv7/ARMv8 instructions |
| Instruction mode | ARM | `-marm` |
| ABI | AAPCS Linux / EABI | `-mabi=aapcs-linux` |
| Floating point | Soft float | `-mfloat-abi=soft` |
| MMU | Present | Standard Linux ELF programs |
| C library | glibc | Built internally |
| C++ | Enabled | Target sysroot includes libstdc++ |
| LTO | Enabled | Link-time optimization is available |

The GNU target triplet and prefix are:

```text
arm-buildroot-linux-gnueabi
arm-buildroot-linux-gnueabi-
```

Do not substitute `arm-linux-gnueabihf` or the bare-metal `arm-none-eabi` toolchain. Similar names do not imply ABI or sysroot compatibility.

Default components in this Buildroot snapshot are GCC 8.4.0, Binutils 2.32, a pinned glibc 2.30 revision, and Linux UAPI headers from the selected 5.4.99 source. Changing any ABI component requires a complete rebuild of target programs and libraries.

## Internal toolchain build flow

```text
Binutils
   ↓
Initial GCC
   ↓
Linux UAPI headers
   ↓
glibc
   ↓
Final GCC / G++
   ↓
Buildroot wrapper, sysroot, and target runtime libraries
```

The initial compiler breaks the compiler/C-library dependency cycle. `toolchain-buildroot/toolchain-buildroot.mk` registers the final compiler as the provider; users should not manually execute individual stages.

## Main files

### `Config.in`

Defines internal/external selection, C-library features, wchar/locale/thread/RPC/SSP/OpenMP/PIE capabilities, target optimization, gconv modules, version capability symbols, and known architecture/compiler restrictions. Many packages depend on these symbols.

### `toolchain.mk`

Performs target cleanup after the toolchain is ready. For glibc, it copies only selected gconv modules and creates a reduced module list for a size-constrained rootfs.

### `helpers.mk`

Provides external-toolchain import and validation helpers: runtime-library discovery, sysroot and multilib detection, kernel-header checks, ARM/floating ABI checks, feature detection, C-library identification, and required symlink creation.

### Compiler wrapper

`toolchain-wrapper.c` runs before the real `.br_real` compiler and adds the correct sysroot, CPU/ABI flags, global optimization/security flags, reproducibility settings, PIE/RELRO behavior, and ccache integration. `toolchain-wrapper.mk` compiles and installs it.

To inspect wrapper arguments temporarily:

```sh
export BR2_DEBUG_WRAPPER=2
```

Unset it after debugging. Do not bypass the wrapper for long-lived builds.

### Internal and external providers

`toolchain-buildroot/` describes the source-built toolchain. `toolchain/toolchain.mk` is the virtual provider selected by the configuration. `toolchain-external/` imports compatible vendor or custom toolchains and verifies architecture, endianness, ABI, C library, headers, threads, and language features.

An external toolchain is not merely an arbitrary `CROSS_COMPILE` prefix; every declared ABI and runtime property must match.

## Generated output

```text
output/
├── host/
│   ├── bin/arm-buildroot-linux-gnueabi-*
│   ├── arm-buildroot-linux-gnueabi/sysroot/
│   ├── environment-setup
│   └── share/buildroot/toolchainfile.cmake
└── staging -> host/arm-buildroot-linux-gnueabi/sysroot
```

`output/host/bin/` contains tools that run on the build computer. The sysroot contains target ARM headers and libraries; `output/staging` is its convenience link. `output/target` is runtime content, not a development sysroot.

The source directory `toolchain/` only defines how to create these files. It is not itself an installed ARM compiler.

## Why the sysroot matters

The sysroot presents the target's `/usr/include`, `/usr/lib`, and `/lib` to the compiler. Never mix it with the build host's native headers/libraries or with another distribution's cross-toolchain sysroot. A binary may link successfully yet fail on-device due to the ELF interpreter, glibc symbol versions, or floating-point ABI.

## Recommended application workflow

### Build through Buildroot

This is the preferred path. For a neighboring local `drm_app_neo` checkout, use an uncommitted `local.mk`:

```make
EPASS_DRM_APP_OVERRIDE_SRCDIR = $(TOPDIR)/../drm_app_neo
```

Then run:

```sh
make epass_drm_app-reconfigure
```

Use `make epass_drm_app-rebuild` when only the build/install stages must repeat. Neither command deploys to a physical device.

### Build in the exported environment

```sh
. output/host/environment-setup
```

This sets `PATH`, `CC`, `CXX`, `AR`, `LD`, `PKG_CONFIG`, and `CROSS_COMPILE`. CMake may also use:

```sh
cmake -S <source> -B <build> \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/output/host/share/buildroot/toolchainfile.cmake"
cmake --build <build>
```

Do not commit host-specific Windows workspace paths.

## Verification

```sh
output/host/bin/arm-buildroot-linux-gnueabi-gcc -dumpmachine
output/host/bin/arm-buildroot-linux-gnueabi-gcc -dumpfullversion
output/host/bin/arm-buildroot-linux-gnueabi-gcc -print-sysroot
output/host/bin/arm-buildroot-linux-gnueabi-readelf -h <target-program>
output/host/bin/arm-buildroot-linux-gnueabi-readelf -A <target-program>
file <target-program>
```

Expect a 32-bit little-endian ARM ELF, ARMv5/soft-float attributes, and the current Buildroot sysroot/runtime. Use the target `readelf` and `strip`, not the host-native tools.

## Current caveats

- A Windows checkout may contain CRLF-converted shell scripts and symbolic links restored as plain files. Generate the toolchain in a Linux filesystem that preserves LF, executable bits, and symlinks.
- A source tree containing `toolchain/` does not imply that `output/host/` and the compiler have been generated.
- `package/libcedarc/libcedarc.mk` contains a hard-coded `arm-buildroot-linux-gnueabi` sysroot path; `$(STAGING_DIR)` would be more portable.
- LTO objects and archives must be processed by the matching GCC, `gcc-ar`, and `gcc-ranlib` versions.

## Development guidelines

- Keep device business logic out of upstream toolchain infrastructure.
- Retain ARM926EJ-S, ARMv5, EABI soft-float, and glibc unless intentionally performing an ABI migration.
- Build all target programs and libraries with the same Buildroot compiler and sysroot.
- Do not mix hard-float, bare-metal, or host-native libraries.
- Do not modify `output/host/` or `output/staging/` as a source fix.
- Perform a full rebuild after any compiler, C-library, headers, or ABI change.
- Keep building, verification, and physical-device deployment as separate steps.

---

<div align="center">

<sub><b>toolchain/</b> · compiler, C library, sysroot, and ABI infrastructure for Buildroot</sub>

</div>
