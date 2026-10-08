# CRA Electric Pass Windows Utilities

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains precompiled Windows programs used for CRA Electric Pass configuration, USB diagnostics, and firmware flashing.

## Directory Contents

```text
binary/
├── epass_flasher.exe
├── UsbTreeView.exe
├── zadig-2.9.exe
└── README.md
```

| File | Platform | Current Size | Primary Purpose |
| --- | --- | ---: | --- |
| `epass_flasher.exe` | Windows x86-64, console application | 9,916,293 bytes | Generate device-tree configurations and flash the device through FEL and related procedures |
| `UsbTreeView.exe` | Windows x86-64, graphical application | 977,664 bytes | Inspect USB topology, descriptors, VID/PID values, and driver status |
| `zadig-2.9.exe` | Windows x86, graphical application | 5,334,088 bytes | Install or replace WinUSB/libusb-family drivers for a selected USB device |

## `epass_flasher.exe`

This is the Windows console flashing utility for the Electric Pass.

Static inspection shows that the executable has the following structure:

- Written in Python 3.12 and packaged with PyInstaller;
- its original entry-point filename is `main.py`;
- includes Python modules such as `dt_patcher`, `interact`, and `flasher`;
- bundles `xfel.exe`, `dtc.exe`, `libfdt-1.dll`, `libusb-1.0.dll`, and `libyaml-0-2.dll`;
- supports loading flashing configurations;
- supports generating, modifying, and compiling device trees;
- can detect and reuse the previously generated device tree and flashing configuration;
- supports a configuration-file-driven batch flashing workflow through `--config_path`;
- guides the user through placing the device in FEL mode before downloading the firmware.

Its internal workflow is approximately:

```text
Load or interactively create a configuration
        │
        ├─Generate the device-tree source
        ├─Apply device-tree changes
        ├─Compile the device tree with dtc
        └─Generate a flashing summary
                 │
                 ▼
        Wait for the device to enter FEL
                 │
                 ▼
             Flash with xfel
```

The program explicitly warns that flashing may erase all data on the device. Before running it, confirm that the hardware revision, display configuration, firmware files, and device-tree selections are all correct.

## `UsbTreeView.exe`

This is USB Device Tree Viewer by Uwe Sieber, file version `4.5.1.0`.

It can be used to inspect:

- USB host controllers and hub hierarchy;
- the VID, PID, and serial number of connected devices;
- USB descriptors;
- the Windows driver currently assigned to a device;
- connection speed and endpoint information;
- whether a device has enumerated in FEL, DFU, RNDIS, or another expected mode.

This tool is intended only for diagnostics and inspection. It does not flash the device or automatically modify USB drivers.

## `zadig-2.9.exe`

This is Zadig `2.9.788`, published by Akeo Consulting.

It installs or replaces Windows drivers for a selected USB device. Common targets include:

- WinUSB;
- libusbK;
- libusb-win32.

If a device appears in the USB device list but cannot be accessed by `xfel`, DFU, or another user-space tool, Zadig can be used to inspect and adjust the driver assigned to the relevant interface.

Always verify the device's VID, PID, interface number, and name before making changes. Replacing the driver for the wrong USB device may temporarily prevent a keyboard, mouse, network adapter, debugger, or another device from working correctly. Administrator privileges are generally required.

The current file carries a valid Akeo Consulting Authenticode signature.

## Current File Checksums

The SHA-256 values below correspond to the files currently stored in this repository. Recalculate and update this section whenever any of these files changes.

| File | SHA-256 |
| --- | --- |
| `epass_flasher.exe` | `EDFB44E5558C772FA4E09B54628FE85901379359A5F6D9D953532FD58173DC3D` |
| `UsbTreeView.exe` | `1B22F76E90D824F2404B364174252E5F6A8AFA2FB34543AEADD91BE1AD86D2DF` |
| `zadig-2.9.exe` | `4ECAA95DF3DA3621486A043AEF8B3050B8BAFE7C901402871E816229EF82039B` |

Recalculate them in PowerShell with:

```powershell
Get-FileHash -Algorithm SHA256 .\epass_flasher.exe
Get-FileHash -Algorithm SHA256 .\UsbTreeView.exe
Get-FileHash -Algorithm SHA256 .\zadig-2.9.exe
```

## Security and Maintenance Guidelines

- Do not run an EXE from an unknown source, with a mismatched checksum, or with unexpected modifications.
- Back up themes, applications, images, and other user data on the device before flashing.
- Do not treat a mounted physical device as an ordinary build-output directory for bulk file replacement.
- Do not place these Windows executables in the target device's rootfs.
- When upgrading a third-party tool, record its version, source, license, and new SHA-256 value.
- Before public distribution, add source links and the relevant license information for all third-party tools.
- The project's own flashing utility should be accompanied by its complete source code, dependency versions, and reproducible build instructions.

