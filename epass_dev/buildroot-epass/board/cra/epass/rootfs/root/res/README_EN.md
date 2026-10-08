<div align="center">

# Fixed Main-Program Resources

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> This directory contains device-side resources shared by all themes and the built-in fallback used when no user theme can be loaded.

<p align="center">
  <a href="#program-load-path">Load path</a> ·
  <a href="#boundary-between-fixed-resources-and-theme-packages">Resource boundary</a> ·
  <a href="fallback/README_EN.md">Fallback theme</a>
</p>

---

This directory appears on the device as:

```text
/root/res/
```

## Program Load Path

`drm_app_neo/src/config.h` defines the fixed resource path:

```c
#define CACHED_ASSETS_ASSET_PATH "/root/res/"
```

During initialization, `src/main.c` loads six PNG files into the shared cache. `src/overlay/theme_info.c` then draws them using fixed resource IDs. Each filename corresponds directly to a `CACHED_ASSETS_ASSET_PATH_*` macro.

## Boundary Between Fixed Resources and Theme Packages

| Resource Type | Fixed Resources in `/root/res/` | Theme Package in `/assets/<theme>/` |
| --- | --- | --- |
| Frames and marks shared by all themes | Yes | Usually no |
| Default theme/application icon | Yes | Each package may provide its own icon |
| Loop video | Only the fallback video under `fallback/` | Provided separately by each theme |
| `epconfig.json` | Present only under `fallback/` | One file in each theme subdirectory |
| Installed or removed by users over MTP | Not recommended | Yes |

> [!WARNING]
> Fixed filenames map directly to device-side cache IDs. Before renaming, deleting, or changing a format, update the corresponding path macros and loading code in `drm_app_neo`.

---

<div align="center">

<sub><b>rootfs/root/res/</b> · fixed runtime assets and fallback theme</sub>

</div>
