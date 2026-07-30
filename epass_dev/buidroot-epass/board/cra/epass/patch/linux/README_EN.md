# CRA Electric Pass Linux Kernel Patches

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the board-level patches added to Linux 5.4.99 for CRA Electric Pass. Buildroot applies them in sequence while building the Linux kernel. Together, they provide the additional F1C100S/F1C200S hardware support required by the display pipeline, panel initialization, USB, audio, keyboard, and GPIO userspace interfaces.

## Build Integration

The board-level Buildroot configuration specifies the patch directories as follows:

```make
BR2_LINUX_KERNEL_PATCH="board/allwinner/suniv-f1c100s/patch/linux board/cra/epass/patch/linux"
```

The patches are applied in this order:

```text
Official Linux 5.4.99 source
        │
        ▼
board/allwinner/suniv-f1c100s/patch/linux
        │
        ▼
board/cra/epass/patch/linux
        │
        ▼
board/cra/epass/linux.defconfig
        │
        ▼
Cross-compile the Linux kernel and modules
```

## Directory Overview

```text
patch/linux/
├─ 0000-f1c100s-gpadc-regs.patch
├─ 0001-epass-icon.patch
├─ 0002-panel-simple.patch
├─ 0003-f1c100s-defe-debe-fix.patch
├─ 0004-swap_rb_as_config.patch
├─ 0005-gpadc-low-freq.patch
├─ 0006-initalize-st7701.patch
├─ 0007-srgn-drm-atomic-ioctl.patch
├─ 0008-force-usb-fs-dt-switch.patch
├─ 0009-m5stack-cardkb-driver.patch
├─ 0010-i2s-and-es-driver.patch
├─ 0011-fbcon-cra-width-hack.patch
├─ 0012-gpio-backport-pulls.patch
└─ README.md
```

| Number | Primary purpose | Affected subsystem |
| --- | --- | --- |
| `0000` | Correct the F1C100S GPADC register bit definitions | MFD, ADC |
| `0001` | Replace the Linux kernel boot logo | Framebuffer boot image |
| `0002` | Register the CRA 384×640 LCD panel timing | DRM panel-simple |
| `0003` | Correct DEFE/DEBE operation, scaling, and YUV output | SUN4I DRM |
| `0004` | Allow red and blue channels to be swapped through the device tree | SUN4I TCON |
| `0005` | Adjust GPADC sampling division and filtering | ADC |
| `0006` | Add a GPIO-based ST7701 initialization driver | Staging, display |
| `0007` | Add the project-specific DRM fast-commit interface | DRM UAPI, main application |
| `0008` | Select USB Full-Speed or High-Speed through the device tree | MUSB |
| `0009` | Add an I²C input driver for M5Stack CardKB | Staging, input |
| `0010` | Add ES-series audio drivers and adjust I²S | ALSA SoC |
| `0011` | Reduce the usable width of the framebuffer console | fbcon |
| `0012` | Backport GPIO bias configuration support to Linux 5.4 | GPIO UAPI, libgpiod |

## Patch Details

### 0000: F1C100S GPADC Register Corrections

Target file:

```text
include/linux/mfd/sun4i-gpadc.h
```

This patch adjusts the GPADC and touchscreen-control register bit definitions for the F1C100S/F1C200S, including the calibration, dual-touch, operating-mode, ADC-selection, and channel-selection fields.

Together with `0005`, it forms the foundation of the project's ADC support. Without this patch, the ADC channels associated with PA0 through PA3 may not operate according to the actual register layout of the SoC.

### 0001: Kernel Boot Logo

Target file:

```text
drivers/video/logo/logo_linux_clut224.ppm
```

### 0002: CRA LCD Panel Timing

Target file:

```text
drivers/gpu/drm/panel/panel-simple.c
```

This patch registers the following device-tree compatible string:

```dts
compatible = "cra,epass-panel";
```

The shared display mode currently uses:

| Parameter | Value |
| --- | --- |
| Physical output resolution | 384×640 |
| Pixel clock | 24 MHz |
| Refresh rate | Approximately 60 Hz |
| Bus format | RGB565 |
| Declared color depth | 6 bpc |

The corresponding device trees are located under:

```text
board/cra/epass/devicetree/linux/base/
board/cra/epass/devicetree/linux/screen/
```

The patch also changes the physical width reported for the `foxlink_fl500wvr00_a0t` panel. The current CRA device trees do not use that panel, so this appears to be an unrelated modification inherited from the original project. It should be reviewed separately when the patch series is reorganized.

### 0003: F1C100S DEFE/DEBE Display Corrections

Primary target files:

```text
drivers/gpu/drm/sun4i/sun4i_backend.c
drivers/gpu/drm/sun4i/sun4i_frontend.c
drivers/gpu/drm/sun4i/sun4i_frontend.h
drivers/gpu/drm/sun4i/sunxi_detab.h
```

This patch:

