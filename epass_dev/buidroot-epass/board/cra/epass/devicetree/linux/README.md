# CRA Electric Pass Linux 设备树

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 CRA Electric Pass 在 Linux 阶段使用的基础设备树和设备树覆盖层。当前工程仅面向白银 v0.6 板型。

这里的文件描述 Linux 启动后能够使用的主板硬件、屏幕、SoC 接口和外接设备。SPL 与 U-Boot 自身使用的设备树位于：

```text
board/cra/epass/devicetree/uboot/
```

两套设备树相互独立。修改本目录不会自动改变 SPL 或 U-Boot 阶段的硬件配置。

## 目录结构

```text
devicetree/linux/
├─ base/
│  ├─ epass.dtsi
│  └─ devicetree.dts
├─ screen/
│  ├─ boe.dts
│  ├─ hsd.dts
│  └─ laowu.dts
├─ interface/
│  ├─ adc_pa1.dts
│  ├─ adc_pa123.dts
│  ├─ i2c0.dts
│  ├─ i2s0_pa.dts
│  ├─ i2s0_pe.dts
│  ├─ spi1.dts
│  ├─ uart1.dts
│  ├─ uart2.dts
│  ├─ usbhost.dts
│  └─ usbhs.dts
└─ ext/
   ├─ cardkb.dts
   ├─ es8311_sound.dts
   └─ lsm6ds3_pre0.4.dts
```

四个子目录的职责如下：

| 目录 | 生成类型 | 职责 | 详细说明 |
| --- | --- | --- | --- |
| `base/` | DTB | 描述主板上始终存在的基础硬件，并提供其他覆盖层引用的节点和标签 | [base/README.md](base/README.md) |
| `screen/` | DTBO | 选择实体 LCD 对应的 ST7701 初始化序列和显示差异 | [screen/README.md](screen/README.md) |
| `interface/` | DTBO | 启用 SoC 控制器、选择引脚复用或切换 USB 工作模式 | [interface/README.md](interface/README.md) |
| `ext/` | DTBO | 声明连接在相应接口上的具体外部设备 | [ext/README.md](ext/README.md) |

## 设备树组合关系

Linux 最终使用的设备树不是某一个 `.dts` 文件单独编译后的结果，而是由基础 DTB 和若干 DTBO 在 U-Boot 中动态组合而成：

```text
Allwinner SUNIV SoC 定义
board/allwinner/suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi
        │
        ▼
CRA 公共主板定义
base/epass.dtsi
        │
        ▼
白银 v0.6 基础入口
base/devicetree.dts
        │
        ▼
screen overlay
        │
        ▼
interface overlay（零个或多个）
        │
        ▼
ext overlay（零个或多个）
        │
        ▼
U-Boot 将组合后的 DTB 交给 Linux
```

应用顺序固定为：

```text
base → screen → interface → ext
```

若多个覆盖层修改同一个属性，后应用的覆盖层可能覆盖前面的值。若多个覆盖层分别启用占用相同引脚的控制器，设备树语法仍可能通过，但 Linux 驱动会在运行时发生资源冲突。

## 基础设备树

Buildroot 配置通过：

```text
BR2_LINUX_KERNEL_DTS_SUPPORT=y
BR2_LINUX_KERNEL_CUSTOM_DTS_PATH="
    board/allwinner/suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi
    board/cra/epass/devicetree/linux/base/epass.dtsi
    board/cra/epass/devicetree/linux/base/devicetree.dts"
```

向 Linux 构建系统提供 SoC 公共定义、CRA 公共主板定义和当前板型入口。

基础文件的包含关系为：

```text
suniv-f1c100s.dtsi
        ↓
epass.dtsi
        ↓
devicetree.dts
```

其中：

- `epass.dtsi` 描述设备型号、显示链路、电源、背光、GPIO 复用、SPI-NAND、串口、SD、USB、视频引擎和基础外设状态。
- `devicetree.dts` 是白银 v0.6 当前唯一的基础入口，补充关机 GPIO、ST7701 初始化引脚和 LRADC 按键参数。

