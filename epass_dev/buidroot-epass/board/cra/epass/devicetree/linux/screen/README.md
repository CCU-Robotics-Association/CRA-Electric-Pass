# CRA Electric Pass 屏幕设备树覆盖层

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 CRA Electric Pass 的 Linux LCD 屏幕设备树覆盖层，用于在启动时选择与实体屏幕相匹配的 ST7701 初始化序列。当前工程仅面向白银 v0.6 板型。

## 文件说明

| 文件 | 初始化序列 | 额外处理 | 启动参数 |
| --- | --- | --- | --- |
| `boe.dts` | BOE 屏幕使用的 ST7701 初始化序列 | 无 | `screen=boe` |
| `hsd.dts` | HSD 屏幕使用的 ST7701 初始化序列 | 无 | `screen=hsd` |
| `laowu.dts` | 与 `hsd.dts` 相同的 ST7701 初始化序列 | 在 TCON0 中交换红、蓝通道 | `screen=laowu` |

## 显示系统中的位置

完整显示链路为：

```text
Linux DRM
   │
   ▼
Allwinner DE
   │
   ▼
TCON0
   │
   ▼
RGB565 并行总线
   │
   ▼
ST7701 LCD 面板
```

相关配置分布在多个位置：

```text
base/epass.dtsi
├─ 声明 panel 节点
├─ 声明 PWM 背光
├─ 声明 RGB565 引脚组
├─ 启用 TCON0
└─ 声明默认关闭的 st7701initseq 节点

base/devicetree.dts
└─ 为 st7701initseq 分配 SDA、SCL、CS GPIO

screen/*.dts
├─ 启用 st7701initseq
├─ 提供对应屏幕的初始化序列
├─ 指定公共 panel compatible
└─ 必要时启用红蓝通道交换

内核补丁
├─ 0002-panel-simple.patch：公共分辨率、时序和 RGB565 格式
├─ 0004-swap_rb_as_config.patch：TCON0 红蓝通道交换
└─ 0006-initalize-st7701.patch：GPIO 模拟的 ST7701 初始化驱动
```

## 公共基础配置

### RGB 显示总线

`base/epass.dtsi` 中的 `lcd_rgb565_no_de_pins` 使用以下引脚：

```text
PD1～PD11
PD13～PD18
PD20
PD21
```

这些引脚组成 RGB565 数据、像素时钟和同步信号。配置名称中的 `no_de` 表示该引脚组不使用独立的 Data Enable 引脚。

公共面板描述采用：

```dts
compatible = "lattland,mostima", "simple-panel";
```

其中 `lattland,mostima` 是原项目内核补丁注册的自定义兼容字符串，不是主线 Linux 中自带的标准 ST7701 面板型号。

### ST7701 初始化引脚

`base/devicetree.dts` 为初始化驱动配置：

| 信号 | GPIO | 作用 |
| --- | --- | --- |
| SDA | PE4 | 串行命令或数据位 |
| SCL | PD19 | 串行时钟 |
| CS | PE11 | 片选 |
| RST | 未声明 | 驱动支持可选复位脚，但当前板级设备树未使用 |

该通信由自定义内核驱动直接翻转 GPIO 完成，不使用 F1C200S 的硬件 SPI 控制器。

每次传输先发送一位命令/数据标志，再发送八位内容：

- 标志位为 `0` 时表示命令。
- 标志位为 `1` 时表示数据。
- 每个字节按最高位优先发送。
- `CS` 拉低开始一组写入，拉高结束。

这是一种面向当前屏幕接法的 GPIO 模拟初始化方式，不能把这些引脚直接当作普通 SPI 覆盖层使用。

### 背光

背光不由本目录配置，而由 `base/epass.dtsi` 中的 `pwm-backlight` 节点统一管理：

- PWM 控制器：PWM0
- 周期：`10000 ns`
- 频率：约 `100 kHz`
- 亮度表：`0 4 8 16 32 64 128 196 220 255`
- 默认亮度索引：`6`
- 默认亮度值：`128`

## 公共显示模式

三种屏幕覆盖层最终都使用同一个 `lattland,mostima` 面板描述。公共显示模式由：

```text
board/cra/epass/patch/linux/0002-panel-simple.patch
```

加入 Linux `panel-simple` 驱动。

当前参数为：

