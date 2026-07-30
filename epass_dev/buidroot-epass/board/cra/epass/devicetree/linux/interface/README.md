# CRA Electric Pass 硬件接口覆盖层

其他语言版本: [English](README_EN.md), [中文](README.md).

本目录保存 CRA Electric Pass 的 Linux 硬件接口设备树覆盖层，用于按需启用 F1C200S 内部控制器、选择对应的 GPIO 复用功能，以及切换 USB 工作模式。

这里的 `interface` 负责提供总线和接口，`ext` 负责声明连接在这些接口上的具体外部设备。当前工程仅面向白银 v0.6 板型。

## 文件说明

| 文件 | 目标控制器 | 作用 | 使用引脚 |
| --- | --- | --- | --- |
| `adc_pa1.dts` | RTP/GPADC | 在默认 PA0 基础上增加 PA1 ADC 引脚 | PA0、PA1 |
| `adc_pa123.dts` | RTP/GPADC | 启用全部四个 ADC 引脚 | PA0、PA1、PA2、PA3 |
| `i2c0.dts` | I²C0 | 启用硬件 I²C0 控制器 | PD0、PD12 |
| `i2s0_pa.dts` | I²S0 | 启用 PA 引脚布局的 I²S0 | PE3、PA1、PA2、PA3 |
| `i2s0_pe.dts` | I²S0 | 启用 PE 引脚布局的 I²S0 | PE3、PE5、PE6、PA1 |
| `spi1.dts` | SPI1 | 启用 SPI1 和预设的 Spidev 子设备 | PE7、PE8、PE9、PE10 |
| `uart1.dts` | UART1 | 启用 UART1 TX/RX | PA2、PA3 |
| `uart2.dts` | UART2 | 启用 UART2 TX/RX | PE7、PE8 |
| `usbhost.dts` | USB OTG | 将 USB 控制器固定为主机模式 | USB 专用引脚 |
| `usbhs.dts` | USB OTG | 请求启用项目自定义的 USB High-Speed 模式 | USB 专用引脚 |

## 与其他设备树目录的关系

Linux 设备树按以下顺序组成：

```text
Linux suniv-f1c100s.dtsi
        ↓
base/epass.dtsi
        ↓
base/devicetree.dts
        ↓
screen overlay
        ↓
interface overlay
        ↓
ext overlay
        ↓
U-Boot 启动 Linux
```

各目录职责如下：

- `base/`：描述主板上始终存在的基础硬件，并为可选控制器预先设置引脚组和默认状态。
- `screen/`：选择实际安装的 LCD 面板及显示时序。
- `interface/`：启用 SoC 控制器、选择引脚复用或切换控制器工作模式。
- `ext/`：声明连接在已启用总线上的 CardKB、ES8311、LSM6DS3 等具体设备。

例如：

```text
interface=i2c0
ext=cardkb
```

`i2c0` 负责启用硬件 I²C0，`cardkb` 负责在该总线上声明地址为 `0x5f` 的 CardKB。

## 覆盖层基本结构

本目录文件均为 Device Tree Overlay：

```dts
/dts-v1/;
/plugin/;

/ {
    fragment@1 {
        target = <&some_controller>;
        __overlay__ {
            status = "okay";
        };
    };
};
```

主要字段含义：

- `/dts-v1/;`：声明设备树源码格式。
- `/plugin/;`：声明该文件是附加到基础设备树上的覆盖层。
- `fragment@1`：一个覆盖片段，编号只用于区分不同片段。
- `target`：通过标签指定要修改的基础设备树节点。
- `__overlay__`：需要新增或覆盖的属性。
- `status = "okay"`：启用基础设备树中默认关闭的控制器。
- `pinctrl-0`：选择该控制器使用的默认引脚组。
- `pinctrl-names = "default"`：将 `pinctrl-0` 声明为默认引脚状态。

## `adc_pa1.dts`

### 功能

```dts
fragment@1 {
    target = <&rtp>;
    __overlay__ {
        pinctrl-0 = <&rtp_pins_01>;
    };
};
```

F1C200S 的 RTP 控制器原本用于电阻触摸屏，同时也提供 GPADC 功能。

基础设备树默认配置为：

```dts
&rtp {
    status = "okay";
    pinctrl-0 = <&rtp_pins_0>;
    pinctrl-names = "default";
};
```

默认引脚组是：

```dts
rtp_pins_0: rtp-pins-0 {
    pins = "PA0";
    function = "rtp";
};
```

