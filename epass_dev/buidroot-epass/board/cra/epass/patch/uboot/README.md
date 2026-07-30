# CRA Electric Pass U-Boot 补丁

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 CRA Electric Pass 针对 U-Boot 2020.07 增加的板级补丁。它们主要处理启动串口、USB DFU、SPI-NAND 识别、SUNIV SPI 时钟和 NAND 写后校验。

这些补丁同时影响 SPL 和主 U-Boot。SPL 负责完成最早期的 DRAM、SPI 和 NAND 启动；主 U-Boot 负责加载启动环境、读取 FIT 镜像、组合 Linux 设备树，并在启动失败或用户请求时提供 DFU。

## 构建位置

补丁目录由板级 Buildroot 配置指定：

```make
BR2_TARGET_UBOOT_CUSTOM_VERSION_VALUE="2020.07"
BR2_TARGET_UBOOT_PATCH="board/allwinner/suniv-f1c100s/patch/u-boot board/cra/epass/patch/uboot"
BR2_TARGET_UBOOT_CUSTOM_CONFIG_FILE="board/cra/epass/uboot.defconfig"
```

实际构建关系为：

```text
官方 U-Boot 2020.07 源码
        │
        ▼
board/allwinner/suniv-f1c100s/patch/u-boot
        │
        ▼
board/cra/epass/patch/uboot
        │
        ▼
board/cra/epass/uboot.defconfig
        │
        ▼
编译 SPL 与主 U-Boot
        │
        ▼
board/cra/epass/scripts/mknanduboot.sh
        │
        ▼
u-boot-sunxi-with-nand-spl.bin
```

## 文件概览

```text
patch/uboot/
├─ 0001-uart-pull.patch
├─ 0002-musb-force-fs.patch
├─ 0003-spi-nand-mx35lf1g.patch
├─ 0004-f1c-spi-fix.patch
├─ 0005-dfu-verify-block.patch
└─ README.md
```

| 编号 | 主要功能 | 影响阶段 |
| --- | --- | --- |
| `0001` | 为 UART0 TX/RX 配置上拉 | SPL、U-Boot 早期串口 |
| `0002` | 强制 U-Boot MUSB Gadget 使用 Full-Speed | U-Boot USB、DFU |
| `0003` | 在 SPL 中识别两种 Macronix SPI-NAND | SPL 从 NAND 启动 |
| `0004` | 修正 SUNIV SPI 父时钟和分频计算 | SPL、主 U-Boot SPI |
| `0005` | 为 MTD DFU 增加写后校验和坏块处理 | 主 U-Boot DFU |

## 补丁详细说明

### 0001：UART0 引脚上拉

目标文件：

```text
arch/arm/mach-sunxi/board.c
```

SUNIV 的 UART0 使用：

```text
PE0：UART0 TX
PE1：UART0 RX
```

原代码已经给 PE1 配置上拉，本补丁进一步为 PE0 配置：

```c
sunxi_gpio_set_pull(SUNXI_GPE(0), SUNXI_GPIO_PULL_UP);
```

这样可以在启动早期、串口驱动尚未完全稳定或外部串口设备未连接时，为 TX 和 RX 提供一致的默认电平。

该补丁只改变引脚上下拉，不改变：

- UART 波特率；
- 串口控制器编号；
- `console=ttyS0,115200` 内核参数；
- Linux 启动后的 UART 驱动。

### 0002：U-Boot MUSB 强制 Full-Speed

目标文件：

```text
drivers/usb/musb-new/musb_core.c
drivers/usb/musb-new/musb_gadget.c
```

本补丁在 MUSB 启动时不再设置：

```text
MUSB_POWER_HSENAB
```

并在 USB Gadget 唤醒和恢复流程中再次清除该位，从而防止控制器重新进入 High-Speed。

它影响的是 U-Boot 阶段的 USB Gadget，包括：

- DFU；
- U-Boot USB 下载；
- U-Boot 中依赖 MUSB Gadget 的其他功能。

它不控制 Linux 启动后的 USB 速度。Linux 阶段由 Linux MUSB 驱动及 Linux 补丁中的 `cra,usb-hs-enabled` 属性决定。

当前补丁只修改 U-Boot 2020.07 实际使用的：

```text
drivers/usb/musb-new/
```

早期版本曾包含一个已经不存在的旧 MUSB 路径，该无效部分已经删除。

### 0003：Macronix SPI-NAND SPL 识别

目标文件：

```text
arch/arm/mach-sunxi/spl_spi_sunxi.c
```

SPL 在 DRAM 初始化后，需要先判断 SPI0 上连接的是 SPI-NOR 还是 SPI-NAND，才能采用正确方式加载主 U-Boot。

