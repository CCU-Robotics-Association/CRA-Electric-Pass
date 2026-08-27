# Allwinner 公共板级支持

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录保存 Buildroot 工程中由多块 Allwinner 开发板共用的平台支持文件：

```text
board/allwinner/
```

## 目录结构

```text
allwinner/
├── generic/                 多种 Allwinner 板可复用的启动与镜像支持
│   ├── uboot.env            通用 U-Boot 默认环境
│   ├── kernel.its           通用 FIT 镜像描述
│   ├── splash.bmp           通用启动图
│   ├── genimage-*.cfg       SD、SPI-NOR 和 SPI-NAND 镜像布局
│   ├── scripts/             通用镜像生成脚本
│   ├── rootfs/              通用根文件系统覆盖层
│   └── legacy/              旧版启动环境和烧录镜像配置
├── suniv-f1c100s/           F1C100S/F1C200S 所属 SUNIV 平台公共支持
│   ├── linux.defconfig      通用 Linux 配置基线
│   ├── uboot.defconfig      通用 U-Boot 配置基线
│   ├── devicetree/          Linux 与 U-Boot 的公共 SoC 设备树
│   ├── patch/               SUNIV 公共 Linux 与 U-Boot 补丁
│   └── rootfs/              SUNIV 公共根文件系统覆盖层
└── sun8i-v3/                Allwinner V3s/S3 平台公共支持
    ├── linux.defconfig      V3s/S3 Linux 配置基线
    └── devicetree/          V3s 与 S3 的公共 SoC 设备树
```

## 与 CRA Electric Pass 的装配关系

当前 CRA 板级配置源文件为：

```text
board/cra/epass/cra_epass_defconfig
```

它按以下层次组装系统：

```text
Allwinner 通用层
board/allwinner/generic/
        ↓
SUNIV F1C100S 公共层
board/allwinner/suniv-f1c100s/
        ↓
CRA Electric Pass 专用层
board/cra/epass/
```

实际引用关系如下：

| 公共文件 | 当前用途 |
| --- | --- |
| `generic/rootfs/` | 作为第一层 rootfs 覆盖层安装通用 `preinit` |
| `suniv-f1c100s/rootfs/` | 作为第二层 rootfs 覆盖层安装 SUNIV 平台工具 |
| `suniv-f1c100s/patch/linux/` | 在 CRA Linux 补丁之前应用公共 SUNIV 内核补丁 |
| `suniv-f1c100s/patch/u-boot/` | 在 CRA U-Boot 补丁之前应用公共 SUNIV 启动支持补丁 |
| `suniv-f1c100s/devicetree/linux/suniv-f1c100s.dtsi` | 为 CRA Linux DTS 提供 SoC 控制器、时钟、中断、DMA 和外设基础节点 |
| `suniv-f1c100s/devicetree/uboot/suniv-f1c100s.dtsi` | 为 CRA U-Boot DTS 提供 SUNIV SoC 基础节点 |

以下文件存在于公共目录中，但当前 CRA 配置有自己的替代实现，因此不直接引用：

```text
generic/uboot.env
generic/kernel.its
generic/genimage-*.cfg
generic/scripts/
generic/splash.bmp
suniv-f1c100s/linux.defconfig
suniv-f1c100s/uboot.defconfig
sun8i-v3/
```

## `generic/`：Allwinner 通用层

### `uboot.env`

这是面向多种 Allwinner 板的通用 U-Boot 环境，主要提供：

- 从 MMC0、MMC1、SPI-NOR 或 SPI-NAND 查找并启动 `kernel.itb`；
- 在不同启动介质之间扫描可用启动项；
- 加载和显示 `splash.bmp`；
- 配置 MMC、SPI flash 和 MTD 的 DFU 下载目标；
- 在无正常启动介质时回退到 FEL 或 DFU；
- 为 SPI-NOR 与 SPI-NAND 定义通用镜像偏移和读取长度。

它使用的是公共镜像约定，不等同于 CRA Electric Pass 当前使用的 NAND 分区、FIT 内容和启动流程。电子通行证的默认环境应修改：

```text
board/cra/epass/uboot.env
```

而不是直接改这个公共文件。

### `kernel.its`

该文件是通用 FIT（Flattened Image Tree）描述，负责把以下内容封装到 `kernel.itb`：

