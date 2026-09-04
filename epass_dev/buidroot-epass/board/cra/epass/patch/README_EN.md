<div align="center">

# CRA Electric Pass Board Patches

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> This directory contains the board-specific source patches that CRA Electric Pass applies to **Linux 5.4.99** and **U-Boot** during the Buildroot build.

<p align="center">
  <a href="#directory-structure">Directory Structure</a> ·
  <a href="#patch-architecture">Patch Architecture</a> ·
  <a href="#application-order">Application Order</a> ·
  <a href="#linux-patches">Linux</a> ·
  <a href="#u-boot-patches">U-Boot</a> ·
  <a href="#cross-directory-coupling">Coupling</a> ·
  <a href="#correct-rebuild-procedure">Rebuild Procedure</a> ·
  <a href="#windows-and-line-endings">Windows / LF</a> ·
  <a href="#validation-status">Validation Status</a>
</p>

---

## Directory Structure

<table>
<tr>
<td width="50%" valign="top">

### `linux/`

**Target: Linux 5.4.99**

Contains **13 patches**

Covers:

- Display and panel initialization
- ADC
- USB
- Private DRM interface
- CardKB
- I²S / ES8311
- GPIO UAPI

[Linux patch details](linux/README_EN.md)

</td>
<td width="50%" valign="top">

### `uboot/`

**Target: U-Boot**

Contains **5 patches**

Covers:

- UART0
- USB DFU
- SPI-NAND
- SUNIV SPI clocks
- NAND post-write verification

[U-Boot patch details](uboot/README_EN.md)

</td>
</tr>
</table>

```text
patch/
├─ linux/
│  ├─ 0000-f1c100s-gpadc-regs.patch
│  ├─ 0001-epass-icon.patch
│  ├─ 0002-panel-simple.patch
│  ├─ 0003-f1c100s-defe-debe-fix.patch
│  ├─ 0004-swap_rb_as_config.patch
│  ├─ 0005-gpadc-low-freq.patch
│  ├─ 0006-initalize-st7701.patch
│  ├─ 0007-srgn-drm-atomic-ioctl.patch
│  ├─ 0008-force-usb-fs-dt-switch.patch
│  ├─ 0009-m5stack-cardkb-driver.patch
│  ├─ 0010-i2s-and-es-driver.patch
│  ├─ 0011-fbcon-cra-width-hack.patch
│  ├─ 0012-gpio-backport-pulls.patch
│  └─ README.md
├─ uboot/
│  ├─ 0001-uart-pull.patch
│  ├─ 0002-musb-force-fs.patch
│  ├─ 0003-spi-nand-mx35lf1g.patch
│  ├─ 0004-f1c-spi-fix.patch
│  ├─ 0005-dfu-verify-block.patch
│  └─ README.md
└─ README.md
```

| Subdirectory | Target source | Patch count | Details |
| :--- | :--- | ---: | :--- |
| `linux/` | Linux 5.4.99 | 13 | [Linux patch details](linux/README_EN.md) |
| `uboot/` | U-Boot | 5 | [U-Boot patch details](uboot/README_EN.md) |

---

## Patch Architecture

Both Linux and U-Boot use a two-tier patch structure in CRA Electric Pass.

```mermaid
flowchart TB
    A["Official upstream source"]
    B["Shared SUNIV / F1C100S patches"]
    C["CRA Electric Pass board patches"]
    D["Board defconfig"]
    E["Build artifacts"]

    A --> B --> C --> D --> E
```

Buildroot configuration entry point:

```text
board/cra/epass/cra_epass_defconfig
```

### Linux

```make
BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE="5.4.99"
BR2_LINUX_KERNEL_PATCH="board/allwinner/suniv-f1c100s/patch/linux board/cra/epass/patch/linux"
BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE="board/cra/epass/linux.defconfig"
```

### U-Boot

```make
BR2_TARGET_UBOOT_CUSTOM_VERSION_VALUE="2020.07"
BR2_TARGET_UBOOT_PATCH="board/allwinner/suniv-f1c100s/patch/u-boot board/cra/epass/patch/uboot"
BR2_TARGET_UBOOT_CUSTOM_CONFIG_FILE="board/cra/epass/uboot.defconfig"
```

> [!IMPORTANT]
> The shared patches provide the SoC-level foundation, while the patches in this directory implement CRA Electric Pass board features and legacy adaptations. The two tiers have contextual dependencies, so the CRA patches cannot be applied directly to the original source without the shared SUNIV patches.

