# CRA Electric Pass 板级补丁

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 CRA Electric Pass 在 Buildroot 构建过程中应用到 Linux 5.4.99 和 U-Boot 2020.07 的板级源码补丁。

这些补丁用于补充上游版本尚未包含的 F1C100S/F1C200S 支持，并实现当前硬件所需的显示、屏幕初始化、ADC、USB、SPI-NAND、音频、键盘、GPIO 和 DFU 功能。

## 目录结构

```text
patch/
├─ linux/
│  ├─ 0000-f1c100s-gpadc-regs.patch
│  ├─ 0001-epass-icon.patch
│  ├─ 0002-panel-simple.patch
│  ├─ 0003-f1c100s-defe-debe-fix.patch
│  ├─ 0004-swap_rb_as_config.patch
│  ├─ 0005-gpadc-low-freq.patch
│  ├─ 0006-initalize-st7701.patch
│  ├─ 0007-srgn-drm-atomic-ioctl.patch
│  ├─ 0008-force-usb-fs-dt-switch.patch
│  ├─ 0009-m5stack-cardkb-driver.patch
│  ├─ 0010-i2s-and-es-driver.patch
│  ├─ 0011-fbcon-cra-width-hack.patch
│  ├─ 0012-gpio-backport-pulls.patch
│  └─ README.md
├─ uboot/
│  ├─ 0001-uart-pull.patch
│  ├─ 0002-musb-force-fs.patch
│  ├─ 0003-spi-nand-mx35lf1g.patch
│  ├─ 0004-f1c-spi-fix.patch
│  ├─ 0005-dfu-verify-block.patch
│  └─ README.md
└─ README.md
```

| 子目录 | 目标源码 | 补丁数量 | 详细说明 |
| --- | --- | ---: | --- |
| `linux/` | Linux 5.4.99 | 13 | [Linux 补丁说明](linux/README.md) |
| `uboot/` | U-Boot 2020.07 | 5 | [U-Boot 补丁说明](uboot/README.md) |

## Buildroot 配置入口

补丁路径由：

```text
board/cra/epass/cra_epass_defconfig
```

中的以下配置指定：

```make
BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE="5.4.99"
BR2_LINUX_KERNEL_PATCH="board/allwinner/suniv-f1c100s/patch/linux board/cra/epass/patch/linux"
BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE="board/cra/epass/linux.defconfig"

BR2_TARGET_UBOOT_CUSTOM_VERSION_VALUE="2020.07"
BR2_TARGET_UBOOT_PATCH="board/allwinner/suniv-f1c100s/patch/u-boot board/cra/epass/patch/uboot"
BR2_TARGET_UBOOT_CUSTOM_CONFIG_FILE="board/cra/epass/uboot.defconfig"
```

Linux 和 U-Boot 都采用两级补丁结构：

```text
上游官方源码
      │
      ▼
公共 SUNIV/F1C100S 补丁
      │
      ▼
CRA Electric Pass 板级补丁
      │
      ▼
板级 defconfig
      │
      ▼
编译产物
```

公共补丁提供 SoC 级基础支持，本目录补丁处理 CRA Electric Pass 所需的板级功能和原项目遗留适配。两级补丁存在上下文依赖，不能只取本目录补丁直接应用到未经过公共补丁处理的源码。

## 补丁应用顺序

Buildroot 会按照补丁文件名的字典顺序依次应用：

```text
0000
0001
0002
……
0012
```

同一子目录中，后面的补丁可以修改前面补丁新建或改动的文件。例如：

```text
Linux 0006
  └─ 新建 drivers/staging/cra/ 和基础 Kconfig

Linux 0009
  └─ 向同一目录追加 CardKB 驱动和 Kconfig 选项
```

因此：

- 编号不仅用于排序，也表达补丁之间的依赖；
- 重命名或移动补丁可能改变应用顺序；
- 修改前一个补丁的上下文时，需要检查后续补丁是否仍能匹配；
- 所有补丁成功解析，不代表整组补丁一定可以按顺序应用。

## Linux 补丁职责

Linux 补丁主要覆盖以下功能：

| 功能 | 相关补丁 |
| --- | --- |
| GPADC 寄存器、采样和滤波 | `0000`、`0005` |
| 内核 framebuffer 启动 Logo | `0001` |
| 384×640 面板时序 | `0002` |
| DEFE/DEBE、缩放和 YUV 显示 | `0003` |
| 红蓝通道交换 | `0004` |
| ST7701 GPIO 初始化 | `0006` |
| `drm_app_neo` 私有 DRM 接口 | `0007` |
| Linux MUSB Full/High-Speed 选择 | `0008` |
| M5Stack CardKB | `0009` |
| I²S 和 ES8311 等音频 codec | `0010` |
| framebuffer 文本控制台宽度规避 | `0011` |
| GPIO 上拉、下拉和运行时配置 | `0012` |

Linux 补丁中风险较高的部分包括：

- `0003` 中与当前分辨率和厂商 BSP 表格绑定的显示代码；
- `0007` 中直接操作 DRM 寄存器和用户内存映射的私有接口；
- `0008` 中尚未修复的 MUSB `power` 局部变量初始化问题；
- `0011` 对通用 framebuffer 控制台的全局修改；
- `0012` 对 Linux GPIO 核心和 UAPI 的回移。

详细实现、依赖和已知问题见：

```text
linux/README.md
```

## U-Boot 补丁职责

U-Boot 补丁主要覆盖以下功能：

