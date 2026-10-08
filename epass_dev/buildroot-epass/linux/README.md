<div align="center">

# Buildroot Linux 内核构建框架

<sub>Linux source, configuration, patching and image integration</sub>

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `linux/` 是 Buildroot 的 Linux 内核包实现，负责选择、下载、校验、解压、打补丁、配置、编译并安装 Linux 内核及设备树。
>
> CRA 专用内核配置、设备树与补丁分别位于 `board/cra/epass/` 和 Allwinner 公共板级目录。

<p align="center">
  <a href="#目录内容">目录内容</a> ·
  <a href="#文件说明">文件说明</a> ·
  <a href="#补丁应用顺序">补丁顺序</a> ·
  <a href="#源文件与生成物边界">维护边界</a> ·
  <a href="#常用构建命令">构建命令</a> ·
  <a href="#推荐的二次开发方式">二次开发</a>
</p>

---

## 目录内容

```text
linux/
├── Config.in
├── Config.ext.in
├── linux.mk
├── linux.hash
├── 0001-timeconst.pl-Eliminate-Perl-warning.patch.conditional
├── linux-ext-aufs.mk
├── linux-ext-ev3dev-linux-drivers.mk
├── linux-ext-fbtft.mk
├── linux-ext-rtai.mk
└── linux-ext-xenomai.mk
```

---

## 文件说明

### `Config.in`

`Config.in` 定义 Buildroot 菜单中的 `Kernel` 配置项，主要包括：

- 是否构建 Linux 内核；
- 内核版本来源；
- 官方版本、自定义版本、压缩包、Git、Mercurial 或 SVN 源；
- 附加补丁文件或补丁目录；
- 架构自带 defconfig、架构默认配置或自定义配置文件；
- Kconfig 配置片段；
- 自定义 Linux 启动画面 Logo；
- `uImage`、`zImage`、`Image`、`vmlinux` 等内核镜像类型；
- 内核压缩方式；
- 设备树、追加 DTB 和设备树 Overlay；
- 是否把内核安装到目标根文件系统；
- 构建内核时需要的主机 OpenSSL、libelf 等依赖。

该文件描述的是 Buildroot 的通用选择界面。CRA 目标对这些选项的实际取值来自 `board/cra/epass/cra_epass_defconfig`。

### `Config.ext.in`

`Config.ext.in` 定义可选的 Linux 内核扩展：

| 扩展 | 作用 |
| --- | --- |
| Xenomai | 为支持的架构加入 Adeos/I-pipe 实时内核能力 |
| RTAI | 加入 RTAI 实时内核补丁 |
| ev3dev Linux drivers | 加入 LEGO MINDSTORMS EV3 驱动 |
| FBTFT | 为旧内核加入小型 TFT Framebuffer 驱动 |
| AUFS | 加入 AUFS 文件系统内核模块和补丁 |

### `linux.mk`

`linux.mk` 是本目录的核心构建规则，主要负责：

1. 根据 Buildroot 配置计算内核版本、下载地址和源码包名称；
2. 声明许可证、主机工具和交叉工具链依赖；
3. 下载远程补丁，并按配置顺序应用本地补丁目录；
4. 在需要时应用旧内核的 Perl 兼容补丁；
5. 读取自定义内核配置或架构 defconfig，并执行 Buildroot 所需的配置修正；
6. 把项目提供的 DTS/DTSI 文件复制进临时内核源码树；
7. 调用内核 Makefile 编译内核镜像、DTB 和模块；
8. 把内核镜像和 DTB 安装到 `output/images/`；
9. 把内核模块安装到目标根文件系统；
10. 按需安装由内核生成的主机工具；
11. 接入 AUFS、Xenomai 等可选扩展；
12. 提供内核配置保存和重建目标。

### `linux.hash`

`linux.hash` 保存 Buildroot 默认支持的 Linux 源码包及许可证文件的 SHA-256 校验值，用于发现下载损坏或来源不一致。

当前文件记录了 Buildroot 2020.02.7 当时使用的 5.4.70、多个 4.x LTS 版本以及 CIP 内核压缩包。

### `0001-timeconst.pl-Eliminate-Perl-warning.patch.conditional`

这是一份有条件应用的上游兼容补丁。旧 Linux 内核中的 `kernel/timeconst.pl` 使用了新版本 Perl 已移除的写法，可能导致构建失败。

`linux.mk` 会先执行补丁试应用：

- 如果目标内核仍包含旧代码，就应用该补丁；
- 如果目标内核已经修复，则跳过；

### `linux-ext-*.mk`

这些文件分别定义可选内核扩展的接入方式：

| 文件 | 行为 |
| --- | --- |
| `linux-ext-aufs.mk` | 将 AUFS 补丁、源码和 UAPI 头文件加入内核树 |
| `linux-ext-ev3dev-linux-drivers.mk` | 将 ev3dev 驱动复制到 `drivers/lego/` |
| `linux-ext-fbtft.mk` | 将 FBTFT 驱动加入旧内核的 Framebuffer 驱动目录 |
| `linux-ext-rtai.mk` | 按内核版本和架构查找并应用 RTAI HAL 补丁 |
| `linux-ext-xenomai.mk` | 调用 Xenomai 的 `prepare-kernel.sh` 准备内核树 |

