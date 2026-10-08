<div align="center">

# CRA Electric Pass 镜像构建脚本

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> 本目录保存板级镜像生成脚本、设备树打包清单和 UBI 卷配置，位于 Buildroot 编译流程末端，负责把 U-Boot、Linux、设备树和 rootfs 整理为可启动或烧录的镜像。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#buildroot-调用入口">调用入口</a> ·
  <a href="#总体输入与输出">输入与输出</a> ·
  <a href="#buildimagesh">镜像生成</a> ·
  <a href="#构建依赖">构建依赖</a> ·
  <a href="#安全与维护原则">安全维护</a>
</p>

---

`binary/` 子目录中的 EXE 是 Windows 主机端辅助程序，详见 [Windows 辅助程序说明](binary/README.md)。

## 目录结构

```text
scripts/
├── binary/
│   ├── epass_flasher.exe
│   ├── UsbTreeView.exe
│   ├── zadig-2.9.exe
│   └── README.md
├── buildimage.sh
├── gensdimage.py
├── kernel.its
├── mkdt.sh
├── mknanduboot.sh
├── ubinize-rootfs.cfg
└── README.md
```

| 文件 | 类型 | 作用 |
| --- | --- | --- |
| `mknanduboot.sh` | Bash 脚本 | 把普通 SUNXI SPL/U-Boot 输出整理为适用于当前 SPI-NAND 页布局的镜像 |
| `mkdt.sh` | Bash 脚本 | 预处理并编译基础设备树和所有设备树叠加层 |
| `buildimage.sh` | POSIX Shell 脚本 | 生成 UBIFS/UBI 根文件系统、FIT 启动包和简化 SD 镜像 |
| `kernel.its` | FIT Image Tree Source | 定义 `boot.itb` 中需要包含的内核、基础设备树和叠加层 |
| `ubinize-rootfs.cfg` | UBI 配置 | 定义根文件系统 UBI 卷 |
| `gensdimage.py` | Python 脚本 | 生成只包含启动偏移和 SPL/U-Boot 的简化 SD 镜像 |
| `binary/` | Windows 程序 | 提供 USB 诊断、驱动配置和设备烧录工具 |

## Buildroot 调用入口

这些脚本由板级配置中的以下选项调用：

```make
BR2_ROOTFS_POST_IMAGE_SCRIPT="board/cra/epass/scripts/mknanduboot.sh board/cra/epass/scripts/mkdt.sh board/cra/epass/scripts/buildimage.sh"
```

Buildroot 会在根文件系统和主要编译产物完成后，按照下列顺序执行：

```text
mknanduboot.sh
        │
        ▼
     mkdt.sh
        │
        ▼
  buildimage.sh
```

1. `mknanduboot.sh` 先生成 NAND 使用的 U-Boot 镜像；
2. `mkdt.sh` 再生成 `kernel.its` 所引用的 DTB/DTBO；
3. `buildimage.sh` 最后把内核和设备树打进 `boot.itb`，并生成根文件系统镜像。

脚本依赖 Buildroot 导出的环境变量：

| 变量 | 常见位置 | 用途 |
| --- | --- | --- |
| `BINARIES_DIR` | `output/images` | 保存最终镜像及临时打包文件 |
| `BUILD_DIR` | `output/build` | 查找 Linux 源码和头文件 |
| `HOST_DIR` | `output/host` | 查找 Buildroot 构建的主机端工具 |

因此，不建议脱离 Buildroot 环境直接运行这些脚本。

## 总体输入与输出

主要输入：

```text
output/images/
├── u-boot-sunxi-with-spl.bin
├── zImage
└── rootfs.tar

board/cra/epass/devicetree/linux/
├── base/
├── screen/
├── interface/
└── ext/
```

预期输出：

```text
output/images/
├── u-boot-sunxi-with-nand-spl.bin
├── boot.itb
├── rootfs_ubi.img
├── sd_image.img
└── dt/
    ├── base/
    ├── screen/
    ├── interface/
    └── ext/
```

这些脚本只生成文件，不会自动烧录实体设备。

