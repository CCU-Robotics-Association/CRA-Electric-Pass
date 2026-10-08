<div align="center">

# Buildroot 配置入口

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `configs/` 保存可由 Buildroot 直接载入的板级 defconfig 入口。当前主要目标是 CRA Electric Pass，其他条目来自仓库保留的历史板型支持。
>
> 这些入口通常是指向 `board/` 中规范配置源文件的 Git 符号链接，不应当作彼此独立的配置副本维护。

<p align="center">
  <a href="#当前入口">当前入口</a> ·
  <a href="#入口与规范配置的关系">配置关系</a> ·
  <a href="#git-符号链接">符号链接</a> ·
  <a href="#载入配置">载入配置</a> ·
  <a href="#更新-defconfig">更新配置</a> ·
  <a href="#新增板级目标">新增目标</a> ·
  <a href="#与生成文件的边界">维护边界</a>
</p>

---

## 当前入口

| 入口文件 | Buildroot 命令 | 规范配置源文件 | 用途 |
| --- | --- | --- | --- |
| `cra_epass_defconfig` | `make cra_epass_defconfig` | `board/cra/epass/cra_epass_defconfig` | 当前 CRA Electric Pass 目标 |
| `hatlab_badge200_defconfig` | `make hatlab_badge200_defconfig` | `board/hatlab/badge200/hatlab_badge200_defconfig` | HatLab BADGE200 历史目标 |
| `sipeed_lichee_nano_defconfig` | `make sipeed_lichee_nano_defconfig` | `board/sipeed/lichee/nano/sipeed_lichee_nano_defconfig` | Sipeed Lichee Nano 历史目标 |
| `widora_mangopi_r1_defconfig` | `make widora_mangopi_r1_defconfig` | `board/widora/mangopi/r1/widora_mangopi_r1_defconfig` | Widora MangoPi R1 历史目标 |
| `widora_mangopi_r2_defconfig` | `make widora_mangopi_r2_defconfig` | `board/widora/mangopi/r2/widora_mangopi_r2_defconfig` | Widora MangoPi R2 历史目标 |
| `widora_mangopi_r3_defconfig` | `make widora_mangopi_r3_defconfig` | `board/widora/mangopi/r3/widora_mangopi_r3_defconfig` | Widora MangoPi R3 历史目标 |

---

## 入口与规范配置的关系

以 CRA Electric Pass 为例：

```text
configs/cra_epass_defconfig
        ↓ 符号链接
board/cra/epass/cra_epass_defconfig
        ↓ make cra_epass_defconfig
仓库根目录 .config
        ↓ make
output/
```

三者职责不同：

| 文件 | 性质 |
| :--- | :--- |
| `configs/cra_epass_defconfig` | Buildroot 命令入口，通常为符号链接。 |
| `board/cra/epass/cra_epass_defconfig` | CRA 板级配置的规范源文件。 |
| `.config` | 当前输出目录使用的完整展开配置，属于本地构建状态。 |

---

## Git 符号链接

这些入口在 Git 中应使用模式：

```text
120000
```

即 Git 符号链接。例如 `configs/hatlab_badge200_defconfig` 的链接内容为：

```text
../board/hatlab/badge200/hatlab_badge200_defconfig
```

在正常的 Linux 检出中，`ls -l` 应显示它指向板级配置源文件，而不是一个独立的小型文本文件。

### Windows 检出问题

Windows 未启用符号链接支持时，Git 可能把链接恢复成普通文件，文件内容只有一行目标路径：

```text
../board/cra/epass/cra_epass_defconfig
```

这种文件看起来存在，但 Buildroot 不会把这一行解释为“继续读取另一个配置”。它会把文件本身当成 defconfig，导致配置未被正确载入。

可通过以下方式识别：

```sh
ls -l configs/cra_epass_defconfig
file configs/cra_epass_defconfig
```

如果它是正常链接，`ls -l` 会显示 `->`；如果只是普通 ASCII 文本文件，则说明链接已经在检出过程中损坏。

建议在 Linux 或 WSL 的 Linux 文件系统中检出和构建仓库，并确保 Git 保留符号链接。不要只把 Windows 产生的路径占位文件复制进 WSL。

---

## 载入配置

构建 CRA Electric Pass 前，建议从干净配置开始：

```sh
make distclean
make cra_epass_defconfig
```

然后检查关键架构选项：

```sh
grep -E 'BR2_(arm|arm926t|ARM_EABI|ARM_SOFT_FLOAT|ARM_INSTRUCTIONS_ARM)' .config
```

当前 CRA 目标应使用：

- ARM 架构；
- ARM926T；
- ARM EABI；
- soft-float；
- ARM 指令集。

完成确认后再构建：

```sh
make
```

载入其他板型前也应执行 `make distclean`。不要在同一个输出目录中直接叠加 CRA、BADGE200、Lichee Nano 或 MangoPi 的配置。

---

## 更新 defconfig

需要调整 Buildroot 软件包或系统选项时，可按以下流程操作：

```sh
make cra_epass_defconfig
make menuconfig
make update-defconfig
```

`make update-defconfig` 会把当前配置压缩为最小 defconfig，并写回 `BR2_DEFCONFIG` 指定的入口。如果入口是真实符号链接，写入会落到 `board/cra/epass/cra_epass_defconfig`。

如果 Windows 已把入口变成普通路径文本文件，更新操作可能覆盖 `configs/cra_epass_defconfig` 本身，却没有更新 `board/cra/epass/cra_epass_defconfig`，从而形成两份互相矛盾的配置。因此在执行 `update-defconfig` 前，必须先确认链接状态。

更新后至少检查：

```sh
git diff -- board/cra/epass/cra_epass_defconfig
make cra_epass_defconfig
```

并确认重新载入后关键架构、工具链、内核、U-Boot、rootfs 覆盖层和 post-image 脚本仍然正确。

---

## 新增板级目标

新增目标时应采用以下结构：

```text
board/<vendor>/<board>/<target>_defconfig
configs/<target>_defconfig -> ../board/<vendor>/<board>/<target>_defconfig
```

目标名称应：

- 使用小写字母、数字和下划线；
- 以 `_defconfig` 结尾；
- 与 `make <target>_defconfig` 命令一致；
- 避免只用 `default`、`test` 或 `new` 等模糊名称。

新增链接后应执行：

```sh
make list-defconfigs
make <target>_defconfig
```

确认 Buildroot 能发现入口，并验证其引用的 U-Boot、Linux、设备树、rootfs 和镜像脚本均存在。

---

## 删除或重命名目标

删除板级目标时，不能只删除 `board/` 目录或只删除 `configs/` 入口。至少应同步检查：

1. `configs/<target>_defconfig`；
2. `board/<vendor>/<board>/` 中的规范配置和相关文件；
3. README 中的目标列表与链接；
4. 自动化构建脚本；
5. CI、发布脚本和烧录工具；
6. 其他 defconfig 是否引用被删除的公共层。

目标改名也应同时更新入口名、规范配置名、内部路径、文档和调用命令，避免保留失效链接。

---

## 与生成文件的边界

`configs/` 是构建输入目录，不是生成目录。常见生成或本地状态文件位于：

```text
.config
.config.old
output/build/
output/host/
output/target/
output/images/
```

清理 `output/` 不应删除 `configs/` 中的入口；反过来，修改 `output/` 也不能替代对板级 defconfig 的修改。

---

<div align="center">

<sub><b>configs/</b> · discoverable Buildroot defconfig entries</sub>

</div>
