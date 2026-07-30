# CRA Electric Pass U-Boot 设备树

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 CRA Electric Pass 在 SPL 和 U-Boot 阶段使用的板级设备树。当前工程仅面向白银 v0.6 板型。

## 文件说明

| 文件 | 作用 |
| --- | --- |
| `suniv-f1c100s-generic.dts` | CRA Electric Pass 当前使用的 U-Boot 板级设备树入口 |

该文件本身只包含板级选择和外设启用状态。F1C100S/F1C200S 的寄存器地址、时钟、复位、中断及基础控制器节点来自公共文件：

```text
board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
```

## 与 Linux 设备树的区别

启动过程使用两套设备树：

```text
上电
 │
 ▼
SPL / U-Boot
 │
 └─ 使用 devicetree/uboot/suniv-f1c100s-generic.dts
 │
 ▼
U-Boot 读取 boot.itb
 │
 ├─ 提取 Linux 基础设备树
 ├─ 应用 screen overlay
 ├─ 应用 interface overlay
 └─ 应用 ext overlay
 │
 ▼
Linux
    └─ 使用 devicetree/linux/ 下组合后的设备树
```

两者职责不同：

| U-Boot 设备树 | Linux 设备树 |
| --- | --- |
| 供 SPL/U-Boot 驱动使用 | 供 Linux 内核驱动使用 |
| 只需描述启动前会访问的硬件 | 描述完整系统硬件 |
| 负责访问 SPI-NAND、DFU、串口等启动资源 | 负责 LCD、背光、按键、应用外设等 |
| 编译进 U-Boot 自身 | 作为 DTB/DTBO 打包进 `boot.itb` |

当前 U-Boot 配置明确关闭：

```text
# CONFIG_VIDEO_SUNXI is not set
```

## 设备树组合关系

Buildroot 配置通过：

```text
BR2_TARGET_UBOOT_CUSTOM_DTS_PATH=
    board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
    board/cra/epass/devicetree/uboot/suniv-f1c100s-generic.dts
```

向 U-Boot 提供两份自定义设备树源文件。

构建 U-Boot 前，Buildroot 会将它们复制到：

```text
output/build/uboot-2020.07/arch/arm/dts/
```

板级 DTS 再通过：

```dts
#include "suniv-f1c100s.dtsi"
```

包含公共 SUNIV SoC 定义。

最终关系为：

```text
公共 SUNIV SoC 描述
board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi
        │
        ▼
CRA Electric Pass 板级选择
board/cra/epass/devicetree/uboot/suniv-f1c100s-generic.dts
        │
        ▼
suniv-f1c100s-generic.dtb
        │
        ▼
编译进 U-Boot
```

`uboot.defconfig` 使用：

```text
CONFIG_DEFAULT_DEVICE_TREE="suniv-f1c100s-generic"
```

选择该 DTS 作为默认设备树。

## 公共 SUNIV 设备树

公共 `suniv-f1c100s.dtsi` 定义 SoC 内部资源，包括：

| 节点 | 地址或参数 | 作用 |
| --- | --- | --- |
| `osc24M` | 24 MHz | 主晶振 |
| `osc32k` | 32768 Hz | 低速时钟 |
| `cpu` | ARM926EJ-S | CPU 类型 |
| `sram-controller` | `0x01c00000` | SRAM 控制器 |
| `spi0` | `0x01c05000` | SPI0 控制器 |
| `ccu` | `0x01c20000` | 时钟和复位控制 |
| `intc` | `0x01c20400` | 中断控制器 |
| `pio` | `0x01c20800` | GPIO 和引脚复用 |
| `timer` | `0x01c20c00` | 定时器 |
| `watchdog` | `0x01c20ca0` | 看门狗 |
| `uart0` | `0x01c25000` | 串口 0 |
| `uart1` | `0x01c25400` | 串口 1 |
| `uart2` | `0x01c25800` | 串口 2 |
| `usb_otg` | `0x01c13000` | USB OTG 控制器 |
| `usbphy` | `0x01c13400` | USB PHY |
| `mmc0` | `0x01c0f000` | 第一组 MMC 控制器 |
| `mmc2` | `0x01c10000` | 第二组 MMC 控制器资源 |

公共文件中多数外设默认：

```dts
status = "disabled";
```

板级 DTS 只启用启动过程实际需要的节点。

