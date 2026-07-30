# CRA Electric Pass 设备树

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 CRA Electric Pass 的板级设备树源码，分为 U-Boot 和 Linux 两套相互独立的配置。当前工程仅面向白银 v0.6 板型。

设备树用于描述处理器内部控制器、主板引脚、启动存储、显示系统、接口和外接设备。Linux 或 U-Boot 驱动通过设备树中的节点和 `compatible` 字符串匹配实际硬件。

## 目录结构

```text
devicetree/
├─ uboot/
│  ├─ suniv-f1c100s-generic.dts
│  └─ README.md
└─ linux/
   ├─ base/
   ├─ screen/
   ├─ interface/
   ├─ ext/
   └─ README.md
```

| 目录 | 使用阶段 | 主要职责 | 详细说明 |
| --- | --- | --- | --- |
| `uboot/` | SPL 与 U-Boot | 启动闪存、启动串口、USB DFU、启动阶段 MMC 和必要的 SoC 资源 | [uboot/README.md](uboot/README.md) |
| `linux/` | Linux | 完整主板硬件、显示链路、屏幕、接口、外接设备和 Linux 驱动参数 | [linux/README.md](linux/README.md) |

设备从上电到进入应用程序会经历多个软件阶段：

```text
Allwinner BROM
        │
        ▼
SPL
        │
        ▼
U-Boot
        │
        ├─ 读取启动环境
        ├─ 读取 boot.itb
        ├─ 提取 Linux 基础 DTB
        └─ 应用 Linux DTBO
        │
        ▼
Linux
        │
        ▼
CRA Electric Pass 应用程序
```

SPL 和 U-Boot 必须在 Linux 尚未启动时访问 SPI-NAND、串口和 USB DFU，因此需要一套供启动加载器自身驱动使用的设备树。

Linux 启动后会重新初始化硬件，并使用另一套更完整的设备树。该设备树描述 LCD、背光、按键、存储、接口和外接设备等 Linux 驱动需要的内容。

两套设备树不会自动同步：

- 修改 `uboot/` 不会改变 Linux 启动后的硬件状态。
- 修改 `linux/` 不会改变 SPL 或 U-Boot 的硬件初始化。
- 同一个控制器可能在两套设备树中具有不同的启用状态、引脚配置和用途。
- 同时影响启动加载器和 Linux 的硬件改动，必须分别检查两套设备树。

## 两套设备树的职责边界

| 硬件或功能 | U-Boot 设备树 | Linux 设备树 |
| --- | --- | --- |
| SPI-NAND 启动读取 | 必须配置 | 负责 MTD、分区和运行时访问 |
| 启动串口控制台 | 必须配置 | Linux 串口控制台和普通 UART |
| USB DFU | 必须配置 | Linux USB Gadget、Host 或其他模式 |
| SD/MMC 启动阶段访问 | 按启动需求配置 | Linux SD 卡和 MMC 驱动 |
| LCD 与背光 | 当前不由 U-Boot 接管 | 完整配置 |
| ST7701 初始化 | 不负责 | 由 Linux 设备树和屏幕覆盖层配置 |
| LRADC 按键 | 不负责 | 由 Linux 设备树配置 |
| I²C、I²S、SPI1、UART 扩展 | 通常不需要 | 由 Linux 接口覆盖层配置 |
| CardKB、ES8311、LSM6DS3 | 不需要 | 由 Linux 外接设备覆盖层配置 |

当前 U-Boot 配置关闭：

```text
# CONFIG_VIDEO_SUNXI is not set
```

## 公共 SoC 定义

本目录中的 CRA 板级 DTS 并没有重新定义 F1C100S/F1C200S 的所有寄存器和控制器。两套设备树分别引用 Buildroot 中的公共 SUNIV 定义。

U-Boot 公共文件：

```text
board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
```

Linux 公共文件：

```text
board/allwinner/suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi
```

组合关系为：

```text
公共 SUNIV SoC 定义
        │
        ▼
CRA 板级基础定义
        │
        ▼
当前板型入口或覆盖层
```

公共 `.dtsi` 提供寄存器地址、时钟、复位、中断、DMA、GPIO 和基础控制器节点；CRA 文件负责选择当前 PCB 实际使用的控制器、引脚和参数。

