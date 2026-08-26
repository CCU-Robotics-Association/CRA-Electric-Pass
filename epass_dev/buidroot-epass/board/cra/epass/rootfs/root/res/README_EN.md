# Fixed Main-Program Resources

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
