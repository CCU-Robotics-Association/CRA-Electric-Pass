# Electronic Pass Main Program Development Handoff

## User intent

The user owns an existing Rhodes Island Electronic Pass v0.6 device.

The intended work is a substantial redesign and secondary development of the device's built-in main application.

The user does not merely want custom assets or a third-party application.

The intended direction is:

`original drm_app_neo -> understand architecture -> preserve useful infrastructure -> substantially redesign UI and functions -> custom main application`

A hypothetical example discussed was changing the main-menu label `干员` to `人员`, but this was only an example used to clarify that the target is the built-in main program.

The actual goal is much larger than a text replacement.

## Work completed before main-program development

### Asset system

The device was connected over USB/MTP.

The `assets` directory was visible from Windows.

A new material package was directly copied into the device's `assets` directory.

The material loaded successfully.

### Third-party application loading

A third-party App package was created with:

* `appconfig.json`
* launcher script
* target executable

The application initially failed configuration parsing because the `screens` field was missing.

Adding:

`"screens": ["360x640"]`

fixed App loading.

A shell application executed successfully.

### ARM binary compatibility testing

A C framebuffer demo was first compiled using:

`arm-linux-gnueabihf-gcc`

The binary launched through the App framework but immediately crashed:

`Segmentation fault`

exit code:

`139`

A minimal C program compiled the same way also crashed.

The compiler was changed to:

`arm-linux-gnueabi-gcc`

with options approximately equivalent to:

`-march=armv5te -marm -mfloat-abi=soft`

The minimal C application then executed successfully and returned exit code 0.

This confirmed soft-float ARM compatibility.

### Framebuffer probing

A probe program successfully opened `/dev/fb0`.

Reported values:

`xres=384`

`yres=640`

`xres_virtual=384`

`yres_virtual=640`

`bits_per_pixel=32`

`line_length=1536`

`smem_len=983040`

A framebuffer drawing program was successfully displayed.

Initially only the left, top, and bottom borders were visible when drawing using the full 384-pixel width.

Changing the logical visible drawing area to 360x640 caused all four borders to appear.

This confirmed direct framebuffer drawing is possible, but this experiment should not be used as justification to replace the original DRM/LVGL renderer without architectural analysis.

## Main program source and Buildroot

Repositories:

`/home/xcrane/epass_dev/drm_app_neo`

`/home/xcrane/epass_dev/buildroot-epass`

The repositories were originally cloned on Windows because WSL had GitHub proxy connectivity problems.

They were then copied into WSL.

### Proxy environment

Windows uses Clash Verge.

Proxy port:

`7897`

WSL default gateway observed during setup:

`172.17.64.1`

The WSL proxy environment used:

`http_proxy=http://172.17.64.1:7897`

`https_proxy=http://172.17.64.1:7897`

`all_proxy=http://172.17.64.1:7897`

This network value may change after WSL or Windows networking restarts.

Do not hard-code it without checking:

`ip route | grep default`

### CRLF problem

Copying the Windows checkout into WSL caused script errors including:

`/bin/sh^M: bad interpreter`

and:

`/usr/bin/env: 'bash\r': No such file or directory`

Relevant text/scripts were converted to Unix line endings.

### Symbolic-link problem

The Windows checkout also caused Git symlinks to become regular files.

For example:

`package/gcc/gcc-initial/gcc-initial.hash`

contained the literal text:

`../gcc.hash`

instead of being a symlink.

This caused Buildroot errors such as:

`ERROR: No hash found for gcc-8.4.0.tar.xz`

even though `package/gcc/gcc.hash` contained the correct SHA-512 entry.

The downloaded GCC archive was independently verified to have SHA-512:

`6de904f552a02de33b11ef52312bb664396efd7e1ce3bbe37bfad5ef617f133095b3767b4804bc7fe78df335cb53bc83f1ac055baed40979ce4c2c3e46b70280`

The repository symlink problem was then identified and repaired.

Do not work around these errors by globally disabling hash verification unless absolutely necessary.

### Wrong architecture build

An early Buildroot build accidentally used:

`BR2_i386=y`

`BR2_ARCH="i586"`

This was wrong.

The build was discarded.

Buildroot was reset and configured using:

`make distclean`

`make cra_epass_defconfig`

The configuration was verified as:

`BR2_arm=y`

`BR2_ARCH="arm"`

`BR2_arm926t=y`

`BR2_ARM_EABI=y`

`BR2_ARM_SOFT_FLOAT=y`

`BR2_ARM_INSTRUCTIONS_ARM=y`

### CMake --fresh failure

Buildroot later failed configuring `epass_drm_app` with:

`The source directory ".../epass_drm_app-a2.7.0/--fresh" does not exist`

Verbose inspection identified:

`package/epass_drm_app/epass_drm_app.mk`

containing:

`EPASS_DRM_APP_CONF_OPTS = -DBUILD_SHARED_LIBS=OFF --fresh`

`--fresh` was removed.

The resulting intended line is:

`EPASS_DRM_APP_CONF_OPTS = -DBUILD_SHARED_LIBS=OFF`

After this correction the Buildroot build progressed successfully.

## Current successful Buildroot output

`output/images` currently contains:

* `boot.itb`
* `devicetree.dtb`
* `dt/`
* `gensdimage.py`
* `kernel.its`
* `rootfs.tar`
* `rootfs_ubi.img`
* `sd_image.img`
* `u-boot-sunxi-with-nand-spl.bin`
* `u-boot-sunxi-with-spl.bin`
* `u-boot.bin`
* `zImage`

The compiled main application exists at:

`output/build/epass_drm_app-a2.7.0/epass_drm_app`

and:

`output/target/root/epass_drm_app`

The next task is NOT to flash these complete images.

## Pending work

1. Verify the compiled main executable with `file`.
2. Inspect the `drm_app_neo` source repository.
3. Determine the actual build workflow for developing the checked-out `drm_app_neo` repository against the completed Buildroot sysroot/toolchain.
4. Build the checked-out original source without functional modifications.
5. Identify the source of the v0.6 main-menu UI.
6. Locate the widgets/text corresponding to:

   * 干员
   * 扩列图
   * 应用
   * 文件
   * 设置
   * 设备
7. Determine whether those UI definitions are generated from EEZ Studio.
8. Make one minimal visible test modification.
9. Build the modified binary.
10. Establish physical-device system access.
11. Identify the real running `epass_drm_app` path and service configuration.
12. Back up the original device executable.
13. Replace only the main executable for the first deployment test.
14. After the deployment loop is reliable, begin the major redesign.

## Important unresolved issue

SSH access to the physical v0.6 device has not yet been established.

Do not assume:

* the device IP
* `/root/epass_drm_app` is the live executable
* the service name
* the service ExecStart path

These must be verified on the physical device before replacement.
