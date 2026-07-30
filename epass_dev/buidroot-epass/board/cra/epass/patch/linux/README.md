# CRA Electric Pass Linux 内核补丁

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 CRA Electric Pass 针对 Linux 5.4.99 增加的板级补丁。它们在 Buildroot 构建 Linux 内核时依次应用，用于补充 F1C100S/F1C200S 硬件支持、显示链路、屏幕初始化、USB、音频、键盘和 GPIO 用户空间接口。

## 构建位置

补丁目录由板级 Buildroot 配置指定：

```make
BR2_LINUX_KERNEL_PATCH="board/allwinner/suniv-f1c100s/patch/linux board/cra/epass/patch/linux"
```

实际应用顺序为：

```text
官方 Linux 5.4.99 源码
        │
        ▼
board/allwinner/suniv-f1c100s/patch/linux
        │
        ▼
board/cra/epass/patch/linux
        │
        ▼
board/cra/epass/linux.defconfig
        │
        ▼
交叉编译 Linux 内核与模块
```

## 文件概览

```text
patch/linux/
├─ 0000-f1c100s-gpadc-regs.patch
├─ 0001-epass-icon.patch
├─ 0002-panel-simple.patch
├─ 0003-f1c100s-defe-debe-fix.patch
├─ 0004-swap_rb_as_config.patch
├─ 0005-gpadc-low-freq.patch
├─ 0006-initalize-st7701.patch
├─ 0007-srgn-drm-atomic-ioctl.patch
├─ 0008-force-usb-fs-dt-switch.patch
├─ 0009-m5stack-cardkb-driver.patch
├─ 0010-i2s-and-es-driver.patch
├─ 0011-fbcon-cra-width-hack.patch
├─ 0012-gpio-backport-pulls.patch
└─ README.md
```

| 编号 | 主要功能 | 影响范围 |
| --- | --- | --- |
| `0000` | 修正 F1C100S GPADC 寄存器位定义 | MFD、ADC |
| `0001` | 替换 Linux 内核启动 Logo | framebuffer 启动图 |
| `0002` | 注册 CRA 384×640 LCD 面板时序 | DRM panel-simple |
| `0003` | 修正 DEFE/DEBE、缩放和 YUV 显示 | SUN4I DRM |
| `0004` | 支持通过设备树交换红蓝通道 | SUN4I TCON |
| `0005` | 调整 GPADC 采样分频和滤波 | ADC |
| `0006` | 新增 ST7701 GPIO 初始化驱动 | staging、显示 |
| `0007` | 新增项目私有 DRM 快速提交接口 | DRM UAPI、主程序 |
| `0008` | 通过设备树控制 USB Full/High-Speed | MUSB |
| `0009` | 新增 M5Stack CardKB I²C 键盘驱动 | staging、input |
| `0010` | 增加 ES 系列音频驱动并调整 I²S | ALSA SoC |
| `0011` | 缩减 framebuffer 控制台可用宽度 | fbcon |
| `0012` | 向 Linux 5.4 回移 GPIO 偏置配置接口 | GPIO UAPI、libgpiod |

## 补丁详细说明

### 0000：F1C100S GPADC 寄存器修正

目标文件：

```text
include/linux/mfd/sun4i-gpadc.h
```

该补丁调整 F1C100S/F1C200S 的 GPADC 和触摸控制寄存器位定义，包括校准、双点、工作模式、ADC 选择和通道选择字段。

它与 `0005` 共同构成当前项目的 ADC 适配基础。缺少本补丁时，PA0～PA3 对应的 ADC 通道可能无法按照实际芯片寄存器布局工作。

### 0001：内核启动 Logo

目标文件：

```text
drivers/video/logo/logo_linux_clut224.ppm
```

### 0002：CRA LCD 面板时序

目标文件：

```text
drivers/gpu/drm/panel/panel-simple.c
```

该补丁注册以下设备树兼容标识：

```dts
compatible = "cra,epass-panel";
```

当前公共显示模式为：

| 参数 | 数值 |
| --- | --- |
| 物理输出分辨率 | 384×640 |
| 像素时钟 | 24 MHz |
| 刷新率 | 约 60 Hz |
| 总线格式 | RGB565 |
| 色深描述 | 6 bpc |

对应设备树位于：

```text
board/cra/epass/devicetree/linux/base/
board/cra/epass/devicetree/linux/screen/
```

补丁还包含一项对 `foxlink_fl500wvr00_a0t` 面板物理宽度的修改。CRA 当前设备树不使用该面板，这一修改疑似原项目遗留的无关改动，后续整理补丁时应单独复核。

### 0003：F1C100S DEFE/DEBE 显示修正

主要目标文件：

```text
drivers/gpu/drm/sun4i/sun4i_backend.c
drivers/gpu/drm/sun4i/sun4i_frontend.c
drivers/gpu/drm/sun4i/sun4i_frontend.h
drivers/gpu/drm/sun4i/sunxi_detab.h
```

该补丁负责：