公共文件来自原项目和对应上游作者。修改时应保留原有 SPDX、版权和提交历史，不应把公共 SoC 定义重新标记为 CRA 原创。

## U-Boot 设备树

U-Boot 板级入口为：

```text
uboot/suniv-f1c100s-generic.dts
```

它通过：

```dts
#include "suniv-f1c100s.dtsi"
```

包含公共 SUNIV 定义，并启用当前启动流程需要的：

- UART0 启动控制台。
- UART1。
- SPI0 和 SPI-NAND。
- USB OTG、USB PHY 与 OTG SRAM。
- 第一组 MMC。

当前代码显式关闭与 SPI0 引脚冲突的第二组 MMC。

Buildroot 配置引用：

```text
BR2_TARGET_UBOOT_CUSTOM_DTS_PATH="
    board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
    board/cra/epass/devicetree/uboot/suniv-f1c100s-generic.dts"
```

U-Boot 配置文件：

```text
board/cra/epass/uboot.defconfig
```

使用：

```text
CONFIG_DEFAULT_DEVICE_TREE="suniv-f1c100s-generic"
```

选择该板级 DTS。

## Linux 设备树

Linux 基础设备树由以下文件组成：

```text
linux/base/epass.dtsi
linux/base/devicetree.dts
```

Buildroot 配置引用：

```text
BR2_LINUX_KERNEL_CUSTOM_DTS_PATH="
    board/allwinner/suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi
    board/cra/epass/devicetree/linux/base/epass.dtsi
    board/cra/epass/devicetree/linux/base/devicetree.dts"
```

基础包含关系为：

```text
suniv-f1c100s.dtsi
        ↓
linux/base/epass.dtsi
        ↓
linux/base/devicetree.dts
```

Linux 设备树在基础 DTB 之外还包含三类覆盖层：

| 类型 | 目录 | 是否必选 | 作用 |
| --- | --- | --- | --- |
| 屏幕 | `linux/screen/` | 必须选择一个 | 选择 BOE、HSD 或 Laowu 屏幕初始化 |
| 接口 | `linux/interface/` | 可选，可多个 | 启用 I²C、I²S、SPI、UART、ADC 或 USB 模式 |
| 外设 | `linux/ext/` | 可选，可多个 | 声明 CardKB、ES8311、LSM6DS3 等设备 |

U-Boot 按以下顺序组合 Linux 设备树：

```text
base → screen → interface → ext
```

## Buildroot 构建关系

当前主要版本为：

| 组件 | 版本 |
| --- | --- |
| Linux | `5.4.99` |
| U-Boot | `2020.07` |

Buildroot 配置文件：

```text
board/cra/epass/cra_epass_defconfig
```

其中分别指定：

- Linux 版本、补丁、内核配置和 Linux DTS。
- U-Boot 版本、补丁、U-Boot 配置和 U-Boot DTS。
- 镜像后处理脚本。

镜像后处理顺序为：

```text
board/cra/epass/scripts/mknanduboot.sh
        ↓
board/cra/epass/scripts/mkdt.sh
        ↓
board/cra/epass/scripts/buildimage.sh
```

三个脚本的职责如下：

| 脚本 | 作用 |
| --- | --- |
| `mknanduboot.sh` | 将普通 SPL/U-Boot 输出整理为当前 SPI-NAND 页布局 |
| `mkdt.sh` | 编译 Linux 基础 DTB 和所有 DTBO |
| `buildimage.sh` | 生成 UBI 根文件系统，并根据 `kernel.its` 打包 `boot.itb` |

## 从源码到启动

整体构建和启动关系为：

```text
U-Boot 公共 SUNIV .dtsi
        +
CRA uboot/*.dts
        │
        ▼
U-Boot/SPL 构建
        │
        ▼
u-boot-sunxi-with-spl.bin
        │
        ▼
mknanduboot.sh
        │
        ▼
u-boot-sunxi-with-nand-spl.bin

Linux 公共 SUNIV .dtsi
        +
linux/base/*.dts*
        +
linux/screen/*.dts
        +
linux/interface/*.dts
        +
linux/ext/*.dts
        │
        ▼
mkdt.sh
        │
        ├─ devicetree.dtb
        └─ *.dtbo
        │
        ▼
kernel.its + zImage
        │
        ▼
buildimage.sh
        │
        ▼
boot.itb
```

