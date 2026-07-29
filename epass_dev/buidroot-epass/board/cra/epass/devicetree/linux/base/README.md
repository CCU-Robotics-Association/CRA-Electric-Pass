# CRA Electric Pass 基础设备树

其他语言版本: [English](README_EN.md), [中文](README.md).

本目录保存 CRA Electric Pass 的 Linux 基础设备树。目前工程仅面向白银 v0.6 板型。

## 文件说明

| 文件 | 作用 |
| --- | --- |
| `epass.dtsi` | 电子通行证公共硬件描述，包括设备型号、LCD 显示链路、电源、背光、GPIO 复用、SPI-NAND、串口、SD、USB、视频引擎和基础外设状态。 |
| `devicetree.dts` | 当前实体板的最终基础设备树入口，引用 `epass.dtsi`，并补充关机 GPIO、ST7701 初始化引脚和 LRADC 按键参数。 |

生成的基础设备树文件为：

```text
output/images/dt/base/devicetree.dtb
```

## 组合关系

设备树按照以下顺序组成：

```text
Linux suniv-f1c100s.dtsi
        ↓
base/epass.dtsi
        ↓
base/devicetree.dts
        ↓
screen、interface、ext overlay
        ↓
U-Boot 应用 overlay 后启动 Linux
```

`suniv-f1c100s.dtsi` 提供 F1C100S/F1C200S SoC 内部控制器定义。

## `epass.dtsi`

### 设备身份

```dts
model = "CRA Electric Pass";
compatible = "cra,electric-pass",
             "allwinner,suniv-f1c200s",
             "allwinner,suniv-f1c100s";
```

`model` 为用户可读的设备型号；`compatible` 按照从专用到通用的顺序供内核匹配硬件。

### 启动参数

`chosen/bootargs` 仅作占位值。实际内核命令行由 U-Boot 传入，其中包含串口控制台、UBI/UBIFS 根文件系统和 NAND 分区参数。

### 显示系统

基础显示链路为：

```text
DE/FE/BE → TCON0 → RGB565 → LCD panel
```

面板使用 `lattland,mostima` 匹配项目中的定制 `panel-simple` 内核补丁。当前底层时序为 384×640、60Hz，应用可视界面应按照约 360×640 进行设计。

`st7701initseq` 在基础设备树中默认关闭。启动时选择的屏幕 overlay 会启用它并提供 BOE、HSD 或 Laowu 面板对应的初始化序列。

### 电源与背光

- `vcc3v3`：3.3V 固定电源，供 SD 等外设使用。
- `lradc_vref`：3.0V LRADC 参考电压。
- `pwm-backlight`：使用 PWM0 控制背光。
- PWM 周期：10000ns，约 100kHz。
- 默认亮度索引：6，对应亮度值 128。

### GPIO 复用

本文件定义以下可复用引脚组：

- `spi1_pins`：PE7、PE8、PE9、PE10。
- `rtp_pins_0`：PA0。
- `rtp_pins_01`：PA0、PA1。
- `lcd_rgb565_no_de_pins`：LCD RGB565 数据、时钟和同步信号。
- `i2s_pins_pe`、`i2s_pins_pa`：两种 I²S 引脚布局。

配置务必与 PCB 走线保持一致。

### SPI-NAND 分区

板载 SPI-NAND 按 128MiB 布局：

| 分区 | 起始地址 | 大小 | 用途 |
| --- | ---: | ---: | --- |
| `u-boot` | `0x000000` | 1MiB | SPL、U-Boot 和启动环境。 |
| `boot` | `0x100000` | 6MiB | Linux 内核、基础设备树和 overlay。 |
| `rootfs` | `0x700000` | 121MiB | UBIFS 根文件系统、程序和资源。 |

修改分区时，必须同步检查设备树、U-Boot 命令行、镜像生成脚本和烧录地址。

### 默认外设状态