---

## Application Order

Buildroot applies patches sequentially in lexical file-name order:

```text
0000
0001
0002
...
0012
```

The numbering determines both ordering and dependencies between patches.

For example:

```mermaid
flowchart LR
    A["Linux 0006<br/>Creates drivers/staging/cra/<br/>and base Kconfig"]
    B["Linux 0009<br/>Adds the CardKB driver<br/>and Kconfig option"]

    A --> B
```

> [!WARNING]
> Renaming, moving, or inserting a patch may change the application order. After modifying an earlier patch, verify that every later patch still matches the resulting source context.

### Patch Validation Is Not a Single Step

```mermaid
flowchart LR
    A["Diff parses"] --> B["Complete patch set applies"]
    B --> C["Kconfig recognizes options"]
    C --> D["Code compiles"]
    D --> E["Image boots"]
    E --> F["Physical hardware works"]
```

Each stage requires independent validation.

---

# Linux Patches

The Linux patches primarily cover:

| Feature | Related patches |
| :--- | :--- |
| GPADC registers / sampling / filtering | `0000`, `0005` |
| Framebuffer boot logo | `0001` |
| 384×640 panel timings | `0002` |
| DEFE / DEBE, scaling, and YUV | `0003` |
| Red/blue channel swapping | `0004` |
| ST7701 GPIO initialization | `0006` |
| Private DRM interface for `drm_app_neo` | `0007` |
| MUSB Full-Speed / High-Speed selection | `0008` |
| M5Stack CardKB | `0009` |
| I²S / ES-series codecs | `0010` |
| Framebuffer console width workaround | `0011` |
| GPIO bias / runtime configuration | `0012` |

### Feature Groups

<table>
<tr>
<td width="33%" valign="top">

### Display

`0001` `0002` `0003`  
`0004` `0006` `0007` `0011`

LCD, ST7701, DEFE/DEBE, DRM, and fbcon.

</td>
<td width="33%" valign="top">

### Input / Audio / ADC

`0000` `0005`  
`0009` `0010`

GPADC, CardKB, I²S, and ES8311.

</td>
<td width="33%" valign="top">

### USB / GPIO

`0008` `0012`

USB speed control and the GPIO UAPI backport.

</td>
</tr>
</table>

### High-Risk Maintenance Areas

| Patch | Risk |
| :--- | :--- |
| `0003` | Closely tied to the current resolution, YUV path, and vendor BSP parameters |
| `0007` | Private DRM UAPI, direct register access, and user-memory mapping |
| `0008` | Known initialization issue in the MUSB local variable `power` |
| `0011` | Modifies the generic framebuffer console |
| `0012` | Backports GPIO Core / UAPI functionality |

> [!CAUTION]
> The high-risk parts of the Linux patch set cannot be considered safe at runtime merely because the patches apply successfully.

Detailed implementations, dependencies, and known issues:

```text
linux/README_EN.md
```

---

# U-Boot Patches

| Feature | Related patch |
| :--- | :--- |
| UART0 TX / RX pull-ups | `0001` |
| Force MUSB Gadget to Full-Speed | `0002` |
| Detect Macronix SPI-NAND in SPL | `0003` |
| SUNIV SPI parent clock / divider | `0004` |
| DFU post-write verification and bad-block handling | `0005` |

### Position in the Boot Flow

```mermaid
flowchart TB
    A["Device powers on"]
    B["SPL initializes DRAM / UART0 / SPI0"]
    C["Detect SPI-NAND"]
    D["Load main U-Boot"]
    E["Main U-Boot reads environment / boot.itb"]
    F{"Boot result"}
    G["Linux"]
    H["USB DFU"]
    I["Write / read-back verification / bad-block handling"]

    A --> B --> C --> D --> E --> F
    F -- Normal --> G
    F -- Failure or user request --> H --> I
```

Specifically:

- `0001`: stabilizes UART0 levels during early boot
- `0003`: helps SPL detect Macronix SPI-NAND
- `0004`: corrects the SUNIV SPI clock / divider
- `0002`: forces U-Boot MUSB to Full-Speed
- `0005`: reads data back after DFU writes and handles bad blocks

Detailed implementation, NAND layout, and DFU behavior:

```text
uboot/README_EN.md
```

---

## Cross-Directory Coupling

