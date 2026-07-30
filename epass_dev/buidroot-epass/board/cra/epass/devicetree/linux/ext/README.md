# CRA Electric Pass 外接设备覆盖层

其他语言版本: [English](README_EN.md), [中文](README.md).

本目录保存 CRA Electric Pass 的 Linux 外接设备树覆盖层，用于描述连接在主板接口上的可选硬件。

文件仅于 U-Boot 启动阶段按需附加到基础设备树上。当前工程仅面向白银 v0.6 板型。

## 文件说明

| 文件 | 外接设备 | 总线与地址 |
| --- | --- | --- |
| `cardkb.dts` | M5Stack Unit CardKB 小键盘 | 硬件 I²C0，地址 `0x5f` | 
| `es8311_sound.dts` | Everest ES8311 音频编解码器 | GPIO 模拟 I²C，地址 `0x18`；音频数据使用 I²S0 |
| `lsm6ds3_pre0.4.dts` | ST LSM6DS3 六轴惯性传感器 | 硬件 I²C0，地址 `0x6a`；PE2 中断 |

## 与其他设备树目录的关系

Linux 设备树按以下层次组成：

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

目录职责如下：

- `base/`：描述主板上始终存在的基础硬件。
- `screen/`：选择实际安装的 LCD 面板及其时序。
- `interface/`：启用 SoC 控制器并配置引脚复用，例如 I²C0、I²S0、SPI1 或 UART。
- `ext/`：在已经具备相应总线条件的基础上，声明连接到总线上的具体外部设备。

例如，CardKB 本身属于 `ext` 外设，但它依赖 `interface/i2c0.dts` 先启用硬件 I²C0。

## 覆盖层基本语法

本目录中的文件均为 Device Tree Overlay：

```dts
/dts-v1/;
/plugin/;

/ {
    fragment@1 {
        target = <&some_node>;
        __overlay__ {
            /* 新增或覆盖的设备树内容 */
        };
    };
};
```

主要字段含义如下：

- `/dts-v1/;`：声明设备树源码版本。
- `/plugin/;`：声明该文件是可附加到基础设备树的覆盖层。
- `fragment@1`：一个覆盖片段，编号只用于区分片段。
- `target`：通过标签引用需要修改的基础设备树节点。
- `target-path`：通过节点路径指定修改目标。
- `__overlay__`：真正需要添加或覆盖的属性和子节点。
- `compatible`：Linux 用于匹配驱动的设备兼容字符串。
- `reg`：设备在 I²C 等总线上的地址。
- `status = "okay"`：显式启用节点。新建节点未设置 `status` 时默认也视为可用。

下列写法中的冒号前部分是标签，后部分是节点名：

```dts
es8311: es8311@18
```

其他节点可以通过 `&es8311` 引用该设备。标签主要用于设备树内部连接，不一定等于 Linux 最终显示的设备名称。

## `cardkb.dts`

### 功能

`cardkb.dts` 描述 M5Stack Unit CardKB 小键盘：

```dts
fragment@1 {
    target = <&i2c0>;
    __overlay__ {
        cardkb:cardkb@5f {
            compatible = "m5stack,cardkb";
            reg = <0x5f>;
            polling-interval = <50>;
        };
    };
};
```

### I²C0 依赖

```dts
target = <&i2c0>;
```

表示 CardKB 被添加到 F1C200S 的硬件 I²C0 控制器下。

基础设备树中的 I²C0 默认关闭：

```dts
&i2c0 {
    pinctrl-names = "default";
    pinctrl-0 = <&i2c0_pd_pins>;
    status = "disabled";
};
```

因此启用 CardKB 时必须同时启用：

```text
interface=i2c0
ext=cardkb
```

硬件 I²C0 使用：

```text
PD0  = SDA
PD12 = SCL
```

### 设备地址

```dts
reg = <0x5f>;
```

表示 CardKB 的 7 位 I²C 地址为 `0x5f`。除非外设固件实际使用了其他地址，否则不应修改此值。

### 轮询间隔

```dts
polling-interval = <50>;
```

驱动每隔 50ms 读取一次键盘，轮询频率约为 20Hz：

