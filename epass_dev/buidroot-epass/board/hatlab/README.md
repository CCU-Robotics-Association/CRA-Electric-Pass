# HatLab 板级支持

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 HatLab BADGE200 在 Buildroot 中的板级支持文件：

```text
board/hatlab/
```

## 目录结构

```text
hatlab/
└── badge200/
    ├── hatlab_badge200_defconfig  Buildroot 板级配置
    ├── linux.defconfig            Linux 内核配置
    ├── uboot.defconfig            U-Boot 配置
    ├── devicetree/                Linux 与 U-Boot 设备树
    ├── rootfs/                    BADGE200 根文件系统覆盖层
    ├── scripts/                   固件镜像后处理脚本
    ├── helper/                    vendor 分区制作与下载脚本
    ├── mcu/                       独立的 ATmega328P 示例固件
    └── driver/                    Windows RNDIS 驱动归档
```

## 支持状态

`badge200/` 包含相对完整的 Buildroot 配置、Linux 与 U-Boot 配置、设备树、rootfs 覆盖层和镜像脚本。仓库同时在 `configs/` 中提供入口：

```text
configs/hatlab_badge200_defconfig
```

在保留 Git 符号链接的 Linux 检出中，可通过以下命令载入配置：

```sh
make hatlab_badge200_defconfig
make
```

Windows 检出可能把 `configs/hatlab_badge200_defconfig` 还原成只包含目标路径文字的普通文件。此时它不是有效的 Buildroot 配置文件，应在 Linux 或 WSL 中恢复符号链接后再构建。

## Buildroot 配置

`badge200/hatlab_badge200_defconfig` 是该目标的总装配入口，主要配置包括：

- ARMv5 目标与 Buildroot 内部 glibc 工具链；
- U-Boot `2020.07`、SPL、SPI flash、SPI-NAND、USB 下载和 DFU；
- Linux `5.4.100` 及 SUNIV F1C100S/F1C200S 公共补丁；
- ext4、JFFS2 与 SquashFS 文件系统；
- BADGE200 专用 rootfs 覆盖层；
- framebuffer、触摸、音频、摄像头、蓝牙、D-Bus、Python 3 和调试工具；
- 使用 `scripts/genimage.sh` 生成 NAND 镜像。

配置按以下层次组合：

```text
board/allwinner/generic/
        ↓
board/allwinner/suniv-f1c100s/
        ↓
board/hatlab/badge200/
```

## U-Boot 与启动镜像

### `uboot.defconfig`

该文件配置 BADGE200 的 U-Boot，主要内容包括：

- SUNIV/F1C100S 架构与 SPL；
- 408 MHz 系统时钟和 168 MHz DRAM；
- 串口控制台、SPI、SPI-NOR、SPI-NAND、MMC 和 USB OTG；
- USB Mass Storage 与 DFU 下载支持；
- 480×272 LCD 时序；
- 使用 `board/allwinner/generic/uboot.env` 作为默认环境。

### `scripts/genimage.sh`

镜像脚本完成以下工作：

1. 使用公共 `kernel.its` 生成 `kernel.itb`；
2. 将公共 `splash.bmp` 复制到镜像输出目录；
3. 生成带 NAND SPL 的 U-Boot；
4. 使用公共 NAND 镜像布局生成最终镜像。

因此 BADGE200 目标的启动图来自：

```text
board/allwinner/generic/splash.bmp
```

## Linux 配置与设备树

### `linux.defconfig`

该文件保存 BADGE200 使用的 Linux 内核配置。它决定内核驱动、文件系统、网络、蓝牙、媒体、调试和 SoC 外设支持。修改后应通过实际内核构建验证，不应只检查文本语法。

### `devicetree/linux/devicetree.dts`

Linux 设备树描述的主要硬件包括：

- F1C200S/F1C100S 兼容 SoC；
- SPI-NAND 及 `u-boot`、`kernel`、`rom`、`vendor`、`overlay` 分区；
- RGB LCD、显示引擎和音频 codec；
- UART、MMC、USB OTG；
- AW9523B GPIO 扩展器；
- AXP199 电源管理与电池、电源检测；
- PCF8563 RTC；
- Goodix GT911 触摸控制器；
- OV2640 摄像头和 CSI；
- LRADC 三个按键；
- 通过 UART 连接的 Broadcom 蓝牙控制器。

### `devicetree/uboot/suniv-f1c100s-generic.dts`

U-Boot 设备树只提供启动阶段需要的基础节点，包括串口、SPI flash、MMC 和 USB。它不等同于 Linux 运行时设备树。

## Rootfs 覆盖层

`rootfs/` 会被复制到目标根文件系统，包含：

| 路径 | 用途 |
| --- | --- |
| `etc/init.d/S30rndis` | 加载 `g_ether`，启动 USB RNDIS 网络 |
| `etc/init.d/S50dropbear` | 启动 Dropbear SSH 服务 |
| `etc/init.d/S90btbcm` | 加载蓝牙 HCI UART 驱动 |
| `etc/init.d/S99application` | 挂载 JFFS2 vendor 分区并运行应用启动脚本 |
| `etc/network/interfaces` | 配置目标网络接口 |
| `etc/dnsmasq.conf` | 配置 USB 网络侧 DHCP |
| `usr/lib/python3.8/site-packages/pyclui/` | 彩色命令行日志辅助模块 |
| `usr/lib/python3.8/site-packages/bthci/` | 蓝牙 HCI 操作辅助模块 |

这些 Python 模块是随板级覆盖层直接安装的代码，不由 Buildroot 的标准 Python 包机制管理。升级 Python、BlueZ、PyBlueZ、Scapy 或 D-Bus 时，应单独验证兼容性。

## Helper 与 MCU

### `helper/`

- `mkvendor.sh`：将指定目录制作成固定参数的 JFFS2 `vendor.img`；
- `dfu-nand-vendor.sh`：等待 DFU 设备出现，然后把镜像写入 `vendor` DFU alternate setting。

这两个脚本会生成或写入设备分区。使用前必须确认目标板、分区布局和镜像文件，不能用于 CRA 实体设备。

### `mcu/`

`mcu/` 是独立于 Allwinner Linux 系统的 AVR 示例工程：

- 目标 MCU 为 ATmega328P；
- 默认时钟为 16 MHz；
- `main.c` 周期性翻转 PB0；
- Makefile 使用 `avr-gcc`、`avr-objcopy` 和 `avrdude`。

它不会被 `make hatlab_badge200_defconfig && make` 自动构建，也与 CRA Electric Pass 主程序无关。

## 二进制文件

`driver/windows-rndis-driver.zip` 是预打包的 Windows 驱动归档。Git 可以保存该文件，但无法像源代码一样审查其内部改动。更新时应记录来源、版本、哈希和适用系统。

## 与 CRA Electric Pass 的关系

当前 CRA 配置不引用 `board/hatlab/`。该目录保留的意义主要是：

- 支持原仓库中的 BADGE200 构建目标；
- 提供 F1C200S 外设、蓝牙、电源管理和摄像头配置参考；
- 保存旧板级镜像和辅助工具。

不要为了修改 CRA 设备而直接改动本目录。CRA 专用修改应优先放在：

```text
board/cra/epass/
```
