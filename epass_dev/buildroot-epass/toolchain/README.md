<div align="center">

# Buildroot 交叉工具链基础设施

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `toolchain/` 保存 Buildroot 2020.02.7 的交叉工具链配置、生成规则、编译器包装层以及外部工具链适配定义。它决定 C 库、编译器能力、目标 ABI，以及目标头文件和库如何组成 sysroot。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#本目录解决什么问题">职责边界</a> ·
  <a href="#当前-cra-electric-pass-工具链">当前工具链</a> ·
  <a href="#内部工具链的生成过程">生成流程</a> ·
  <a href="#生成后的目录与文件">输出目录</a> ·
  <a href="#编译项目程序的推荐方式">推荐用法</a> ·
  <a href="#工具链核验">工具链核验</a> ·
  <a href="#二次开发原则">开发原则</a>
</p>

---

## 目录结构

```text
toolchain/
├── README.md
├── Config.in
├── helpers.mk
├── toolchain.mk
├── toolchain-wrapper.c
├── toolchain-wrapper.mk
├── toolchain/
│   └── toolchain.mk
├── toolchain-buildroot/
│   ├── Config.in
│   └── toolchain-buildroot.mk
└── toolchain-external/
    ├── Config.in
    ├── pkg-toolchain-external.mk
    ├── toolchain-external.mk
    ├── custom/
    └── <各厂商和架构的外部工具链定义>/
```

当前目录约有 20 个子目录、78 个受版本控制的文件，主要使用 Kconfig、GNU Make 和 C。

## 本目录解决什么问题

Buildroot 工具链层主要负责：

1. 选择内部生成工具链或导入外部工具链；
2. 选择 glibc、uClibc-ng 或 musl；
3. 确定 CPU、指令集、端序、ABI 和浮点 ABI；
4. 构建或导入 Binutils、GCC、内核头文件和 C 运行库；
5. 建立供目标软件编译使用的 sysroot；
6. 为 GCC/G++ 增加 Buildroot 统一编译参数；
7. 向 Kconfig 公布线程、C++、SSP、OpenMP、locale 等能力；
8. 检查外部工具链声明与实际能力是否一致；
9. 为所有目标软件包提供统一的 `TARGET_CC`、`TARGET_CXX`、`TARGET_LD` 等变量。

它不负责主程序 UI、设备树、内核驱动、rootfs 内容或固件烧录。

## 当前 CRA Electric Pass 工具链

当前配置入口为：

```text
board/cra/epass/cra_epass_defconfig
```

该 defconfig 选择了 Buildroot 内部工具链，而不是 `toolchain-external/` 中的预编译工具链。

### 目标架构与 ABI

| 项目 | 当前值 | 含义 |
| --- | --- | --- |
| 架构 | ARM 32 位、小端 | `BR2_arm=y` |
| CPU | ARM926EJ-S | Buildroot 的 `arm926t` 变体 |
| 指令集代际 | ARMv5TEJ | 不具备现代 ARMv7/ARMv8 指令能力 |
| 指令模式 | ARM | 编译器目标模式为 `-marm` |
| ABI | AAPCS Linux / EABI | `-mabi=aapcs-linux` |
| 浮点 ABI | Soft float | `-mfloat-abi=soft`，不是 `gnueabihf` |
| MMU | 有 | 生成常规 Linux ELF 程序 |
| C 库 | glibc | Buildroot 内部构建 |
| C++ | 启用 | 目标 sysroot 包含 libstdc++ |
| LTO | 启用 | GCC 与 Binutils支持链接时优化 |

由 `package/Makefile.in` 的 triplet 规则可得当前 GNU 目标名称：

```text
arm-buildroot-linux-gnueabi
```

因此工具链前缀为：

```text
arm-buildroot-linux-gnueabi-
```

`gnueabi` 表示使用 glibc 的 ARM EABI 软浮点环境。它与 `arm-linux-gnueabihf` 不兼容，也不应与面向裸机的 `arm-none-eabi` 混用。

### 默认组件版本

根据当前 Buildroot 源码和 defconfig 的默认选择：

| 组件 | 当前版本或来源 |
| --- | --- |
| Buildroot | 2020.02.7 |
| GCC | 8.4.0 |
| Binutils | 2.32 |
| glibc | `2.30-73-gd59630f9959b0bb8991964758ab854ff4378b20d` |
| Linux UAPI headers | 与当前 Linux 5.4.99 内核源码一致 |

这些版本共同构成运行时 ABI。单独替换 GCC、glibc 或内核头文件后，应对所有目标程序和库执行完整重建，不能只替换一个最终可执行文件后假定其仍然兼容。

## 内部工具链的生成过程

当前内部工具链的大致依赖链为：

