# CRA Electric Pass Download and Flashing Utilities

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains legacy helper scripts for communicating with the device through Allwinner FEL and U-Boot DFU, along with the Windows download tools distributed in this repository:

```text
flashutils/
```

## Directory Structure

```text
flashutils/
├── README.md
├── linux/
│   ├── README.md
│   ├── fel-uboot.sh
│   ├── fel-linux.sh
│   ├── dfu-kernel.sh
│   ├── dfu-mmc-all.sh
│   ├── dfu-nand-all.sh
│   ├── dfu-nand-fast.sh
│   ├── dfu-nor-all.sh
│   └── dfu-nor-fast.sh
└── windows/
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

## FEL and DFU

### FEL

FEL is the USB recovery and download mode provided by the Boot ROM in Allwinner SoCs. The host uses `sunxi-fel` to inspect the device, write data to RAM, or upload U-Boot to RAM and start it.

### DFU

DFU is a download interface provided by the U-Boot USB Gadget implementation. The host uses `dfu-util` to select an area such as U-Boot, a boot partition, or the root filesystem by its Alternate Setting name.

| Option | Meaning |
| --- | --- |
| `-l` | List the current DFU device and its Alternate Settings without writing |
| `-a <name>` | Select an area exported by U-Boot |
| `-D <file>` | Download the specified file to the selected area |
| `-R` | Request a device reset after the transfer |

## Windows Scripts

| Script | Actual behavior | Current status |
| --- | --- | --- |
| `fel-uboot.bat` | Starts U-Boot in RAM through FEL | The file name remains, but its post-start behavior must be verified |
| `fel-linux.bat` | Boots a legacy `zImage + DTB + initramfs` combination through FEL | Incompatible |
| `dfu-mmc-all.bat` | Writes the legacy `sysimage-sdcard.img` | Incompatible |
| `dfu-nand-all.bat` | Writes the legacy `sysimage-nand.img` | Incompatible |
| `dfu-nor-all.bat` | Writes the legacy `sysimage-nor.img` | Incompatible |

The batch files use `%~dp0` to locate `windows/bin/`, but image paths remain relative to the current working directory as `output\images\...`. Even after the scripts are repaired, they must either be launched from the Buildroot repository root or changed to use reliable absolute or repository-relative paths.

## Bundled Windows Tools

| File | Purpose | Readable version metadata |
| --- | --- | --- |
| `dfu-util.exe` | Host-side U-Boot DFU tool | Not provided in the PE metadata |
| `sunxi-fel.exe` | Host-side Allwinner FEL tool | Not provided in the PE metadata |
| `libusb-1.0.dll` | User-space USB communication library | `1.0.20.11004` |
| `libwinpthread-1.dll` | MinGW-w64 pthread runtime | `1.0.0.0` |
| `zadig-2.5.exe` | Windows USB driver installation utility | `2.5.730` |

Current file SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `dfu-util.exe` | `1F4687D0F11F0EEDE72D582FB5174537D5820DE515862A8041AC9506B4A6FA1E` |
| `sunxi-fel.exe` | `626A16C6FA8FF9638A52222452D7BED7F5922537F530494BE4BA0845BCA23203` |
| `libusb-1.0.dll` | `20E5CE87947C79C83624DEA087FE195CC8219BBD06E7EC2CD7EB025D9D16A73F` |
| `libwinpthread-1.dll` | `213B6ADDAB856FEB85DF1A22A75CDB9C010B2E3656322E1319D0DEF3E406531C` |
| `zadig-2.5.exe` | `78A1A26854FBC848284588A62C7FBEC9C652F6A3218BA543783D369265DF00D6` |

## USB Drivers

### Linux

`linux/README_EN.md` describes the installation of `sunxi-tools`, `dfu-util`, and udev rules using the following VID/PID pairs:

| Mode | VID:PID |
| --- | --- |
| Allwinner FEL | `1f3a:efe8` |
| U-Boot DFU | `1f3a:1010` |

The existing rules use `SUBSYSTEM!="usb_device"`, which may not match correctly on some modern udev installations. Verify the rules with `udevadm` and `lsusb` before relying on them.

### Windows

`windows/README_EN.md` uses Zadig to create the following devices manually:

| Name | VID:PID |
| --- | --- |
| Allwinner FEL Device | `1F3A:EFE8` |
| Allwinner DFU Device | `1F3A:1010` |

Confirm that the selected hardware is actually in FEL or U-Boot DFU mode. Do not replace the driver for the Linux system's RNDIS interface, storage device, or any unrelated USB device with WinUSB/libusb. The images under `windows/res/` are retained only as historical setup illustrations.

## Read-Only Inspection

Before rewriting any flashing script, use read-only commands to identify the connection:

```sh
sunxi-fel ver
dfu-util -l
```

Record the USB VID/PID, serial number, Alternate Settings, current device mode, and number of connected devices. These queries do not download an image, but starting U-Boot, entering `rundfu`, or running any command with `-D` can change device state.
