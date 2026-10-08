<div align="center">

# Buildroot 启动程序构建目录

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `boot/` 保存 Buildroot 对引导程序、第一阶段启动固件和可信执行环境固件的通用构建支持。
>
> CRA Electric Pass 在本目录中实际使用 U-Boot；板级配置、设备树、补丁和最终镜像逻辑位于 `board/cra/epass/` 及 Allwinner 公共层。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#顶层文件">顶层文件</a> ·
  <a href="#uboot-目录">U-Boot</a> ·
  <a href="#cra-electric-pass-的实际-u-boot-配置">CRA 配置</a> ·
  <a href="#cra-启动链">启动链</a> ·
  <a href="#源文件与生成物边界">维护边界</a> ·
  <a href="#常用构建命令">构建命令</a>
</p>

---

## 目录结构

```text
boot/
├── Config.in
├── common.mk
├── afboot-stm32/
├── arm-trusted-firmware/
├── at91bootstrap/
├── at91bootstrap3/
├── at91dataflashboot/
├── barebox/
├── binaries-marvell/
├── boot-wrapper-aarch64/
├── grub2/
├── gummiboot/
├── lpc32xxcdl/
├── mv-ddr-marvell/
├── mxs-bootlets/
├── opensbi/
├── optee-os/
├── s500-bootloader/
├── shim/
├── syslinux/
├── uboot/
└── vexpress-firmware/
```

---

## 顶层文件

### `Config.in`

该文件创建 Buildroot 配置界面中的 `Bootloaders` 菜单，并依次载入各子目录的 `Config.in`。

它只负责组织配置入口，不会让所有引导程序都参与构建。某个包只有在对应的 `BR2_TARGET_*` 选项启用且架构条件满足时才会被选择。

### `common.mk`

文件内容为：

```make
include $(sort $(wildcard boot/*/*.mk))
```

它按名称排序并载入各启动程序包的 `.mk` 文件，使这些包的下载、配置、编译和安装规则进入 Buildroot 主构建系统。

---

## 子目录说明

| 子目录 | 主要用途 |
| :--- | :--- |
| `afboot-stm32/` | STM32 平台使用的小型引导程序。 |
| `arm-trusted-firmware/` | ARMv7-A/ARMv8-A 平台的 Trusted Firmware-A/ATF 启动阶段。 |
| `at91bootstrap/` | Atmel AT91 的旧版第一阶段引导程序。 |
| `at91bootstrap3/` | Atmel/Microchip AT91 的第三代第一阶段引导程序。 |
| `at91dataflashboot/` | AT91 DataFlash 启动支持。 |
| `barebox/` | Barebox 引导程序及其辅助组件。 |
| `binaries-marvell/` | Marvell Armada 平台构建 ATF 所需的 SCP 固件。 |
| `boot-wrapper-aarch64/` | 在 AArch64 软件模拟器中启动内核的轻量包装器。 |
| `grub2/` | x86、EFI 及部分 ARM 平台使用的 GNU GRUB 2。 |
| `gummiboot/` | x86 UEFI 系统的简单启动管理器。 |
| `lpc32xxcdl/` | NXP LPC32xx 的 kickstart 和 S1L 引导组件。 |
| `mv-ddr-marvell/` | Marvell Armada ATF 所需的 DDR Training 源码。 |
| `mxs-bootlets/` | Freescale/NXP i.MX23、i.MX28 第一阶段 Bootlets。 |
| `opensbi/` | RISC-V SBI 固件实现。 |
| `optee-os/` | ARM TrustZone 安全世界镜像和 TA 开发组件。 |
| `s500-bootloader/` | Actions Semiconductor S500 第一阶段引导程序。 |
| `shim/` | UEFI Secure Boot 环境中的签名链式加载器。 |
| `syslinux/` | x86 BIOS、PXE、ISO 和 EFI 启动程序集合。 |
| `uboot/` | U-Boot 通用构建集成。 |
| `vexpress-firmware/` | ARM Versatile Express 平台固件。 |

---

## 子目录中的常见文件

每个启动程序包通常包含以下几类文件：

| 类型 | 作用 |
| --- | --- |
| `Config.in` | 定义是否启用、版本、平台、输出格式及附加选项 |
| `<包名>.mk` | 定义源码来源、依赖、构建命令和安装位置 |
| `<包名>.hash` | 保存源码包和许可证文件的校验值 |
| `*.patch` | 修复特定上游版本的兼容性、构建或安全问题 |
| 配置和资源文件 | 供 GRUB、Gummiboot 等特定包生成最终启动配置 |

这些文件大部分来自 Buildroot 上游，是构建输入而非生成物。某个补丁是否应用，取决于被选择的包版本及 Buildroot 的版本补丁规则。

