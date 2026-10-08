# AGENTS.md

## Project goal

This repository is being used for major secondary development of the Rhodes Island Electronic Pass main program.

The goal is NOT to create another third-party application under `/app`.

The goal is to use the existing `drm_app_neo` main program as the base and progressively redesign/rewrite its main UI, navigation, pages, and functionality, eventually producing a custom main program that replaces the original device UI.

Do not redirect the project toward the third-party App mechanism unless explicitly requested.

## Hardware and target

Device: Rhodes Island Electronic Pass v0.6.

Target architecture verified from the project Buildroot configuration:

* ARM
* ARM926T
* ARMv5
* EABI
* soft-float
* ARM instruction set

Relevant Buildroot configuration values:

* `BR2_arm=y`
* `BR2_ARCH="arm"`
* `BR2_arm926t=y`
* `BR2_ARM_EABI=y`
* `BR2_ARM_SOFT_FLOAT=y`
* `BR2_ARM_INSTRUCTIONS_ARM=y`

Do not use a hard-float toolchain for target binaries.

Earlier standalone testing showed binaries built with `arm-linux-gnueabihf-gcc` crashed with SIGSEGV/exit code 139.

A soft-float `arm-linux-gnueabi-gcc` standalone test program executed successfully.

For `drm_app_neo` itself, use the project's Buildroot toolchain and sysroot rather than the Ubuntu standalone cross compiler.

## Local paths

Main application source:

`/home/xcrane/epass_dev/drm_app_neo`

Buildroot:

`/home/xcrane/epass_dev/buildroot-epass`

Buildroot output:

`/home/xcrane/epass_dev/buildroot-epass/output`

Current Buildroot main application outputs:

`/home/xcrane/epass_dev/buildroot-epass/output/build/epass_drm_app-a2.7.0/epass_drm_app`

`/home/xcrane/epass_dev/buildroot-epass/output/target/root/epass_drm_app`

## Buildroot status

The Buildroot ARM build has completed successfully and generated image outputs including:

* `boot.itb`
* `devicetree.dtb`
* `rootfs.tar`
* `rootfs_ubi.img`
* `sd_image.img`
* `u-boot-sunxi-with-nand-spl.bin`
* `u-boot-sunxi-with-spl.bin`
* `u-boot.bin`
* `zImage`

The original Buildroot configuration was accidentally built as i586 once. That output was discarded.

The configuration was reset using:

`make distclean`

`make cra_epass_defconfig`

ARM configuration was then explicitly verified before rebuilding.

## Buildroot modifications already made

The repository was originally cloned on Windows and copied into WSL.

This caused CRLF and Git symbolic-link problems.

CRLF script problems such as `/bin/sh^M` and `bash\r` were fixed.

Git symbolic links must be treated carefully. Do not replace symlinks with text files containing their target path.

One previous failure occurred because `gcc-initial.hash` became a normal text file containing `../gcc.hash` instead of a symlink.

The Buildroot symlink state was repaired.

Before making Buildroot changes, inspect:

`git status`

and check repository symlinks.

The package file:

`package/epass_drm_app/epass_drm_app.mk`

was modified to remove `--fresh` from:

`EPASS_DRM_APP_CONF_OPTS`

The original value caused CMake to interpret `--fresh` as a source directory:

`epass_drm_app-a2.7.0/--fresh`

Current intended configuration is:

`EPASS_DRM_APP_CONF_OPTS = -DBUILD_SHARED_LIBS=OFF`

Do not reintroduce `--fresh` without first checking the Buildroot/CMake invocation semantics.

## Display information discovered by standalone testing

The Linux framebuffer exists at:

`/dev/fb0`

Framebuffer information:

* xres: 384
* yres: 640
* xres_virtual: 384
* yres_virtual: 640
* bits_per_pixel: 32
* line_length: 1536
* smem_len: 983040

Standalone framebuffer testing showed the visible UI area behaves like approximately 360x640.

A test using a 384-pixel logical right border placed the right border outside the visible region.

Drawing in a 360x640 visible region showed all four borders.

These framebuffer tests were only exploratory. The original `drm_app_neo` architecture uses its own DRM/LVGL rendering stack and should be understood before changing rendering architecture.

## Existing device capabilities already verified

The v0.6 device exposes user data over USB/MTP.

The `assets` directory can accept material packages directly.

Third-party applications under `/app` were successfully loaded.

A shell App and ARM soft-float C application were successfully launched through the original application's App menu.

This was only used to validate the hardware and execution environment.

The current project is no longer focused on third-party App development.

## Development strategy

Use the existing main program as the base.

First understand the source tree and current runtime architecture.

Preserve existing functionality until its role is understood.

Recommended sequence:

1. Inspect repository structure and build scripts.
2. Identify the main UI implementation and generated EEZ/LVGL code.
3. Identify the UI action/callback layer.
4. Identify page transition and navigation logic.
5. Identify material/video/overlay integration points.
6. Build the unmodified source successfully.
7. Make one minimal visible UI change.
8. Rebuild.
9. Verify the resulting binary.
10. Only after the build/deployment loop is proven, begin major UI and functionality restructuring.

## Important constraints

Do not immediately rewrite the complete application from scratch.

Do not convert the project back into a third-party `/app` application.

Do not change the kernel, U-Boot, device tree, or Buildroot architecture unless the task explicitly requires it.

Do not flash a full image merely to test a UI change.

Prefer testing by replacing only the main application binary after the original binary and service configuration have been identified and backed up.

Do not assume the actual device path of `epass_drm_app`.

The Buildroot rootfs installs a copy at `/root/epass_drm_app`, but the running v0.6 device path and service ExecStart have not yet been confirmed.

SSH access to the physical device has not yet been established in the current workflow.

Do not invent an IP address or device system path.

Before deployment, identify the running process, executable path, and service configuration from the actual device.

## Code editing rules

Before editing generated EEZ UI code, determine whether the source should instead be changed in the EEZ project and regenerated.

Clearly distinguish generated files from hand-written application logic.

Avoid broad global string replacement.

For UI text changes, identify the exact widget/page and modify the correct source of truth.

Keep changes small and reviewable during the initial bring-up phase.

After each meaningful change:

* show the files changed
* summarize the architectural reason
* give the exact build command
* report build results
* identify any assumptions that still require physical-device verification

## Current next milestone

The immediate milestone is:

1. Inspect the existing `drm_app_neo` codebase.
2. Explain the main program architecture based on the actual source.
3. Identify exactly which source files define the main menu shown on the v0.6 device.
4. Identify where menu labels such as `干员`, `扩列图`, `应用`, `文件`, `设置`, and `设备` originate.
5. Determine which UI files are generated and which are intended for manual modification.
6. Build the current unmodified `drm_app_neo` source using the completed Buildroot environment.
7. Do not make a large redesign yet.
8. Propose the safest first visible UI modification to verify the source-build-deployment loop.