- ARM Linux `zImage`；
- 单个 `devicetree.dtb`；
- 对内核和设备树的 CRC32 校验；
- 默认启动配置 `conf@0`。

通用内核加载地址和入口地址均为 `0x80000000`。当前 CRA 项目需要多个屏幕、接口和扩展设备 DTBO，因此使用自己的 `scripts/kernel.its`，没有直接采用本文件。

### `genimage-*.cfg`

三个配置文件描述公共镜像布局：

| 文件 | 目标介质 | 主要布局 |
| --- | --- | --- |
| `genimage-sdcard.cfg` | SD 卡 | 在 `0x2000` 放置 U-Boot，另建 FAT Boot 分区和 ext4 rootfs 分区 |
| `genimage-nor.cfg` | 16 MiB SPI-NOR | U-Boot、启动图、内核和只读 rootfs 按固定偏移排列 |
| `genimage-nand.cfg` | 128 MiB SPI-NAND | U-Boot、启动图、内核和只读 rootfs 按固定偏移排列 |

其中通用 SPI flash 布局采用以下主要偏移：

| 内容 | 偏移 | 预留大小 |
| --- | --- | --- |
| U-Boot | `0x000000` | `0x080000`（512 KiB） |
| 启动图 | `0x080000` | `0x080000`（512 KiB） |
| 内核/FIT | `0x100000` | `0x500000`（5 MiB） |
| rootfs | `0x600000` | 使用介质剩余空间 |

这些数值属于通用镜像方案，不是当前 CRA Electric Pass 的权威分区定义。改变布局时，必须同步检查 U-Boot 环境、Linux `bootargs`、设备树分区、生成脚本和烧录端配置。

### `scripts/`

| 文件 | 作用 |
| --- | --- |
| `mknanduboot.sh` | 按 2 KiB NAND 页布局重排 SPL/U-Boot，使其可用于 SPI-NAND 启动 |
| `genimage.sh` | 生成通用 FIT、复制启动图、转换 NAND U-Boot，并调用 `genimage` 生成 SD/NOR/NAND 镜像 |

这些脚本由 Buildroot post-image 阶段调用，依赖 `BINARIES_DIR`、`HOST_DIR` 等 Buildroot 环境变量，不适合脱离构建环境直接运行。

### `rootfs/preinit`

该脚本会检查内核命令行中的 `overlayfsdev=` 参数。参数存在时，它尝试：

1. 把指定 MTD 设备以 JFFS2 挂载到 `/overlay`；
2. 以当前只读根文件系统作为 lowerdir；
3. 创建 overlayfs 的 upperdir 和 workdir；
4. 把合并后的文件系统挂载到 `/tmp`；
5. `chroot` 进入合并后的系统继续启动。

如果没有 `overlayfsdev=`，脚本直接执行正常的 `init`。文件被加入 rootfs 并不自动证明当前设备一定启用了 overlayfs；实际行为还取决于启动参数和 init 流程。

### `legacy/`

`legacy/` 保存旧版兼容流程：

- `uboot.env` 使用分离的 `zImage` 和 DTB，而不是当前通用 FIT 流程；
- `genimage-flasher.cfg` 生成包含 NOR/NAND 系统镜像的 FAT 烧录介质。

除非正在维护使用该旧启动方案的硬件，否则不应把这里的配置混入 CRA 当前构建。

## `suniv-f1c100s/`：SUNIV 公共层

该目录提供 F1C100S/F1C200S 系列 SoC 的公共支持。CRA Electric Pass 使用的 ARM926T、时钟控制器、DMA、USB、音频、SPI flash 和片上外设基础支持主要由这一层提供。

### 公共设备树

| 文件 | 服务对象 | 作用 |
| --- | --- | --- |
| `devicetree/linux/suniv-f1c100s.dtsi` | Linux 5.4 系列 | 定义 CPU、时钟、中断、复位、DMA、GPIO 和片上控制器等 SoC 级节点 |
| `devicetree/uboot/suniv-f1c100s.dtsi` | U-Boot 2020.07 系列 | 定义 U-Boot 启动阶段需要的 SUNIV SoC 级节点 |