- 减小该值会降低输入延迟，但会增加 I²C 访问频率和 CPU 唤醒次数。
- 增大该值会降低系统负担，但按键响应会变慢。
- 当前 50ms 适合普通按键输入。

### Linux 驱动

CardKB 驱动由以下补丁加入 Linux 内核：

```text
board/cra/epass/patch/linux/0009-m5stack-cardkb-driver.patch
```

相关内核配置为：

```text
CONFIG_INPUT_EVDEV=y
CONFIG_SHIROGANE_KEYBOARD_CARDKB=m
```

驱动通过 I²C 每次读取一个字节，将 CardKB 键值转换为 Linux 标准按键事件，并注册为 `/dev/input/event*` 输入设备。驱动内包含字母、数字、标点、方向键以及 Shift、Ctrl 组合键映射。

`=m` 表示驱动被编译为内核模块。设备树节点出现后，Linux 可根据 `compatible = "m5stack,cardkb"` 自动匹配并加载该模块。

## `es8311_sound.dts`

### 功能结构

ES8311 覆盖层在设备树根节点下新增两部分：

```text
/
├── sound_i2s
└── i2c_bitbang
    └── es8311@18
```

其中：

- `sound_i2s` 把 F1C200S 的 I²S0 与 ES8311 组合成一张 ALSA 声卡。
- `i2c_bitbang` 使用普通 GPIO 软件模拟 I²C，以配置 ES8311 寄存器。
- I²C 负责控制，I²S 负责传输数字音频数据，两者缺一不可。

### 声卡节点

```dts
sound_i2s {
    compatible = "simple-audio-card";
    status = "okay";
    simple-audio-card,name = "es8311";
    simple-audio-card,format = "i2s";
    simple-audio-card,mclk-fs = <256>;
};
```

字段含义：

- `simple-audio-card`：使用 Linux 通用 ASoC 简单声卡驱动。
- `simple-audio-card,name`：将声卡命名为 `es8311`。
- `simple-audio-card,format = "i2s"`：使用标准 I²S 数据格式。
- `simple-audio-card,mclk-fs = <256>`：主时钟频率为采样率的 256 倍。

例如采样率为 48kHz：

```text
48000 × 256 = 12.288MHz
```

### CPU DAI 与 Codec DAI

```dts
simple-audio-card,cpu {
    sound-dai = <&i2s0>;
};

simple-audio-card,codec {
    sound-dai = <&es8311>;
};
```

这两段把 SoC 的 I²S0 数字音频接口和 ES8311 编解码器连接起来：

```text
F1C200S I²S0 ←→ ES8311
```

### I²S0 接口依赖

基础设备树不会默认启用 I²S0。使用 ES8311 时，必须根据 PCB 实际走线，从以下两个接口覆盖层中选择一个：

```text
interface=i2s0_pa
```

或：

```text
interface=i2s0_pe
```

两种引脚布局为：

```text
i2s0_pa：PE3、PA2、PA3、PA1
i2s0_pe：PE3、PE5、PE6、PA1
```

二者不能同时启用，也不能只根据名称猜测选择。错误的引脚组会导致声卡无法正常传输音频。

两种布局都会使用 PA1，因此均与占用 PA1 的 ADC 接口配置冲突。`i2s0_pa` 还会使用 PA2、PA3，冲突范围更大。

### GPIO 模拟 I²C

```dts
i2c_bitbang {
    compatible = "i2c-gpio";
    sda-gpios = <&pio 3 0 (GPIO_ACTIVE_HIGH|GPIO_OPEN_DRAIN)>;
    scl-gpios = <&pio 3 12 (GPIO_ACTIVE_HIGH|GPIO_OPEN_DRAIN)>;
    i2c-gpio,delay-us = <5>;
    status = "okay";
};
```

该节点使用：

```text
PD0  = SDA
PD12 = SCL
```

`GPIO_OPEN_DRAIN` 表示引脚以 I²C 所需的开漏方式工作。

```dts
i2c-gpio,delay-us = <5>;
```

表示每个半周期约延时 5μs，完整时钟周期约为 10μs，对应约 100kHz 的标准模式 I²C。

内核必须启用：