生成文件为：

```text
output/images/dt/base/devicetree.dtb
```

## 屏幕覆盖层

启动时必须选择一个与实体 LCD 相匹配的屏幕覆盖层：

| 启动值 | 源文件 | 主要区别 |
| --- | --- | --- |
| `screen=boe` | `screen/boe.dts` | BOE 屏幕 ST7701 初始化序列 |
| `screen=hsd` | `screen/hsd.dts` | HSD 屏幕 ST7701 初始化序列 |
| `screen=laowu` | `screen/laowu.dts` | 使用 HSD 初始化序列，并交换 TCON0 红、蓝通道 |

生成文件位于：

```text
output/images/dt/screen/boe.dtbo
output/images/dt/screen/hsd.dtbo
output/images/dt/screen/laowu.dtbo
```

`screen` 为空或名称不匹配时，U-Boot 无法从 FIT 镜像中提取正确的 `fdt-screen-*` 节点，显示初始化将不可用，启动也可能失败。

## 接口覆盖层

`interface/` 负责启用 SoC 内部控制器和选择引脚布局，不负责描述连接在总线上的具体外设。

可用名称如下：

| 启动值 | 作用 |
| --- | --- |
| `adc_pa1` | 在默认 PA0 基础上增加 PA1 ADC |
| `adc_pa123` | 启用 PA0～PA3 四路 ADC |
| `i2c0` | 启用硬件 I²C0 |
| `i2s0_pa` | 使用 PA 引脚布局启用 I²S0 |
| `i2s0_pe` | 使用 PE 引脚布局启用 I²S0 |
| `spi1` | 启用 SPI1 和预设的 Spidev 子设备 |
| `uart1` | 启用 UART1 |
| `uart2` | 启用 UART2 |
| `usbhost` | 将 USB OTG 控制器固定为主机模式 |
| `usbhs` | 请求启用项目定制的 USB High-Speed 模式 |

启动环境可以用空格分隔多个名称：

```text
interface=i2c0 uart1
```

U-Boot 会按书写顺序逐个应用。接口名称必须与源文件名和 FIT 节点后缀完全一致。

## 外接设备覆盖层

`ext/` 描述连接在主板接口上的具体外设。它们通常依赖一个接口覆盖层先启用相应控制器。

| 启动值 | 外接设备 | 主要依赖 |
| --- | --- | --- |
| `cardkb` | M5Stack Unit CardKB | `interface=i2c0` |
| `es8311_sound` | Everest ES8311 音频编解码器 | 选择一个合适的 I²S0 接口布局 |
| `lsm6ds3_pre0.4` | ST LSM6DS3 六轴惯性传感器 | `interface=i2c0`，并需处理 PE2 冲突 |

例如：

```text
interface=i2c0
ext=cardkb
```

多个外设名称同样使用空格分隔：

```text
ext=cardkb lsm6ds3_pre0.4
```

启用前必须检查供电、电平、总线地址、物理接线和引脚复用。设备树能够编译不代表该组合适合当前实体硬件。

## 编译过程

设备树由 Buildroot 镜像后处理脚本：

```text
board/cra/epass/scripts/mkdt.sh
```

统一生成。该脚本依次遍历 `base/`、`interface/`、`ext/` 和 `screen/` 中的 `.dts` 文件。

每个源文件先通过 C 预处理器展开：

```sh
cpp -nostdinc \
    -I "${BUILD_DIR}/linux-5.4.99/include/" \
    -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
    -P -undef -x assembler-with-cpp
```

随后通过：

```sh
dtc -@ -I dts -O dtb
```

生成 DTB 或 DTBO。

`-@` 会保留覆盖层所需的符号和修复信息，使 DTBO 能够引用基础设备树中的 `&i2c0`、`&i2s0`、`&pio`、`&uart1`、`&usb_otg` 等标签。