公共 `.dtsi` 描述的是芯片能力，不应包含 CRA 板载 LCD、按键、音频芯片或接口选择等单板专用信息。CRA 硬件连接应放在 `board/cra/epass/devicetree/` 的 `.dts` 或叠加层中。

Linux 公共设备树带有：

```text
SPDX-License-Identifier: (GPL-2.0+ OR X11)
```

U-Boot 公共设备树还保留了原作者版权信息。修改时不得删除这些声明。

### Linux 公共补丁

`patch/linux/` 当前包含 16 个按编号应用的补丁：

| 编号 | 主要内容 |
| --- | --- |
| `0001` | 增加 SUNIV USB 支持 |
| `0002` | 修正 CCU 定义 |
| `0003` | 增加 DMA 控制器支持 |
| `0004` | 增加片上音频 Codec 支持 |
| `0005` | 增加 sun4i CSI packed format 支持 |
| `0006` | 增加 `rfkill-gpio` 设备树支持 |
| `0007` | 修正 SPI-NAND 坏块标记大小 |
| `0008` | 增加 CedarX 驱动支持 |
| `0009` | 兼容较早的 GDF5 A 系列 NAND |
| `0010` | 增加 AW9523B GPIO 扩展器支持 |
| `0011` | 允许 `phy-sun4i-usb` 使用嵌套中断 |
| `0012` | 增加 AXP199 支持 |
| `0013` | 增加 XT25F128 SPI-NOR 支持 |
| `0014` | 增加 GD5F1GQ5UExxG SPI-NAND 支持 |
| `0015` | 为旧版 W25N01G 增加兼容处理 |
| `0016` | 为 SPI-NAND 增加片上 ECC quirk 开关 |

当前 CRA 构建先应用这组公共补丁，再应用 `board/cra/epass/patch/linux/`。后续补丁可能依赖前面补丁加入的 Kconfig、驱动或设备树绑定，因此不能仅按文件名挑选、随意改号或交换顺序。

### U-Boot 公共补丁

`patch/u-boot/0001-v2020.07.11.patch` 是一份体积较大的 SUNIV 平台补丁，为 U-Boot `2020.07` 基线补充 ARM926EJ-S/SUNIV 启动、SPL、DRAM、时钟、GPIO、PWM、SPI、NAND 和相关设备树支持。

该补丁与固定 U-Boot 版本紧密相关。升级 U-Boot 时，应重新检查补丁能否应用及上游是否已吸收相同功能，不能通过忽略失败的 hunk 继续构建。

### `linux.defconfig`

这是通用 SUNIV Linux 配置基线。文件头表明它由 Linux 配置系统自动生成，基于 Linux `5.4.92`，使用 ARM EABI 工具链。它不是当前 CRA Linux `5.4.99` 配置的来源，也不应与 `board/cra/epass/linux.defconfig` 混用。

由于它是完整生成配置而不是精简的 `savedefconfig`，手工修改后应通过目标 Linux 源树的 Kconfig 工具重新校验，避免留下已经改名、被依赖条件关闭或不属于目标版本的选项。

### `uboot.defconfig`

这是通用 SUNIV U-Boot 配置基线，主要启用：

- `CONFIG_MACH_SUNIV` 和 ARM 架构；
- SPL 与 SUNXI SPI 启动；
- 通用 `suniv-f1c100s-generic` 设备树；
- 408 MHz 系统时钟和 168 MHz DRAM 时钟；
- MMC、SPI-NOR、SPI-NAND、MTD；
- USB Mass Storage、DFU 和 MUSB Gadget；
- `generic/uboot.env` 作为默认环境；
- 800×480 通用 LCD 参数。

当前 CRA 项目使用自己的 U-Boot 配置和环境，不能通过修改本文件改变电子通行证固件。

### `rootfs/usr/sbin/mtd`

该文件是一个预编译的 32 位 ARM ELF 用户空间程序，不是 Shell 脚本或源码。由于 `suniv-f1c100s/rootfs/` 被当前 CRA 配置引用，它会被复制到目标系统的：

```text
/usr/sbin/mtd
```

此目录中没有与该二进制一一对应的源代码。维护时应注意：

- 不要用文本编辑器打开、改写或转换其行尾；
- Windows 主机不能直接执行这个 ARM 程序；
- 更换文件前应确认目标 ABI、动态链接器、依赖库、许可证和来源；
- 若要长期维护，宜追溯构建来源并保留可复现的源码与构建方法。