---

## `uboot/` 目录

`boot/uboot/` 是 CRA Electric Pass 与本目录的实际连接点：

```text
boot/uboot/
├── Config.in
├── uboot.mk
├── uboot.hash
├── 2015.07/
├── 2016.07/
└── 2016.09.01/
```

### `uboot/Config.in`

该文件定义 U-Boot 的 Buildroot 配置项，包括：

- U-Boot 版本和源码来源；
- Kconfig 或旧式构建系统；
- 上游 defconfig 或自定义配置文件；
- 附加配置片段和补丁目录；
- DTC、OpenSSL、Python 等主机依赖；
- `u-boot.bin`、`u-boot.img`、`u-boot.itb` 等输出格式；
- SPL/TPL 输出文件；
- U-Boot 环境镜像；
- `boot.scr` 启动脚本；
- 构建前复制的自定义 U-Boot DTS/DTSI；
- 附加 Make 参数。

### `uboot/uboot.mk`

该文件负责：

1. 根据配置计算 U-Boot 版本、源码包和下载地址；
2. 声明交叉工具链与主机工具依赖；
3. 下载远程补丁并按顺序应用本地补丁目录；
4. 读取自定义 U-Boot 配置；
5. 把自定义 DTS/DTSI 复制到临时 U-Boot 源码树；
6. 调用 U-Boot Makefile 完成编译；
7. 把选定的 U-Boot 二进制和 SPL 文件复制到 `output/images/`；
8. 按需生成环境镜像或 `boot.scr`；
9. 提供 `uboot-menuconfig`、配置保存、重建和清理目标。

### `uboot/uboot.hash`

该文件保存 Buildroot 默认 U-Boot 源码包和许可证文件的 SHA-256 值。当前版本记录的是 Buildroot 2020.02.7 默认使用的 U-Boot 2020.01

### 版本补丁目录

`2015.07/`、`2016.07/` 和 `2016.09.01/` 中保存旧 U-Boot 版本的 Buildroot 兼容补丁，主要处理旧版本与 ARC 工具链或构建配置的兼容问题。

---

## CRA Electric Pass 的实际 U-Boot 配置

`board/cra/epass/cra_epass_defconfig` 当前选择：

| 项目 | 当前值 |
| --- | --- |
| 引导程序 | U-Boot |
| U-Boot 版本 | `2020.07` |
| 构建系统 | Kconfig |
| U-Boot 配置 | `board/cra/epass/uboot.defconfig` |
| 公共补丁目录 | `board/allwinner/suniv-f1c100s/patch/u-boot` |
| CRA 补丁目录 | `board/cra/epass/patch/uboot` |
| 设备树编译器 | 需要 DTC |
| 默认 U-Boot 输出 | `u-boot.bin` |
| SPL | 启用 |
| SPL/组合镜像 | `u-boot-sunxi-with-spl.bin` |
| 公共 U-Boot DTSI | `board/allwinner/suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi` |
| CRA U-Boot DTS | `board/cra/epass/devicetree/uboot/suniv-f1c100s-generic.dts` |

`board/cra/epass/uboot.defconfig` 进一步启用了 SUNIV、SPL、SPI、MTD、DFU、USB Mass Storage 和 USB Gadget 等功能，并指定：

```text
CONFIG_DEFAULT_DEVICE_TREE="suniv-f1c100s-generic"
CONFIG_DEFAULT_ENV_FILE="../../../board/cra/epass/uboot.env"
CONFIG_BOOTCOMMAND="run distro_bootcmd;"
```

因此，CRA 启动行为不能只从 `boot/uboot/` 判断，还必须同时查看：

```text
board/cra/epass/cra_epass_defconfig
board/cra/epass/uboot.defconfig
board/cra/epass/uboot.env
board/cra/epass/uEnv.txt
board/cra/epass/devicetree/uboot/
board/cra/epass/patch/uboot/
board/allwinner/suniv-f1c100s/patch/u-boot/
```

---

## CRA 启动链

CRA Electric Pass 的简化启动关系为：

```text
Allwinner SUNIV Boot ROM
          │
          ▼
     U-Boot SPL
  初始化时钟和 DRAM
          │
          ▼
      完整 U-Boot
  读取默认环境和外部环境
          │
          ▼
       boot.itb
  提取 Linux、基础 DTB 和 DTBO
          │
          ▼
     Linux 5.4.99
          │
          ▼
       CRA rootfs
```

Buildroot 首先生成 `u-boot-sunxi-with-spl.bin`。CRA 的 post-image 脚本随后依据 SPI-NAND 页和块布局生成：

```text
output/images/u-boot-sunxi-with-nand-spl.bin
```