因此不启用任何 ADC 接口覆盖层时，PA0 已经处于 RTP/ADC 功能。

`adc_pa1.dts` 将引脚组替换为：

```dts
rtp_pins_01: rtp-pins-01 {
    pins = "PA0", "PA1";
    function = "rtp";
};
```

它的实际含义是：

```text
保留 PA0，并额外启用 PA1
```

文件名中的 `pa1` 指新增开放的 PA1，而不是只使用 PA1。

### Linux 驱动

相关内核配置为：

```text
CONFIG_MFD_SUN4I_GPADC=y
CONFIG_SUN4I_GPADC=y
```

项目还包含：

```text
board/cra/epass/patch/linux/0000-f1c100s-gpadc-regs.patch
board/cra/epass/patch/linux/0005-gpadc-low-freq.patch
```

前者调整 F1C100S/F1C200S 的 GPADC 寄存器位定义，后者调整 ADC 采样频率和滤波设置。

### 冲突

PA1 同时被两种 I²S0 引脚布局使用：

```text
adc_pa1 + i2s0_pa = PA1 冲突
adc_pa1 + i2s0_pe = PA1 冲突
```

UART1 使用 PA2、PA3，因此 `adc_pa1` 与 `uart1` 没有直接引脚冲突。

## `adc_pa123.dts`

### 功能

```dts
fragment@1 {
    target = <&rtp>;
    __overlay__ {
        pinctrl-0 = <&rtp_pins>;
    };
};
```

该文件使用 SoC 公共设备树中的完整 RTP 引脚组：

```dts
rtp_pins: rtp-pins {
    pins = "PA0", "PA1", "PA2", "PA3";
    function = "rtp";
};
```

最终启用：

```text
PA0、PA1、PA2、PA3
```

文件名 `adc_pa123` 表示在默认 PA0 的基础上增加 PA1、PA2、PA3。

### 冲突

该覆盖层占用整个 PA0～PA3，因此冲突范围较大：

```text
adc_pa123 + i2s0_pa = PA1、PA2、PA3 冲突
adc_pa123 + i2s0_pe = PA1 冲突
adc_pa123 + uart1   = PA2、PA3 冲突
```

`adc_pa1` 与 `adc_pa123` 也不应同时使用。它们都修改 `&rtp` 的 `pinctrl-0`，后应用的覆盖层会覆盖前一个，并不会合并引脚组。

## `i2c0.dts`

### 功能

```dts
fragment@1 {
    target = <&i2c0>;
    __overlay__ {
        status = "okay";
    };
};
```

基础设备树已经配置：

```dts
&i2c0 {
    pinctrl-names = "default";
    pinctrl-0 = <&i2c0_pd_pins>;
    status = "disabled";
};
```

对应引脚为：

```text
PD0  = SDA
PD12 = SCL
```

RGB LCD 引脚组没有占用 PD0、PD12，因此硬件 I²C0 可以在屏幕工作时使用。

### Linux 驱动

相关内核配置为：

```text
CONFIG_I2C_CHARDEV=y
CONFIG_I2C_MV64XXX=y
```

启用后，Linux 通常会生成：

```text
/dev/i2c-0
```

实际编号应以实体设备的 `/dev/i2c-*` 和启动日志为准。

`i2c0.dts` 只启用控制器，不会声明总线上的具体芯片。连接外设还需要相应 `ext` 覆盖层。

### 电气要求

I²C 使用开漏信号，需要 SDA、SCL 上拉电阻。当前 `i2c0_pd_pins` 没有声明内部上拉，因此需要 PCB 或外接模块提供合适的上拉。

### 冲突

`ext/es8311_sound.dts` 使用相同的 PD0、PD12 建立 GPIO 模拟 I²C。

当前写法下不得同时配置：

```text
interface=i2c0
ext=es8311_sound
```

否则两个 Linux I²C 控制器会争用同一组物理引脚。

## `i2s0_pa.dts`

### 功能

```dts
fragment@1 {
    target = <&i2s0>;
    __overlay__ {
        status = "okay";
        pinctrl-0 = <&i2s_pins_pa>;
        pinctrl-names = "default";
    };
};
```

该覆盖层：

1. 启用 F1C200S 的 I²S0 控制器。
2. 选择 PA 布局的 I²S0 引脚组。

使用引脚：

```text
PE3、PA2、PA3、PA1
```

I²S0 只负责传输数字音频数据。单独启用它不会自动产生完整 ALSA 声卡，还需要 Codec 和声卡设备树节点，例如：