| 参数 | 数值 |
| --- | ---: |
| 像素时钟 | 24000 kHz |
| 水平有效像素 | 384 |
| 水平同步起点 | 444 |
| 水平同步终点 | 450 |
| 水平总计 | 528 |
| 垂直有效像素 | 640 |
| 垂直同步起点 | 656 |
| 垂直同步终点 | 660 |
| 垂直总计 | 669 |
| 声明刷新率 | 60 Hz |
| 总线格式 | `MEDIA_BUS_FMT_RGB565_1X16` |
| 每颜色分量位数 | 6 bpc |

需要注意：按照像素时钟和总计值计算，

```text
24,000,000 ÷ (528 × 669) ≈ 67.9 Hz
```
## 覆盖层基本结构

三个文件都是 Device Tree Overlay：

```dts
#include <dt-bindings/display/st7701initseq.h>

/dts-v1/;
/plugin/;

/ {
    fragment@1 {
        target = <&st7701initseq>;
        __overlay__ {
            status = "okay";
            init-sequence = <...>;
        };
    };

    fragment@2 {
        target = <&panel>;
        __overlay__ {
            compatible = "lattland,mostima", "simple-panel";
        };
    };
};
```

### `fragment@1`

目标是基础设备树中的 `&st7701initseq`：

- 将默认的 `status = "disabled"` 改为 `status = "okay"`。
- 写入当前屏幕对应的 `init-sequence`。
- 使自定义 ST7701 初始化驱动在 Linux 启动时匹配并执行。

### `fragment@2`

目标是基础设备树中的 `&panel`，指定公共的面板驱动兼容字符串。

三个文件目前写入的 `compatible` 完全相同，因此分辨率、同步时序和 RGB565 格式也完全相同。它们之间的主要差异是 ST7701 寄存器初始化内容，而不是 DRM 显示模式。

当前 `base/epass.dtsi` 已经声明了完全相同的 `compatible`，所以这个片段在现有组合中不会改变最终值，更像是原覆盖层结构中保留下来的重复声明。若以后为不同屏幕增加真正独立的面板时序，可以在这里改为各自的兼容字符串，同时在内核面板驱动中提供对应描述。

片段编号从 `fragment@1` 开始而没有 `fragment@0` 并不构成设备树语法错误；编号只需在同一个覆盖层中保持唯一。

### `fragment@3`

只有 `laowu.dts` 包含：

```dts
fragment@3 {
    target = <&tcon0>;
    __overlay__ {
        srgn,swap-b-r;
    };
};
```

`srgn,swap-b-r` 是原项目内核补丁定义的私有布尔属性。`0004-swap_rb_as_config.patch` 读取该属性，并设置 TCON0 控制寄存器中的颜色交换位。

如果屏幕显示正常但红色与蓝色相反，应优先检查是否选错 `hsd`/`laowu` 配置，而不是在应用程序中交换每个像素的颜色。

该属性名与内核补丁严格绑定。若以后将它重命名为 `cra,swap-b-r`，必须同时修改：

```text
screen/laowu.dts
board/cra/epass/patch/linux/0004-swap_rb_as_config.patch
```

只改一处会使颜色交换失效。

## 初始化序列指令

`init-sequence` 并不是普通字节数组。它由：

```text
include/dt-bindings/display/st7701initseq.h
```

中的宏编码，再由自定义内核驱动逐项解释。

| 宏 | 参数 | 作用 |
| --- | --- | --- |
| `ST7701INIT_BEGIN_WRITE` | 无 | 将 `CS` 拉低，开始一组传输 |
| `ST7701INIT_WRITE_COMMAND_8` | 1 个命令 | 写入一个 8 位命令 |
| `ST7701INIT_WRITE_C8_D8` | 1 个命令、1 个数据 | 写入命令及一个数据字节 |
| `ST7701INIT_WRITE_C8_D16` | 1 个命令、2 个数据 | 写入命令及两个数据字节 |
| `ST7701INIT_WRITE_BYTES` | 长度、对应数量的数据 | 继续写入指定数量的数据字节 |
| `ST7701INIT_END_WRITE` | 无 | 将 `CS` 拉高，结束传输 |
| `ST7701INIT_DELAY` | 毫秒数 | 休眠指定时间 |

例如：

```dts
ST7701INIT_BEGIN_WRITE

ST7701INIT_WRITE_COMMAND_8 0xFF
ST7701INIT_WRITE_BYTES 5
0x77 0x01 0x00 0x00 0x10

ST7701INIT_WRITE_C8_D16 0xC1 0x07 0x02

ST7701INIT_END_WRITE
ST7701INIT_DELAY 100
```

表示：