- 修正 DEBE packed-YUV 帧缓冲地址单位；
- 配置 DEFE 缩放器；
- 增加缩放滤波系数表；
- 增加 YUV 到 RGB 的 CSC 参数；
- 处理前端寄存器访问和初始化；
- 为项目的视频图层显示提供底层支持。

这是显示链路中较重要、也较难维护的补丁。部分初始化参数与当前 384×640 输出、YUV422 数据和厂商 BSP 表格紧密相关，不适合作为通用 SUN4I DRM 实现直接移植到其他板型。

### 0004：红蓝通道交换

主要目标文件：

```text
drivers/gpu/drm/sun4i/sun4i_tcon.c
drivers/gpu/drm/sun4i/sun4i_tcon.h
```

该补丁读取项目专用设备树属性：

```dts
cra,swap-b-r;
```

属性存在时，驱动设置 TCON0 的颜色交换位，用于处理部分屏幕红蓝通道接线或初始化差异。当前 `laowu.dts` 使用该属性。

### 0005：GPADC 采样参数调整

目标文件：

```text
drivers/iio/adc/sun4i-gpadc-iio.c
```

该补丁修改 GPADC 的采样分频和滤波类型，用于改善电池电压等低速模拟量的稳定性。

补丁名中的 `low-freq` 是原项目对修改目的的概括。实际采样频率仍取决于芯片时钟、寄存器分频公式和驱动配置，不能只根据文件名判断最终采样率。

### 0006：ST7701 初始化驱动

该补丁新增：

```text
include/dt-bindings/display/st7701initseq.h
drivers/staging/cra/Kconfig
drivers/staging/cra/Makefile
drivers/staging/cra/st7701init.c
```

当前配置符号为：

```text
CONFIG_CRA_EP_STAGING
CONFIG_CRA_EP_ST7701_INIT
```

驱动匹配：

```dts
compatible = "cra,st7701-initseq";
```

它通过 SDA、SCL、CS 和可选 RST GPIO，以软件时序向 ST7701 发送初始化命令。具体命令序列不写死在驱动中，而是由各屏幕设备树的 `init-sequence` 数组提供。

驱动使用工作队列异步执行初始化。当前实现对必需 GPIO、序列长度和异常操作码的检查仍不够严格，修改设备树初始化序列时应谨慎核对数组边界。

### 0007：私有 DRM 原子提交接口

主要目标文件：

```text
drivers/gpu/drm/sun4i/sun4i_backend.c
drivers/gpu/drm/sun4i/sun4i_drv.c
drivers/gpu/drm/sun4i/sun4i_drv.h
include/uapi/drm/srgn_drm.h
```

该补丁增加两个私有 DRM IOCTL：

```text
DRM_IOCTL_SRGN_ATOMIC_COMMIT
DRM_IOCTL_SRGN_RESET_FB_CACHE
```

支持的操作包括：

- 挂载普通 RGB 帧缓冲；
- 挂载 YUV 帧缓冲；
- 设置图层坐标；
- 设置图层全局透明度；
- 清除用户虚拟地址到物理地址的缓存。

当前 `drm_app_neo` 的渲染、视频播放和叠加层代码直接依赖这个接口。因此，本补丁虽然仍保留 `SRGN` 命名，但不能只在内核侧进行简单替换。

若以后迁移为 CRA 命名，至少需要同步修改：

```text
本补丁中的 UAPI 头文件和 DRM 注册代码
drm_app_neo/src/driver/srgn_drm.h
drm_app_neo/src/driver/drm_warpper.c
drm_app_neo/src/render/
drm_app_neo/src/overlay/
```

IOCTL 数字、结构体布局和字段宽度共同构成内核与用户空间 ABI。迁移时必须保持两侧完全一致。

当前实现会直接操作显示寄存器，并通过用户虚拟地址获取物理页地址。其锁、VMA 状态恢复、缓存生命周期和进程隔离均不够完善，是本目录维护风险最高的补丁。

### 0008：USB 速度设备树开关

主要目标文件：

```text
drivers/usb/musb/sunxi.c
drivers/usb/musb/musb_core.c
```

该补丁读取：

```dts
cra,usb-hs-enabled;
```

未设置属性时默认限制为 USB Full-Speed；设置属性后允许 MUSB 尝试 High-Speed。对应覆盖层为：

```text
board/cra/epass/devicetree/linux/interface/usbhs.dts
```

当前补丁中局部变量 `power` 在位运算前没有可靠初始化，可能使 MUSB `POWER` 寄存器的其他位带入未定义值。这是已知功能性问题，在修复并验证 PCB 高速信号完整性前，不应仅为了提高标称速率而默认启用 High-Speed。

### 0009：M5Stack CardKB 驱动

该补丁新增：

```text
drivers/staging/cra/cardkb.c
```

并向 CRA staging Kconfig 和 Makefile 增加：

```text
CONFIG_CRA_EP_CARDKB
```

驱动通过 I²C 定期轮询 M5Stack Unit CardKB，将键盘控制器返回的字符码转换为 Linux input 子系统按键事件。

当前板级配置将它编译为模块：

```text
CONFIG_CRA_EP_CARDKB=m
```

