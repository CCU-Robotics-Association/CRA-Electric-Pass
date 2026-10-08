# Windows Download and Flashing Tools

Read this document in other languages: [English](README_EN.md), [中文](README.md).

Return to the parent documentation: [flashutils](../README_EN.md).

This directory contains legacy Windows batch files for Allwinner FEL and U-Boot DFU, prebuilt third-party command-line tools, their runtime libraries, and driver installation illustrations.

## Directory Structure

```text
windows/
├── README.md
├── fel-uboot.bat
├── fel-linux.bat
├── dfu-mmc-all.bat
├── dfu-nand-all.bat
├── dfu-nor-all.bat
├── bin/
│   ├── sunxi-fel.exe
│   ├── dfu-util.exe
│   ├── libusb-1.0.dll
│   ├── libwinpthread-1.dll
│   └── zadig-2.5.exe
└── res/
    ├── Create New Device.png
    ├── FEL Driver.png
    └── DFU Driver.png
```

## File Origins and Roles

| Type | Files | Role |
| --- | --- | --- |
| Project helper scripts | `*.bat` | Batch files written or inherited for an older image layout; not part of the Buildroot compilation |
| Third-party host tools | `sunxi-fel.exe`, `dfu-util.exe`, `zadig-2.5.exe` | Prebuilt Windows executables, not project application code |
| Third-party runtime libraries | `libusb-1.0.dll`, `libwinpthread-1.dll` | Dynamic libraries required by the command-line tools |
| Documentation resources | `res/*.png` | Historical driver installation screenshots; not used at runtime |

## FEL and DFU

### FEL

FEL is the USB recovery and download mode provided by the Boot ROM in Allwinner SoCs. A Windows host uses `sunxi-fel.exe` to detect the device, write data to RAM, or upload and start U-Boot.

Uploaded U-Boot normally starts from RAM, but commands subsequently executed from its environment can still access or erase non-volatile storage. Starting through FEL therefore does not automatically mean that device contents cannot be changed.

### DFU

DFU is provided by the U-Boot USB Gadget implementation on the device. A Windows host uses `dfu-util.exe` to select an Alternate Setting and download an image.

The `-D` option in the batch files performs a write, while `-R` requests a device reset after the transfer. The Alternate Setting must exactly match a name exported by the U-Boot instance currently running on the target, as reported by `dfu-util.exe -l`.

## Bundled Tools

| File | Purpose | Version readable from PE metadata |
| --- | --- | --- |
| `bin/sunxi-fel.exe` | Host-side Allwinner FEL tool | No version field provided |
| `bin/dfu-util.exe` | Host-side U-Boot DFU tool | No version field provided |
| `bin/libusb-1.0.dll` | User-space USB communication library | `1.0.20.11004` |
| `bin/libwinpthread-1.dll` | MinGW-w64 pthread runtime | `1.0.0.0` |
| `bin/zadig-2.5.exe` | Windows USB driver installation utility | `2.5.730` |

Current file SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `dfu-util.exe` | `1F4687D0F11F0EEDE72D582FB5174537D5820DE515862A8041AC9506B4A6FA1E` |
| `sunxi-fel.exe` | `626A16C6FA8FF9638A52222452D7BED7F5922537F530494BE4BA0845BCA23203` |
| `libusb-1.0.dll` | `20E5CE87947C79C83624DEA087FE195CC8219BBD06E7EC2CD7EB025D9D16A73F` |
| `libwinpthread-1.dll` | `213B6ADDAB856FEB85DF1A22A75CDB9C010B2E3656322E1319D0DEF3E406531C` |
| `zadig-2.5.exe` | `78A1A26854FBC848284588A62C7FBEC9C652F6A3218BA543783D369265DF00D6` |