1. 拉低片选。
2. 发送命令 `0xFF`。
3. 紧接着发送五个数据字节。
4. 发送命令 `0xC1` 和两个数据字节。
5. 拉高片选。
6. 等待 100 ms。

驱动不会根据 ST7701 数据手册验证寄存器值，也没有完整检查每条宏指令后是否还存在足够的参数。长度写错、漏写参数或打乱指令边界，可能导致错误初始化，甚至让驱动越过预期数组边界读取数据。

## 初始化序列的功能范围

这些长序列主要包含：

- ST7701 扩展命令页选择。
- 电源、电压和模拟参数。
- 正负极性 Gamma 曲线。
- Source/Gate 输出及 GIP 映射。
- 扫描方向和显示方向相关设置。
- 像素格式设置。
- 退出休眠。
- 开启显示。
- 各阶段所需延时。

其中部分寄存器属于 ST7701 厂商扩展页，不能只根据通用 MIPI DCS 命令名称推断作用。修改前应取得对应面板和控制器的数据手册，并保留实体屏已验证的初始化表。

三个序列都包含：

- `0x11`：退出休眠。
- `0x29`：开启显示。
- `0x3A 0x50`：设置当前使用的像素格式。

BOE 与 HSD/Laowu 在 Gamma、电源、时序控制、GIP 映射和等待时间等多处存在差异，不能仅复制文件名后混用。

## 三种配置的差异

### `boe.dts`

`boe.dts` 使用独立的初始化表。与 HSD/Laowu 相比，其 Gamma、电源和多组扩展寄存器值均不同。

主要等待阶段为：

```text
120 ms
10 ms
20 ms
```

显示开启后还会发送：

```text
0x35 0x00
```

这一步在 HSD/Laowu 序列中不存在。

### `hsd.dts`

`hsd.dts` 使用另一套 ST7701 初始化表，不启用 TCON0 红蓝通道交换。

主要等待阶段为：

```text
150 ms
100 ms
20 ms
```

它是当前 `flash.py` 主入口使用的默认参数：

```python
flash("hsd", {...})
```

这里的“默认”来自当前烧录脚本调用，不代表所有实体设备都安装 HSD 屏幕。

### `laowu.dts`

`laowu.dts` 与 `hsd.dts` 的初始化序列逐项相同。两者唯一的源码差异是 `laowu.dts` 增加：

```dts
srgn,swap-b-r;
```

因此：

```text
hsd    = HSD 初始化表
laowu = HSD 初始化表 + TCON0 红蓝交换
```

如果未来修改 HSD 初始化序列，应检查 `laowu.dts` 是否也需要同步修改，避免两个本应相同的寄存器表意外分叉。

## 内核初始化驱动

ST7701 初始化支持并非 Linux 5.4.99 原生功能，而是由：

```text
board/cra/epass/patch/linux/0006-initalize-st7701.patch
```

加入内核源码。

该补丁新增：

```text
include/dt-bindings/display/st7701initseq.h
drivers/staging/shirogane/Kconfig
drivers/staging/shirogane/Makefile
drivers/staging/shirogane/st7701init.c
```

内核配置通过：

```text
CONFIG_SHIROGANE_SIMPLE_ST7701_INIT=y
```

将驱动直接编入内核。

驱动匹配：

```dts
compatible = "lattland,st7701-initseq";
```

匹配成功后，它会：

1. 申请 SDA、SCL、CS 和可选 RST GPIO。
2. 从设备树读取 `init-sequence` 的 32 位单元数组。
3. 创建工作队列任务。
4. 在工作队列中异步执行 GPIO 初始化序列。

当前设备树未声明 RST GPIO，驱动会在没有独立复位脚的情况下继续执行。

`lattland,*`、`srgn,*` 和 `SHIROGANE_*` 都是原项目遗留的内部命名。它们可以在二次开发中逐步改为 CRA 命名，但必须同步修改设备树、内核补丁、配置符号和所有引用，不能只做字符串替换。

## 编译与打包

Buildroot 的镜像后处理脚本会调用：

```text
board/cra/epass/scripts/mkdt.sh
```

该脚本遍历本目录的所有 `.dts` 文件，先使用内核头文件进行 C 预处理：

```sh
cpp -nostdinc \
    -I "${BUILD_DIR}/linux-5.4.99/include/" \
    -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
    -P -undef -x assembler-with-cpp
```

随后使用：

```sh
dtc -@ -I dts -O dtb
```

生成支持符号重定位的设备树覆盖层。

输出文件为：