另一个 post-image 阶段将 Linux 内核、基础 DTB 和各类设备树 Overlay 打包为 `boot.itb`。这些镜像生成逻辑属于 `board/cra/epass/scripts/`，不是由根目录 `boot/` 单独完成的。

---

## 补丁应用顺序

CRA 的 U-Boot 补丁路径按以下顺序配置：

```text
board/allwinner/suniv-f1c100s/patch/u-boot
board/cra/epass/patch/uboot
```

`uboot.mk` 会按列表顺序处理本地文件或目录。目录中的 `*.patch` 按稳定排序应用，因此：

- SUNIV/F1C100S/F1C200S 公共支持先应用；
- CRA Electric Pass 专用修复后应用；
- CRA 补丁可以建立在公共补丁提供的功能之上；
- 新增补丁应使用连续、可排序的编号；
- 不应把 CRA 专用补丁放进 `boot/uboot/`。

---

## 源文件与生成物边界

### 应当纳入版本控制

| 内容 | 规范位置 |
| --- | --- |
| Buildroot 启动程序包规则 | `boot/` |
| CRA Buildroot 目标配置 | `board/cra/epass/cra_epass_defconfig` |
| CRA U-Boot 配置 | `board/cra/epass/uboot.defconfig` |
| CRA 默认 U-Boot 环境 | `board/cra/epass/uboot.env` |
| CRA 外部环境模板 | `board/cra/epass/uEnv.txt` |
| CRA U-Boot 设备树 | `board/cra/epass/devicetree/uboot/` |
| CRA U-Boot 补丁 | `board/cra/epass/patch/uboot/` |
| SUNIV 公共补丁 | `board/allwinner/suniv-f1c100s/patch/u-boot/` |
| CRA 镜像生成脚本 | `board/cra/epass/scripts/` |

### 构建时生成，不应直接维护

| 内容 | 典型位置 |
| --- | --- |
| 解压并打过补丁的 U-Boot 源码 | `output/build/uboot-2020.07/` |
| 临时 U-Boot `.config` | `output/build/uboot-2020.07/.config` |
| 编译生成的对象和中间文件 | `output/build/uboot-2020.07/` |
| 标准 U-Boot 二进制 | `output/images/u-boot.bin` |
| SUNIV SPL/U-Boot 组合镜像 | `output/images/u-boot-sunxi-with-spl.bin` |
| SPI-NAND 启动镜像 | `output/images/u-boot-sunxi-with-nand-spl.bin` |

可以在 `output/build/uboot-2020.07/` 中进行临时诊断，但其中的手工修改会在清理或重新解压后丢失。正式修改应写回 CRA 配置、环境、设备树或补丁目录。

---

## 常用构建命令

以下命令应在 Linux 或配置正确的 WSL 环境中，从 Buildroot 仓库根目录执行。

### 载入 CRA 配置并完整构建

```sh
make cra_epass_defconfig
make -j$(nproc)
```

完整构建还会运行 CRA post-image 脚本，生成适用于 SPI-NAND 和 SD 启动的最终镜像。构建不会自动烧录实体设备。

### 打开 U-Boot 配置界面

```sh
make uboot-menuconfig
```

该命令修改临时构建树中的 U-Boot `.config`。验证完成后，应保存到项目的规范配置文件。

### 保存 U-Boot defconfig

```sh
make uboot-update-defconfig
```

当前 CRA 配置使用 `BR2_TARGET_UBOOT_CUSTOM_CONFIG_FILE`，因此保存目标为：

```text
board/cra/epass/uboot.defconfig
```

如确实需要保存完整 `.config`，可以使用：

```sh
make uboot-update-config
```

本项目通常应优先维护精简、可审查的 `uboot.defconfig`。

### 仅重新编译 U-Boot

```sh
make uboot-rebuild -j$(nproc)
make
```

第一条命令从 U-Boot 编译阶段重新执行；第二条命令让后续镜像和 post-image 流程根据新的 U-Boot 产物刷新。

`uboot-rebuild` 不会重新下载、重新解压或从头应用补丁。

### 从干净 U-Boot 源码树重建

```sh
make uboot-dirclean
make -j$(nproc)
```

下列情况应使用 `uboot-dirclean`：

- 新增、删除、重命名或调整 U-Boot 补丁；
- 修改已有补丁后需要从干净源码验证；
- 切换 U-Boot 版本或源码来源；
- 临时构建树已经被手动修改；
- 怀疑旧构建状态影响结果。

该命令只清除 Buildroot 的 U-Boot 构建目录，不会擦写实体设备。

---

<div align="center">

<sub><b>boot/</b> · bootloader build infrastructure for Buildroot</sub>

</div>
