# Linux Download and Flashing Scripts

Read this document in other languages: [English](README_EN.md), [中文](README.md).

Return to the parent documentation: [flashutils](../README_EN.md).

This directory contains legacy Shell scripts for communicating with Allwinner F1C100s/F1C200s devices from a Linux host through `sunxi-fel` and `dfu-util`.

## Directory Contents

```text
linux/
├── README.md
├── fel-uboot.sh
├── fel-linux.sh
├── dfu-kernel.sh
├── dfu-mmc-all.sh
├── dfu-nand-all.sh
├── dfu-nand-fast.sh
├── dfu-nor-all.sh
└── dfu-nor-fast.sh
```

## Communication Methods

### FEL

FEL is the USB recovery and download mode provided by the Boot ROM in Allwinner SoCs. The host uses `sunxi-fel` to detect the device, write data to RAM, or upload U-Boot to RAM and start it.

FEL commands can operate only on RAM, but an uploaded U-Boot instance still executes the boot logic defined by its own environment. Whether U-Boot accesses or erases non-volatile storage after it starts must therefore be determined from the active `uboot.env`.

### DFU

DFU is provided by the U-Boot USB Gadget implementation on the device. The host uses `dfu-util` to select a device partition or area by its Alternate Setting name and then download an image.

`dfu-util -a <name> -D <image>` performs a write. `<name>` must exactly match an entry exported by the U-Boot instance currently running on the device, as reported by `dfu-util -l`.