```text
Binutils
   ↓
初始 GCC
   ↓
Linux UAPI headers
   ↓
glibc
   ↓
最终 GCC / G++
   ↓
Buildroot wrapper、sysroot 与目标运行库
```

初始 GCC 用于打破“编译器依赖 C 库、C 库又需要编译器”的循环。glibc 和目标头文件准备完成后，Buildroot 再生成具备完整语言和运行库支持的最终 GCC。

`toolchain-buildroot/toolchain-buildroot.mk` 通过依赖 `host-gcc-final` 触发整条内部工具链构建链。通常不应直接逐项手工执行上述步骤。

## 根目录文件

### `Config.in`

这是工具链总配置入口，定义：

- 内部工具链与外部工具链选择；
- C 库类型和能力标志；
- 宽字符、locale、线程、RPC、SSP、OpenMP、PIE 等特性；
- 目标优化和链接参数；
- glibc gconv 字符集转换模块；
- GCC 与内核头文件的版本能力标志；
- 部分架构和编译器组合的已知限制。

这些能力符号会被大量软件包的 `depends on` 和 `select` 使用。删除或错误声明某项能力，可能让不兼容的软件包进入构建，或让原本兼容的软件包从菜单消失。

### `toolchain.mk`

该文件执行工具链完成后的目标目录整理。对 glibc 配置，它会按选择复制需要的 gconv 字符集转换模块，并生成精简后的模块列表，避免把所有转换库无条件放进体积受限的 rootfs。

### `helpers.mk`

该文件主要为外部工具链提供导入和检查函数，包括：

- 查找并复制动态链接器与运行库；
- 识别 sysroot 和多库目录；
- 检查内核头文件版本；
- 检查 ARM ABI 与浮点 ABI；
- 检查 C++、线程、RPC、SSP、OpenMP 等能力；
- 判断工具链实际使用的 C 库；
- 建立 sysroot 内所需符号链接。

这些检查面向“外部工具链声明是否真实”，不用于检测实体设备运行状态。

### `toolchain-wrapper.c`

这是 Buildroot 编译器包装程序的 C 源码。生成后，用户看到的目标 `gcc`、`g++` 和 `cpp` 会先进入该包装层，再调用后缀为 `.br_real` 的真实编译器。

包装层会统一加入：

- 正确的 Buildroot sysroot；
- CPU、ABI、浮点 ABI 和指令模式参数；
- defconfig 选择的全局优化与 SSP 参数；
- 按配置启用的可复现构建、PIE、RELRO 或 ccache 处理。

可在 Linux 构建环境中临时设置以下变量查看包装层实际传递的参数：

```sh
export BR2_DEBUG_WRAPPER=2
```

调试完成后应取消该变量，避免构建日志长期充满参数跟踪信息。

### `toolchain-wrapper.mk`

该文件负责编译并安装包装程序，将 sysroot 子目录和额外编译选项以宏的形式写入包装程序。包装层是工具链行为的一部分，不应绕过它直接长期调用 `.br_real`。

## `toolchain-buildroot/`

该目录描述由 Buildroot 从源码生成的内部工具链。

| 文件 | 作用 |
| --- | --- |
| `Config.in` | 选择内部工具链厂商名、C 库和 GCC/Binutils/headers 等配置入口 |
| `toolchain-buildroot.mk` | 将内部工具链注册为虚拟 toolchain provider，并依赖最终 GCC |

默认 vendor 为 `buildroot`，它也是当前 triplet 中第二段名称的来源。

## `toolchain/toolchain.mk`

该文件定义名为 `toolchain` 的虚拟包，根据配置在内部工具链和外部工具链之间选择 provider。它还执行少量 C 库相关整理，例如安装 glibc 的 `nsswitch.conf`。

虚拟包本身不包含一个独立编译器源码包；它只是统一表示“当前目标工具链已经准备完成”。

## `toolchain-external/`

该目录用于导入已经存在的外部交叉工具链，包含：

- ARM、AArch64、MIPS、Nios II、ARC 等厂商工具链定义；
- Linaro、CodeSourcery、Codescape 等历史预定义工具链；
- `custom/` 自定义下载地址或本地安装路径支持；
- sysroot、运行库、包装层和能力验证的通用规则。

外部工具链不是简单把 `CROSS_COMPILE` 指向任意 GCC。其架构、字节序、ABI、C 库、内核头文件、线程模型和 C++ 支持必须与 Kconfig 声明完全一致，否则 Buildroot 会在检查阶段报错，或在运行时出现动态链接器不存在、符号版本不匹配等问题。

## 生成后的目录与文件

完成构建后，默认输出树中的关键路径为：

