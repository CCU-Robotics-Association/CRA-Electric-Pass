# CRA Electric Pass auxiliary components

This directory contains auxiliary software written and maintained as part of
CRA Electric Pass.  Components live in the main repository so a complete
firmware build does not depend on small, separately mirrored repositories.

## Components

- `protocol`: shared definitions for the current local IPC compatibility ABI.
- `epassctl`: command-line control client for the main display application.
- `usb-responder`: FunctionFS device-management and file-transfer service.
- `device-config`: v0.6 boot-overlay configuration tool.

The remaining media-adapter component will be added here as an independent CRA
implementation.

## Ownership boundary

Code in this directory is a new CRA implementation.  It communicates with the
existing main application and system services through documented interfaces.
Buildroot, the Linux kernel, LVGL, Cedar/Allwinner libraries, and other upstream
software retain their original copyright and license notices; placing a build
integration file in this repository does not make those projects CRA-owned.

Unless a file says otherwise, new component code is licensed under
GPL-3.0-or-later.  The license text is available in `../drm_app_neo/COPYING`.