- Corrects the framebuffer address units used for DEBE packed-YUV buffers.
- Configures the DEFE scaler.
- Adds scaling filter coefficient tables.
- Adds color-space conversion parameters for YUV-to-RGB conversion.
- Handles frontend register access and initialization.
- Provides the low-level support required by the project's video layers.

This is one of the most important and difficult-to-maintain patches in the display pipeline. Some initialization values are closely tied to the current 384×640 output, YUV422 data, and vendor BSP tables. It should not be treated as a general-purpose SUN4I DRM implementation for unrelated boards.

### 0004: Red/Blue Channel Swap

Primary target files:

```text
drivers/gpu/drm/sun4i/sun4i_tcon.c
drivers/gpu/drm/sun4i/sun4i_tcon.h
```

This patch reads the project-specific device-tree property:

```dts
cra,swap-b-r;
```

When the property is present, the driver sets the color-channel swap bit in TCON0. This compensates for red/blue wiring or initialization differences on certain panels. The current `laowu.dts` overlay uses this property.

### 0005: GPADC Sampling Adjustments

Target file:

```text
drivers/iio/adc/sun4i-gpadc-iio.c
```

This patch changes the GPADC sampling divider and filter type to improve the stability of slowly changing analog values such as the battery voltage.

The `low-freq` name summarizes the intent of the original project. The effective sampling rate still depends on the SoC clock, the register-divider formula, and the driver configuration, so it should not be inferred from the patch name alone.

### 0006: ST7701 Initialization Driver

This patch adds:

```text
include/dt-bindings/display/st7701initseq.h
drivers/staging/cra/Kconfig
drivers/staging/cra/Makefile
drivers/staging/cra/st7701init.c
```

The current configuration symbols are:

```text
CONFIG_CRA_EP_STAGING
CONFIG_CRA_EP_ST7701_INIT
```

The driver matches:

```dts
compatible = "cra,st7701-initseq";
```

It sends initialization commands to the ST7701 through software-driven SDA, SCL, CS, and optional RST GPIO lines. The command sequence is not hard-coded in the driver; each panel device tree supplies its own `init-sequence` array.

Initialization runs asynchronously through a workqueue. The current implementation does not yet validate required GPIOs, sequence lengths, or unknown operation codes as rigorously as it should. Changes to a device-tree initialization sequence must therefore be checked carefully for array-boundary errors.

### 0007: Private DRM Atomic-Commit Interface

Primary target files:

```text
drivers/gpu/drm/sun4i/sun4i_backend.c
drivers/gpu/drm/sun4i/sun4i_drv.c
drivers/gpu/drm/sun4i/sun4i_drv.h
include/uapi/drm/srgn_drm.h
```

This patch adds two private DRM IOCTLs:

```text
DRM_IOCTL_SRGN_ATOMIC_COMMIT
DRM_IOCTL_SRGN_RESET_FB_CACHE
```

The supported operations include:

- Mounting a regular RGB framebuffer.
- Mounting a YUV framebuffer.
- Setting layer coordinates.
- Setting the global alpha value of a layer.
- Clearing the cached mapping from userspace virtual addresses to physical addresses.

The rendering, video playback, and overlay code in `drm_app_neo` depends directly on this interface. Although the patch still uses the `SRGN` name, it cannot be renamed only on the kernel side.

A future migration to CRA naming would need to update at least:

```text
The UAPI header and DRM registration code in this patch
drm_app_neo/src/driver/srgn_drm.h
drm_app_neo/src/driver/drm_warpper.c
drm_app_neo/src/render/
drm_app_neo/src/overlay/
```

The IOCTL numbers, structure layouts, and field widths together form the ABI between the kernel and userspace. Both sides must remain exactly aligned during any migration.

The current implementation programs display registers directly and resolves physical pages from userspace virtual addresses. Its locking, VMA-state restoration, cache lifetime, and process isolation are not sufficiently robust. This is the highest-risk patch in the directory from a maintenance perspective.

### 0008: Device-Tree Switch for USB Speed

Primary target files:

```text
drivers/usb/musb/sunxi.c
drivers/usb/musb/musb_core.c
```

This patch reads:

```dts
cra,usb-hs-enabled;
```

When the property is absent, USB is restricted to Full-Speed by default. When it is present, MUSB is allowed to attempt High-Speed operation. The corresponding overlay is:

```text
board/cra/epass/devicetree/linux/interface/usbhs.dts
```

The local `power` variable in the current patch is not reliably initialized before bitwise operations are applied to it. Other bits written to the MUSB `POWER` register may therefore contain undefined values. This is a known functional defect. High-Speed should not be enabled by default merely to increase the nominal transfer rate until this issue has been corrected and the PCB's high-speed signal integrity has been verified.

### 0009: M5Stack CardKB Driver

This patch adds:

```text
drivers/staging/cra/cardkb.c
```

It also adds the following symbol to the CRA staging Kconfig and Makefile:

```text
CONFIG_CRA_EP_CARDKB
```