实体设备启动时：

```text
SPI-NAND 中的 U-Boot
        │
        ├─ 使用 U-Boot 自身设备树初始化启动硬件
        ├─ 读取 0xFA000 的文本启动环境
        ├─ 读取 0x100000 的 boot.itb
        ├─ 提取 Linux 基础 DTB
        ├─ 应用 screen/interface/ext DTBO
        └─ 将组合后的 DTB 传给 Linux
```

## 生成文件

常见生成物包括：

| 文件 | 来源 | 用途 |
| --- | --- | --- |
| `output/images/u-boot-sunxi-with-spl.bin` | U-Boot 构建 | 尚未完成 NAND 页布局处理的 U-Boot 镜像 |
| `output/images/u-boot-sunxi-with-nand-spl.bin` | `mknanduboot.sh` | 实体 SPI-NAND 使用的最终 U-Boot 镜像 |
| `output/images/dt/base/devicetree.dtb` | `mkdt.sh` | Linux 基础设备树 |
| `output/images/dt/screen/*.dtbo` | `mkdt.sh` | 屏幕覆盖层 |
| `output/images/dt/interface/*.dtbo` | `mkdt.sh` | 接口覆盖层 |
| `output/images/dt/ext/*.dtbo` | `mkdt.sh` | 外接设备覆盖层 |
| `output/images/boot.itb` | `buildimage.sh` | Linux 内核、基础 DTB 和全部 DTBO 的 FIT 镜像 |

请勿直接修改：

```text
output/build/
output/images/dt/
```

这些目录属于构建生成物，清理或重新构建后会被覆盖。长期修改必须写入 `board/cra/epass/devicetree/`、相关配置或构建脚本。

## 文件类型

| 后缀 | 含义 |
| --- | --- |
| `.dtsi` | 可由其他设备树包含的公共源文件 |
| `.dts` | 可直接编译的设备树入口或设备树覆盖层源码 |
| `.dtb` | 编译后的基础设备树二进制 |
| `.dtbo` | 编译后的设备树覆盖层二进制 |
| `.its` | FIT 镜像的文本描述 |
| `.itb` | 由 ITS 打包生成的 FIT 二进制镜像 |

设备树源码使用 DTS 语法，支持 C 风格注释：

```dts
/* 块注释 */
```

当前工程中也存在：

```dts
// 行注释
```

优先使用 `/* ... */`，以保持对设备树工具链的兼容性。

## 修改说明

| 需求 | 应修改的位置 |
| --- | --- |
| 修改 U-Boot 串口输出 | `uboot/`、`uboot.defconfig` 或 U-Boot 补丁 |
| 修改 Linux 串口或启用 UART | `linux/base/` 或 `linux/interface/` |
| 修改 SPL/U-Boot 访问的 SPI-NAND | `uboot/`、U-Boot 配置和补丁 |
| 修改 Linux MTD 分区 | `linux/base/`，并同步启动参数和镜像布局 |
| 修改屏幕初始化序列 | `linux/screen/` |
| 修改 LCD 时序或显示链路 | `linux/base/`、屏幕覆盖层及对应内核补丁 |
| 修改背光或按键 | `linux/base/` |
| 启用 I²C、I²S、SPI1、UART | `linux/interface/` |
| 添加具体外接设备 | `linux/ext/`，并检查所需接口 |
| 修改 USB DFU | `uboot/`、U-Boot 补丁和配置 |
| 修改 Linux USB 模式 | `linux/interface/` 和对应 Linux 补丁 |
| 修改应用程序 UI | 不属于设备树，应修改 `drm_app_neo` |

若改动涉及启动存储、串口、USB、时钟或同一组物理引脚，不能只检查表格中的单一位置，必须搜索两套设备树、配置、补丁和脚本中的全部引用。

## 文件名和标签的联动

修改 DTS 文件名时需要同步检查：

- `cra_epass_defconfig` 中的 `CUSTOM_DTS_PATH`。
- `uboot.defconfig` 中的 `CONFIG_DEFAULT_DEVICE_TREE`。
- `mkdt.sh` 的目录和输出规则。
- `kernel.its` 中的 DTB/DTBO 路径与 FIT 节点。
- `uboot.env` 中的 FIT 节点提取名称。
- `uEnv.txt`、`flash.py` 或实际启动环境中的选择值。
- 各层 README 和构建命令。