公共文件开头的版权信息为：

```text
Copyright 2018 Icenowy Zheng
```

它在本仓库的 Git 历史中最早由 Aodzip 于 2021 年导入，不是 CRA 二次开发阶段重新编写的 SoC 寄存器表。

## 别名与控制台

### `aliases`

```dts
aliases {
    serial0 = &uart0;
    spi0 = &spi0;
};
```

作用如下：

| 别名 | 目标 | 作用 |
| --- | --- | --- |
| `serial0` | `uart0` | 将 UART0 设为逻辑串口 0 |
| `spi0` | `spi0` | 为 SPI0 提供稳定别名 |

别名不是新增硬件，只是为已有节点提供稳定名称。

### `chosen`

```dts
chosen {
    stdout-path = "serial0:115200n8";
};
```

U-Boot 控制台参数为：

| 参数 | 数值 |
| --- | --- |
| 串口 | UART0 |
| 波特率 | 115200 |
| 数据位 | 8 |
| 校验 | 无 |
| 停止位 | 1 |

UART0 使用：

```text
PE0：UART0 TX
PE1：UART0 RX
```

对应的项目补丁 `0001-uart-pull.patch` 为 PE0 和 PE1 都启用上拉。

## USB SRAM

```dts
&otg_sram {
    status = "okay";
};
```

USB OTG 控制器需要使用内部 SRAM D 区。公共设备树默认关闭该 SRAM 分区，板级 DTS 在启用 USB OTG 的同时将它打开。

如果只启用 `usb_otg` 而未启用 `otg_sram`，MUSB 驱动可能无法正常取得所需 SRAM。

## SPI0 与启动闪存

### SPI0 控制器

```dts
&spi0 {
    pinctrl-names = "default";
    pinctrl-0 = <&spi0_pins_a>;
    status = "okay";
};
```

SPI0 使用：

```text
PC0
PC1
PC2
PC3
```

具体信号方向由 SUNIV SPI0 引脚复用决定。

当前 SPI-NAND 节点将传输频率上限设为：

```dts
spi-max-frequency = <80000000>;
```

`80 MHz` 是请求的频率上限，不表示硬件一定以 80 MHz 运行。`0004-f1c-spi-fix.patch` 会根据 SUNIV 的 200 MHz SPI 父时钟选择一个不高于请求值的分频结果。

### SPI-NAND 节点

```dts
spi-nand@0 {
    reg = <0>;
    compatible = "spi-nand";
    spi-max-frequency = <80000000>;
};
```

该节点描述 SPI-NAND。当前电子通行证的实际启动存储为 SPI-NAND。

项目补丁额外支持：

| 芯片 ID | 型号 |
| --- | --- |
| `c2 12` | Macronix MX35LF1GE4AB |
| `c2 14` | Macronix MX35LF1G24AD |

基础 SUNIV U-Boot 补丁还包含 Winbond、GigaDevice 等 SPI-NAND 支持。

节点名称中的 `@0` 与 `reg = <0>` 一致，均表示 SPI0 片选 0。

## 串口

### UART0

```dts
&uart0 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart0_pins_a>;
    status = "okay";
};
```

UART0 使用 PE0、PE1，并作为 U-Boot 主控制台。

### UART1

```dts
&uart1 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart1_pins_a>;
    status = "okay";
};
```

UART1 使用：

```text
PA2
PA3
```

U-Boot 与 Linux 的引脚使用相互独立。Linux 启动后会重新根据 Linux 设备树配置 PA2、PA3。

## USB OTG 与 DFU

```dts
&usb_otg {
    status = "okay";
};

&usbphy {
    status = "okay";
};
```

这两个节点共同启用：

- SUNIV USB OTG 控制器。
- USB PHY。
- U-Boot USB Gadget。
- DFU 下载功能。

`uboot.defconfig` 启用：

```text
CONFIG_CMD_DFU=y
CONFIG_DFU_MTD=y
CONFIG_USB_MUSB_GADGET=y
CONFIG_USB_GADGET_DOWNLOAD=y
```

项目通过 `0002-musb-force-fs.patch` 清除 MUSB 的 High-Speed Enable 位，强制 U-Boot USB 以 Full-Speed 模式工作。

`0005-dfu-verify-block.patch` 为 MTD DFU 写入增加：