This directory cannot be maintained independently of the configurations, device trees, user space, and image scripts.

```mermaid
flowchart TB
    A["Patches"]
    B["Defconfig"]
    C["Device Tree"]
    D["User Space"]
    E["Image Scripts"]
    F["Final system"]

    A --> F
    B --> F
    C --> F
    D --> F
    E --> F
```

---

### Defconfig

```text
board/cra/epass/linux.defconfig
board/cra/epass/uboot.defconfig
```

> [!IMPORTANT]
> A Kconfig feature added by a patch must be selected by the corresponding defconfig. The presence of code in the source tree does not mean that it will be included in the final kernel or U-Boot image.

---

### Device Tree

```text
board/cra/epass/devicetree/linux/
board/cra/epass/devicetree/uboot/
```

The following identifiers must remain consistent with the driver implementations:

```text
cra,epass-panel
cra,st7701-initseq
cra,swap-b-r
cra,usb-hs-enabled
```

Also verify references to:

- SPI0
- SPI-NAND
- `spi-max-frequency`
- CardKB
- ES8311
- Other expansion nodes

---

### Private DRM ABI

Linux:

```text
0007-srgn-drm-atomic-ioctl.patch
```

and user space:

```text
drm_app_neo/src/driver/srgn_drm.h
drm_app_neo/src/driver/drm_warpper.c
drm_app_neo/src/render/
drm_app_neo/src/overlay/
```

together define the kernel/user-space ABI.

```mermaid
flowchart LR
    A["Kernel DRM UAPI"] <--> B["drm_app_neo"]
```

> [!CAUTION]
> IOCTL numbers, structure layouts, field widths, and command semantics must remain identical on both sides. A unilateral change can break the ABI.

---

### Image Scripts

```text
board/cra/epass/scripts/mknanduboot.sh
board/cra/epass/scripts/mkdt.sh
board/cra/epass/scripts/buildimage.sh
```

| Script | Responsibility |
| :--- | :--- |
| `mknanduboot.sh` | NAND SPL / U-Boot layout |
| `mkdt.sh` | Linux DTB / DTBO compilation |
| `buildimage.sh` | UBI and `boot.itb` |

> [!NOTE]
> Patches modify source code only. Device-tree compilation, NAND binary layout, and final system-image generation are separate build stages.

---

## Correct Rebuild Procedure

### After Modifying a Linux Patch

```bash
make cra_epass_defconfig
make linux-dirclean
make linux
```

### After Modifying a U-Boot Patch

```bash
make cra_epass_defconfig
make uboot-dirclean
make uboot
```

### Complete System

```bash
make
```

### Why `*-dirclean` Is Required

```mermaid
flowchart LR
    A["Modify a patch"] --> B["*-dirclean"]
    B --> C["Delete the old source tree"]
    C --> D["Extract the official source again"]
    D --> E["Reapply shared patches"]
    E --> F["Reapply CRA patches"]
    F --> G["Reconfigure / rebuild"]
```

Running only:

```bash
make linux-rebuild
make uboot-rebuild
```

does not normally rerun a patch stage that has already completed.

> [!WARNING]
> Modifying a patch file does not mean that the source under `output/build/` has been updated.

> [!NOTE]
> These commands only generate files; they do not flash a physical device automatically.

---

## Validation Status

The following checks have been completed:

- All 13 Linux patches parse correctly as unified diffs
- All 5 U-Boot patches parse correctly as unified diffs
- Every patch uses LF line endings
- The shared SUNIV Linux patches and CRA Linux patches apply in sequence to a clean Linux 5.4.99 tree
- Linux Kconfig recognizes the `CONFIG_CRA_EP_*` options
- The shared SUNIV U-Boot patches and CRA U-Boot patches apply in Buildroot order
- The current U-Boot patches have passed compilation validation

```mermaid
flowchart LR
    A["Patch parse"] --> B["Patch apply"]
    B --> C["Kconfig"]
    C --> D["Linux / U-Boot build"]
    D --> E["System image"]
    E --> F["Device boot"]
    F --> G["Display / USB / audio / NAND / DFU"]
```

> [!IMPORTANT]
> The existing checks confirm that the current patch sequence, naming, and configuration relationships remain suitable for builds. They do not replace a complete system build or physical-hardware validation.

---

<div align="center">

<sub><b>CRA Electric Pass</b> · Linux 5.4.99 & U-Boot board patch set</sub>

</div>