| 外设 | 默认状态 | 说明 |
| --- | --- | --- |
| PWM0 | 启用 | 控制屏幕背光。 |
| SPI0 | 启用 | 连接板载 SPI-NAND。 |
| SPI1 | 关闭 | 需要时由 interface overlay 启用。 |
| UART0 | 启用 | 系统调试串口。 |
| UART1/UART2 | 关闭 | 需要时由 interface overlay 启用。 |
| MMC0 | 启用 | 4-bit SD/MMC，3.3V。 |
| USB OTG/PHY | 启用 | 支持 USB Device、RNDIS 及可选 Host 模式。 |
| Cedar/ION/DE/FE/BE | 启用 | 视频解码、显示内存和显示引擎。 |
| TVE0 | 关闭 | 不使用模拟电视输出。 |
| LRADC | 启用 | 读取电阻按键。 |
| I²C0 | 关闭 | 需要时由 interface 或 ext overlay 启用。 |

## `devicetree.dts`

### 关机控制

```dts
gpios = <&pio 4 2 GPIO_ACTIVE_HIGH>;
timeout-ms = <3000>;
```

PE2 高电平触发硬件断电，触发后等待时间为 3000ms。

注意，此引脚配置错误会导致 Linux 完成关机流程后设备仍不断电。

### ST7701 初始化接口

| 信号 | GPIO |
| --- | --- |
| SDA | PE4 |
| SCL | PD19 |
| CS | PE11 |

这些引脚只负责向 ST7701 写入初始化命令；LCD 像素数据通过 RGB565 总线传输。

### LRADC 按键

五个按键共用 LRADC 通道 0，通过不同电阻产生不同目标电压：

| Linux 按键码 | 目标电压 |
| --- | ---: |
| `KEY_0` | 0V |
| `KEY_1` | 1.396826V |
| `KEY_2` | 1.111111V |
| `KEY_3` | 0.825396V |
| `KEY_4` | 0.444444V |

`voltage` 的单位为微伏。

注意，请勿擅自修改数值，可能导致按键无响应、错键或临界电压抖动。

## Overlay

基础设备树只定义整机骨架，以下功能仍通过 overlay 选择：

- `screen/`：BOE、HSD、Laowu 屏幕初始化。
- `interface/`：ADC、I²C、I²S、SPI、UART、USB 等接口。
- `ext/`：CardKB、ES8311、LSM6DS3 等扩展设备。

## 未来及二次开发建议

低风险修改：

- 背光亮度表和默认亮度。
- 按键 `label`。

修改后需要同步检查程序或其他配置：

- Linux 按键码。
- 串口、SPI、I²C 和 I²S 的启用状态。
- USB 工作模式。
- screen/interface/ext overlay。

高风险配置：

- 关机 GPIO。
- ST7701 初始化 GPIO。
- LCD RGB 引脚和面板 `compatible`。
- LRADC 按键电压。
- SPI-NAND 分区。
- 电压和时钟参数。

`lattland,mostima` 和 `lattland,st7701-initseq` 为内核驱动匹配字符串，并非界面显示文字。若需重命名，必须同步修改相应 Linux 内核补丁。

## 注释与格式

推荐使用 C 风格注释：

```dts
/* 单行说明 */

/*
 * 多行说明。
 * 重点解释硬件原因、单位和修改风险。
 */
```

文件应保持 UTF-8 编码。

## 构建与验证

在 WSL Buildroot 根目录运行：

```sh
make cra_epass_defconfig
make -j$(nproc)
```

验证生成的设备型号：

```sh
output/host/bin/fdtget \
    output/images/dt/base/devicetree.dtb \
    / model
```

预期输出：

```text
CRA Electric Pass
```

检查 FIT 镜像内容：

```sh
output/host/bin/dumpimage -l output/images/boot.itb
```

基础设备树应以 `fdt-base` 出现。构建和验证不会自动写入实体设备。