```text
CONFIG_I2C_GPIO=y
```

### ES8311 节点

```dts
es8311: es8311@18 {
    compatible = "everest,es8311";
    status = "okay";
    reg = <0x18>;
    pinctrl-names = "default";
    #sound-dai-cells = <0>;
};
```

字段含义：

- `reg = <0x18>`：ES8311 的 7 位 I²C 地址。
- `compatible = "everest,es8311"`：匹配 ES8311 ASoC Codec 驱动。
- `es8311:`：供声卡节点引用的标签。
- `#sound-dai-cells = <0>`：引用该数字音频接口时不需要附加参数。

当前节点只有 `pinctrl-names = "default"`，没有对应的 `pinctrl-0`，因此没有额外分配引脚。该属性可以保留，但当前作用有限。

### Linux 驱动

相关内核配置为：

```text
CONFIG_SOUND=m
CONFIG_SND=m
CONFIG_SND_SOC=m
CONFIG_SND_SUN4I_I2S=m
CONFIG_SND_SOC_ES8311=m
CONFIG_SND_SIMPLE_CARD=m
CONFIG_I2C_GPIO=y
```

I²S 时钟修改和 ES 系列驱动由以下补丁加入：

```text
board/cra/epass/patch/linux/0010-i2s-and-es-driver.patch
```

该补丁不只包含 ES8311，还包含 ES8156、ES8375 和 ES8389 等 Codec 驱动；当前内核配置只选择了 ES8311。

### 与硬件 I²C0 的冲突

F1C200S 硬件 I²C0 和本覆盖层的软件模拟 I²C 都使用 PD0、PD12。

因此当前写法下不得同时配置：

```text
interface=i2c0
ext=es8311_sound
```

否则两个不同的 Linux I²C 控制器会争用相同物理引脚。

这也意味着当前 ES8311 覆盖层不能直接与依赖硬件 I²C0 的 CardKB 或 LSM6DS3 同时启用。如果未来确实需要这些设备共存，应重新设计设备树，让它们共享同一个硬件 I²C0 控制器，而不是同时使用硬件 I²C0 和 GPIO 模拟 I²C。

## `lsm6ds3_pre0.4.dts`

### 功能

该覆盖层描述 ST LSM6DS3 六轴惯性传感器：

```dts
fragment@1 {
    target = <&i2c0>;
    __overlay__ {
        lsm6ds3:lsm6ds3@6a {
            compatible = "st,lsm6ds3";
            reg = <0x6a>;
            interrupt-parent = <&pio>;
            interrupts = <4 2 2>;
        };
    };
};
```

LSM6DS3 包含：

- 三轴加速度计。
- 三轴陀螺仪。

Linux 通过 IIO（Industrial I/O）子系统管理该设备，而不是将其注册为普通键盘输入设备。

### I²C0 依赖

```dts
target = <&i2c0>;
reg = <0x6a>;
```

表示 LSM6DS3 位于硬件 I²C0 上，地址为 `0x6a`。

旧版硬件使用时需要：

```text
interface=i2c0
ext=lsm6ds3_pre0.4
```

CardKB 地址为 `0x5f`，LSM6DS3 地址为 `0x6a`。仅从 I²C 地址看，两者可以位于同一条硬件 I²C0 总线上。

### 中断配置

```dts
interrupt-parent = <&pio>;
interrupts = <4 2 2>;
```

三个数依次表示：

```text
4 = GPIO E 组
2 = 第 2 号引脚
2 = 下降沿触发
```

所以该传感器使用 PE2 作为下降沿中断输入。

最后一个 `2` 是裸数值。为了提高可读性，后续可以包含中断类型头文件，并使用 `IRQ_TYPE_EDGE_FALLING` 表达相同含义；当前写法在语法和数值上仍然有效。

### Linux 驱动

相关内核配置为：

```text
CONFIG_IIO=y
CONFIG_IIO_ST_LSM6DSX=m
```

LSM6DS3 使用 Linux 5.4.99 已有的 ST LSM6DSX IIO 驱动，不依赖本项目新增的专用传感器驱动补丁。

### 关机引脚冲突

