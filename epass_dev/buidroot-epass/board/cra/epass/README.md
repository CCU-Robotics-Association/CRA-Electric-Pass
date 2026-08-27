# CRA Electric Pass 板级支持包

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录为 CRA Electric Pass 在 Buildroot 中的板级支持目录：

```text
board/cra/epass/
```

本目录将 Buildroot 系统配置、Linux 与 U-Boot 配置、设备树、补丁、rootfs 覆盖层以及镜像后处理脚本组合为一套可构建的固件定义。目标处理器为 Allwinner SUNIV F1C100S/F1C200S 系列的 ARM926T，用户空间采用 ARM EABI soft-float 环境。

## 目录结构

```text
epass/
├── cra_epass_defconfig   Buildroot 板级配置
├── linux.defconfig       Linux 5.4.99 内核配置
├── uboot.defconfig       U-Boot 2020.07 配置
├── uboot.env             编译进 U-Boot 的默认环境
├── uEnv.txt              烧录时使用的文本环境模板
├── devicetree/           U-Boot 与 Linux 设备树
├── patch/                Linux 与 U-Boot 补丁序列
├── rootfs/               根文件系统覆盖层
├── scripts/              设备树及固件镜像生成脚本
└── tools/                开发电脑端资源生成工具
```

| 路径 | 主要职责 | 详细说明 |
| --- | --- | --- |
| [`devicetree/`](devicetree/README.md) | 描述启动硬件、基础主板、屏幕、接口和外接设备 | [设备树说明](devicetree/README.md) |
| [`patch/`](patch/README.md) | 在固定版本的 Linux 与 U-Boot 源码上增加 SUNIV 和通行证硬件支持 | [补丁说明](patch/README.md) |
| [`rootfs/`](rootfs/README.md) | 覆盖目标系统中的 `/bin`、`/etc`、`/root`、`/assets` 和 `/app` | [rootfs 说明](rootfs/README.md) |
| [`scripts/`](scripts/README.md) | 生成 NAND U-Boot、DTB/DTBO、FIT、UBI 和 SD 启动镜像 | [镜像脚本说明](scripts/README.md) |
| [`tools/`](tools/README.md) | 在开发电脑上生成关机提示等资源 | [辅助工具说明](tools/README.md) |

## Buildroot 配置入口

板级配置的规范源文件：

```text
board/cra/epass/cra_epass_defconfig
```

Buildroot 的用户入口：

```text
configs/cra_epass_defconfig
```

该入口应指向板级配置源文件，使以下命令能够载入本项目配置：

```sh
make cra_epass_defconfig
```

在保留 Git 符号链接的 Linux 检出中，`configs/cra_epass_defconfig` 通常是指向：

```text
../board/cra/epass/cra_epass_defconfig
```

的符号链接。Windows 在未启用符号链接支持时，可能把它检出为只包含目标路径文字的普通文件；这种文件不能作为真正的 Buildroot defconfig 使用。构建前应在 Linux 或 WSL 中确认该入口是有效符号链接或内容完整的配置副本，不要把 Windows 产生的链接占位文件误当成有效配置。

## `cra_epass_defconfig`

该文件为板级支持包的总装配入口，主要决定：

- 使用 Buildroot 内部工具链、glibc 与 C++ 支持；
- 目标主机名为 `epass`，登录提示为 `Welcome to CRA Electric Pass`；
- 使用 eudev 动态创建设备节点；
- 依次叠加 Allwinner 公共 rootfs、SUNIV 公共 rootfs 和 CRA rootfs；
- 构建 Linux `5.4.99`，并应用公共 SUNIV 与 CRA Linux 补丁；
- 使用本目录的 `linux.defconfig` 和 Linux 设备树；
- 构建 U-Boot `2020.07`，并应用公共 SUNIV 与 CRA U-Boot 补丁；
- 使用本目录的 `uboot.defconfig`、`uboot.env` 和 U-Boot 设备树；
- 启用 CRA Electric Pass 主程序、USB Responder、控制工具、MTP、Cedar 视频解码和相关运行库；
- 启用生成 UBI、FIT、SD 镜像和交叉编译环境所需的主机端工具；
- 在镜像完成后依次执行三个 post-image 脚本。

当前 root 登录密码在该配置中设为：

```text
toor
```

这适合受控的开发设备，但不应被视为安全部署方案。设备暴露串口、RNDIS、MTP 或其他调试入口时，应同时评估 root 登录和文件系统写权限风险。

### Rootfs 覆盖顺序

当前配置按以下顺序合并覆盖层：