```text
ext=es8311_sound
```

### Linux 驱动

相关内核配置为：

```text
CONFIG_SND_SOC=m
CONFIG_SND_SUN4I_I2S=m
CONFIG_SND_SIMPLE_CARD=m
```

I²S 时钟修改和音频驱动位于：

```text
board/cra/epass/patch/linux/0010-i2s-and-es-driver.patch
```

### 冲突

```text
i2s0_pa + adc_pa1   = PA1 冲突
i2s0_pa + adc_pa123 = PA1、PA2、PA3 冲突
i2s0_pa + uart1     = PA2、PA3 冲突
```

`i2s0_pa` 是两种 I²S0 布局中冲突范围较大的一种。

## `i2s0_pe.dts`

### 功能

```dts
fragment@1 {
    target = <&i2s0>;
    __overlay__ {
        status = "okay";
        pinctrl-0 = <&i2s_pins_pe>;
        pinctrl-names = "default";
    };
};
```

使用引脚：

```text
PE3、PE5、PE6、PA1
```

它不占用 UART1 的 PA2、PA3，但仍使用 PA1。

### 冲突

```text
i2s0_pe + adc_pa1   = PA1 冲突
i2s0_pe + adc_pa123 = PA1 冲突
```

`i2s0_pa` 与 `i2s0_pe` 不能同时作为有效布局使用。若两者同时出现在 `interface=` 中，后应用的覆盖层会覆盖 `pinctrl-0`，最终结果取决于排列顺序。

## `spi1.dts`

### 功能

```dts
fragment@1 {
    target = <&spi1>;
    __overlay__ {
        status = "okay";
    };
};
```

基础设备树已经配置：

```dts
&spi1 {
    pinctrl-names = "default";
    pinctrl-0 = <&spi1_pins>;
    status = "disabled";

    spidev@0 {
        compatible = "rohm,dh2228fv";
        spi-max-frequency = <80000000>;
        reg = <0>;
    };
};
```

使用引脚：

```text
PE7、PE8、PE9、PE10
```

`spi1_pins` 还为这组引脚配置了上拉。

### Spidev

`spidev@0` 表示：

- 设备位于 SPI1。
- 使用片选 0。
- 设备树允许的最大频率为 80MHz。
- 启用后通常生成 `/dev/spidev1.0`。

80MHz 只是设备树声明的上限，不代表外部芯片、连接器和飞线一定能稳定工作在该频率。实际使用时应从较低频率开始验证。

### `rohm,dh2228fv`

这里不表示主板真实连接了 Rohm DH2228FV。

Linux 不鼓励设备树直接声明：

```dts
compatible = "spidev";
```

否则会产生 `buggy DT` 警告。原项目借用了 Spidev 驱动匹配表中的 `rohm,dh2228fv`，以创建通用用户态 SPI 设备。

该字符串是兼容性手段，不是实际器件型号。

### Linux 驱动

相关内核配置为：

```text
CONFIG_SPI=y
CONFIG_SPI_SUN6I=y
CONFIG_SPI_SPIDEV=y
```

### 冲突

UART2 使用 PE7、PE8，因此：

```text
spi1 + uart2 = PE7、PE8 冲突
```

如果同时启用，两个控制器都会在最终设备树中处于可用状态。Linux pinctrl 子系统通常会使其中一个驱动申请引脚失败，但失败先后取决于驱动探测顺序，不能依赖。

## `uart1.dts`

### 功能

```dts
fragment@1 {
    target = <&uart1>;
    __overlay__ {
        status = "okay";
    };
};
```

基础设备树已经指定：

```dts
&uart1 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart1_pa_pins>;
    status = "disabled";
};
```

使用：

```text
PA2、PA3
```

这里只启用普通 TX/RX，没有启用 UART1 的 RTS/CTS 引脚组。

UART0 已经使用 PE0、PE1 作为系统控制台。`uart1.dts` 不会取代启动控制台，而是启用第二路串口。

启用后通常对应：

```text
/dev/ttyS1
```

实际编号应以实体设备的 `/dev/ttyS*` 和启动日志为准。

### Linux 驱动

相关内核配置为：

```text
CONFIG_SERIAL_8250=y
CONFIG_SERIAL_8250_CONSOLE=y
CONFIG_SERIAL_8250_NR_UARTS=3
CONFIG_SERIAL_8250_RUNTIME_UARTS=3
CONFIG_SERIAL_8250_DW=y
```

