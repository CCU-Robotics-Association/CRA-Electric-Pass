# Buildroot 架构配置

Read this in other languages: [English](README_EN.md), [中文](README.md).

本目录为 Buildroot 的目标架构配置层，用于描述 Buildroot 支持的处理器架构、CPU 核心、指令集、ABI、字节序、浮点模式和相关工具链参数。

CRA Electric Pass 当前仅使用其中的 32 位 ARM 配置，目标处理器为 Allwinner F1C200S 内的 ARM926EJ-S。

> 此处的 `arch/` 并非指 Linux 内核源码中的 `arch/arm/`，也不包含 CRA Electric Pass 的设备树、GPIO 或板级驱动。

## 来源与归属

这些文件属于 Buildroot 2020.02.7 的通用架构支持代码。

CRA Electric Pass 的板级配置主要位于：

```text
board/cra/epass/
configs/cra_epass_defconfig
```

通常不应为了修改电通硬件而直接改动本目录。

## 目录职责

本目录主要负责：

- 在 Buildroot `menuconfig` 中提供目标架构选项。
- 定义不同架构支持的 CPU 核心和指令集。
- 选择大小端模式。
- 选择 ABI 和浮点调用约定。
- 声明 MMU、FPU、原子操作等架构能力。
- 生成 GCC、Binutils 和工具链包装器所需的目标参数。
- 限制某些架构可用的编译器和工具链版本。
- 选择 ELF 或 FLAT 等可执行文件格式。

## 文件组织

### 通用入口

| 文件 | 作用 |
| --- | --- |
| `Config.in` | 所有目标架构的总入口，提供架构选择、通用能力、工具链约束和可执行文件格式选项。 |
| `arch.mk` | 将 Kconfig 生成的 `BR2_GCC_TARGET_*` 值转换为构建系统使用的 `GCC_TARGET_*` 变量，并加载架构专用 Makefile。 |

### 架构专用 Kconfig

| 文件 | 目标架构 |
| --- | --- |
| `Config.in.arm` | 32 位 ARM、ARM 大端、AArch64 和 AArch64 大端。 |
| `Config.in.arc` | Synopsys ARC。 |
| `Config.in.csky` | C-SKY。 |
| `Config.in.m68k` | Motorola 68000 系列。 |
| `Config.in.microblaze` | Xilinx MicroBlaze。 |
| `Config.in.mips` | MIPS32/MIPS64 及大小端模式。 |
| `Config.in.nds32` | Andes NDS32。 |
| `Config.in.nios2` | Altera/Intel Nios II。 |
| `Config.in.or1k` | OpenRISC 1000。 |
| `Config.in.powerpc` | PowerPC 32/64 位。 |
| `Config.in.riscv` | RISC-V 32/64 位及 ISA 扩展。 |
| `Config.in.sh` | Renesas SuperH。 |
| `Config.in.sparc` | SPARC 32/64 位。 |
| `Config.in.x86` | i386 和 x86_64。 |
| `Config.in.xtensa` | Xtensa。 |

### 架构专用 Makefile

| 文件 | 作用 |
| --- | --- |
| `arch.mk.arc` | 为 ARC 设置原子指令和链接页大小等额外参数。 |
| `arch.mk.csky` | 根据 C-SKY 核心、FPU 和 VDSP 选项构造 GCC CPU 参数。 |
| `arch.mk.riscv` | 根据 RV32/RV64 及 M/A/F/D/C 扩展构造 RISC-V ISA 字符串。 |
| `arch.mk.xtensa` | 处理 Xtensa 架构 overlay 的获取和解包。 |

## CRA Electric Pass 的配置路径

当前目标配置沿以下路径解析：

```text
configs/cra_epass_defconfig
        ↓
BR2_arm=y
        ↓
arch/Config.in
        ↓
arch/Config.in.arm
        ↓
BR2_arm926t=y
        ↓
ARM926EJ-S + ARMv5 + EABI + soft-float
        ↓
arch/arch.mk
        ↓
Buildroot 交叉工具链和所有目标软件包
```

### 当前选项

| 配置 | 当前值 | 含义 |
| --- | --- | --- |
| `BR2_arm` | `y` | 32 位小端 ARM。 |
| `BR2_arm926t` | `y` | ARM926T/ARM926EJ-S CPU 核心。 |
| `BR2_ARCH` | `arm` | Buildroot 目标架构名称。 |
| `BR2_GCC_TARGET_CPU` | `arm926ej-s` | GCC 的目标 CPU。 |
| `BR2_ARM_CPU_ARMV5` | `y` | 使用 ARMv5 指令集架构。 |
| `BR2_ARM_EABI` | `y` | 使用 ARM EABI。 |
| `BR2_GCC_TARGET_ABI` | `aapcs-linux` | 使用 Linux AAPCS 调用约定。 |
| `BR2_ARM_SOFT_FLOAT` | `y` | 浮点运算由软件实现。 |
| `BR2_GCC_TARGET_FLOAT_ABI` | `soft` | 生成 soft-float ABI 程序。 |
| `BR2_ARM_INSTRUCTIONS_ARM` | `y` | 生成标准 32 位 ARM 指令，而不是 Thumb。 |
| `BR2_USE_MMU` | `y` | 启用 MMU，运行标准 Linux 用户空间。 |
| `BR2_BINFMT_ELF` | `y` | 使用 ELF 可执行文件格式。 |

最终工具链前缀为：

```text
arm-buildroot-linux-gnueabi-
```

可执行文件必须针对 ARMv5 EABI soft-float 构建。ARMv7、AArch64、NEON 或 `gnueabihf` 硬浮点程序不能直接在该设备上运行。

## 配置如何影响构建

`Config.in` 和 `Config.in.arm` 生成的 Kconfig 结果会写入 Buildroot 的 `.config`。`arch.mk` 随后读取这些值，并向工具链和软件包构建过程提供目标参数。

对于当前设备，最终效果相当于要求编译器面向：

```text
CPU: arm926ej-s
Architecture: ARMv5
Endianness: little-endian
ABI: AAPCS Linux / EABI
Floating point ABI: soft
Instruction mode: ARM
```

这些设置会影响：

- Buildroot 内部工具链。
- glibc。
- BusyBox。
- Linux 用户空间程序。
- `drm_app_neo` 及其他目标软件包。
- 第三方预编译库是否能够加载。

## 验证当前架构

在 WSL Buildroot 根目录运行：

```sh
make cra_epass_defconfig
```

检查生成配置：

```sh
grep -E \
    'BR2_arm=|BR2_arm926t=|BR2_ARM_EABI=|BR2_ARM_SOFT_FLOAT=|BR2_ARM_INSTRUCTIONS_ARM=' \
    .config
```

预期包含：

```text
BR2_arm=y
BR2_arm926t=y
BR2_ARM_EABI=y
BR2_ARM_SOFT_FLOAT=y
BR2_ARM_INSTRUCTIONS_ARM=y
```

检查工具链目标：

```sh
output/host/bin/arm-buildroot-linux-gnueabi-gcc -dumpmachine
```

预期输出：

```text
arm-buildroot-linux-gnueabi
```

完整构建：

```sh
make -j$(nproc)
```

这些命令只生成配置和镜像，不会自动写入实体设备。

## 维护说明

- 保持该目录与所使用的 Buildroot 版本一致。
- 请勿删除与当前 ARM 目标无关的架构文件。
- 请勿将板级 GPIO、设备树或应用配置放入本目录。
- 若升级 Buildroot，应以上游新版本的 `arch/` 为基础解决差异，而不是继续叠加本地临时修改。
