<div align="center">

# Buildroot 开发辅助工具

<sub>Configuration, package, analysis and maintenance utilities for Buildroot</sub>

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `utils/` 保存 Buildroot 2020.02.7 面向开发者和维护者的主机端辅助工具。它们用于操作配置、检查软件包、生成测试配置、定位维护者、生成包定义和比较体积，不会被复制到 CRA Electric Pass 的目标 rootfs。

<p align="center">
  <a href="#目录职责">目录职责</a> ·
  <a href="#工具分类">工具分类</a> ·
  <a href="#常用方式">常用方式</a> ·
  <a href="#依赖与兼容性">依赖与兼容性</a> ·
  <a href="#维护与安全边界">维护边界</a>
</p>

---

## 目录职责

<table>
<tr>
<td width="25%" valign="top">

### 配置处理

读取、修改或比较 Buildroot `.config`，并生成随机测试配置。

</td>
<td width="25%" valign="top">

### 质量检查

检查软件包元数据、补丁和 Buildroot 文件风格，并跨工具链测试单个软件包。

</td>
<td width="25%" valign="top">

### 包定义生成

从 PyPI 或 CPAN 元数据生成 Buildroot 软件包骨架，供维护者人工审核。

</td>
<td width="25%" valign="top">

### 维护分析

定位受影响维护者、记录构建日志，并比较不同构建结果的 rootfs 体积。

</td>
</tr>
</table>

```text
utils/
├── brmake
├── check-package
├── checkpackagelib/
├── config
├── diffconfig
├── genrandconfig
├── get-developers
├── getdeveloperlib.py
├── scancpan
├── scanpypi
├── size-stats-compare
├── test-pkg
└── readme.txt
```

`readme.txt` 与 `checkpackagelib/readme.txt` 是随 Buildroot 上游提供的原始说明；本文件提供面向当前仓库的中文入口，不替代脚本自身的 `--help`。

---

## 工具分类

### 配置与构建

| 工具 | 作用 | 主要影响 |
| :--- | :--- | :--- |
| `brmake` | 包装 `make`，给日志逐行加时间戳，将完整输出追加到 `br.log`，终端只显示 Buildroot 的 `>>>` 阶段信息 | 执行用户传入的真实构建目标，并写入 `br.log` |
| `config` | 从命令行启用、禁用、设置或查询 Kconfig 符号 | 直接修改指定 `.config`；不验证配置合法性 |
| `diffconfig` | 比较两个 `.config`，仅显示 Kconfig 值变化 | 默认只读；可输出适合合并的配置片段 |
| `genrandconfig` | 随机选择工具链和软件包配置，供 autobuilder 测试 | 会生成测试配置，并可能访问网络或触发后续构建流程 |

### 软件包维护

| 工具 | 作用 | 说明 |
| :--- | :--- | :--- |
| `check-package` | 检查 `Config.in`、`.mk`、`.hash` 和补丁等文件的格式与常见错误 | 具体规则位于 `checkpackagelib/` |
| `checkpackagelib/` | `check-package` 的 Python 检查模块 | 包含 Kconfig、hash、Makefile 与 patch 检查器 |
| `test-pkg` | 在多套工具链配置下测试一个软件包 | 面向维护者，可能创建或清理构建目录并执行长时间构建 |
| `scanpypi` | 根据 PyPI 元数据生成 Python 软件包骨架 | 结果必须人工检查依赖、许可证和 hash |
| `scancpan` | 根据 CPAN 元数据生成 Perl 软件包骨架 | 内含打包后的 Perl 依赖代码，生成结果仍需审核 |

### 分析与协作

| 工具 | 作用 | 数据来源 |
| :--- | :--- | :--- |
| `get-developers` | 按补丁、文件、软件包或架构查找应通知的维护者 | 根目录 `DEVELOPERS` 与 `getdeveloperlib.py` |
| `getdeveloperlib.py` | 为 `get-developers` 提供补丁解析、路径匹配和维护者数据处理 | Python 库，不是主要命令入口 |
| `size-stats-compare` | 比较两份 `file-size-stats.csv`，定位 rootfs 体积变化 | `make graph-size` 生成的 CSV |

---

## 常用方式

以下命令均应在 Buildroot 仓库根目录执行。

### 检查软件包文件

```sh
./utils/check-package package/<name>/Config.in \
  package/<name>/<name>.mk \
  package/<name>/<name>.hash
```

### 比较配置变化

```sh
./utils/diffconfig .config.old .config
```

生成可作为配置片段使用的形式：

```sh
./utils/diffconfig -m .config.old .config
```

### 查询配置或修改副本

```sh
./utils/config --file .config --state BR2_PACKAGE_BUSYBOX
cp .config /tmp/cra-epass.config
./utils/config --file /tmp/cra-epass.config --enable BR2_PACKAGE_BUSYBOX
```

> [!IMPORTANT]
> `utils/config` 默认会为符号补上 `BR2_` 前缀，并把名称转换为大写；它不会执行 Kconfig 依赖检查。修改后仍需运行 `make olddefconfig` 或相应配置目标进行规范化与验证。

### 查找维护者

```sh
./utils/get-developers -f board/cra/epass/
./utils/get-developers -p busybox
```

### 比较 rootfs 体积

```sh
./utils/size-stats-compare \
  before/file-size-stats.csv \
  after/file-size-stats.csv
```

具体参数以各工具的 `--help` 输出为准。

---

## 依赖与兼容性

| 类别 | 注意事项 |
| :--- | :--- |
| Shell 工具 | 依赖 Linux/Unix shell 环境；`brmake` 还需要由 `expect` 提供的 `unbuffer` |
| Python 工具 | 该 Buildroot 版本的部分脚本保留 Python 2/3 兼容写法，并可能依赖 `six` 等模块 |
| Perl 工具 | `scancpan` 需要 Perl；脚本内含 FatPacker 打包的模块代码 |
| 网络访问 | `genrandconfig`、`scanpypi`、`scancpan` 等功能可能访问外部服务或下载元数据 |
| 构建环境 | 应在保留 LF、可执行位和符号链接的 Linux 工作树中运行 |

> [!WARNING]
> 当前 Windows 工作树适合浏览和编辑，但不能据此假定所有脚本可直接在 PowerShell 中运行。不要把 CRLF、丢失的可执行位或失效符号链接带入正式构建环境。

---

## 维护与安全边界

- 本目录主要来自 Buildroot 上游，应优先保持与当前 Buildroot 版本一致。
- 运行前先阅读 `--help`；对没有 `--help` 的脚本，检查源码和上游 `readme.txt`。
- 对 `.config` 进行自动修改前先保留副本，并在修改后重新解析 Kconfig。
- `test-pkg`、`genrandconfig` 和 `brmake` 可能执行真实构建并改写输出目录，不能当作纯只读检查。
- `scanpypi` 与 `scancpan` 只生成初始骨架；许可证、下载地址、依赖、hash 和安装路径必须人工核验。
- `get-developers` 产生的是维护协作信息，不会替代代码审查或构建验证。
- 本目录工具不负责固件烧录；若某个后续命令涉及设备写入，应与构建和静态检查明确分开。

---

<div align="center">

<sub><b>utils/</b> · developer utilities shipped with Buildroot 2020.02.7</sub>

</div>
