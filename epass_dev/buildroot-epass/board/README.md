<div align="center">

# Buildroot 板级支持目录

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `board/` 保存该 Buildroot 工程的公共 SoC 层、开发板专用配置，以及 CRA Electric Pass 的完整板级固件定义。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#支持状态总览">支持状态</a> ·
  <a href="#子目录文档">子目录文档</a> ·
  <a href="#buildroot-配置入口">配置入口</a> ·
  <a href="#公共层与专用层">分层边界</a> ·
  <a href="#常见文件职责">文件职责</a> ·
  <a href="#二次开发原则">开发原则</a>
</p>

---

板级支持的根路径为：

```text
board/
```

## 目录结构

```text
board/
├── allwinner/              Allwinner 公共平台层
│   ├── generic/            多种板卡共用的启动、镜像和 rootfs 支持
│   ├── suniv-f1c100s/      F1C100S/F1C200S 公共支持
│   └── sun8i-v3/           V3s/S3 公共支持
├── cra/
│   └── epass/              CRA Electric Pass 专用板级支持
├── hatlab/
│   └── badge200/           HatLab BADGE200 支持
├── hqembed/
│   └── hq050ips/           尚未完成的占位目录
├── sipeed/
│   └── lichee/
│       ├── nano/           Sipeed Lichee Nano 支持
│       └── zero/           不完整的 Lichee Zero 历史配置
└── widora/
    └── mangopi/
        ├── r1/             Widora MangoPi R1 支持
        ├── r2/             Widora MangoPi R2 支持
        └── r3/             Widora MangoPi R3 支持
```

## 支持状态总览

| 目录 | 目标 |
| :--- | :--- |
| `allwinner/generic/` | Allwinner 公共启动和镜像层 |
| `allwinner/suniv-f1c100s/` | F1C100S/F1C200S 公共层 |
| `allwinner/sun8i-v3/` | V3s/S3 公共层 |
| `cra/epass/` | CRA Electric Pass |
| `hatlab/badge200/` | HatLab BADGE200 |
| `hqembed/hq050ips/` | HQEmbed/HQ050IPS |
| `sipeed/lichee/nano/` | Sipeed Lichee Nano |
| `sipeed/lichee/zero/` | Sipeed Lichee Zero |
| `widora/mangopi/r1/` | Widora MangoPi R1 |
| `widora/mangopi/r2/` | Widora MangoPi R2 |
| `widora/mangopi/r3/` | Widora MangoPi R3 |

## 子目录文档

| 目录 | 说明文档 |
| --- | --- |
| `allwinner/` | [Allwinner 公共板级支持](allwinner/README.md) |
| `cra/epass/` | [CRA Electric Pass 板级支持包](cra/epass/README.md) |
| `hatlab/` | [HatLab 板级支持](hatlab/README.md) |
| `hqembed/` | [HQEmbed 板级目录](hqembed/README.md) |
| `sipeed/` | [Sipeed 板级支持](sipeed/README.md) |
| `widora/` | [Widora MangoPi 板级支持](widora/README.md) |

## Buildroot 配置入口

Buildroot 通常从仓库根目录的 `configs/` 载入板级配置，而不是直接要求用户输入 `board/` 下的路径。

当前仓库提供以下入口：

| Buildroot 命令 | 规范配置源文件 |
| --- | --- |
| `make cra_epass_defconfig` | `board/cra/epass/cra_epass_defconfig` |
| `make hatlab_badge200_defconfig` | `board/hatlab/badge200/hatlab_badge200_defconfig` |
| `make sipeed_lichee_nano_defconfig` | `board/sipeed/lichee/nano/sipeed_lichee_nano_defconfig` |
| `make widora_mangopi_r1_defconfig` | `board/widora/mangopi/r1/widora_mangopi_r1_defconfig` |
| `make widora_mangopi_r2_defconfig` | `board/widora/mangopi/r2/widora_mangopi_r2_defconfig` |
| `make widora_mangopi_r3_defconfig` | `board/widora/mangopi/r3/widora_mangopi_r3_defconfig` |

HQEmbed 和 Lichee Zero 当前没有可用的 `configs/` 入口。

### Windows 符号链接问题

这些 `configs/*_defconfig` 在 Git 中通常是指向 `board/` 中规范配置文件的符号链接。Windows 未启用 Git 符号链接支持时，可能把链接检出成只包含相对路径文字的普通文件，例如：

```text
../board/cra/epass/cra_epass_defconfig
```

这种普通文件不是完整的 defconfig，直接构建会失败或得到错误配置。应在 Linux 或 WSL 中确认：

```sh
ls -l configs/cra_epass_defconfig
```

并恢复有效符号链接。不要把链接占位文件复制成新的板级配置来源。

## 公共层与专用层

### 公共层

`board/allwinner/` 中的文件面向多个硬件目标，适合保存：

- SoC 控制器、时钟、中断和通用外设节点；
- 多板共用的 Linux 或 U-Boot 补丁；
- 通用 rootfs 初始化脚本；
- 旧目标共用的镜像布局、启动环境和启动图。

修改公共层可能同时影响多个构建目标，必须先搜索完整引用关系。

例如：

```text
board/allwinner/generic/splash.bmp
```

由 HatLab BADGE200、Sipeed Lichee Nano 和 Widora MangoPi 等公共镜像流程使用。

### 专用层

板卡专用目录应保存只适用于该硬件的内容，例如：

- LCD 分辨率和时序；
- 背光、按键和电源 GPIO；
- flash 类型与分区布局；
- 触摸、摄像头、音频和传感器节点；
- USB 产品名称；
- 板卡专用 rootfs 服务；
- 最终镜像生成流程。

## 常见文件职责

| 文件或目录 | 职责 |
| --- | --- |
| `*_defconfig` | 选择工具链、内核、U-Boot、软件包、rootfs 和镜像脚本 |
| `linux.defconfig` | Linux 内核功能和驱动配置 |
| `uboot.defconfig` | U-Boot、SPL、启动介质、显示和下载功能配置 |
| `uboot.env` | 编译进 U-Boot 的默认环境和启动命令 |
| `uEnv.txt` | 外部文本形式的 U-Boot 环境模板 |
| `devicetree/linux/` | Linux 运行阶段的硬件描述 |
| `devicetree/uboot/` | U-Boot 启动阶段的硬件描述 |
| `patch/linux/` | 针对固定 Linux 版本应用的补丁 |
| `patch/u-boot/` | 针对固定 U-Boot 版本应用的补丁 |
| `rootfs/` | 构建时覆盖到目标根文件系统的文件 |
| `scripts/` | 设备树、FIT、UBI、SD 卡或 NAND 镜像生成脚本 |
| `tools/` | 开发电脑端辅助工具，不直接运行在目标设备上 |

---

<div align="center">

<sub><b>board/</b> · shared SoC layers and board-specific support for Buildroot</sub>

</div>
