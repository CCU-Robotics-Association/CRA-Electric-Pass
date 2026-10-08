<div align="center">

# Buildroot 开发辅助脚本

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `helper/` 保存三个面向开发电脑的项目辅助脚本，用于重建 Linux、重建 U-Boot，或通过 QEMU/chroot 临时进入已生成的 ARM 根文件系统。

<p align="center">
  <a href="#目录内容">目录内容</a> ·
  <a href="#通用前提">运行前提</a> ·
  <a href="#rebuild-kernelsh">重建 Linux</a> ·
  <a href="#rebuild-ubootsh">重建 U-Boot</a> ·
  <a href="#emulate-chrootsh">QEMU / chroot</a> ·
  <a href="#输入与输出">输入与输出</a>
</p>

---

## 目录内容

```text
helper/
├── rebuild-kernel.sh   重新构建 Linux 并刷新完整 Buildroot 输出
├── rebuild-uboot.sh    重新构建 U-Boot 并刷新完整 Buildroot 输出
└── emulate-chroot.sh   解压 rootfs.tar，并以 QEMU/chroot 进入目标系统
```

三个脚本在 Git 中均带有可执行权限。它们使用 Shell、相对路径和 Linux 系统调用，必须在 Linux 或合适的 WSL 环境中运行，不能在 Windows PowerShell 中直接执行。

---

## 通用前提

运行前应满足：

- 当前目录是 Buildroot 仓库根目录，而不是 `helper/`；
- 已通过目标 defconfig 生成有效 `.config`；
- Git 符号链接和 Shell 脚本保持正常；
- 脚本使用 Unix LF 行尾；
- Buildroot 主机依赖已经安装；
- 当前 `output/` 与要维护的目标板一致。

CRA Electric Pass 的典型准备流程为：

```sh
cd /path/to/buildroot-epass
make cra_epass_defconfig
```

不要在刚构建过其他板型的 `output/` 上直接运行这些脚本。切换 CRA、BADGE200、Lichee Nano 或 MangoPi 目标时，应先使用干净输出目录或执行 `make distclean` 后重新载入配置。

---

## `rebuild-kernel.sh`

脚本内容等价于：

```sh
rm ./output/images/*.dtb
make linux-rebuild -j8
make
```

### 执行过程

1. 删除 `output/images/` 中已有的 `*.dtb`；
2. 以 8 个并行任务执行 Buildroot 的 `linux-rebuild`；
3. 再执行一次完整 `make`，让依赖内核输出的镜像和后处理阶段重新运行。

### 适用场景

- 修改 Linux 内核配置后重新编译；
- 修改已进入内核构建树的源码或补丁后验证；
- 修改 Linux DTS/DTB 相关内容后刷新镜像；
- 需要重新运行依赖内核产物的后续镜像流程。

### 注意事项

- 脚本会先删除当前 DTB 输出，不会创建备份；
- 它没有启用 `set -e`，删除命令失败时仍可能继续执行后续构建；
- `*.dtb` 通配符没有匹配文件时，部分 Shell 环境中的 `rm` 会报告错误；
- `-j8` 是固定并行度，不会根据主机 CPU 或内存自动调整；
- `linux-rebuild` 不是完整的 `linux-dirclean`，不会重新下载或完全清空内核构建目录；
- 如果修改了补丁序列而旧内核构建目录已经打过补丁，可能需要更彻底的清理后再构建。

运行方式：

```sh
./helper/rebuild-kernel.sh
```

该脚本只修改 `output/` 中的构建产物，不会自动烧录实体设备。

---

## `rebuild-uboot.sh`

脚本内容等价于：

```sh
make uboot-rebuild -j8
make
```

### 执行过程

1. 以 8 个并行任务重新构建 U-Boot；
2. 再运行完整 `make`，刷新依赖 U-Boot 的最终镜像。

### 适用场景

- 修改 `uboot.defconfig` 后重新构建；
- 修改 U-Boot 设备树后重新构建；
- 修改已经应用到 U-Boot 构建树中的源码后验证；
- 需要重新生成包含 U-Boot/SPL 的 NAND 或 SD 镜像。