修改基础设备树节点标签时，必须搜索所有覆盖层中的引用。例如重命名 `i2c0`、`i2s0`、`pio`、`st7701initseq` 或 `usb_otg` 标签，会导致引用这些标签的 DTBO 无法正确生成或应用。

## 常见问题

| 现象 | 优先检查 |
| --- | --- |
| 修改 Linux DTS 后 U-Boot 没有变化 | 两套设备树独立，需修改 `uboot/` |
| 修改 U-Boot DTS 后 Linux 驱动没有变化 | Linux 使用 `linux/` 下的另一套设备树 |
| 新增覆盖层已经生成但启动时找不到 | 是否同步添加到 `kernel.its` |
| 修改覆盖层后实体设备仍使用旧配置 | 是否重新生成 `boot.itb`，启动分区是否仍为旧镜像 |
| 修改 U-Boot DTS 后未生效 | 是否清理并重新构建 U-Boot |
| DTBO 编译成功但应用失败 | 基础 DTB 是否保留符号，标签和 `target` 是否存在 |
| 多个接口单独可用、组合后失败 | 是否存在 GPIO、时钟、DMA、中断或总线资源冲突 |
| 屏幕无显示或颜色异常 | `screen` 选择、初始化序列、RGB 通道和内核显示补丁 |
| USB 启动阶段和 Linux 阶段行为不同 | 分别检查 U-Boot USB 配置和 Linux USB 覆盖层 |
| Windows 下修改后脚本构建失败 | `.sh` 是否被转换为 CRLF |
| 修改源码后输出仍未更新 | 是否使用了旧的 `output/build` 或 `output/images` 缓存 |

## 构建与验证

完整配置与构建：

```sh
make cra_epass_defconfig
make
```

构建后检查 FIT 内容：

```sh
output/host/bin/mkimage -l output/images/boot.itb
```

反编译 Linux 基础设备树：

```sh
dtc -I dtb -O dts \
    -o devicetree-linux.decoded.dts \
    output/images/dt/base/devicetree.dtb
```

反编译 U-Boot 设备树：

```sh
dtc -I dtb -O dts \
    -o devicetree-uboot.decoded.dts \
    output/build/uboot-2020.07/u-boot.dtb
```

检查时应确认：

- U-Boot DTB 只启用了启动阶段实际需要的控制器。
- Linux 基础 DTB 包含 DTBO 所需的符号。
- `kernel.its` 引用的每个 DTB 和 DTBO 都实际存在。
- FIT 节点名称与启动环境中的 `screen`、`interface`、`ext` 完全一致。
- 没有同时启用占用相同物理引脚的控制器。
- `boot.itb` 总大小未超过 U-Boot `checkfit` 的 5 MiB 限制。
- 最终 U-Boot 文件为经过 NAND 布局处理的 `u-boot-sunxi-with-nand-spl.bin`。

设备树编译成功只代表语法、引用和结构满足工具要求，不代表硬件电平、PCB 接线、时序、驱动依赖和所有覆盖层组合已经验证。

## 二次开发原则

- 修改前先判断问题发生在 SPL/U-Boot 阶段还是 Linux 阶段。
- 不应为了统一外观而强行让两套设备树包含相同节点。
- 修改公共 SUNIV `.dtsi` 前，应确认改动是否真的适用于所有引用该文件的板级配置。
- 修改 `compatible` 时，必须同步检查匹配它的 U-Boot 或 Linux 驱动。
- 修改引脚组时，必须搜索两套设备树及全部覆盖层中的物理引脚占用。
- 修改启动存储布局时，必须同步检查 U-Boot、Linux MTD、UBI、启动环境、镜像脚本和烧录工具。
- 新增 Linux 覆盖层后，必须同步更新 `kernel.its` 和启动环境。
- 不应直接修改 `output/build` 或 `output/images/dt` 中的生成文件。
- 所有由 Linux 或 Buildroot 执行的脚本必须保持 LF 换行。
- 在实体设备验证前，应完成干净构建、DTB/DTBO 反编译检查、FIT 节点检查，并保留可恢复镜像和串口恢复方式。
