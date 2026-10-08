<div align="center">

# Buildroot 文档源码

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `docs/` 保存 Buildroot 2020.02.7 用户手册的 AsciiDoc 源码、文档生成规则和必要资源。
>
> 该目录属于上游构建基础的一部分；顶层 Makefile 会直接载入 `docs/manual/manual.mk`

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#conf">conf</a> ·
  <a href="#images">images</a> ·
  <a href="#manual">manual</a> ·
  <a href="#手册章节分类">章节分类</a> ·
  <a href="#生成手册">生成手册</a> ·
  <a href="#文档生成依赖">构建依赖</a>
</p>

---

## 目录结构

```text
docs/
├── README.md
├── conf/
│   └── asciidoc-text.conf
├── images/
│   └── github_hash_mongrel2.png
└── manual/
    ├── manual.txt
    ├── manual.mk
    ├── docbook-xsl.css
    ├── adding-board-support.txt
    ├── adding-packages*.txt
    ├── configure*.txt
    ├── customize*.txt
    ├── rebuilding-packages.txt
    ├── writing-rules.txt
    └── 其他手册章节源码
```

当前共保留：

| 目录 | 文件数 | 作用 |
| --- | ---: | --- |
| `conf/` | 1 | 纯文本输出格式配置 |
| `images/` | 1 | 手册章节引用的示例图片 |
| `manual/` | 71 | 手册入口、构建规则、样式和章节源码 |

---

## `conf/`

### `asciidoc-text.conf`

该文件只用于生成纯文本格式的 Buildroot 手册，主要控制：

- HTTP、HTTPS、FTP、文件、IRC 和邮件链接的文本表现形式；
- 有显示名称和无显示名称的链接如何展开；
- 生成纯文本时隐藏图片宏，避免输出无意义的图像占位内容。

它会由 Buildroot 的通用 AsciiDoc 构建框架按输出格式自动检测和加载。

---

## `images/`

该目录保存手册生成时使用的外部图片资源。

当前仅保留：

| 文件 | 引用位置 | 作用 |
| --- | --- | --- |
| `github_hash_mongrel2.png` | `manual/adding-packages-tips.txt` | 展示 GitHub 源码归档哈希相关示例 |

原始 Buildroot 目录使用 `docs/images -> website/images` 符号链接
---

## `manual/`

`manual/` 是本目录的主体。除 `.mk` 和 CSS 文件外，章节采用 AsciiDoc 风格的 `.txt` 文本编写。

### `manual.txt`

`manual.txt` 是整本手册的入口文件，定义手册标题、版本信息、许可证说明和章节包含顺序。

它将内容组织为四部分：

```text
Getting started
User guide
Developer guide
Appendix
```

章节通过以下语法组合：

```text
include::introduction.txt[]
include::configure.txt[]
include::adding-packages.txt[]
```

因此，单独修改某个章节文件后不需要手工把内容复制回 `manual.txt`；重新生成手册时，AsciiDoc 会按入口文件中的顺序读取所有引用。

### `manual.mk`

`manual.mk` 将 Buildroot 手册接入顶层构建系统：

```make
MANUAL_SOURCES = $(sort $(wildcard docs/manual/*.txt) $(wildcard docs/images/*))
MANUAL_RESOURCES = $(TOPDIR)/docs/images

$(eval $(call asciidoc-document))
```

其作用包括：

- 收集所有手册章节和图片作为构建依赖；
- 指定图片资源目录；
- 调用 `package/doc-asciidoc.mk` 提供的通用文档构建宏；
- 生成 HTML、拆分 HTML、PDF、纯文本和 ePub 目标；
- 把结果输出到 `output/docs/manual/`。

顶层 `Makefile` 会直接执行：

```make
include docs/manual/manual.mk
```

因此不能在不修改顶层 `Makefile` 的情况下删除整个 `docs/` 或 `docs/manual/manual.mk`。否则即使只想构建固件，GNU Make 在解析阶段也可能因为找不到被包含文件而失败。

### `docbook-xsl.css`

该文件为生成的 HTML/DocBook 手册提供样式。

---

## 手册章节分类

当前章节源码可以按用途分为以下几类。

### 入门与基本使用

| 文件 | 内容 |
| --- | --- |
| `introduction.txt` | Buildroot 的目标和基本概念 |
| `prerequisite.txt` | 构建主机依赖 |
| `getting.txt` | 获取 Buildroot 源码 |
| `quickstart.txt` | 快速构建流程 |
| `resources.txt` | Buildroot 社区与参考资源 |
| `configure.txt` | Buildroot 配置界面和主要选项 |
| `configure-other-components.txt` | Linux、U-Boot 等组件配置入口 |
| `common-usage.txt` | 常见使用方式 |
| `make-tips.txt` | Make 命令技巧 |
| `rebuilding-packages.txt` | 单包重建、重新配置和完整清理的区别 |

