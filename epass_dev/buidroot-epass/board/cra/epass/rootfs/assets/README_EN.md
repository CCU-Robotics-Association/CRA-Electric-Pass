# Theme Asset Directory

After Buildroot merges the rootfs overlay, this directory appears on the device as:

```text
/assets/
```

It stores themes installed in the device's internal NAND. It is neither a main-program source directory nor the location of the fixed UI components under `/root/res/`.

## How the Main Program Scans It

`drm_app_neo/src/config.h` defines two theme locations:

```c
#define THEME_DIR "/assets/"
#define THEME_DIR_SD "/sd/assets/"
```

The main program scans only the **immediate subdirectories** of these locations. A standalone `epconfig.json` placed directly in the `/assets/` root is not treated as a theme.

A basic layout is:

```text
/assets/
└── example_theme/
    ├── epconfig.json
    ├── loop.mp4
    ├── icon.png
    └── other optional resources
```

Each theme requires at least:

- a parseable `epconfig.json` with a supported version;
- a unique, valid `uuid`;
- a `screen` value matching the current firmware, which is `360x640` in this project;
- a readable loop video referenced by `loop.file`.

Whether icons, intro videos, transition images, and overlay UI resources are required depends on the configuration. Theme parsing failures are written to:

```text
/root/asset.log
```

## NAND, SD Card, and the Fallback Theme

| Path | Meaning |
| --- | --- |
| `/assets/` | Themes in internal NAND, supplied by this overlay or installed later |
| `/sd/assets/` | Themes on the SD card; scanned only when the card is mounted and the main program starts in SD mode |
| `/root/res/fallback/` | Built-in firmware theme used when no valid theme is available |

The program loads `/root/res/fallback/` only when neither `/assets/` nor `/sd/assets/` contains a valid theme. The fallback theme should not be moved into this directory.

## Access from a Computer

Both uMTP Responder configurations expose this directory as writable MTP storage:

```text
storage "/assets" "assets" "rw"
```

Themes can therefore be copied or removed from a computer while the device is in MTP mode. Externally supplied theme configurations and media files are not protected by digital signatures. Verify their origin, dimensions, encoding, storage requirements, and configuration contents before use.