| 功能 | 相关补丁 |
| --- | --- |
| UART0 TX/RX 上拉 | `0001` |
| U-Boot MUSB Gadget 强制 Full-Speed | `0002` |
| SPL 识别 Macronix MX35LF1G SPI-NAND | `0003` |
| SUNIV SPI 父时钟和分频计算 | `0004` |
| DFU 写后校验、坏块标记和跳过 | `0005` |

它们参与的启动链路为：

```text
设备上电
   │
   ▼
SPL 初始化 DRAM、UART0 和 SPI0
   │
   ▼
识别 SPI-NAND 并加载主 U-Boot
   │
   ▼
主 U-Boot 读取环境与 boot.itb
   │
   ├─ 正常启动 Linux
   │
   └─ 进入 USB DFU
          └─ 写入、读回校验和坏块处理
```

详细实现、分区关系和 NAND 镜像后处理见：

```text
uboot/README.md
```

## 与其他目录的关系

本目录不能脱离以下文件单独维护。

### 内核和 U-Boot 配置

```text
board/cra/epass/linux.defconfig
board/cra/epass/uboot.defconfig
```

补丁增加新的 Kconfig 功能后，必须由对应 defconfig 选择，否则代码可能存在于源码树中但不会被编译。

### 设备树

```text
board/cra/epass/devicetree/linux/
board/cra/epass/devicetree/uboot/
```

以下内容必须在设备树和补丁之间保持一致：

- `cra,epass-panel`；
- `cra,st7701-initseq`；
- `cra,swap-b-r`；
- `cra,usb-hs-enabled`；
- SPI0 和 SPI-NAND 节点；
- `spi-max-frequency`；
- CardKB、ES8311 和其他扩展设备节点。

### 主程序

Linux `0007-srgn-drm-atomic-ioctl.patch` 与：

```text
drm_app_neo/src/driver/srgn_drm.h
drm_app_neo/src/driver/drm_warpper.c
drm_app_neo/src/render/
drm_app_neo/src/overlay/
```

共同定义内核与用户空间之间的私有 DRM ABI。修改 IOCTL 编号、结构体、字段宽度或命令含义时，必须同步修改并重新编译两侧。

### 镜像脚本

```text
board/cra/epass/scripts/mknanduboot.sh
board/cra/epass/scripts/mkdt.sh
board/cra/epass/scripts/buildimage.sh
```

补丁只负责修改源码。设备树编译、NAND SPL 布局转换和最终镜像生成由脚本完成，是独立的构建阶段。

## 修改补丁后的正确构建方法

### 修改 Linux 补丁

在 Buildroot 根目录、Linux/WSL 环境中执行：

```bash
make cra_epass_defconfig
make linux-dirclean
make linux
```

### 修改 U-Boot 补丁

```bash
make cra_epass_defconfig
make uboot-dirclean
make uboot
```

### 完整系统构建

```bash
make
```

补丁发生变化后需要执行对应的 `*-dirclean`，使 Buildroot 重新解压源码并重新应用补丁。只执行：

```bash
make linux-rebuild
make uboot-rebuild
```

通常不会重新执行已经完成的 patch 阶段，可能继续使用旧的 `output/build/` 源码。

这些构建命令只生成文件，不会自动写入实体设备。刷写完整镜像、替换 U-Boot、更新内核或上传主程序均属于独立操作。

## 行尾和 Windows 检出

本项目曾因 Windows 检出产生以下问题：

- Shell 脚本出现 CRLF，导致 `/bin/sh^M`；
- Git 符号链接变成包含目标路径文字的普通文件；
- 不同补丁混用 CRLF 和 LF；
- 前一个补丁创建 LF 文件，后一个 CRLF 补丁无法匹配上下文。

当前本目录所有 Linux 和 U-Boot 补丁均已统一为 LF。

新增或修改补丁后，可在提交前检查：

```bash
file board/cra/epass/patch/linux/*.patch
file board/cra/epass/patch/uboot/*.patch
```

不应通过关闭 Buildroot 哈希验证、跳过失败补丁或直接修改 `output/build/` 来掩盖检出问题。

## 二次开发原则

1. 永久修改应记录在补丁、配置、设备树或可追踪的上游提交中。
2. 不要长期直接修改 `output/build/linux-5.4.99/` 或 `output/build/uboot-2020.07/`。
3. 新补丁应使用四位数字编号，并注明依赖和影响范围。
4. 修改前一个补丁后，应重新检查所有后续补丁。
5. Kconfig、defconfig、设备树和驱动必须保持一致。
6. 私有 UAPI 必须保持内核和用户空间 ABI 一致。
7. 所有文本补丁统一使用 LF。
8. 保留原作者署名和许可证信息。
9. 不要把补丁应用成功等同于代码编译成功。
10. 不要把编译成功等同于实体硬件运行正常。
11. 构建验证不授权刷写或修改实体设备。

## 当前验证状态

当前目录已经完成以下检查：

- 13 个 Linux 补丁可以作为统一 diff 正确解析；
- 5 个 U-Boot 补丁可以作为统一 diff 正确解析；
- 所有补丁均已统一为 LF；
- 公共 SUNIV Linux 补丁和 CRA Linux 补丁可以在干净 Linux 5.4.99 源码上按顺序应用；
- Linux Kconfig 可以识别 `CONFIG_CRA_EP_*` 配置；
- 公共 SUNIV U-Boot 补丁和 CRA U-Boot 补丁可以按 Buildroot 顺序应用；
- 当前 U-Boot 补丁已经完成过编译验证。

以上检查证明当前补丁序列、命名和配置关系可以继续用于构建，但不能替代完整系统构建、实体设备启动、显示、USB、音频、SPI-NAND 和 DFU 测试。