- 按擦除块分段写入。
- 写后读回校验。
- 写入或校验失败时将块标记为坏块。
- 跳过坏块后在后续可用块重试。
- 可用空间耗尽时返回 `-ENOSPC`。

因此，当前 U-Boot 的 DFU 不只是普通数据传输，还承担 SPI-NAND 坏块处理和写入验证。

## MMC 控制器

### `mmc0`

```dts
&mmc0 {
    status = "okay";
};
```

使用：

```text
PF0～PF5
```

对应第一组 MMC/SD 控制器，公共设备树配置了上拉，并使用 `broken-cd` 表示不依赖标准卡检测信号。

### `mmc2`

```dts
&mmc2 {
    status = "disabled";
};
```

虽然标签名为 `mmc2`，该节点实际使用：

```text
寄存器：0x01c10000
时钟：MMC1
复位：MMC1
引脚：PC0、PC1、PC2
功能：mmc1
```

这是公共设备树沿用的标签命名，不表示芯片内部真的存在编号为 2 的独立控制器。

当前板级 DTS 显式保持该节点关闭，`uboot.defconfig` 也不再设置 `CONFIG_MMC_SUNXI_SLOT_EXTRA=1`。

### 与 SPI0 的引脚冲突

`mmc2` 使用：

```text
PC0、PC1、PC2
```

SPI0 使用：

```text
PC0、PC1、PC2、PC3
```

因此两者不能在同一时间按当前引脚组工作。当前代码通过关闭 `mmc2` 并取消额外 MMC 插槽配置，保证 PC0～PC3 只供 SPI0 启动闪存使用。

若后续需要使用第二组 MMC，必须结合 PCB 走线和启动介质重新设计选择逻辑，不能只把 `status` 改回 `"okay"`。

## U-Boot 配置

工程使用：

```text
U-Boot 2020.07
```

Buildroot 配置位于：

```text
board/cra/epass/cra_epass_defconfig
```

U-Boot Kconfig 配置位于：

```text
board/cra/epass/uboot.defconfig
```

关键参数如下：

| 配置 | 当前值 | 作用 |
| --- | --- | --- |
| `CONFIG_ARCH_SUNXI` | `y` | Allwinner SUNXI 平台 |
| `CONFIG_MACH_SUNIV` | `y` | F1C100S/F1C200S SUNIV 平台 |
| `CONFIG_SYS_TEXT_BASE` | `0x81700000` | 主 U-Boot 运行地址 |
| `CONFIG_SPL` | `y` | 启用 SPL |
| `CONFIG_DRAM_CLK` | `204` | DRAM 时钟配置 |
| `CONFIG_SYS_CLK_FREQ` | `604000000` | CPU/系统目标时钟 |
| `CONFIG_SPL_SPI_SUNXI` | `y` | SPL 从 SUNXI SPI 启动 |
| `CONFIG_BOOTDELAY` | `0` | 不显示常规倒计时 |
| `CONFIG_BOOTCOMMAND` | `run distro_bootcmd;` | 默认启动命令 |
| `CONFIG_MTD_SPI_NAND` | `y` | SPI-NAND 支持 |
| `CONFIG_CMD_DFU` | `y` | DFU 命令 |
| `CONFIG_DFU_MTD` | `y` | 通过 DFU 操作 MTD |
| `CONFIG_NET` | 关闭 | U-Boot 不启用网络栈 |

## U-Boot 补丁

Buildroot 按顺序应用两组补丁：

```text
board/allwinner/suniv-f1c100s/patch/u-boot
board/cra/epass/patch/uboot
```

### SUNIV 基础补丁

```text
board/allwinner/suniv-f1c100s/patch/u-boot/0001-v2020.07.11.patch
```

该补丁为 U-Boot 2020.07 增加或扩展：

- ARM926EJ-S SUNXI 启动代码。
- SUNIV SPL 链接脚本。
- F1C100S/F1C200S 时钟支持。
- SUNIV DRAM 初始化。
- GPIO、串口、MMC、SPI 和 USB PHY 支持。
- SPI-NAND SPL 启动。
- SUNIV CCU 时钟和复位绑定。
- 部分 GigaDevice SPI-NAND 支持。
- SUNIV 内存地址布局。

### CRA 板级补丁