只有对应 `BR2_LINUX_KERNEL_EXT_*` 选项启用后，扩展准备函数才会进入实际构建流程。

---

## 补丁应用顺序

CRA 配置中的补丁路径按以下顺序列出：

```text
board/allwinner/suniv-f1c100s/patch/linux
board/cra/epass/patch/linux
```

`linux.mk` 会依次遍历这些路径，并按各目录中的 `*.patch` 文件顺序应用补丁。因此：

- 公共 F1C100S/F1C200S 修复先应用；
- CRA Electric Pass 专用修改后应用；
- 后一层可以建立在前一层已经修改过的源码之上；
- 新增补丁时应使用稳定、可排序的编号文件名；

---

## 源文件与生成物边界

### 应当纳入版本控制

| 内容 | 规范位置 |
| --- | --- |
| Buildroot 内核包规则 | `linux/` |
| CRA Buildroot 目标配置 | `board/cra/epass/cra_epass_defconfig` |
| CRA 内核配置 | `board/cra/epass/linux.defconfig` |
| CRA Linux 补丁 | `board/cra/epass/patch/linux/` |
| Allwinner 公共补丁 | `board/allwinner/suniv-f1c100s/patch/linux/` |
| CRA Linux 设备树 | `board/cra/epass/devicetree/linux/` |
| Allwinner 公共 DTSI | `board/allwinner/suniv-f1c100s/devicetree/linux/` |

---

## 常用构建命令

以下命令应在 Linux 或合适的 WSL 环境中，从 Buildroot 仓库根目录运行。

### 载入 CRA 配置并完整构建

```sh
make cra_epass_defconfig
make -j$(nproc)
```

这会构建工具链、Linux、U-Boot、rootfs 和最终镜像，不会自动烧录实体设备。

### 打开 Linux 内核配置界面

```sh
make linux-menuconfig
```

此命令编辑的是 `output/build/linux-5.4.99/.config`。退出并保存后，还必须把结果写回仓库中的规范配置文件。

### 保存精简后的内核配置

```sh
make linux-update-defconfig
```

当前项目使用 `BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE`，因此该命令会把 `savedefconfig` 结果保存到：

```text
board/cra/epass/linux.defconfig
```

如需保存完整 `.config`，Buildroot 还提供：

```sh
make linux-update-config
```

本项目通常应优先保留精简且可审查的 `linux.defconfig`。

### 仅从编译阶段重建内核

```sh
make linux-rebuild -j$(nproc)
```

该命令适合重新执行内核编译和安装阶段，但不会重新下载、重新解压或从头应用补丁。

修改项目 DTS 后，构建规则会在内核编译阶段重新复制自定义 DTS；完成后仍建议再执行一次完整 `make`，以刷新依赖内核产物的最终镜像。

### 清除内核构建树并重新开始

```sh
make linux-dirclean
make -j$(nproc)
```

下列情况应使用 `linux-dirclean`，而不是只用 `linux-rebuild`：

- 新增、删除、重命名或调整 Linux 补丁；
- 修改补丁内容后需要从未打补丁的源码重新验证；
- 切换内核版本或源码来源；
- 临时内核源码树已经被手动改乱；
- 怀疑旧的构建状态影响结果。

`linux-dirclean` 会删除 `output/build/linux-<版本>/`，但不会烧录设备。

---

## 二次开发方式

### 修改内核选项

```sh
make linux-menuconfig
make linux-update-defconfig
git diff -- board/cra/epass/linux.defconfig
```

### 修改设备树

根据修改的适用范围选择位置：

- F1C100S/F1C200S 公共定义：`board/allwinner/suniv-f1c100s/devicetree/linux/`；
- CRA Electric Pass 公共板级定义：`board/cra/epass/devicetree/linux/base/epass.dtsi`；
- 当前设备树入口：`board/cra/epass/devicetree/linux/base/devicetree.dts`。

不要直接修改 `output/build/linux-5.4.99/arch/arm/boot/dts/` 中复制出来的文件。

### 修改 Linux 源码或驱动

推荐流程为：

1. 在临时内核树中验证修改；
2. 将差异整理成标准 Git patch；
3. 把 CRA 专用补丁放入 `board/cra/epass/patch/linux/`；
4. 使用连续编号控制应用顺序；
5. 执行 `make linux-dirclean`；
6. 重新构建并检查补丁是否能从干净源码稳定应用。

如果修改对所有 suniv F1C100S/F1C200S 板型都成立，才考虑放入 Allwinner 公共补丁目录。

---

<div align="center">

<sub><b>linux/</b> · Linux kernel build infrastructure for Buildroot</sub>

</div>