输出目录结构为：

```text
output/images/dt/
├─ base/
│  └─ devicetree.dtb
├─ screen/
│  ├─ boe.dtbo
│  ├─ hsd.dtbo
│  └─ laowu.dtbo
├─ interface/
│  └─ *.dtbo
└─ ext/
   └─ *.dtbo
```

`mkdt.sh` 当前将 Linux 源码目录写为 `linux-5.4.99`。若以后升级内核版本，必须同步修改脚本中的两个头文件搜索路径。

## 打包进 `boot.itb`

设备树生成后：

```text
board/cra/epass/scripts/kernel.its
```

将 Linux 内核、基础 DTB 和所有 DTBO 打包为 FIT 镜像：

```text
output/images/boot.itb
```

FIT 中使用以下节点命名规则：

| 文件类型 | FIT 节点 |
| --- | --- |
| Linux 内核 | `kernel` |
| 基础设备树 | `fdt-base` |
| 屏幕覆盖层 | `fdt-screen-<名称>` |
| 接口覆盖层 | `fdt-iface-<名称>` |
| 外接设备覆盖层 | `fdt-ext-<名称>` |

例如：

```text
screen/boe.dtbo
        ↓
fdt-screen-boe

interface/i2c0.dtbo
        ↓
fdt-iface-i2c0

ext/cardkb.dtbo
        ↓
fdt-ext-cardkb
```

`mkdt.sh` 会自动编译目录中的所有 `.dts`，但 `kernel.its` 不会自动发现新增文件。因此新增、删除或重命名覆盖层后，必须同步修改 `kernel.its`，否则文件虽然已经生成，也不会被打包进 `boot.itb`。

## U-Boot 启动时组合

U-Boot 默认环境位于：

```text
board/cra/epass/uboot.env
```

启动时先从 SPI-NAND 的 `0xFA000` 偏移读取文本环境，再从 `boot.itb` 提取基础设备树：

```text
imxtract $fitaddr fdt-base $dtbaddr
```

随后提取并应用屏幕覆盖层：

```text
imxtract $fitaddr fdt-screen-${screen} $dtboaddr
fdt apply $dtboaddr
```

接口和外设覆盖层通过循环依次处理：

```text
for ov in ${interface}
    imxtract $fitaddr fdt-iface-${ov} $dtboaddr
    fdt apply $dtboaddr

for ov in ${ext}
    imxtract $fitaddr fdt-ext-${ov} $dtboaddr
    fdt apply $dtboaddr
```

完整顺序为：

```text
读取 fdt-base
        ↓
应用 fdt-screen-${screen}
        ↓
依次应用 fdt-iface-${interface}
        ↓
依次应用 fdt-ext-${ext}
        ↓
补充 bootargs
        ↓
bootz
```

## 启动环境

仓库中的模板：

```text
board/cra/epass/uEnv.txt
```

当前包含：

```text
interface=
ext=
```

即默认不启用可选接口和外接设备。

模板本身没有设置 `screen=`。现有 `flash.py` 会在运行时生成 `.bootenv.txt`，并根据 `flash()` 或 `flash2()` 的第一个参数写入屏幕类型：

```text
screen=hsd
```

手动创建或修改启动环境时，必须写入有效的 `screen`。一个完整示例为：

```text
screen=hsd
interface=i2c0
ext=cardkb
```

启动环境中的名称必须与 `kernel.its` 中对应 FIT 节点的后缀完全一致，包括大小写、下划线和小数点。

## 常见依赖与冲突

以下内容只是总览，具体引脚和限制以各子目录 README 为准：