这里是 SoC 的 3.3V 逻辑电平 UART，不是使用正负电压的传统 RS-232。不得直接连接 RS-232 接口。

### 冲突

```text
uart1 + adc_pa123 = PA2、PA3 冲突
uart1 + i2s0_pa   = PA2、PA3 冲突
```

`uart1` 与 `adc_pa1` 没有直接引脚冲突。

## `uart2.dts`

### 功能

```dts
fragment@1 {
    target = <&uart2>;
    __overlay__ {
        status = "okay";
    };
};
```

基础设备树指定：

```dts
&uart2 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart2_pe_pins>;
    status = "disabled";
};
```

使用：

```text
PE7、PE8
```

启用后通常对应：

```text
/dev/ttyS2
```

实际编号仍应以实体设备为准。

### 冲突

SPI1 使用 PE7～PE10，其中包含 UART2 的 PE7、PE8：

```text
uart2 + spi1 = PE7、PE8 冲突
```

UART2 与当前两种 I²S0 引脚布局没有直接重叠。

## `usbhost.dts`

### 功能

```dts
fragment@1 {
    target = <&usb_otg>;
    __overlay__ {
        dr_mode = "host";
    };
};
```

基础设备树已经启用 USB OTG 控制器、PHY 和 OTG SRAM：

```dts
&otg_sram {
    status = "okay";
};

&usb_otg {
    dr_mode = "otg";
    status = "okay";
};

&usbphy {
    status = "okay";
};
```

`usbhost.dts` 只把：

```text
OTG 双角色模式
```

改为：

```text
固定主机模式
```

### 对 USB Gadget 的影响

主机模式用于连接 U 盘、USB 串口等 USB 从设备，但会失去 USB Gadget 功能。

启用 `usbhost` 后，以下当前用于连接电脑的功能不能继续按原方式工作：

- RNDIS USB 网卡。
- USB ACM 串口 Gadget。
- USB Mass Storage Gadget。
- FunctionFS Gadget。

如果仍需通过 RNDIS 连接电脑，不应启用 `usbhost`。

该覆盖层没有配置 VBUS 5V 电源或电源开关。若 PCB 不能主动为 USB 外设提供 VBUS，仅设置 `dr_mode = "host"` 不会自动产生 5V。

## `usbhs.dts`

### 功能

```dts
fragment@1 {
    target = <&usb_otg>;
    __overlay__ {
        srgn,usb-hs-enabled;
    };
};
```

`srgn,usb-hs-enabled` 不是 Linux 标准设备树属性，而是原项目添加的自定义布尔属性。

其设计含义为：

- 属性存在：请求启用 USB High-Speed。
- 属性不存在：项目补丁关闭 High-Speed，使用 Full-Speed。

它不负责选择 USB 主机或从机角色：

```text
usbhost = 角色选择
usbhs   = 速度选择
```

理论上可以同时配置：

```text
interface=usbhost usbhs
```

表示固定主机模式并请求 High-Speed。

### 速度区别

```text
USB Full-Speed：12Mbit/s
USB High-Speed：480Mbit/s
```

High-Speed 对 PCB 差分走线、阻抗、长度匹配和信号完整性的要求远高于 Full-Speed。

### 内核补丁中的未初始化变量

对应内核补丁存在一个实际 C 代码问题。

补丁将原来的初始化：

```c
power = MUSB_POWER_ISOUPDATE;
```

注释掉，但后面直接执行：

```c
power |= MUSB_POWER_HSENAB;
```

或：

```c
power &= ~MUSB_POWER_HSENAB;
```

局部变量 `power` 在位运算前没有被初始化，写入 MUSB `POWER` 寄存器的其他位可能来自未定义栈数据。默认 Full-Speed 分支与启用 High-Speed 的分支都会受到影响。

补丁中还保留了一条不适合正式产品日志的调试输出：

```c
printk(KERN_INFO "Conclusion: SRGN SUXX!\n");
```

这些问题属于内核补丁，不是 `usbhs.dts` 的语法错误。修复前不应仅为了提高标称速度而启用 `usbhs`。

### 自定义属性名称

`srgn` 是原项目使用的自定义厂商前缀。若以后改成：

```dts
cra,usb-hs-enabled;
```

必须同步修改内核补丁中的：

```c
of_property_read_bool(np, "srgn,usb-hs-enabled")
```

只修改设备树或只修改驱动都会导致属性无法匹配。