```text
output/images/dt/screen/boe.dtbo
output/images/dt/screen/hsd.dtbo
output/images/dt/screen/laowu.dtbo
```

`kernel.its` 再将它们分别打包为 FIT 镜像节点：

```text
fdt-screen-boe
fdt-screen-hsd
fdt-screen-laowu
```

仅仅在本目录新增 `.dts` 文件并不足以让 U-Boot 使用它。新增屏幕类型后还必须同步修改：

```text
board/cra/epass/scripts/kernel.its
```

为新的 `.dtbo` 建立对应的 `fdt-screen-*` 节点。

## 启动时选择屏幕

烧录脚本会向启动环境文本写入：

```text
screen=hsd
```

U-Boot 启动时从 SPI-NAND 的 `0xFA000` 位置导入环境，然后执行：

```text
imxtract $fitaddr fdt-screen-${screen} $dtboaddr
```

例如：

```text
screen=boe
        ↓
fdt-screen-boe
        ↓
boe.dtbo
```

提取完成后，U-Boot 使用：

```text
fdt apply $dtboaddr
```

将屏幕覆盖层应用到基础设备树。

有效值必须与 FIT 节点后缀完全一致：

```text
boe
hsd
laowu
```

当前仓库中的 `uEnv.txt` 模板没有写入 `screen=`，而 `flash.py` 会在运行时生成包含 `screen` 的 `.bootenv.txt`。因此：

- 使用现有 `flash.py` 时，屏幕类型来自 `flash()` 或 `flash2()` 的第一个参数。
- 手动生成或修改启动环境时，必须补充有效的 `screen=`。
- `screen` 为空或拼写错误时，U-Boot 无法提取正确的 `fdt-screen-*` 节点。

## 常见现象与检查方向

| 现象 | 优先检查 |
| --- | --- |
| 背光亮但无图像 | `screen=` 是否有效、ST7701 初始化是否执行、RGB 时钟和同步信号 |
| 完全不亮 | PWM 背光、电源、面板连接，不要只检查初始化序列 |
| 红蓝互换 | `hsd` 与 `laowu` 是否选错，`srgn,swap-b-r` 是否生效 |
| 颜色层次异常 | RGB565 接线、像素格式、Gamma 表和面板型号 |
| 图像滚动或撕裂 | 公共 DRM 时序、ST7701 扫描配置、像素时钟 |
| 图像方向错误 | 初始化表中的扫描方向设置，不要在未确认前修改 TCON |
| 开机偶尔白屏 | 初始化顺序、延时、供电稳定性及异步初始化时机 |
| 编译时找不到宏 | Linux 头文件是否已应用 `0006-initalize-st7701.patch` |
| `.dtbo` 已生成但启动时找不到 | `kernel.its` 是否包含对应 `fdt-screen-*` 节点 |

可通过内核日志搜索原驱动输出：

```sh
dmesg | grep -i st7701
dmesg | grep -i srgn
```

原驱动成功执行时会输出初始化任务开始和完成信息；选择 `laowu` 时还会输出红蓝交换相关信息。

## 二次开发原则

修改本目录前，应先确认实体屏幕型号、排线定义、PCB 连接和已验证的启动配置：

- 严禁在没有面板资料和实体测试条件时随机修改电源、Gamma 或 GIP 寄存器。
- 严禁假设所有 ST7701 屏幕都能共用同一套初始化序列。
- 严禁将应用层颜色错误直接归因于面板；先使用纯红、纯绿、纯蓝测试图确认通道顺序。
- 修改 `ST7701INIT_WRITE_BYTES` 后，必须同步核对长度和实际数据数量。
- 每组 `BEGIN_WRITE` 都应有对应的 `END_WRITE`。
- 退出休眠、开启显示及其等待时间应符合面板资料。
- 调整公共分辨率或同步时序时，应修改并验证 `0002-panel-simple.patch`，而不是只改本目录。
- 修改红蓝交换属性名时，必须同步修改 `0004-swap_rb_as_config.patch`。
- 修改初始化驱动兼容字符串或配置符号时，必须同步修改 `0006-initalize-st7701.patch`、基础设备树和屏幕覆盖层。
- 新增屏幕覆盖层后，必须同步更新 `kernel.its` 和烧录工具中的可选屏幕名称。
- 实机验证应从已知可恢复的配置开始，并保留串口日志和原始初始化表。
- 屏幕测试不需要改写整机系统或应用资源；优先只替换启动环境中的 `screen` 选择并观察结果。