| 组合 | 说明 |
| --- | --- |
| `cardkb` + `i2c0` | CardKB 需要硬件 I²C0 |
| `lsm6ds3_pre0.4` + `i2c0` | LSM6DS3 需要硬件 I²C0 |
| `es8311_sound` + `i2s0_pa` 或 `i2s0_pe` | ES8311 音频数据需要 I²S0 |
| `i2s0_pa` 与 `i2s0_pe` | 两种 I²S0 引脚布局只能择一 |
| `adc_pa1` 与 `adc_pa123` | 两种 ADC 引脚范围通常只能择一 |
| `usbhost` 与 `usbhs` | 两者修改同一 USB 控制器，不能未经验证同时启用 |
| `lsm6ds3_pre0.4` 与关机控制 | 两者涉及 PE2，必须结合板级版本确认 |
| ADC、UART1、I²S0 PA 布局 | 部分配置会复用 PA1～PA3，必须逐项核对 |
| UART2、SPI1 | 部分引脚位于 PE7、PE8，组合前必须核对 |

U-Boot 不会自动检测这些硬件冲突。覆盖层成功应用只表示设备树结构可以合并，不表示 PCB 引脚、电气连接和 Linux 驱动一定能够同时工作。

## 添加新设备树配置

新增配置时建议按以下顺序操作：

1. 确认它属于 `base`、`screen`、`interface` 还是 `ext`。
2. 在对应目录创建 `.dts`，并使用基础设备树中已经存在的标签；若标签不存在，应先在 `base/` 中建立稳定定义。
3. 检查引脚复用、供电、电平、总线地址、中断和其他覆盖层的冲突。
4. 运行构建，确认 `mkdt.sh` 能生成对应 DTB 或 DTBO。
5. 在 `kernel.its` 中添加与命名规则一致的 FIT 节点。
6. 在启动环境中使用与 FIT 节点后缀完全一致的名称。
7. 检查 `boot.itb` 确实包含新节点。
8. 先保留可恢复镜像和串口日志，再进行实体设备验证。

不要直接编辑：

```text
output/images/dt/
```

该目录是构建生成物，下次运行 `mkdt.sh` 时会被删除并重新生成。所有长期修改都应写入本目录中的 `.dts`、`.dtsi` 或相关构建脚本。

## 构建与检查

完整构建使用：

```sh
make cra_epass_defconfig
make
```

若只需要重新生成相关镜像，可使用项目提供的重建流程，但仍应确认 Linux、设备树和 FIT 镜像之间没有使用旧缓存。

构建后重点检查：

```text
output/images/dt/base/devicetree.dtb
output/images/dt/screen/*.dtbo
output/images/dt/interface/*.dtbo
output/images/dt/ext/*.dtbo
output/images/boot.itb
```

可使用以下命令查看 FIT 镜像中的节点：

```sh
output/host/bin/mkimage -l output/images/boot.itb
```

可将生成的设备树反编译为文本进行核对：

```sh
dtc -I dtb -O dts \
    -o devicetree.decoded.dts \
    output/images/dt/base/devicetree.dtb
```

检查时应重点确认：

- 基础 DTB 包含覆盖层需要引用的符号。
- 所有 `kernel.its` 引用的文件都实际存在。
- `screen` 对应的 FIT 节点存在。
- `interface` 和 `ext` 中的每个名称都能映射到正确节点。
- 没有错误启用占用同一物理引脚的控制器。
- `boot.itb` 总大小没有超过 U-Boot `checkfit` 的 5 MiB 限制。

## 二次开发原则

- 修改基础节点标签前，必须搜索所有覆盖层对该标签的引用。
- 修改 `compatible` 时，必须同步检查对应的 Linux 驱动或内核补丁。
- 新增 `.dts` 后必须同步更新 `kernel.its`。
- 修改文件名后必须同步更新 FIT 节点、启动环境和文档。
- 不应把具体外接设备直接写入 `interface/`；接口与外设应保持分层。
- 不应为解决一个覆盖层冲突而在另一个覆盖层中静默覆盖无关属性。
- 不应仅凭 DTC 编译成功判断硬件组合可用。
- 所有由 Linux 或 Buildroot 执行的脚本必须保持 LF 换行。
- 修改后应先完成编译、FIT 节点检查和反编译检查，再进行实体设备验证。
