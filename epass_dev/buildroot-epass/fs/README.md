<div align="center">

# Buildroot 根文件系统镜像基础设施

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `fs/` 定义 Buildroot 支持的根文件系统镜像格式及通用打包流程。
>
> CRA Electric Pass 使用 SPI-NAND，并由板级 post-image 脚本继续组装 UBI/UBIFS 镜像；不能仅凭本目录的通用格式配置判断最终闪存布局。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#fsrootfs-overlay-与-outputtarget-的区别">目录边界</a> ·
  <a href="#总体生成流程">生成流程</a> ·
  <a href="#各格式目录">格式目录</a> ·
  <a href="#只读块设备与原始闪存格式">存储类型</a> ·
  <a href="#tar">tar</a> ·
  <a href="#ubi-与-ubifs-的通用实现">UBI / UBIFS</a>
</p>

---

## 目录结构

```text
fs/
├── README.md
├── Config.in
├── common.mk
├── axfs/
├── btrfs/
├── cloop/
├── cpio/
├── cramfs/
├── ext2/
├── f2fs/
├── initramfs/
├── iso9660/
├── jffs2/
├── romfs/
├── squashfs/
├── tar/
├── ubi/
├── ubifs/
└── yaffs2/
```

每种格式通常包含：

```text
Config.in       # Kconfig 开关和格式参数
<format>.mk     # 所需 Host 工具、生成命令和输出名称
```

当前目录共包含 16 种根文件系统或镜像生成方式。

---

## `fs/`、rootfs Overlay 与 `output/target/` 的区别

| 路径 | 性质 | 是否直接部署 |
| --- | --- | --- |
| `fs/` | Buildroot 镜像生成规则源码 | 否 |
| `board/cra/epass/rootfs/` | CRA 手写运行时文件 Overlay | 否，构建时合并 |
| `output/target/` | 软件包和 Overlay 合并后的中间根目录 | 不建议直接部署 |
| `output/build/buildroot-fs/` | 各镜像格式的临时 fakeroot 工作目录 | 否 |
| `output/images/` | 完成打包的根文件系统和板级镜像 | 是最终构建产物 |

`output/target/` 在普通用户权限下生成，不能完整表达设备节点、最终属主和某些权限。Buildroot 会在 fakeroot 环境中处理这些元数据，因此不要直接把 `output/target/` 当作正式根文件系统复制到设备。

---

## 总体生成流程

根文件系统镜像的大致生成过程如下：

```text
选中的目标软件包
        ↓
安装到 output/target/
        ↓
合并 skeleton 与 BR2_ROOTFS_OVERLAY
        ↓
target-finalize / post-build 脚本
        ↓
生成用户表和设备表
        ↓
复制到各格式的临时 target 目录
        ↓
fakeroot：属主、用户、设备节点、权限
        ↓
mkfs / tar / cpio 等格式生成命令
        ↓
可选 gzip、xz、lzo 等外层压缩
        ↓
output/images/rootfs.*
        ↓
板级 post-image 脚本继续组装最终镜像
```

构建根文件系统不会自动向实体设备写入文件。生成镜像与刷写设备是两个独立步骤。

---

## 根目录文件

### `Config.in`

该文件定义 `Filesystem images` 菜单，并依次引入所有受支持格式的 `Config.in`。它只负责配置入口，不执行镜像生成。

同一次构建可以选择多种格式。例如可以同时生成 `rootfs.tar` 和 `rootfs.ext4`，它们共享同一个目标文件集合，但分别经过自己的 fakeroot 和格式化步骤。

### `common.mk`

这是所有普通根文件系统格式共用的核心基础设施，主要负责：

- 定义 `rootfs-common` 依赖；
- 汇总软件包和用户自定义的用户表、组表、权限表及设备表；
- 为每种格式建立独立临时工作目录；
- 从最终目标目录复制文件；
- 生成并执行 fakeroot 脚本；
- 将文件属主统一为目标系统的 `root:root` 起点；
- 调用 `mkusers` 创建用户和组；
- 调用 `makedevs` 应用设备节点和权限规则；
- 执行 post-fakeroot 脚本和格式专用钩子；
- 调用具体格式的镜像生成命令；
- 按配置生成 gzip、bzip2、lzma、lz4、lzo 或 xz 压缩副本；
- 注册 `rootfs-<format>` 构建目标。

各格式只需要声明自己的依赖和 `ROOTFS_<FORMAT>_CMD`，通用流程由 `common.mk` 统一完成。

---

## 各格式目录

