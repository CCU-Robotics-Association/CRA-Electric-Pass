# CRA Electric Pass Rootfs Overlay

This directory is the board-level **rootfs overlay** for CRA Electric Pass. During a Buildroot image build, its files are copied into the target root filesystem using the same relative paths. For example:

```text
board/cra/epass/rootfs/bin/usbctl
                ↓
target device /bin/usbctl
```
