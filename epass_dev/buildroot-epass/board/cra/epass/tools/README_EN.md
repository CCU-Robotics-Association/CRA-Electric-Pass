# CRA Electric Pass Utilities

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains host-side utilities used while building CRA Electric Pass and generating its resources. These scripts run on a development computer or in the Buildroot environment. They are not resident device-side programs and are not automatically installed on the target system.

## Directory Structure

```text
tools/
├── shutdown_variants.py
└── README.md
```

## `shutdown_variants.py`

This script generates three shutdown helper executables, each displaying a different Chinese message.

### Image Layout

| Item | Value |
| --- | --- |
| Image width | 360 pixels |
| Image height | 129 pixels |
| Pixel format | RGB888 |
| Bytes per pixel | 3 |
| Image data size | 139,320 bytes |
| Image offset within the executable | `0x0DDC` |

The script draws a white rectangle in the center of a black background, then renders the shutdown message in black. The resulting image bytes are written to a fixed offset in the template executable.

### Generated Files

| Output file | Message |
| --- | --- |
| `shutdown_message` | 要走了吗，不再看看 |
| `shutdown_message_2` | 再见，祝愿未来 |
| `shutdown_message_3` | 别忘记这里 |

When a preview directory is specified, the script also generates the following PNG files:

```text
shutdown_message_1.png
shutdown_message_2.png
shutdown_message_3.png
```

### Requirements

- Python 3;
- Pillow;
- a TrueType or OpenType font containing all required Chinese glyphs;
- a verified, working `shutdown_message` template executable.

Install Pillow with:

```sh
python3 -m pip install Pillow
```

### Usage

```sh
python3 board/cra/epass/tools/shutdown_variants.py \
    board/cra/epass/rootfs/bin \
    /path/to/chinese-font.ttf \
    --preview-dir /tmp/cra-shutdown-preview \
    --font-size 28
```

Arguments:

| Argument | Required | Description |
| --- | --- | --- |
| `bin_dir` | Yes | Directory containing the `shutdown_message` template executable |
| `font` | Yes | Font file used to render the Chinese messages |
| `--preview-dir` | No | Output directory for PNG previews |
| `--font-size` | No | Message font size; defaults to `28` |

After execution, the script writes three executables to `bin_dir` and sets their permissions to `0755`.

To display the complete command-line help:

```sh
python3 board/cra/epass/tools/shutdown_variants.py --help
```

## Workflow

The script processes the files in the following order:

1. Read the template executable from `bin_dir/shutdown_message`.
2. Confirm that the template is large enough to contain the fixed-size RGB image.
3. Render each of the three messages with the specified font.
4. Copy the template and replace the image bytes at the fixed offset.
5. Write the three resulting executables.
6. Save additional PNG previews when `--preview-dir` is specified.

## Limitations and Precautions

- The fixed offset `0x0DDC` is tightly coupled to the binary layout of the current template executable. Recompiling the template may change this offset.
- The current validation only confirms that the file is large enough. It does not verify the template version, symbols, or machine instructions. An incompatible template may produce corrupted executables.
- The script overwrites files named `shutdown_message*` in `bin_dir`. Generate and inspect them in the repository's rootfs staging directory; do not use a mounted physical device as `bin_dir`.
- PNG previews can verify text rendering and layout, but they cannot replace framebuffer testing on the target device.
- The `chmod 0755` operation assumes a POSIX environment such as Linux or WSL. Permission handling may differ on a native Windows filesystem.
- Before changing the dimensions, pixel format, or offset, verify the actual layout of the embedded resource in the target executable.

## Coding Conventions

- Source files use UTF-8 encoding and LF line endings.
- Comments and module documentation are written in Chinese.
- Functions use type annotations.
- Function docstrings follow the Google style and use `Args`, `Returns`, and `Raises` to document interfaces and exceptions.
- These utilities only generate files; they do not flash, upload, or replace programs on a physical device.