| 目录 | 主要用途 | 当前 CRA 是否直接使用 |
| --- | --- | --- |
| `axfs/` | 支持 XIP/按需读取的只读 AXFS 镜像 | 否 |
| `btrfs/` | Btrfs 根文件系统镜像 | 否 |
| `cloop/` | 压缩 Loop Block Device 镜像 | 否 |
| `cpio/` | CPIO 归档，常用于 initramfs | 否 |
| `cramfs/` | 传统只读压缩文件系统 | 否 |
| `ext2/` | ext2/ext3/ext4 镜像生成 | 否 |
| `f2fs/` | 面向闪存介质的 F2FS 镜像 | 否 |
| `initramfs/` | 将根文件系统归档嵌入 Linux 内核 | 否 |
| `iso9660/` | 可引导 ISO 光盘镜像 | 否 |
| `jffs2/` | 面向原始闪存的 JFFS2 镜像 | 否 |
| `romfs/` | 简单只读 ROMFS 镜像 | 否 |
| `squashfs/` | 高压缩只读 SquashFS 镜像 | 否 |
| `tar/` | 保留目录结构和目标元数据的 tar 归档 | 是 |
| `ubi/` | 用 `ubinize` 将 UBIFS 等卷封装为 UBI 容器 | 当前未走此通用入口 |
| `ubifs/` | 为 UBI 卷生成 UBIFS 文件系统 | 当前未走此通用入口 |
| `yaffs2/` | 面向原始 NAND 的 YAFFS2 镜像 | 否 |

---

## 只读、块设备与原始闪存格式

不同格式面向的存储层不同，不能只根据文件体积互换。

| 类型 | 示例 | 典型特点 |
| --- | --- | --- |
| 普通归档 | tar、cpio | 保存文件集合，本身不一定是可挂载块设备 |
| 块文件系统 | ext4、Btrfs、F2FS | 通常写入 SD/eMMC 分区或块设备 |
| 只读压缩文件系统 | SquashFS、CramFS、ROMFS | 适合固定系统内容，运行时不可直接写入 |
| 原始闪存文件系统 | JFFS2、YAFFS2 | 直接面向特定原始闪存特征 |
| UBI 管理层 | UBI + UBIFS | UBI 管理擦除块和坏块，UBIFS 位于 UBI 卷之上 |

电子通行证使用 SPI NAND。其 PEB、LEB、最小 I/O 单元、VID header 偏移和卷大小必须与实际 NAND、内核和 U-Boot 配置一致，不能把 ext4 或任意 UBI 参数直接替换进去。

---

## `tar/`

`BR2_TARGET_ROOTFS_TAR` 默认启用，因此 CRA defconfig 即使没有显式写出该选项，也会生成未额外压缩的：

```text
output/images/rootfs.tar
```

`tar.mk` 会：

- 对文件名进行稳定排序；
- 使用数字 UID/GID；
- 保留扩展属性；
- 避免把 atime/ctime 写入 PaxHeaders，以改善可复现性；
- 按配置生成 gzip、bzip2、lz4、lzma、lzo 或 xz 副本。

---

## `ubi/` 与 `ubifs/` 的通用实现

### `ubifs/`

`ubifs.mk` 使用 Host `mkfs.ubifs`，主要参数包括：

| 参数 | Kconfig 项 | 含义 |
| --- | --- | --- |
| `-e` | `BR2_TARGET_ROOTFS_UBIFS_LEBSIZE` | 逻辑擦除块大小 |
| `-m` | `BR2_TARGET_ROOTFS_UBIFS_MINIOSIZE` | 最小 I/O 单元 |
| `-c` | `BR2_TARGET_ROOTFS_UBIFS_MAXLEBCNT` | 最大逻辑擦除块数量 |
| `-x` | 运行时压缩选择 | none、zlib 或 lzo |

它生成默认名为 `rootfs.ubifs` 的文件系统镜像。

### `ubi/`

`ubi.mk` 依赖 `rootfs-ubifs`，再使用 Host `ubinize` 生成 UBI 容器。通用参数包括：

- NAND 物理擦除块 PEB 大小；
- sub-page 大小；
- 最小 I/O 单元；
- 自定义或默认 `ubinize.cfg`；
- 额外 `ubinize` 参数。

默认 [ubi/ubinize.cfg](ubi/ubinize.cfg) 建立一个名为 `rootfs` 的动态卷，并启用自动扩容。配置中的 `BR2_ROOTFS_UBIFS_PATH` 会在构建时替换为实际 UBIFS 镜像路径。

---

## 语法与注释

| 文件类型 | 语言 | 注释和格式要求 |
| --- | --- | --- |
| `Config.in` | Kconfig | `# 注释`；`help` 内容保持正确缩进 |
| `.mk` | GNU Make | `# 注释`；配方命令必须以 Tab 开头 |
| `.cfg` | ubinize/bootloader 配置 | 通常用 `#` 或 `;` 注释，保持键值格式 |
| Shell 脚本 | POSIX Shell | `# 注释`；保留 LF、shebang 和可执行位 |

---

<div align="center">

<sub><b>fs/</b> · root filesystem image infrastructure for Buildroot</sub>

</div>