```text
output/
├── host/
│   ├── bin/
│   │   ├── arm-buildroot-linux-gnueabi-gcc
│   │   ├── arm-buildroot-linux-gnueabi-g++
│   │   ├── arm-buildroot-linux-gnueabi-ar
│   │   ├── arm-buildroot-linux-gnueabi-ld
│   │   ├── arm-buildroot-linux-gnueabi-ranlib
│   │   ├── arm-buildroot-linux-gnueabi-readelf
│   │   ├── arm-buildroot-linux-gnueabi-objcopy
│   │   └── arm-buildroot-linux-gnueabi-strip
│   ├── arm-buildroot-linux-gnueabi/
│   │   └── sysroot/
│   ├── environment-setup
│   └── share/buildroot/toolchainfile.cmake
└── staging -> host/arm-buildroot-linux-gnueabi/sysroot
```

其中：

| 路径 | 含义 |
| --- | --- |
| `output/host/bin/` | 在构建电脑上运行的交叉工具和 Host 工具 |
| `output/host/.../sysroot/` | 目标 ARM 系统的开发根目录，含头文件与链接库 |
| `output/staging` | 指向上述 sysroot 的便捷符号链接 |
| `output/target/` | 目标设备运行文件，不是完整开发 sysroot |
| `output/host/environment-setup` | 当前 defconfig 选择生成的开发环境脚本 |
| `toolchainfile.cmake` | Buildroot 为 CMake 交叉编译生成的工具链文件 |

当前 Windows 源码工作树没有保留一套完整的 `output/host/` 工具链。这里看到的 `toolchain/` 只能定义如何生成工具链，不能直接在 PowerShell 中充当 ARM 编译器。

## sysroot 为什么重要

sysroot 为编译器提供目标系统视角下的：

```text
/usr/include
/usr/lib
/lib
```

这些路径中的文件面向 ARM/glibc 目标，不是构建电脑自身的 x86-64 头文件和库。正确使用 Buildroot GCC wrapper 时，编译器会自动定位它们。

不要手工加入主机的 `/usr/include` 或 `/usr/lib`，也不要把 Ubuntu 其他版本交叉工具链的 sysroot 与本项目 sysroot 混合。否则即使链接成功，也可能在设备上因 ELF 解释器、glibc 符号版本或软浮点 ABI 不一致而无法运行。

## 编译项目程序的推荐方式

### 通过 Buildroot 软件包构建

这是最安全的方式。Buildroot 会自动提供正确的编译器、sysroot、依赖库和安装目录。

对于并列的 `drm_app_neo` 本地源码，可在不提交的顶层 `local.mk` 中指定：

```make
EPASS_DRM_APP_OVERRIDE_SRCDIR = $(TOPDIR)/../drm_app_neo
```

然后在已经生成正确 `.config` 的 Linux 构建树中执行：

```sh
make epass_drm_app-reconfigure
```

若只需重新执行编译和安装阶段，也可使用：

```sh
make epass_drm_app-rebuild
```

该命令只更新 Buildroot 输出树，不会自动向实体设备传输或刷写程序。

### 在工具链环境中单独构建

当前 defconfig 启用了 Host environment setup。进入已完成构建的 Linux 输出目录后，可加载：

```sh
. output/host/environment-setup
```

该脚本会设置 `PATH`、`CC`、`CXX`、`AR`、`LD`、`PKG_CONFIG`、`CROSS_COMPILE` 等变量，并提供使用 Buildroot CMake toolchain file 的别名。

也可以显式指定 CMake 工具链文件：

```sh
cmake -S <源码目录> -B <构建目录> \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/output/host/share/buildroot/toolchainfile.cmake"
cmake --build <构建目录>
```

实际路径应以正在使用的 Linux Buildroot 输出树为准。不要把 Windows 工作区路径写死进可提交的 CMake 配置。

## 工具链核验

在完成工具链生成的 Linux 环境中，可进行只读检查：

```sh
output/host/bin/arm-buildroot-linux-gnueabi-gcc -dumpmachine
output/host/bin/arm-buildroot-linux-gnueabi-gcc -dumpfullversion
output/host/bin/arm-buildroot-linux-gnueabi-gcc -print-sysroot
output/host/bin/arm-buildroot-linux-gnueabi-readelf -h <目标程序>
output/host/bin/arm-buildroot-linux-gnueabi-readelf -A <目标程序>
file <目标程序>
```

预期重点包括：

- machine 为 `arm-buildroot-linux-gnueabi`；
- sysroot 指向当前 Buildroot 的 `output/host/.../sysroot`；
- ELF 为 32 位 ARM、小端；
- ARM attributes 与 ARMv5/软浮点 ABI 一致；
- 动态链接程序和依赖库来自当前 Buildroot rootfs。

检查或裁剪目标程序时应使用目标 `readelf` 和 `strip`，不要使用构建主机原生的 `strip`。

## 当前工作树中的相关问题

### Windows checkout 不能直接作为可靠构建环境