### 注意事项

- `uboot-rebuild` 不一定会重新解压源码或重新应用全部补丁；
- 修改 U-Boot 补丁序列后，旧构建目录可能不再适用；
- 固定使用 `-j8`，资源较少的主机可能需要手动使用标准 Buildroot 命令代替；
- U-Boot、SPL、设备树、默认环境和镜像布局之间存在联动，不能只验证编译是否通过。

运行方式：

```sh
./helper/rebuild-uboot.sh
```

该脚本会生成新的 U-Boot 和镜像文件，但不会自动写入 SPI-NAND、SPI-NOR、SD 卡或实体设备。

---

## `emulate-chroot.sh`

该脚本用于把已经生成的 ARM rootfs 临时展开到：

```text
output/chroot/
```

然后通过 `qemu-arm-static` 和 `chroot` 启动目标系统中的 `/bin/sh`。

### 执行过程

脚本会：

1. 通过 `#!/usr/bin/sudo bash` 以 root 权限运行；
2. 检查当前目录是否存在 `output/`；
3. 如果已有 `output/chroot/`，尝试卸载其中的 `proc/` 并删除整个旧目录；
4. 新建 `output/chroot/`；
5. 将 `output/images/rootfs.tar` 解压到其中；
6. 把宿主机的 procfs 挂载到 `output/chroot/proc/`；
7. 将宿主机 `/usr/bin/qemu-arm-static` 复制进目标 rootfs；
8. 执行 `chroot . /bin/sh`；
9. 用户退出 Shell 后卸载 procfs，并删除整个 `output/chroot/`。

### 主机依赖

主机至少需要：

- `sudo`；
- `bash`；
- `tar`；
- `mount`、`umount` 和 `chroot`；
- `/usr/bin/qemu-arm-static`；
- 能执行 ARM 用户空间程序的 binfmt_misc/QEMU 配置；
- 已生成的 `output/images/rootfs.tar`。

如果 QEMU 或 binfmt_misc 未正确配置，`chroot . /bin/sh` 可能返回 `Exec format error`。

### 高风险行为

此脚本具有比两个 rebuild 脚本更高的主机风险：

- 它通过 `sudo` 获取 root 权限；
- 它把宿主机 procfs 暴露给目标 rootfs；
- 它会无提示删除已有的 `output/chroot/`；
- `cleanup()` 使用相对路径，脚本运行目录和当前目录状态非常重要；
- 如果脚本被中断、终端异常关闭或命令失败，procfs 可能保持挂载；
- 进入不可信 rootfs 等同于以 root 权限运行其中的程序。

因此只能对自己构建、内容可信的 rootfs 使用该脚本。不要把未知镜像或第三方 rootfs 解压后直接进入 chroot。

### 运行与退出

确认依赖和 rootfs 后，从仓库根目录运行：

```sh
./helper/emulate-chroot.sh
```

进入目标 Shell 后，使用：

```sh
exit
```

正常退出，使脚本有机会卸载 procfs 并清理临时目录。不要直接关闭终端。

### 异常中断后的检查

如果脚本异常退出，先检查挂载状态：

```sh
mountpoint output/chroot/proc
```

若仍处于挂载状态，应先正确卸载：

```sh
sudo umount output/chroot/proc
```

确认不再挂载后，才能处理 `output/chroot/`。不能在 procfs 仍挂载时直接删除目录。

---

## 输入与输出

| 脚本 | 主要输入 | 主要修改范围 | 是否写实体设备 |
| --- | --- | --- | --- |
| `rebuild-kernel.sh` | `.config`、Linux 源码/补丁/设备树 | `output/build/linux-*`、`output/images/` 及后续镜像 | 否 |
| `rebuild-uboot.sh` | `.config`、U-Boot 源码/补丁/设备树 | `output/build/uboot-*`、`output/images/` 及后续镜像 | 否 |
| `emulate-chroot.sh` | `output/images/rootfs.tar` | 临时创建并删除 `output/chroot/`，临时挂载 procfs | 否 |

---

<div align="center">

<sub><b>helper/</b> · development helpers for Buildroot workflows</sub>

</div>
