# HatLab Board Support

Read this document in other languages: [English](README_EN.md), [中文](README.md).

This directory contains Buildroot board support for the HatLab BADGE200:

```text
board/hatlab/
```

## Directory Layout

```text
hatlab/
└── badge200/
    ├── hatlab_badge200_defconfig  Buildroot board configuration
    ├── linux.defconfig            Linux kernel configuration
    ├── uboot.defconfig            U-Boot configuration
    ├── devicetree/                Linux and U-Boot device trees
    ├── rootfs/                    BADGE200 root filesystem overlay
    ├── scripts/                   Firmware image post-processing scripts
    ├── helper/                    Vendor-partition creation and download scripts
    ├── mcu/                       Standalone ATmega328P example firmware
    └── driver/                    Windows RNDIS driver archive
```

## Support Status

`badge200/` contains a mostly complete Buildroot configuration, Linux and U-Boot configurations, device trees, a rootfs overlay, and image-generation scripts. The repository also provides this entry under `configs/`:

```text
configs/hatlab_badge200_defconfig
```

In a Linux checkout that preserves Git symbolic links, load and build the configuration with:

```sh
make hatlab_badge200_defconfig
make
```

A Windows checkout may restore `configs/hatlab_badge200_defconfig` as a regular file containing only the path to its target. Such a file is not a valid Buildroot configuration. Restore the symbolic link in Linux or WSL before building.

## Buildroot Configuration

`badge200/hatlab_badge200_defconfig` is the top-level assembly point for this target. It mainly configures:

- an ARMv5 target with Buildroot's internal glibc toolchain;
- U-Boot `2020.07`, SPL, SPI flash, SPI-NAND, USB download, and DFU support;
- Linux `5.4.100` with the shared SUNIV F1C100S/F1C200S patches;
- ext4, JFFS2, and SquashFS filesystems;
- the BADGE200-specific rootfs overlay;
- framebuffer, touch, audio, camera, Bluetooth, D-Bus, Python 3, and debugging tools;
- NAND image generation through `scripts/genimage.sh`.

The target is assembled in the following layers:

```text
board/allwinner/generic/
        ↓
board/allwinner/suniv-f1c100s/
        ↓
board/hatlab/badge200/
```

## U-Boot and Boot Images

### `uboot.defconfig`

This file configures U-Boot for the BADGE200. Its main settings include:

- the SUNIV/F1C100S architecture and SPL;
- a 408 MHz system clock and 168 MHz DRAM clock;
- serial console, SPI, SPI-NOR, SPI-NAND, MMC, and USB OTG;
- USB Mass Storage and DFU download support;
- 480×272 LCD timing;
- `board/allwinner/generic/uboot.env` as the default environment.

### `scripts/genimage.sh`

The image script performs the following steps:

1. builds `kernel.itb` from the shared `kernel.its`;
2. copies the shared `splash.bmp` into the image output directory;
3. creates U-Boot with a NAND-compatible SPL layout;
4. generates the final image using the shared NAND image layout.

The BADGE200 boot splash therefore comes from:

```text
board/allwinner/generic/splash.bmp
```

## Linux Configuration and Device Trees

### `linux.defconfig`

This file contains the Linux kernel configuration used by the BADGE200. It controls kernel drivers, filesystems, networking, Bluetooth, media, debugging, and SoC peripheral support. Validate changes through an actual kernel build rather than relying on text-level checks alone.

### `devicetree/linux/devicetree.dts`

The Linux device tree describes the following major hardware:

- an F1C200S/F1C100S-compatible SoC;
- SPI-NAND with `u-boot`, `kernel`, `rom`, `vendor`, and `overlay` partitions;
- RGB LCD, display engine, and audio codec;
- UART, MMC, and USB OTG;
- an AW9523B GPIO expander;
- AXP199 power management, battery monitoring, and power-source detection;
- a PCF8563 RTC;
- a Goodix GT911 touch controller;
- an OV2640 camera and CSI interface;
- three LRADC buttons;
- a Broadcom Bluetooth controller connected over UART.

### `devicetree/uboot/suniv-f1c100s-generic.dts`

The U-Boot device tree provides only the basic nodes needed during the boot stage, including serial, SPI flash, MMC, and USB. It is not interchangeable with the Linux runtime device tree.

## Rootfs Overlay

`rootfs/` is copied into the target root filesystem and contains:

| Path | Purpose |
| --- | --- |
| `etc/init.d/S30rndis` | Loads `g_ether` and starts USB RNDIS networking |
| `etc/init.d/S50dropbear` | Starts the Dropbear SSH service |
| `etc/init.d/S90btbcm` | Loads the Bluetooth HCI UART driver |
| `etc/init.d/S99application` | Mounts the JFFS2 vendor partition and runs its application startup script |
| `etc/network/interfaces` | Configures target network interfaces |
| `etc/dnsmasq.conf` | Configures DHCP for the USB network side |
| `usr/lib/python3.8/site-packages/pyclui/` | Provides colored command-line logging helpers |
| `usr/lib/python3.8/site-packages/bthci/` | Provides Bluetooth HCI operation helpers |

These Python modules are installed directly by the board overlay rather than through Buildroot's standard Python package mechanism. Revalidate compatibility when upgrading Python, BlueZ, PyBlueZ, Scapy, or D-Bus.

## Helper Scripts and MCU Firmware

### `helper/`

- `mkvendor.sh` creates a JFFS2 `vendor.img` from a specified directory using fixed filesystem parameters;
- `dfu-nand-vendor.sh` waits for a DFU device and writes an image to the `vendor` DFU alternate setting.

These scripts create or write device partitions. Before using them, confirm the target board, partition layout, and image file. They must not be used on a CRA physical device.

### `mcu/`

`mcu/` is an AVR example project independent of the Allwinner Linux system:

- the target MCU is an ATmega328P;
- the default clock is 16 MHz;
- `main.c` toggles PB0 periodically;
- the Makefile uses `avr-gcc`, `avr-objcopy`, and `avrdude`.

It is not built automatically by `make hatlab_badge200_defconfig && make` and is unrelated to the CRA Electric Pass main application.

## Binary Files

`driver/windows-rndis-driver.zip` is a prepackaged Windows driver archive. Git can store the file, but it cannot review its internal changes in the same way as source code. Record the source, version, hash, and supported operating systems whenever it is updated.

## Relationship to CRA Electric Pass

The current CRA configuration does not reference `board/hatlab/`. This directory is retained primarily to:

- support the BADGE200 build target inherited from the original repository;
- provide reference configurations for F1C200S peripherals, Bluetooth, power management, and cameras;
- preserve legacy board image and helper tooling.

Do not modify this directory merely to change the CRA device. CRA-specific changes should normally be placed under:

```text
board/cra/epass/
```