```text
board/allwinner/generic/rootfs
        ↓
board/allwinner/suniv-f1c100s/rootfs
        ↓
board/cra/epass/rootfs
```

后面的同路径文件会覆盖前面的内容。因此，CRA 文件的最终行为必须结合前两层已有文件一起判断。

### 镜像后处理顺序

Buildroot 在主要构建产物完成后依次执行：

```text
scripts/mknanduboot.sh
        ↓
scripts/mkdt.sh
        ↓
scripts/buildimage.sh
```

三个脚本分别生成 SPI-NAND 使用的 U-Boot、Linux DTB/DTBO，以及最终的 FIT、UBI 和简化 SD 镜像。

## `linux.defconfig`

该文件是 Linux `5.4.99` 的内核配置基线，主要启用：

| 子系统 | 当前用途 |
| --- | --- |
| ARM AEABI、SUNXI/SUNIV | F1C100S/F1C200S ARM926T 平台 |
| MTD、SPI-NAND、UBI、UBIFS | 板载 NAND 分区和根文件系统 |
| DRM SUN4I、面板、背光、fbcon | 360×640 LCD 显示链路和控制台 |
| evdev、LRADC、I²C、SPI、GPIO | 按键与板级接口 |
| USB MUSB、ConfigFS、ACM、RNDIS、FunctionFS | USB Gadget 各工作模式 |
| MMC、VFAT、UTF-8 NLS | SD 卡访问 |
| ALSA SoC、SUN4I I²S、ES8311 | 可选音频链路 |
| IIO、GPADC、LSM6DSX | ADC 和可选惯性传感器 |
| `CONFIG_CRA_EP_*` | CRA Electric Pass 专用内核支持 |

该配置与 `patch/linux/` 和 `devicetree/linux/` 相互依赖。仅打开 Kconfig 选项而缺少对应补丁或设备树节点，会导致配置不可识别、驱动未编译或设备无法匹配。

## `uboot.defconfig`

该文件配置 U-Boot `2020.07`，主要内容包括：

- SUNIV SPL 与 SPI-NAND 启动；
- 204 MHz DRAM 时钟和 604 MHz 系统时钟；
- 启动延迟为 0；
- MTD、DFU、USB Mass Storage 和 MUSB Gadget；
- 默认设备树 `suniv-f1c100s-generic`；
- 从 `uboot.env` 编译默认环境；
- 关闭 U-Boot 阶段的视频输出。

默认 SPI-NAND 分区为：

```text
1 MiB  u-boot（只读）
6 MiB  boot
剩余   rootfs
```

分区布局同时出现在 U-Boot 环境、Linux 启动参数、设备树、镜像脚本和烧录工具中，修改时必须整体核对。

## `uboot.env` 与 `uEnv.txt`

两者用途不同：

| 文件 | 性质 | 作用 |
| --- | --- | --- |
| `uboot.env` | U-Boot 默认环境源码 | 通过 `CONFIG_DEFAULT_ENV_FILE` 编译进 U-Boot，定义加载地址、FIT 读取、设备树叠加和 DFU 回退流程 |
| `uEnv.txt` | 文本环境模板 | 烧录时写入 SPI-NAND 的文本环境区，提供 `bootargs`、`screen`、`interface` 和 `ext` 等设备选择值 |

`uboot.env` 当前使用的主要内存地址为：

| 内容 | 地址 |
| --- | --- |
| Linux 内核 | `0x80008000` |
| 基础 DTB | `0x80C00000` |
| 临时 DTBO | `0x80D00000` |
| 文本环境缓冲区 | `0x80E00000` |
| FIT 镜像 | `0x81000000` |

启动时，U-Boot 从 SPI-NAND 偏移 `0xFA000` 读取最多 `0x6000` 字节文本环境；该区域末端正好到达 Boot 分区起点 `0x100000`。随后它从 `0x100000` 读取 `boot.itb`，提取内核和基础 DTB，并按照以下顺序应用叠加层：

```text
base → screen → interface → ext
```

当前仓库中的 `uEnv.txt` 没有预设 `screen=`，`interface=` 和 `ext=` 也为空。现有烧录流程会根据硬件选择生成实际环境；若手工写入该模板，必须补充有效屏幕类型，否则 U-Boot 无法提取对应的 `fdt-screen-${screen}` 节点。

## 构建流程

在 Linux 或正确配置的 WSL Buildroot 根目录中执行：

```sh
make cra_epass_defconfig
make
```

总体关系为：