| 补丁 | 作用 |
| --- | --- |
| `0001-uart-pull.patch` | 为 UART0 的 PE0、PE1 启用上拉 |
| `0002-musb-force-fs.patch` | 禁用 MUSB High-Speed，使 U-Boot USB 强制工作于 Full-Speed |
| `0003-spi-nand-mx35lf1g.patch` | 增加两种 Macronix SPI-NAND ID |
| `0004-f1c-spi-fix.patch` | 修正 SUNIV SPI 父时钟、分频和最大频率处理 |
| `0005-dfu-verify-block.patch` | 为 MTD DFU 增加坏块跳过和写后校验 |

## SPL 与 U-Boot 镜像

启动分为两级：

```text
Allwinner BROM
        │
        ▼
SPL
├─ 初始化时钟和 DRAM
├─ 识别启动介质
└─ 从 SPI-NAND 读取主 U-Boot
        │
        ▼
主 U-Boot
├─ 初始化驱动模型
├─ 读取启动环境
├─ 读取 boot.itb
├─ 组合 Linux 设备树
└─ 启动 Linux
```

SUNIV 基础补丁将 SPI-NAND 中主 U-Boot 的读取位置设为：

```text
CONFIG_SYS_SPI_NAND_U_BOOT_OFFS = 0xD000
```

也就是距 SPI-NAND 起点 52 KiB。

Buildroot 首先产生：

```text
output/images/u-boot-sunxi-with-spl.bin
```

随后 `mknanduboot.sh` 根据 2 KiB NAND 页布局重新排列 SPL，并把主 U-Boot 放到 `0xD000`，生成：

```text
output/images/u-boot-sunxi-with-nand-spl.bin
```

实体设备烧录使用的是带 `nand-spl` 后缀的最终文件，而不是未经 NAND 布局处理的原始 `with-spl` 文件。

## 默认启动环境

U-Boot 默认环境来自：

```text
board/cra/epass/uboot.env
```

因为 `uboot.defconfig` 启用：

```text
CONFIG_USE_DEFAULT_ENV_FILE=y
CONFIG_DEFAULT_ENV_FILE="../../../board/cra/epass/uboot.env"
```

### 内存地址

| 变量 | 地址 | 用途 |
| --- | ---: | --- |
| `kernaddr` | `0x80008000` | Linux zImage |
| `dtbaddr` | `0x80C00000` | Linux 基础 DTB |
| `dtboaddr` | `0x80D00000` | 临时 DTBO |
| `envtxtaddr` | `0x80E00000` | 文本启动环境 |
| `fitaddr` | `0x81000000` | `boot.itb` |

`checkfit` 将 FIT 总大小限制为不超过：

```text
0x500000
```

也就是 5 MiB。该限制可避免 FIT 数据向上覆盖位于 `0x81700000` 的主 U-Boot。

### SPI-NAND 布局

当前启动逻辑使用：

| 区域 | 偏移 | 大小 |
| --- | ---: | ---: |
| U-Boot 分区 | `0x000000` | 1 MiB |
| 文本启动环境 | `0x0FA000` | 24 KiB |
| Boot 分区 | `0x100000` | 6 MiB |
| Rootfs 分区 | `0x700000` | 剩余空间 |

文本启动环境位于 U-Boot 分区末尾：

```text
0xFA000 + 0x6000 = 0x100000
```

刚好到达 Boot 分区起点。

### Linux 启动流程

默认 `distro_bootcmd` 执行：

```text
读取 0xFA000 的文本环境
        │
        ▼
读取并检查 0x100000 的 boot.itb
        │
        ▼
提取 kernel
        │
        ▼
提取 Linux 基础 DTB
        │
        ▼
提取并应用 fdt-screen-${screen}
        │
        ▼
依次应用 fdt-iface-${interface}
        │
        ▼
依次应用 fdt-ext-${ext}
        │
        ▼
补充默认 bootargs
        │
        ▼
bootz
```

如果 FIT 头无效、FIT 大小为零或读取后无法通过 FDT 检查，启动环境会进入 `rundfu`。

## 编译

首次配置：

```sh
make cra_epass_defconfig
```

项目提供：

```sh
./rebuild-uboot.sh
```

所有由 Linux/Buildroot 执行的 `.sh` 文件必须保持 LF 换行。若 Windows 检出将它们转换为 CRLF，Bash 会出现 `$'\r': command not found`、无效 `set` 参数或循环语法错误。

脚本执行：