本补丁增加两个 Macronix 芯片 ID：

| 制造商和设备 ID | 型号 |
| --- | --- |
| `c2 12` | MX35LF1GE4AB |
| `c2 14` | MX35LF1G24AD |

匹配成功后，SPL 将设备类型设置为：

```c
FLASHTYPE_NAND
```

该补丁只负责 SPL 阶段的闪存类型识别。主 U-Boot 中完整的 SPI-NAND 读写、坏块和 MTD 支持还依赖：

- 公共 SUNIV U-Boot 补丁；
- `CONFIG_MTD_SPI_NAND=y`；
- SPI0 设备树节点；
- 对应厂商的 SPI-NAND 驱动。

### 0004：SUNIV SPI 时钟修正

目标文件：

```text
drivers/spi/spi-sunxi.c
```

原 U-Boot 驱动使用同一个 24 MHz 常量表示 SPI 父时钟和最大速率，并在设备树没有设置频率时默认使用 1 MHz。该逻辑不符合 SUNIV/F1C100S/F1C200S 的实际时钟结构。

本补丁为不同 SoC 变体增加：

```c
u32 mod_clk_hz;
u32 max_speed_hz;
```

对于 SUNIV，当前假设为：

| 参数 | 数值 |
| --- | --- |
| SPI 分频器父时钟 | 200 MHz |
| SPI 总线最大频率 | 100 MHz |
| 当前 SPI-NAND 设备树上限 | 80 MHz |

200 MHz 来自 SPL 配置的：

```text
PLL_PERIPH / 3
```

驱动使用向上取整方式计算 CDR1/CDR2 分频值，使实际 SPI 时钟不超过请求值。设备树未指定频率时，则使用对应 SoC 变体的最大值，而不是旧代码中固定的 1 MHz。

当前 U-Boot 设备树使用：

```dts
spi-max-frequency = <80000000>;
```

因此该补丁直接关系到 SPI-NAND 的启动读取速度和稳定性。

如果以后修改 SPL 时钟、AHB 时钟或 `PLL_PERIPH`，必须重新核对这里硬编码的 200 MHz 父时钟。父时钟假设错误会使实际 SPI 时钟偏离请求值，严重时可能导致 SPL 偶发读取失败。

### 0005：DFU 写后校验和坏块处理

目标文件：

```text
drivers/dfu/dfu_mtd.c
```

原 U-Boot MTD DFU 写入流程在写操作返回成功后不会立即读取并比较数据。对于 SPI-NAND，这可能使部分写入异常直到下次启动或读取时才被发现。

本补丁增加以下逻辑：

1. NAND 写入时申请一个擦除块大小的校验缓冲区。
2. 将写入长度限制在当前擦除块范围内。
3. 每次写入完成后立即通过 MTD 读回。
4. 比较读回长度和实际数据。
5. 若写入或校验失败，将当前块标记为坏块。
6. 跳过已知坏块并寻找后续可用块。
7. 擦除替代块后重新尝试当前数据。
8. 可用空间耗尽时返回 `-ENOSPC`。

该补丁可以提高 DFU 写入 SPI-NAND 时的可靠性，但也带来两个需要注意的行为：

- 一次写入或回读失败就可能永久标记当前块为坏块；
- 校验缓冲区大小等于 NAND 擦除块大小，会占用额外的 U-Boot 堆内存。

如果故障来自电源不稳定、USB 中断、SPI 时钟过高或暂时性信号问题，而不是 NAND 介质永久损坏，直接标记坏块可能过于激进。因此进行 DFU 时应保证供电、USB 连接和 SPI 时序稳定。

## 启动链路中的作用

```text
设备上电
   │
   ▼
SPL 初始化 DRAM、UART0 和 SPI0
   │        │
   │        ├─ 0001 稳定 UART0 引脚电平
   │        └─ 0004 正确计算 SUNIV SPI 时钟
   │
   ▼
SPL 读取 SPI 芯片 ID
   │
   └─ 0003 识别 Macronix MX35LF1G 系列为 SPI-NAND
   │
   ▼
从 NAND 中加载主 U-Boot
   │
   ▼
主 U-Boot 读取环境和 boot.itb
   │
   ├─ 正常：启动 Linux
   │
   └─ 失败或用户请求：进入 DFU
             │
             ├─ 0002 强制 USB Full-Speed
             └─ 0005 写后校验、跳过坏块
```

## 与配置、设备树和脚本的关系

### U-Boot 配置

相关配置位于：

```text
board/cra/epass/uboot.defconfig
```

主要选项包括：