The driver periodically polls the M5Stack Unit CardKB over I²C and translates the character codes returned by the keyboard controller into Linux input events.

The current board configuration builds it as a module:

```text
CONFIG_CRA_EP_CARDKB=m
```

Using it requires both the I²C0 interface overlay and the `cardkb` peripheral overlay.

### 0010: I²S and ES-Series Audio Drivers

This patch primarily:

- Allows the SUNIV I²S module clock to propagate rate changes to its parent clock.
- Adjusts the playback and capture DMA `maxburst` values for I²S.
- Adds an ES8156 driver.
- Adds an ES8311 driver.
- Adds an ES8375 driver.
- Adds an ES8389 driver.

The current project configuration is:

```text
CONFIG_SND_SUN4I_I2S=m
CONFIG_SND_SOC_ES8311=m
CONFIG_SND_SIMPLE_CARD=m
```

### 0011: Framebuffer Console Width Workaround

Target file:

```text
drivers/video/fbdev/core/fbcon.c
```

Through `CRA_FB_CONSOLE_WIDTH_HACK`, this patch reduces the framebuffer console width by three character cells to avoid the abnormal region covering approximately the rightmost 24 pixels of the display.

It affects only the Linux text console. It does not change the width of the LVGL or `drm_app_neo` user interface.

This is a global modification to the generic `fbcon` implementation and is not restricted to a particular panel or device tree. If the display timing is corrected later, or the rightmost region is confirmed to work normally, the need for this patch should be reassessed.

### 0012: GPIO Bias Interface Backport

This patch backports selected GPIO character-device features from newer Linux releases to Linux 5.4.99, including:

- Pull-up bias.
- Pull-down bias.
- Bias disable.
- Runtime GPIO-line reconfiguration.
- `GPIOHANDLE_SET_CONFIG_IOCTL`.
- GPIO bias-state reporting.
- Related pin-range and fwnode support.

## Functional Dependencies

### Display Pipeline

```text
0002 Panel timing
  │
  ├─ 0004 Red/blue channel swap
  │
  └─ 0006 ST7701 initialization
          │
          ▼
0003 DEFE/DEBE and scaling corrections
          │
          ▼
0007 drm_app_neo private layer interface
          │
          └─ 0011 Only adjusts the framebuffer text-console width
```

### ADC Pipeline

```text
0000 Register bit definitions
        │
        ▼
0005 Sampling and filtering parameters
        │
        ▼
Battery voltage and external ADC inputs
```

### Expansion Devices

```text
I²C0 + cardkb.dts
        └─ 0009 CardKB driver

I²S + es8311_sound.dts
        └─ 0010 I²S and ES8311 drivers

libgpiod
        └─ 0012 GPIO bias and reconfiguration interface
```

## Rebuilding After a Patch Change

From the Buildroot root directory in Linux or WSL, run:

```bash
make cra_epass_defconfig
make linux-dirclean
make linux
```

After changing a patch, use `linux-dirclean` so that Buildroot extracts a fresh kernel source tree and reapplies the complete patch series. Running only:

```bash
make linux-rebuild
```

will not normally reapply patches after the build has already passed the patch stage. This can create the misleading impression that a modified patch has taken effect while the output directory still contains the old source.

To continue with a complete system build, run:

```bash
make
```

These commands only build files. They do not automatically write an image to a physical device. Flashing, replacing files on the device, and uploading the main application are separate operations.

## Modification and Validation Principles

1. Do not make long-term changes directly under `output/build/linux-5.4.99/`. It is generated output and will be removed by `linux-dirclean`.
2. Permanent changes should be represented by patches in this directory, or reorganized into traceable commits when the kernel is upgraded.
3. New patches should use four-digit numeric prefixes and have an explicit dependency order.
4. Keep every patch file in LF format to prevent Windows CRLF line endings from breaking context matching in later patches.
5. When a Kconfig symbol changes, update `board/cra/epass/linux.defconfig` at the same time.
6. When a `cra,*` device-tree property or compatible string changes, update the relevant DTS/DTSI files and kernel driver together.
7. When the private DRM UAPI changes, update `drm_app_neo` and rebuild both the kernel and the main application.
8. Do not remove the original author attribution. Record CRA modifications and maintainer information through additional documentation or Git history.
9. A patch applying successfully does not prove that its runtime behavior is safe. Patch replay, Kconfig validation, compilation, and physical-hardware testing are separate stages.

## Current Validation Status

The following checks have been completed for this directory:

- Every Linux patch can be parsed as a valid unified diff.
- All CRA Linux and U-Boot patches use LF line endings.
- The shared SUNIV patch series can be applied to a clean Linux 5.4.99 source tree, followed by the complete patch series from this directory.
- `make ARCH=arm olddefconfig` recognizes the `CONFIG_CRA_EP_*` symbols.
- The CRA staging-driver directory, panel compatible string, and USB property appear correctly in the patched source tree.

These checks confirm that the current patch order and Kconfig relationships are valid. They do not replace a complete kernel build or testing on physical hardware.