```text
cra_epass_defconfig
        │
        ├─ Linux 5.4.99 + linux.defconfig
        │       ├─ patch/linux/
        │       └─ devicetree/linux/
        │
        ├─ U-Boot 2020.07 + uboot.defconfig + uboot.env
        │       ├─ patch/uboot/
        │       └─ devicetree/uboot/
        │
        ├─ rootfs/
        │
        └─ post-image scripts/
                │
                ▼
          output/images/
```

常见最终产物包括：

| 文件 | 用途 |
| --- | --- |
| `u-boot-sunxi-with-nand-spl.bin` | SPI-NAND 使用的 SPL/U-Boot 镜像 |
| `boot.itb` | Linux 内核、基础 DTB 和所有 DTBO 的 FIT 包 |
| `rootfs_ubi.img` | UBI/UBIFS 根文件系统镜像 |
| `sd_image.img` | 只包含启动偏移和 SPL/U-Boot 的简化 SD 启动镜像 |
| `dt/base/*.dtb` | Linux 基础设备树 |
| `dt/screen/*.dtbo` | 屏幕叠加层 |
| `dt/interface/*.dtbo` | 接口叠加层 |
| `dt/ext/*.dtbo` | 外接设备叠加层 |

以上命令只生成构建产物，不会自动刷写实体设备。

## 设备启动链

实体设备的主要启动链为：

```text
Allwinner BROM
        ↓
SPL
        ↓
U-Boot
        ├─ 导入文本环境
        ├─ 读取 boot.itb
        ├─ 提取 Linux 内核和基础 DTB
        ├─ 应用 screen/interface/ext DTBO
        └─ 启动 Linux
        ↓
Linux 5.4.99
        ├─ 挂载 UBI/UBIFS rootfs
        └─ BusyBox init → autologin → /root/.profile
        ↓
/root/epass_drm_app
```

## 修改联动关系

| 修改内容 | 必须同步检查 |
| --- | --- |
| Linux 版本 | `cra_epass_defconfig`、`linux.defconfig`、`patch/linux/`、`scripts/mkdt.sh` |
| U-Boot 版本 | `cra_epass_defconfig`、`uboot.defconfig`、`uboot.env`、`patch/uboot/` |
| 屏幕名称或初始化序列 | `devicetree/linux/screen/`、`scripts/kernel.its`、`uboot.env`、实际启动环境 |
| 接口或外设名称 | 对应 DTS、`kernel.its`、`uboot.env`、`uEnv.txt` 或烧录配置 |
| NAND 页和擦除块参数 | `mknanduboot.sh`、`buildimage.sh`、`ubinize-rootfs.cfg`、U-Boot NAND 支持 |
| NAND 分区布局 | `uboot.defconfig`、`uboot.env`、Linux bootargs、设备树、镜像和烧录工具 |
| UBI 卷名或大小 | `ubinize-rootfs.cfg`、`buildimage.sh`、Linux bootargs |
| rootfs 文件 | `rootfs/`、相关软件包安装路径和 MTP 暴露路径 |
| 主程序退出码 | `drm_app_neo/src/config.h` 与 `rootfs/root/.profile` |

## Windows 检出与文件格式

本板级目录同时包含 Shell、Python、DTS、补丁、配置、二进制和 Git 符号链接。Windows 检出时应特别检查：

- Shell、DTS、配置和补丁应使用 UTF-8 与 LF；
- Shell 脚本需要保留可执行权限；
- `configs/cra_epass_defconfig` 等符号链接不能变成路径文本文件；
- 不要用编辑器改写 ARM ELF、PNG、MP4 或 Windows EXE；
- 不要在 PowerShell 中直接运行依赖 Buildroot 环境变量的 Linux 脚本。

出现构建异常时，应先检查行尾、符号链接、文件权限和当前使用的 Buildroot 输出树，不要通过跳过补丁、关闭校验或直接修改生成目录来掩盖问题。

## 二次开发与安全原则

1. 修改前先确定内容属于 Buildroot 配置、内核、U-Boot、设备树、rootfs、镜像脚本还是主程序，不要把不同层级的问题混在一起处理。
2. 配置、补丁、设备树和脚本必须共同保持版本及命名一致。
3. 只修改可追踪源文件，不以 `output/build/` 中的临时修改作为最终方案。
4. 构建成功只证明软件可以生成，不代表 LCD、按键、USB、音频、SPI-NAND 或电源控制已经在实体设备验证。
5. 刷写 U-Boot、Boot、rootfs 或完整镜像前，必须核对设备型号、屏幕类型、NAND 参数、目标分区和备份状态。
6. `format_sd`、DFU、烧录器和 MTP 的高权限入口具有破坏性，不应对未知设备或不可信电脑使用。
7. 构建、文档整理和静态检查不授权自动刷写、替换或删除实体设备上的文件。