```text
CONFIG_SPL=y
CONFIG_SPL_SPI_SUNXI=y
CONFIG_CMD_DFU=y
CONFIG_DFU_MTD=y
CONFIG_MTD=y
CONFIG_DM_MTD=y
CONFIG_MTD_SPI_NAND=y
CONFIG_SPI=y
CONFIG_DM_SPI=y
CONFIG_USB_MUSB_GADGET=y
CONFIG_USB_GADGET_DOWNLOAD=y
```

如果关闭相应配置项，补丁虽然仍可能成功应用，但对应代码不会被编译或运行。

### U-Boot 设备树

相关设备树位于：

```text
board/cra/epass/devicetree/uboot/
```

其中 SPI0 和 `spi-nand@0` 节点负责声明：

- SPI0 引脚；
- SPI-NAND 片选；
- 80 MHz 最大频率；
- NAND 设备状态。

### 分区名称

当前 U-Boot 配置使用：

```text
spi-nand0=cranand
mtdparts=cranand:1M(u-boot)ro,6M(boot),-(rootfs)
```

补丁 `0005` 按 MTD 和 DFU 提供的分区范围执行写入、校验和坏块跳过。修改分区布局时，需要同步检查：

```text
board/cra/epass/uboot.defconfig
board/cra/epass/uboot.env
board/cra/epass/scripts/
Linux bootargs
```

### NAND 启动镜像后处理

Buildroot 首先生成：

```text
u-boot-sunxi-with-spl.bin
```

随后：

```text
board/cra/epass/scripts/mknanduboot.sh
```

按照 2 KiB NAND 页布局重新排列 SPL，并将主 U-Boot 放置到 `0xD000`，最终生成：

```text
output/images/u-boot-sunxi-with-nand-spl.bin
```

本目录补丁不负责执行该二进制重排。补丁编译成功也不代表 NAND 启动镜像后处理一定成功，必须单独检查最终输出文件。

## 修改补丁后的构建方法

在 Buildroot 根目录、Linux/WSL 环境中执行：

```bash
make cra_epass_defconfig
make uboot-dirclean
make uboot
```

修改补丁后应使用 `uboot-dirclean`，使 Buildroot 重新解压 U-Boot 2020.07 并从头应用公共补丁和 CRA 补丁。

只执行：

```bash
make uboot-rebuild
```

通常不会重新执行已经完成的 patch 阶段，可能继续编译旧的 `output/build/uboot-2020.07/` 源码。

若需要生成最终镜像，可继续执行：

```bash
make
```

构建命令不会自动写入实体设备。烧录 `u-boot-sunxi-with-nand-spl.bin` 会修改设备最早期启动区域，风险明显高于替换主程序，必须作为单独操作确认。

## 二次开发原则

1. 请勿将 `output/build/uboot-2020.07/` 当作永久源码目录，`uboot-dirclean` 会删除其中的修改。
2. 永久修改应记录在本目录补丁中，或在升级 U-Boot 时整理为可追踪提交。
3. 新补丁应使用四位数字编号，并明确它依赖 SPL、主 U-Boot、公共 SUNIV 补丁还是其他 CRA 补丁。
4. 所有补丁统一使用 LF 行尾，避免 Windows CRLF 导致补丁上下文匹配失败。
5. 修改 SPI 父时钟或最大频率时，应同时验证 SPL 启动读取和主 U-Boot MTD 读写。
6. 新增 SPI-NAND ID 前，应核对制造商 ID、设备 ID、页大小、擦除块大小、OOB 布局和 ECC 要求。
7. 修改 DFU 坏块策略时，要区分永久介质故障和暂时性通信故障。
8. 修改分区布局时，应同步检查 U-Boot 配置、环境、镜像脚本和 Linux 启动参数。
9. 不要删除原作者署名或硬件厂商名称；CRA 的修改应通过新增说明和 Git 历史表达。
10. 补丁能够应用、代码能够编译、镜像能够启动和 DFU 能够可靠写入，是四个不同的验证阶段。

## 当前验证状态

当前目录已经完成以下检查：

- 五个补丁均可以作为统一 diff 正确解析；
- 所有 CRA Linux 和 U-Boot 补丁均已统一为 LF；
- 公共 SUNIV U-Boot 补丁和本目录五个补丁可以按照 Buildroot 顺序应用；
- 修正后的 `0002-musb-force-fs.patch` 只修改 U-Boot 2020.07 中实际存在的 `drivers/usb/musb-new/`；
- 当前 U-Boot 配置可以识别 CRA 品牌、SPI-NAND、DFU、MTD、SPL 和 MUSB 相关选项；
- 当前五个补丁已经完成过 U-Boot 编译验证。

这些检查不能替代实体设备启动、串口输出、SPI-NAND 长时间读写和 DFU 异常恢复测试。