文件名中的 `pre0.4` 表示该覆盖层仅面向 0.4 以前的旧硬件。

当前基础设备树已经将 PE2 分配给关机控制：

```dts
poweroff: gpio-poweroff {
    compatible = "gpio-poweroff";
    gpios = <&pio 4 2 GPIO_ACTIVE_HIGH>; // PE2
    timeout-ms = <3000>;
};
```

因此同一个 PE2 会同时被声明为：

```text
LSM6DS3 中断输入
        与
设备关机控制输出
```

两者不能在当前设备上同时成立。强行启用可能导致：

- LSM6DS3 中断 GPIO 申请失败。
- `gpio-poweroff` 无法申请 PE2。
- Linux 完成关机后设备仍然不断电。
- 引脚方向或电平状态不符合硬件预期。

所以 `lsm6ds3_pre0.4` 即使能够正常编译和附加，也不得在当前实体设备上启用。

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
output/images/dt/ext/cardkb.dtbo
output/images/dt/ext/es8311_sound.dtbo
output/images/dt/ext/lsm6ds3_pre0.4.dtbo
```

其中 `-@` 会保留覆盖层所需的符号和修复信息，使 U-Boot 能够解析 `&i2c0`、`&pio`、`&i2s0` 等来自基础设备树的标签。

`board/cra/epass/scripts/kernel.its` 随后将这些文件打包进 FIT 镜像：

```text
fdt-ext-cardkb
fdt-ext-es8311_sound
fdt-ext-lsm6ds3_pre0.4
```

## 启动时选择

项目模板 `board/cra/epass/uEnv.txt` 当前为：

```text
interface=
ext=
```

这表示默认不启用任何接口覆盖层或外接设备覆盖层。

U-Boot 使用空格分隔多个覆盖层名称，并按照 `interface` 在前、`ext` 在后的顺序应用：

```text
patchinterface
        ↓
patchext
```

覆盖层名称必须与 FIT 节点后缀完全一致，例如：

```text
interface=i2c0
ext=cardkb
```

不要在没有确认实体板接线、供电、电平和引脚复用的情况下，仅为了测试而启用外设覆盖层。

## 依赖与冲突汇总

| 扩展 | 必需接口 | 使用引脚 | 主要冲突 |
| --- | --- | --- | --- |
| `cardkb` | `i2c0` | PD0、PD12 | 与当前 `es8311_sound` 的 GPIO 模拟 I²C 冲突。 |
| `es8311_sound` | `i2s0_pa` 或 `i2s0_pe` | I²C：PD0、PD12；I²S：由所选接口决定 | 与硬件 I²C0 冲突；I²S 引脚与部分 ADC 配置冲突。 |
| `lsm6ds3_pre0.4` | `i2c0` | I²C：PD0、PD12；中断：PE2 | PE2 与设备的 `gpio-poweroff` 冲突。 |

## 编译警告说明

设备树覆盖层单独编译时，`dtc` 可能对 `reg`、`#address-cells` 或父总线信息给出警告。这是因为覆盖层在独立编译阶段看不到基础设备树目标节点的完整上下文。

判断覆盖层是否真正有效时，应同时检查：

1. `.dts` 是否成功生成 `.dtbo`。
2. 覆盖层是否能正确附加到基础 `.dtb`。
3. U-Boot 是否按正确顺序应用 `interface` 和 `ext`。
4. Linux 驱动是否成功匹配。
5. 实体硬件引脚、电平、地址和中断是否与设备树一致。

仅仅“能够编译”不等于“能够在当前硬件上安全使用”。

## 二次开发原则

修改本目录前应先确认硬件原理图和 PCB 走线：

- 严禁随意修改 I²C 地址。
- 严禁将两个控制器占用同一组 GPIO。
- 严禁让中断输入与电源控制输出复用同一引脚。
- 修改 `compatible` 前必须确认内核驱动的匹配表。
- 新增外设后，需要同步修改 `kernel.its`，否则生成的 `.dtbo` 不会被打包进 FIT 镜像。
- 新增驱动后，需要同步检查 `linux.defconfig` 和相应内核补丁。
- 删除历史覆盖层前，应确认不再需要兼容对应的旧版硬件。