## `mknanduboot.sh`

该脚本把 Buildroot/U-Boot 生成的：

```text
u-boot-sunxi-with-spl.bin
```

转换为：

```text
u-boot-sunxi-with-nand-spl.bin
```

### 主要参数

```sh
UBOOT_OFFSET=32
PAGESIZE=2048
BLOCKSIZE=128
SPLBLOCKS=25
```

- NAND 页大小按 2048 字节处理；
- 使用 1024 字节为 `dd` 操作单位；
- 将 SPL 的前 26 个 1 KiB 数据块依次写入每个 2 KiB 页的起始位置；
- 在输出文件的 52 KiB 位置追加输入文件从第 32 KiB 开始的剩余内容；
- 最后只同步生成的目标文件。
- 检查主机 `od` 是否支持 `--endian`；
-从 SPL 头部读取并打印 SPL 大小；
-从预期的主 U-Boot 头部读取并打印 U-Boot 大小；
-检查 NAND 页大小是否能按 1 KiB 对齐。

### 维护注意事项

- 该脚本与 F1C100S/SUNIV 的 SPL 布局和当前 NAND 页结构紧密绑定；
- `SPLSIZE` 和 `UBOOTSIZE` 当前只用于显示，没有参与边界检查；
- `BLOCKSIZE` 当前已定义但没有实际使用；
- `UBOOT_OFFSET=32` 后面的旧注释与数值含义不完全一致，修改前必须结合当前 U-Boot 二进制布局重新验证；
- `od --endian` 和 `sync -d` 属于 GNU/Linux 环境特性，原生 Windows Shell 不适合直接执行该脚本。

## `mkdt.sh`

该脚本编译 Linux 使用的基础设备树和设备树叠加层。

### 输入目录

```text
board/cra/epass/devicetree/linux/
├── base/
├── screen/
├── interface/
└── ext/
```

### 输出目录

```text
${BINARIES_DIR}/dt/
├── base/*.dtb
├── screen/*.dtbo
├── interface/*.dtbo
└── ext/*.dtbo
```

每个 DTS 文件会经过两个阶段：

1. 使用 C 预处理器 `cpp` 展开 `#include`、宏和 Linux 设备树头文件；
2. 使用 `dtc -@` 编译，并保留应用设备树叠加层所需的符号信息。

基础设备树生成 `.dtb`，屏幕、接口和扩展配置生成 `.dtbo`。

脚本会先删除整个 `${BINARIES_DIR}/dt`，再重新创建输出目录。因此，不应在该目录中手工保存需要长期保留的文件。

### 与 Linux 版本的耦合

脚本直接使用：

```text
${BUILD_DIR}/linux-5.4.99/
```

如果更改 Linux 版本，必须同步修改这里的目录名，否则预处理器将无法找到内核设备树头文件。

### 维护注意事项

- `set -o pipefail` 只影响管道返回值，并不等同于 `set -e`；
- 当前脚本没有在任一命令失败时保证立即退出；
- 某个输入目录没有匹配的 `.dts` 时，Shell 通配符可能以原始字符串形式进入循环；
- 输出文件名必须与 `kernel.its` 及 U-Boot 环境中的 `screen`、`interface`、`ext` 名称保持一致。

## `kernel.its`

`kernel.its` 是 U-Boot FIT 镜像的描述文件，由 `mkimage` 读取并生成：

```text
boot.itb
```

当前 FIT 包含以下内容。

### Linux 内核

| FIT 节点 | 输入文件 | 加载地址 | 入口地址 |
| --- | --- | --- | --- |
| `kernel` | `zImage` | `0x80008000` | `0x80008000` |

内核架构为 ARM，未使用压缩。

### 基础设备树

| FIT 节点 | 输入文件 |
| --- | --- |
| `fdt-base` | `dt/base/devicetree.dtb` |

### 屏幕叠加层