### 项目定制

`customize*.txt` 章节说明：

- 推荐的项目目录结构；
- Buildroot 配置、Linux 配置和 U-Boot 配置的保存方式；
- rootfs overlay；
- 用户与设备权限表；
- post-build 和 post-image 定制；
- 项目补丁与自定义软件包；
- BR2_EXTERNAL 方式的外部项目维护。

### 板级支持与软件包开发

| 文件或文件组 | 内容 |
| --- | --- |
| `adding-board-support.txt` | 新增板型支持的基本结构 |
| `writing-rules.txt` | `Config.in`、Makefile 和文档的编写规范 |
| `adding-packages.txt` | 新增 Buildroot 软件包的总入口 |
| `adding-packages-directory.txt` | 软件包目录结构 |
| `adding-packages-generic.txt` | generic-package 基础设施 |
| `adding-packages-autotools.txt` | Autotools 软件包 |
| `adding-packages-cmake.txt` | CMake 软件包 |
| `adding-packages-meson.txt` | Meson 软件包 |
| `adding-packages-python.txt` | Python 软件包 |
| `adding-packages-cargo.txt` | Cargo/Rust 软件包 |
| `adding-packages-golang.txt` | Go 软件包 |
| `adding-packages-kconfig.txt` | 使用 Kconfig 的软件包 |
| `adding-packages-kernel-module.txt` | 外部 Linux 内核模块 |
| `adding-packages-linux-kernel-spec-infra.txt` | Linux 内核专用包基础设施 |
| `adding-packages-hooks.txt` | 各构建阶段的 Hook |
| `adding-packages-tips.txt` | 软件包维护技巧 |

目录中还保留 Perl、LuaRocks、Rebar、Waf、virtual package、gettext 和 AsciiDoc 等其他包类型的说明。

### 调试、贡献和附录

| 文件 | 内容 |
| --- | --- |
| `faq-troubleshooting.txt` | 常见问题和故障排查 |
| `known-issues.txt` | Buildroot 已知限制 |
| `debugging-buildroot.txt` | 构建系统调试 |
| `using-buildroot-debugger.txt` | 使用交叉调试器 |
| `patch-policy.txt` | 补丁格式、顺序和许可证原则 |
| `contribute.txt` | 向 Buildroot 上游贡献代码 |
| `developers.txt` | `DEVELOPERS` 文件和维护者规则 |
| `release-engineering.txt` | Buildroot 发布流程 |
| `legal-notice.txt` | 开源许可证与合规说明 |
| `makedev-syntax.txt` | 设备表语法 |
| `makeusers-syntax.txt` | 用户表语法 |
| `migrating.txt` | 旧版本迁移说明 |

---

## 生成手册

以下命令应在 Linux 或配置正确的 WSL 环境中，从 Buildroot 仓库根目录执行。

### 生成所有格式

```sh
make manual
```

### 分别生成指定格式

```sh
make manual-html
make manual-split-html
make manual-pdf
make manual-text
make manual-epub
```

生成结果位于：

```text
output/docs/manual/
```

预期文件包括：

```text
manual.html
manual.pdf
manual.text
manual.epub
```

拆分 HTML 会生成多文件页面结构，具体文件名由 AsciiDoc/DocBook 工具链决定。

### 清理生成的手册

```sh
make manual-clean
```

该命令删除文档构建目录中的生成物，不会删除 `docs/manual/` 下的源文件，也不会影响固件输出。

---

## 文档生成依赖

根据当前 `package/doc-asciidoc.mk`，生成文档至少需要：

| 工具 | 用途 |
| --- | --- |
| AsciiDoc/a2x | 解析手册源码并生成各种格式 |
| `w3m` | 文本输出和文档转换辅助 |
| `rsync` | 把手册源码复制到临时文档构建目录 |
| `xsltproc` | DocBook/XSL 转换 |
| `dblatex` | 生成 PDF |

PDF 构建对 `xsltproc` 版本存在额外要求。当前构建规则会检查旧版 `xsltproc` 的已知问题，并在不满足条件时禁用 PDF 生成或给出提示。

---

<div align="center">

<sub><b>docs/</b> · Buildroot 2020.02.7 manual sources</sub>

</div>