本项目的 `support/` 目录中已有大量可执行 Shell 脚本被检出为 CRLF，部分 Git 符号链接也在 Windows 工作树中变成普通文件。工具链构建会调用这些基础设施，因此直接在当前 Windows checkout 上通过 WSL 路径构建，可能出现 `^M`、脚本解释器错误或链接类型错误。

应在正确保留 LF、可执行位和符号链接的 Linux 文件系统工作树中生成工具链。Windows 目录适合浏览和编辑，但不能据此断言一次完整 Buildroot 工具链能够可靠重建。

### 当前源码树没有生成工具链

`output/host/bin/`、目标 sysroot 和 `output/staging` 不存在于当前 Windows 源码快照中。不要把“存在 `toolchain/` 源码目录”误认为“ARM 编译器已经安装”。应使用此前完成构建的 Linux/WSL Buildroot 输出树，或在修复 checkout 后重新生成。

### `libcedarc` 中存在硬编码 triplet

`package/libcedarc/libcedarc.mk` 直接引用：

```text
$(HOST_DIR)/arm-buildroot-linux-gnueabi/sysroot/lib/
```

该路径对当前配置有效，但会妨碍将来更换架构、C 库或工具链 vendor。通用化时应优先使用 `$(STAGING_DIR)` 等 Buildroot 变量。

`BR2_PACKAGE_LIBCEDARC_ARCHLIB="arm-none-linux-gnueabi"` 只是 Cedar 预编译库目录的选择名称，不代表当前实际编译器前缀。当前编译器仍是 `arm-buildroot-linux-gnueabi-`。

### 不应混用其他 ARM 工具链

以下工具链名称看似相近，但用途和 ABI 可能不同：

| 工具链 | 是否可直接替代当前工具链 |
| --- | --- |
| `arm-buildroot-linux-gnueabi` | 是，当前 Buildroot 输出的对应版本 |
| 系统安装的 `arm-linux-gnueabi` | 否，版本、sysroot 和运行库不受本项目控制 |
| `arm-linux-gnueabihf` | 否，硬浮点 ABI 不兼容 |
| `arm-none-eabi` | 否，面向裸机，不是 glibc Linux 工具链 |
| `arm-none-linux-gnueabi` | 只有在完整 ABI、库和 sysroot 一致时才可能兼容，不能仅凭名称判断 |

### LTO 要求工具版本一致

当前启用了 GCC LTO。包含 LTO 中间表示的对象或静态库应由同一套 GCC、`gcc-ar` 和 `gcc-ranlib` 处理。混入不同 GCC 版本生成的 LTO 对象可能在最终链接时失败。

## 修改范围

| 需求 | 推荐位置 |
| --- | --- |
| 调整 CPU、ABI、浮点模式 | `arch/` 配置与 CRA defconfig |
| 更改 GCC、Binutils、C 库选择 | CRA defconfig 和对应 `package/` 配置 |
| 更改内核 UAPI headers 来源 | Linux/headers Kconfig 配置 |
| 修改项目程序编译选项 | 对应软件包 `.mk` 或项目 CMake |
| 临时使用本地主程序源码 | 顶层不提交的 `local.mk` override |
| 支持一种新的外部工具链 | `toolchain-external/`，属于 Buildroot 级改动 |
| 修改所有编译器统一参数注入 | wrapper/全局工具链基础设施，必须评估所有目标包 |

## 语法与注释

| 文件类型 | 语言 | 常用注释和要求 |
| --- | --- | --- |
| `Config.in` | Kconfig | `# 注释`；`help` 内容保持正确缩进 |
| `.mk` | GNU Make | `# 注释`；配方命令必须以 Tab 开头 |
| `.c` | C | `/* ... */` 或 `//`，遵循原文件风格 |

修改工具链基础设施后，至少需要验证一次干净工具链构建和完整目标构建。只做增量主程序编译不足以覆盖工具链级变更。

## 二次开发原则

- 将 `toolchain/` 视为 Buildroot 上游核心基础设施，不写入设备业务逻辑；
- 当前项目固定使用 ARM926EJ-S、ARMv5、EABI soft-float 和 glibc；
- 所有目标程序和目标库使用同一套 Buildroot GCC 与 sysroot；
- 不使用硬浮点、裸机或主机原生库替换当前依赖；
- 不绕过 wrapper 长期调用 `.br_real` 编译器；
- 不直接修改 `output/host/` 或 `output/staging/` 作为源码修复；
- 变更工具链版本或 ABI 后执行完整重建；
- 保留外部工具链 provider，除非维护的是经过裁剪并能完整验证的 Buildroot fork；
- LTO 对象、归档工具和最终链接器保持同一 GCC 工具链版本；
- 构建、核验与向实体设备部署是三个独立步骤，本目录不得隐式刷写设备。

---

<div align="center">

<sub><b>toolchain/</b> · compiler, C library, sysroot and ABI infrastructure for Buildroot</sub>

</div>