```sh
make uboot-clean-for-rebuild
make uboot -j8
make -j8
```

最后一次 `make` 会运行镜像后处理脚本，生成适用于 SPI-NAND 的最终 U-Boot 文件及其他系统镜像。

完整构建结束后，重点检查：

```text
output/images/u-boot-sunxi-with-spl.bin
output/images/u-boot-sunxi-with-nand-spl.bin
output/images/boot.itb
output/images/rootfs_ubi.img
```

只看到 `u-boot-sunxi-with-spl.bin` 并不表示 NAND 启动镜像已经完成，仍需确认 `mknanduboot.sh` 成功执行。

## 常见问题

| 现象 | 优先检查 |
| --- | --- |
| U-Boot 没有串口输出 | UART0 PE0/PE1、115200n8、UART 上拉补丁和供电 |
| SPL 启动后找不到主 U-Boot | SPI-NAND 型号、芯片 ID、`0xD000` 布局和 `mknanduboot.sh` 输出 |
| U-Boot 找不到 SPI-NAND | SPI0 引脚、`spi-nand` 节点、SPI-NAND 驱动和芯片 ID 补丁 |
| DTC 报 SPI-NAND 单元地址不匹配 | 确认节点名和 `reg` 都是 `0`，即 `spi-nand@0` 与 `reg = <0>` |
| 重新启用第二组 MMC 后 SPI 启动异常 | `mmc2` 与 SPI0 共同占用 PC0～PC2；当前板级配置必须保持 `mmc2` 关闭 |
| USB DFU 无法枚举 | `usb_otg`、`usbphy`、`otg_sram`、Full-Speed 补丁和主机驱动 |
| DFU 写入时跳块 | 查看是否发生写入失败、校验失败或已有 NAND 坏块 |
| `boot.itb` 无效后反复进入 DFU | FIT 头、总大小、Boot 分区内容和 `checkfit` 限制 |
| 修改 U-Boot DTS 后没有生效 | 是否清理并重建 U-Boot，Buildroot 是否重新复制自定义 DTS |
| 修改 Linux DTS 后 U-Boot 不变 | 两套设备树相互独立，必须修改对应的 U-Boot DTS |
| U-Boot 没有显示开机图 | 当前关闭 `CONFIG_VIDEO_SUNXI`，显示由 Linux 阶段接管 |

## 二次开发原则

修改本目录和相关 U-Boot 配置前，应先确认启动介质、PCB 引脚和可恢复手段：

- 严禁在没有 FEL/XFEL 恢复条件时测试未知的 SPL 或 U-Boot 镜像。
- 严禁把普通 `u-boot-sunxi-with-spl.bin` 直接当作当前 SPI-NAND 最终布局文件。
- 严禁让 SPI0 与第二组 MMC 同时占用 PC0～PC2。
- 只改变品牌显示时，优先修改 `model`，保留 Allwinner 兼容字符串。
- 修改串口节点时，应同步检查 `stdout-path`、控制台索引、引脚复用和 UART 补丁。
- 修改 SPI 频率时，应同时核对闪存规格、PCB 信号质量、SPL 读取和主 U-Boot SPI 驱动。
- 修改 SPI-NAND 型号时，应确认 SPL 和主 U-Boot 两阶段都能识别该芯片。
- 调整 `0xD000` 主 U-Boot 偏移时，必须同步修改 SUNIV SPL 配置和 `mknanduboot.sh`。
- 修改 SPI-NAND 分区时，必须同步检查 U-Boot 默认环境、Linux `bootargs`、Linux 设备树、UBI 配置、镜像脚本和烧录工具。
- 修改 `0xFA000` 文本环境位置或 `0x6000` 长度时，必须确保它不会覆盖 SPL、主 U-Boot 或 Boot 分区。
- 若需恢复 SPI-NOR 支持，应建立独立硬件配置，不能与 SPI-NAND 共用片选 0。
- 若需重新启用 `mmc2`，应先解决它与 SPI0 在 PC0～PC2 上的物理冲突。
- 修改 USB 模式前，应同时验证 U-Boot DFU、Linux USB Gadget 及实体设备的 USB 信号质量。
- 修改 U-Boot 补丁后，应执行完整清理构建，不能只依赖旧的 `output/build/uboot-*` 目录。
- 实机测试前应保留已验证的 U-Boot 镜像、串口连接和 XFEL 恢复方式。