## 构建过程

`board/cra/epass/scripts/mkdt.sh` 会遍历本目录中的所有 `.dts`：

```bash
cpp -nostdinc \
    -I "${BUILD_DIR}/linux-5.4.99/include/" \
    -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
    -P -undef -x assembler-with-cpp

dtc -@ -I dts -O dtb
```

生成文件位于：

```text
output/images/dt/interface/adc_pa1.dtbo
output/images/dt/interface/adc_pa123.dtbo
output/images/dt/interface/i2c0.dtbo
output/images/dt/interface/i2s0_pa.dtbo
output/images/dt/interface/i2s0_pe.dtbo
output/images/dt/interface/spi1.dtbo
output/images/dt/interface/uart1.dtbo
output/images/dt/interface/uart2.dtbo
output/images/dt/interface/usbhost.dtbo
output/images/dt/interface/usbhs.dtbo
```

`-@` 会保留覆盖层需要的符号和修复信息，使 U-Boot 能解析 `&rtp`、`&i2c0`、`&i2s0`、`&spi1`、`&uart1`、`&uart2` 和 `&usb_otg` 等基础设备树标签。

`board/cra/epass/scripts/kernel.its` 随后把这些文件打包进 FIT 镜像，节点名称采用：

```text
fdt-iface-<接口名称>
```

例如：

```text
fdt-iface-i2c0
fdt-iface-spi1
fdt-iface-uart1
```

## 启动时选择

项目模板 `board/cra/epass/uEnv.txt` 当前为：

```text
interface=
ext=
```

默认不启用任何可选接口或外接设备覆盖层。

U-Boot 使用空格分隔多个接口名称：

```text
interface=i2c0 uart1
```

处理逻辑为：

```text
for ov in ${interface}
        ↓
从 FIT 提取 fdt-iface-${ov}
        ↓
按书写顺序执行 fdt apply
```

接口名称必须与 FIT 节点后缀完全一致。

U-Boot 不会自动检查引脚冲突。若多个覆盖层修改同一属性，后应用的值可能覆盖前一个；若多个控制器占用同一物理引脚，最终设备树可能同时启用它们，随后由 Linux 驱动在运行时发生资源申请失败。

## 依赖与冲突汇总

| 组合 | 冲突位置或原因 |
| --- | --- |
| `adc_pa1` + `i2s0_pa` | PA1 |
| `adc_pa1` + `i2s0_pe` | PA1 |
| `adc_pa123` + `i2s0_pa` | PA1、PA2、PA3 |
| `adc_pa123` + `i2s0_pe` | PA1 |
| `adc_pa123` + `uart1` | PA2、PA3 |
| `i2s0_pa` + `uart1` | PA2、PA3 |
| `spi1` + `uart2` | PE7、PE8 |
| `i2c0` + `es8311_sound` | PD0、PD12 被硬件 I²C0 和 GPIO 模拟 I²C 同时占用 |
| `usbhost` + RNDIS/USB Gadget | USB 主机角色与 USB Gadget 从机角色不兼容 |
| `usbhs` | 原板 High-Speed 信号完整性风险，且对应内核补丁存在未初始化变量 |

以下覆盖层也属于互斥选择，不应同时填写：

```text
adc_pa1 / adc_pa123
i2s0_pa / i2s0_pe
```

## 二次开发原则

对本目录进行二次开发前，必须确认硬件原理图、PCB 走线和基础设备树：

- 严禁让两个控制器占用同一组 GPIO。
- 修改 `pinctrl-0` 时，应同步检查所有使用相同 PA、PD、PE 引脚的节点。
- ADC 输入必须符合 F1C200S 的允许电压范围和电气要求。
- UART 是 SoC 逻辑电平，不得直接连接传统 RS-232 电压。
- SPI 的 `spi-max-frequency` 是上限，不是稳定性保证。
- 强制 USB 主机模式前，应确认 VBUS 供电和 Gadget 功能需求。
- 启用 USB High-Speed 前，应先修复对应内核补丁并验证 PCB 信号完整性。
- 修改 `srgn,usb-hs-enabled` 名称时，必须同步修改内核驱动。
- 新增接口覆盖层后，需要同步修改 `kernel.its`，否则 `.dtbo` 不会被打包进 FIT 镜像。
- 新增控制器驱动后，需要同步检查 `linux.defconfig` 和内核补丁。
- 删除接口覆盖层前，应确认对应的扩展设备和旧版硬件不再需要。