| FIT 节点 | 输入文件 | 启动环境值 |
| --- | --- | --- |
| `fdt-screen-hsd` | `dt/screen/hsd.dtbo` | `screen=hsd` |
| `fdt-screen-boe` | `dt/screen/boe.dtbo` | `screen=boe` |
| `fdt-screen-laowu` | `dt/screen/laowu.dtbo` | `screen=laowu` |

### 接口叠加层

```text
adc_pa1
adc_pa123
i2c0
i2s0_pa
i2s0_pe
spi1
uart1
uart2
usbhost
usbhs
```

对应 FIT 节点使用 `fdt-iface-` 前缀。

### 扩展叠加层

```text
cardkb
es8311_sound
lsm6ds3_pre0.4
```

对应 FIT 节点使用 `fdt-ext-` 前缀。

U-Boot 不依赖 FIT 的默认 `configurations` 节点，而是在 `uboot.env` 中通过 `imxtract` 按节点名称提取内核和设备树，再依次应用屏幕、接口及扩展叠加层。

当前 ITS 没有定义哈希或数字签名节点。虽然 Buildroot 配置启用了带 FIT 签名支持的主机端 U-Boot 工具，但当前生成的 `boot.itb` 本身没有使用该验证机制。

## `buildimage.sh`

这是最终的镜像汇总脚本。

### 第一阶段：准备打包文件

脚本把以下文件复制到 `${BINARIES_DIR}`：

```text
ubinize-rootfs.cfg
gensdimage.py
kernel.its
```

并使用：

```text
${HOST_DIR}/bin/mkimage
```

作为 FIT 镜像制作工具。

### 第二阶段：生成 UBIFS 和 UBI

脚本会：

1. 创建临时 `rootfs/`；
2. 解压 `rootfs.tar`；
3. 使用 `mkfs.ubifs` 生成 `rootfs_ubifs.img`；
4. 使用 `ubinize` 生成最终的 `rootfs_ubi.img`。

主要 NAND/UBI 参数：

| 参数 | 数值 | 含义 |
| --- | ---: | --- |
| 最小 I/O 单元 | 2048 | NAND 页大小 |
| 物理擦除块大小 | 131072 | 128 KiB |
| UBIFS 逻辑擦除块大小 | 126976 | 扣除 UBI 头部后的可用大小 |
| 最大逻辑擦除块数 | 922 | `mkfs.ubifs -c` |
| 压缩算法 | LZO | `mkfs.ubifs -x lzo` |

### 第三阶段：生成 FIT

脚本执行：

```sh
mkimage -f kernel.its boot.itb
```

把 `zImage`、基础设备树和全部叠加层打包为 `boot.itb`。

### 第四阶段：生成 SD 镜像

脚本调用：

```sh
python3 gensdimage.py
```

生成 `sd_image.img`。

### 第五阶段：清理

脚本删除：

-复制到输出目录的 `ubinize-rootfs.cfg`；
-中间文件 `rootfs_ubifs.img`；
-临时解压目录 `rootfs/`。

复制到输出目录的 `kernel.its` 和 `gensdimage.py` 当前不会在脚本末尾删除。

### 维护注意事项

- 脚本没有启用 `set -e`，部分命令失败后仍可能继续执行；
- 开头删除的是 `ubi.img`，而实际输出名称是 `rootfs_ubi.img`，二者并不一致；
- 第一次构建时 `rootfs/` 或 `ubi.img` 不存在会产生删除错误信息，但通常不会阻止后续步骤；
- 如果中途失败，输出目录中可能残留旧镜像或不完整镜像，不能只根据文件存在判断构建成功。

## `ubinize-rootfs.cfg`

该文件定义一个 UBI 动态卷：

```ini
[rootfs]
mode=ubi
image=rootfs_ubifs.img
vol_id=0
vol_type=dynamic
vol_name=rootfs
vol_size=117071872
vol_alignment=1
```

字段含义：

