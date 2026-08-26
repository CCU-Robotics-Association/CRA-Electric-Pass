# CRA Electric Pass rootfs 覆盖层

本目录是 CRA Electric Pass 的板级 **rootfs overlay（根文件系统覆盖层）**。构建 Buildroot 镜像时，其中的文件会按照相同的相对路径复制到目标根文件系统中。例如：

```text
board/cra/epass/rootfs/bin/usbctl
                ↓
目标设备 /bin/usbctl
```