使用时需要同时启用 I²C0 接口覆盖层和 `cardkb` 外设覆盖层。

### 0010：I²S 与 ES 系列音频驱动

该补丁主要包括：

- 为 SUNIV I²S 模块时钟增加父时钟联动；
- 调整 I²S 播放和录音 DMA `maxburst`；
- 增加 ES8156 驱动；
- 增加 ES8311 驱动；
- 增加 ES8375 驱动；
- 增加 ES8389 驱动。

当前项目实际配置为：

```text
CONFIG_SND_SUN4I_I2S=m
CONFIG_SND_SOC_ES8311=m
CONFIG_SND_SIMPLE_CARD=m
```

### 0011：framebuffer 控制台宽度规避

目标文件：

```text
drivers/video/fbdev/core/fbcon.c
```

该补丁通过 `CRA_FB_CONSOLE_WIDTH_HACK` 将 framebuffer 控制台计算出的宽度减少三个字符单元，用于避开屏幕最右侧约 24 像素的异常区域。

它只影响 Linux 文本控制台，不会改变 LVGL 或 `drm_app_neo` 的界面宽度。

这是对通用 `fbcon` 的全局修改，并未限定具体屏幕或设备树。若以后修复显示时序或确认右侧区域可以正常使用，应重新评估是否还需要该补丁。

### 0012：GPIO 偏置接口回移

该补丁将较新 Linux 中的部分 GPIO 字符设备功能回移到 Linux 5.4.99，包括：

- 上拉；
- 下拉；
- 禁用偏置；
- 运行时重新配置 GPIO line；
- `GPIOHANDLE_SET_CONFIG_IOCTL`；
- GPIO 偏置状态报告；
- 相关 pin range 和 fwnode 支持。

## 功能依赖关系

### 显示链路

```text
0002 面板时序
  │
  ├─ 0004 红蓝通道交换
  │
  └─ 0006 ST7701 初始化
          │
          ▼
0003 DEFE/DEBE 与缩放修正
          │
          ▼
0007 drm_app_neo 私有图层接口
          │
          └─ 0011 仅处理 framebuffer 文本控制台宽度
```

### ADC 链路

```text
0000 寄存器位定义
        │
        ▼
0005 采样和滤波参数
        │
        ▼
电池电压及外部 ADC 输入
```

### 扩展设备

```text
I²C0 + cardkb.dts
        └─ 0009 CardKB 驱动

I²S + es8311_sound.dts
        └─ 0010 I²S 与 ES8311 驱动

libgpiod
        └─ 0012 GPIO 偏置和重新配置接口
```

## 修改补丁后的构建方法

在 Buildroot 根目录、Linux/WSL 环境中执行：

```bash
make cra_epass_defconfig
make linux-dirclean
make linux
```

修改补丁后应使用 `linux-dirclean`，使 Buildroot 重新解压内核源码并从头应用补丁。只执行：

```bash
make linux-rebuild
```

通常不会重新应用已经越过 patch 阶段的补丁，容易造成“补丁文件已经修改，但输出目录仍是旧代码”的误判。

完整系统构建可继续执行：

```bash
make
```

这些命令只构建文件，不会自动将镜像写入实体设备。刷写、替换实体设备文件和上传主程序属于独立操作。

## 修改和验证原则

1. 不要直接长期修改 `output/build/linux-5.4.99/`。该目录是生成物，执行 `linux-dirclean` 后会被删除。
2. 永久修改应写入本目录补丁，或在升级内核时重新整理为可追踪的补丁提交。
3. 新补丁应使用四位数字编号，并明确依赖顺序。
4. 所有补丁文件统一使用 LF 行尾，避免 Windows CRLF 导致后续补丁上下文匹配失败。
5. 修改 Kconfig 符号时，必须同步修改 `board/cra/epass/linux.defconfig`。
6. 修改 `cra,*` 设备树属性或兼容字符串时，必须同步修改相应 DTS/DTSI 和内核驱动。
7. 修改 DRM 私有 UAPI 时，必须同步修改 `drm_app_neo`，并重新编译内核和主程序。
8. 不要删除原作者署名；CRA 的修改记录和维护者信息应以新增说明或 Git 历史表达。
9. 不应因为某补丁可以成功应用，就直接认定其运行逻辑安全。补丁重放、Kconfig、编译和实体硬件验证是不同阶段。

## 当前验证状态

当前目录已经完成以下检查：

- 所有 Linux 补丁可以作为统一 diff 正确解析；
- 所有 CRA Linux 和 U-Boot 补丁均已统一为 LF；
- 可以在干净 Linux 5.4.99 源码上先应用公共 SUNIV 补丁，再完整应用本目录补丁；
- `make ARCH=arm olddefconfig` 可以识别 `CONFIG_CRA_EP_*` 配置项；
- CRA staging 驱动目录、面板兼容字符串和 USB 属性可以正确出现在应用补丁后的源码中。

上述检查证明当前补丁序列和 Kconfig 关系有效，但不能替代完整内核编译及实体硬件测试。