## `sun8i-v3/`：V3s/S3 公共层

该目录面向 Allwinner V3s/S3 平台，包含：

- `linux.defconfig`：Linux `5.4.35` 的完整自动生成配置，文件头显示使用 ARM hard-float 工具链；
- `devicetree/linux/sun8i-v3s.dtsi`：V3s SoC 公共设备树；
- `devicetree/linux/sun8i-s3.dtsi`：S3 SoC 公共设备树。

V3s/S3 与当前 CRA Electric Pass 的 SUNIV F1C100S/F1C200S 目标不是同一配置层。当前 `cra_epass_defconfig` 没有引用该目录，不应因为二者都属于 Allwinner 而互换 defconfig 或 `.dtsi`。

## 源文件与生成文件边界

本目录中的配置、DTS、补丁、脚本、环境文件和镜像布局都是构建输入，应纳入版本控制。实际构建后生成的 Linux、U-Boot、DTB 和固件镜像通常位于：

```text
output/build/
output/host/
output/images/
```

这些输出目录不是本目录源文件的替代品。直接修改 `output/build/linux-*` 或 `output/build/uboot-*` 中的内容，清理或重新构建后会丢失。

文件类型边界也需要保留：

| 类型 | 示例 | 注意事项 |
| --- | --- | --- |
| 文本配置 | `*.defconfig`、`*.env`、`*.cfg` | 使用 UTF-8/LF，修改后由相应配置或构建工具校验 |
| 设备树源码 | `*.dts`、`*.dtsi` | 必须由匹配版本的 DTC/内核/U-Boot 设备树流程验证 |
| 补丁 | `*.patch` | 行尾、路径前缀、顺序和目标源码版本都影响应用结果 |
| Shell 脚本 | `*.sh`、`preinit` | 保留 LF、shebang 和可执行权限 |
| 二进制资源 | `splash.bmp`、`usr/sbin/mtd` | 不得进行文本替换或行尾转换 |

## 修改联动关系

| 修改位置 | 需要同步检查 |
| --- | --- |
| `generic/uboot.env` | 所有引用该环境的板级 U-Boot 配置、启动介质和镜像布局 |
| `generic/kernel.its` | 内核/DTB 文件名、加载地址、生成脚本和 U-Boot 启动命令 |
| `generic/genimage-*.cfg` | U-Boot 环境偏移、MTD 分区、镜像大小和烧录工具 |
| `generic/rootfs/preinit` | 内核 `bootargs`、JFFS2/overlayfs 支持和 init 流程 |
| 公共 SUNIV `.dtsi` | 所有包含它的板级 DTS，不仅是 CRA Electric Pass |
| 公共 Linux 补丁 | Linux 版本、公共 defconfig、CRA 后续补丁和所有 SUNIV 消费板 |
| 公共 U-Boot 补丁 | U-Boot 版本、SPL/DRAM/SPI 启动及 CRA 后续补丁 |
| SUNIV `rootfs/` | 所有叠加该目录的目标系统及目标 ABI |

只与 CRA 硬件或界面有关的改动，应优先放在 `board/cra/epass/` 或对应应用程序目录；只有确实适用于多个 Allwinner/SUNIV 板的修改，才应进入本公共目录。

## 构建与验证

当前 CRA Electric Pass 的构建入口仍是：

```sh
make cra_epass_defconfig
make
```

验证公共目录修改时，至少应检查：

1. 所有补丁能按编号无冲突地应用到锁定的 Linux/U-Boot 版本；
2. Linux 和 U-Boot 设备树能够编译，且 CRA 板级 DTS 的引用仍然有效；
3. rootfs 中的 ARM 二进制与当前 EABI soft-float 用户空间兼容；
4. U-Boot、FIT、DTB/DTBO、UBI 和 SD 镜像能够正常生成；
5. 修改公共文件后，其他引用它的板级配置没有被意外破坏。

构建和静态检查只生成、验证软件产物，不等同于刷写实体设备。涉及启动介质、NAND 页布局、分区或 U-Boot 的修改，在实机操作前必须另行确认硬件型号、目标介质、备份和恢复路径。