| 字段 | 作用 |
| --- | --- |
| `mode=ubi` | 创建 UBI 卷 |
| `image=rootfs_ubifs.img` | 使用临时 UBIFS 镜像作为卷内容 |
| `vol_id=0` | 卷编号为 0 |
| `vol_type=dynamic` | 使用可更新的动态卷 |
| `vol_name=rootfs` | 卷名与内核参数 `root=ubi0:rootfs` 对应 |
| `vol_size=117071872` | 为根文件系统分配 117,071,872 字节 |
| `vol_alignment=1` | 使用一个逻辑擦除块的默认对齐 |

卷大小正好等于：

```text
126976 × 922 = 117071872
```

因此，它与 `buildimage.sh` 中 `mkfs.ubifs` 的逻辑擦除块大小及最大块数相互对应，修改时必须保持一致。

## `gensdimage.py`

该脚本当前只执行两项操作：

1. 创建 `sd_image.img`；
2. 写入 4096 字节零填充，再追加完整的 `u-boot-sunxi-with-spl.bin`。

等价布局为：

```text
0x00000000 ─ 0x00000FFF    4 KiB 零填充
0x00001000 ─ 文件末尾      u-boot-sunxi-with-spl.bin
```

它不会写入：

- `boot.itb`；
- `rootfs_ubi.img`；
-分区表；
-文件系统；
-运行时启动环境。

因此，当前 `sd_image.img` 只是一个简化的 U-Boot 启动镜像，不是包含内核和根文件系统的完整 SD 卡系统镜像。

脚本中的 `os` 模块目前没有被使用。

## 构建依赖

脚本实际使用以下主机端工具：

```text
bash
sh
cpp
dtc
mkimage
fakeroot
tar
mkfs.ubifs
ubinize
od
grep
xargs
dd
sync
python3
```

Buildroot 配置已经启用多项对应的主机工具，但构建环境仍应为 Linux 或兼容的 WSL 环境。不要在原生 PowerShell 中直接执行 Shell 脚本。

## 正常构建方式

在正确配置好板级 defconfig 入口后，从 Buildroot 根目录执行：

```sh
make cra_epass_defconfig
make
```

Buildroot 会自动准备环境变量并运行三个 post-image 脚本。通常不需要手工逐个调用它们。

## 修改联动关系

修改下列内容时，需要同步检查相关文件：

| 修改内容 | 必须联动检查 |
| --- | --- |
| Linux 版本 | `mkdt.sh`、Buildroot defconfig、补丁目录 |
| 屏幕名称 | 屏幕 DTS、`kernel.its`、`uboot.env`、烧录环境 |
| 接口或扩展名称 | 对应 DTS、`kernel.its`、`uboot.env` |
| NAND 页/块参数 | `mknanduboot.sh`、`buildimage.sh`、`ubinize-rootfs.cfg` |
| NAND 分区布局 | `uboot.defconfig`、`uboot.env`、Linux DTS、烧录工具 |
| UBI 卷名 | `ubinize-rootfs.cfg`、Linux `bootargs` |
| 内核加载地址 | `kernel.its`、`uboot.env`、内核启动布局 |

## 当前文件格式

当前检出状态中：

- 三个 Shell 脚本使用 LF 换行；
- `gensdimage.py` 和 `ubinize-rootfs.cfg` 使用 CRLF；
- `kernel.its` 同时包含 CRLF 和 LF，属于混合换行。

这些换行差异目前不一定会阻止构建，但后续修改时建议统一为 UTF-8 与 LF，避免在 Linux 构建环境和 Git 差异中产生额外问题。

## 安全与维护原则

- 不要把 `BINARIES_DIR` 指向实体设备挂载目录；
- 构建前确认 `rootfs.tar`、`zImage` 和 U-Boot 输出来自同一次有效构建；
- 不要在未核对 NAND 参数的情况下复用其他硬件版本的镜像；
- 更改设备树文件名后，必须同步更新 FIT 节点和启动环境；
- 构建失败后应检查日志并重新生成，不要直接烧录可能残留的旧文件；
- 烧录前再次核对镜像名称、目标分区和设备型号；
- Windows 二进制工具与 Buildroot 构建脚本应分开维护，不应互相混入目标 rootfs。

---

<div align="center">

<sub><b>scripts/</b> · image assembly and post-build tooling</sub>

</div>

