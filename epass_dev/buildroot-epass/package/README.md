<div align="center">

# Buildroot 软件包系统

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `package/` 是 Buildroot 2020.02.7 的软件包定义与构建基础设施。它描述第三方软件和项目程序的来源、依赖、交叉编译流程，以及主机构建环境、交叉编译 sysroot 与目标 rootfs 的安装边界。

<p align="center">
  <a href="#目录概览">目录概览</a> ·
  <a href="#软件包系统解决什么问题">工作职责</a> ·
  <a href="#根目录基础设施文件">基础设施</a> ·
  <a href="#一个普通软件包的组成">包的组成</a> ·
  <a href="#cra-electric-pass-当前选包">CRA 选包</a> ·
  <a href="#常用包级构建目标">构建目标</a> ·
  <a href="#语法与注释">语法说明</a>
</p>

---

## 目录概览

```text
package/
├── README.md
├── Config.in
├── Config.in.host
├── Makefile.in
├── pkg-generic.mk
├── pkg-download.mk
├── pkg-utils.mk
├── pkg-autotools.mk
├── pkg-cmake.mk
├── pkg-kconfig.mk
├── pkg-kernel-module.mk
├── pkg-python.mk
├── pkg-meson.mk
├── pkg-waf.mk
├── pkg-golang.mk
├── pkg-perl.mk
├── pkg-luarocks.mk
├── pkg-rebar.mk
├── pkg-virtual.mk
├── doc-asciidoc.mk
├── epass_drm_app/
├── epass_usb_responder/
├── epassctl/
├── srgn_config/
├── libcedarc/
├── libcedarx/
└── <大量 Buildroot 上游软件包目录>/
```

当前快照约包含：

| 类型 | 数量 |
| --- | ---: |
| 一级软件包目录 | 2337 |
| `Config.in*` 文件 | 2617 |
| `.mk` 文件 | 2646 |
| `.hash` 文件 | 2592 |
| `.patch` 文件 | 1707 |

这些数量只用于说明目录规模，不是稳定接口。

## 软件包系统解决什么问题

Buildroot 的每个包定义可以处理以下阶段：

1. 根据 Kconfig 判断包是否启用；
2. 解析直接和间接依赖；
3. 从 Git、HTTP、镜像站或本地目录获取源码；
4. 校验下载文件哈希；
5. 解压源码；
6. 按顺序应用补丁；
7. 为目标架构执行配置；
8. 使用交叉工具链编译；
9. 把头文件和开发库安装到 staging sysroot；
10. 把运行程序和动态库安装到目标 rootfs；
11. 记录许可证文件并支持 legal-info；
12. 为增量构建维护各阶段的 stamp 文件。

Buildroot 生成的是完整固件 rootfs，不是面向设备的二进制包仓库。即使选用了 `opkg`、`rpm` 等包管理器，Buildroot 本身也不会自动生成可在线安装的软件包数据库。

## 根目录基础设施文件

### 配置入口

| 文件 | 作用 |
| --- | --- |
| `Config.in` | 定义 Target packages 菜单，并 `source` 各目标软件包的 `Config.in` |
| `Config.in.host` | 定义只在构建主机运行的 Host utilities 菜单 |

`Config.in` 按音视频、调试工具、文件系统、图形、硬件、脚本语言、库、网络、系统工具等类别组织软件包。它只控制菜单、依赖和选择关系，不执行编译。

当前电子通行证相关条目直接加入了这个总配置文件：

- `epass_drm_app`、`epass_usb_responder`、`epassctl` 位于 Audio and video applications；
- `srgn_config` 位于 Hardware handling 附近；
- `libcedarc`、`libcedarx` 位于 Libraries；
- `fb-test-app`、`tinyalsa`、`umtprd` 等沿用其通用分类。

其中 USB responder 和 CLI 放在音视频菜单并不理想。若以后整理菜单，应只调整 `source` 所在分类，不要改动其 Kconfig 符号名称，否则会影响 defconfig 兼容性。

### 通用 Make 基础设施

| 文件 | 主要职责 |
| --- | --- |
| `Makefile.in` | 定义目标交叉编译器、编译参数、安装工具和全局构建环境变量 |
| `pkg-generic.mk` | 核心包生命周期、依赖、stamp、下载到安装目标以及清理目标 |
| `pkg-download.mk` | 下载地址、镜像回退、缓存和获取方法 |
| `pkg-utils.mk` | Kconfig 操作、包名转换、解压器和其他公共函数 |
| `pkg-autotools.mk` | Autoconf/Automake 软件包基础设施 |
| `pkg-cmake.mk` | CMake 软件包基础设施和交叉编译 Toolchain 配置 |
| `pkg-kconfig.mk` | Linux、BusyBox、U-Boot 一类 Kconfig 项目的配置管理 |
| `pkg-kernel-module.mk` | 外部 Linux 内核模块构建接口 |
| `pkg-python.mk` | Python distutils/setuptools 包 |
| `pkg-meson.mk` | Meson/Ninja 包 |
| `pkg-waf.mk` | Waf 包 |
| `pkg-golang.mk` | Go 包和交叉编译环境 |
| `pkg-perl.mk` | Perl 包 |
| `pkg-luarocks.mk` | LuaRocks 包 |
| `pkg-rebar.mk` | Erlang/Rebar 包 |
| `pkg-virtual.mk` | 虚拟包及其具体 provider 选择 |
| `doc-asciidoc.mk` | Buildroot 文档生成规则，不是目标设备软件 |

## 一个普通软件包的组成

典型目录如下：

```text
package/example/
├── Config.in
├── example.mk
├── example.hash
├── 0001-fix-cross-build.patch
└── 0002-fix-runtime-path.patch
```

### `Config.in`

Kconfig 文件通常定义：

- `BR2_PACKAGE_<NAME>` 开关；
- 架构、C 库、线程、动态链接等依赖；
- 自动选择的依赖包；
- 子功能选项；
- 对不满足条件的提示；
- 简短说明和上游项目地址。

Kconfig 中的 `depends on` 只控制配置是否允许选择，不能替代 `.mk` 中的构建依赖。

### `<package>.mk`

GNU Make 配方通常定义：

| 变量 | 含义 |
| --- | --- |
| `<PKG>_VERSION` | 源码版本、标签或提交号 |
| `<PKG>_SITE` | 下载地址或 Git 仓库 |
| `<PKG>_SITE_METHOD` | `git`、`local` 等获取方式 |
| `<PKG>_SOURCE` | 非默认的源码包文件名 |
| `<PKG>_LICENSE` | SPDX 风格许可证标识 |
| `<PKG>_LICENSE_FILES` | 源码中的许可证文件 |
| `<PKG>_DEPENDENCIES` | 编译顺序和 sysroot 依赖 |
| `<PKG>_CONF_OPTS` | 配置系统附加参数 |
| `<PKG>_INSTALL_STAGING` | 是否安装开发文件到 staging |
| `<PKG>_INSTALL_TARGET` | 是否安装运行文件到目标 rootfs |
| `<PKG>_*_CMDS` | 覆盖配置、编译或安装阶段的具体命令 |

文件末尾通过基础设施宏注册软件包，例如：

```make
$(eval $(generic-package))
$(eval $(autotools-package))
$(eval $(cmake-package))
```

包目录名中的连字符会转换为 Make 变量中的下划线。例如 `fb-test-app` 使用 `FB_TEST_APP_*`。

### `<package>.hash`

哈希文件用于校验下载归档和许可证文本，常见格式为：

```text
sha256  <SHA-256>  <文件名>
```

缺少哈希并不一定阻止 Git 获取方式工作，但会削弱供应链审计、离线复现和来源核对能力。

### 补丁

软件包目录中的补丁通常按文件名顺序应用：

```text
0001-...
0002-...
```

补丁应说明目的、上游状态和来源。修改源码版本后，必须重新验证所有补丁能否干净应用。

## Target、Staging 与 Host

Buildroot 中几个目录的职责不同：

| 构建目录 | 内容 |
| --- | --- |
| `dl/` | 下载缓存 |
| `output/build/<pkg>-<version>/` | 解压、打补丁和编译后的临时源码树 |
| `output/host/` | 主机构建工具以及 Buildroot 交叉工具链 |
| `output/staging/` | 指向目标 sysroot 的开发视图，提供头文件和链接库 |
| `output/target/` | 尚未打包的目标根文件系统 |
| `output/images/` | 最终内核、rootfs、Bootloader 和组合镜像 |

目标软件包使用交叉编译器并可能安装到 staging/target；Host package 在电脑上运行，安装到 `output/host/`，不会被直接复制到电子通行证。

常用变量：

| 变量 | 作用 |
| --- | --- |
| `$(@D)` | 当前软件包的构建目录 |
| `$(TARGET_DIR)` | 目标 rootfs |
| `$(STAGING_DIR)` | 目标 sysroot |
| `$(HOST_DIR)` | 主机工具和交叉工具链 |
| `$(TARGET_CC)` | 目标 C 交叉编译器 |
| `$(TARGET_CROSS)` | 交叉工具链前缀 |
| `$(TARGET_CONFIGURE_OPTS)` | 通用交叉配置环境 |

不要直接修改 `output/build/` 或 `output/target/` 中的文件作为长期源码。重新构建或清理后，这些生成内容会被覆盖。

## CRA Electric Pass 当前选包

当前配置入口为：

```text
board/cra/epass/cra_epass_defconfig
```

### 项目专用程序

| 软件包 | 作用 | 安装位置 | 构建方式 |
| --- | --- | --- | --- |
| `epass_drm_app` | DRM/LVGL 主界面程序 | `/root/epass_drm_app` | CMake |
| `epass_usb_responder` | 设备 USB 请求响应程序 | `/usr/bin/usb_responder` | CMake |
| `epassctl` | 主程序控制命令行工具 | `/usr/bin/epassctl` | CMake |
| `srgn_config` | 设备树/底层配置入口 | `/usr/bin/srgn_config` | CMake |

从 Git 历史可确认：

- `srgn_config` 于 2026-01-19 加入当前项目分支；
- `epass_drm_app` 和 `libcedarx` 于 2026-01-20 加入；
- `epass_usb_responder` 和 `epassctl` 于 2026-05-18 加入。

### 多媒体依赖

| 软件包 | 作用 | 当前情况 |
| --- | --- | --- |
| `libcedarc` | Allwinner Cedar 视频引擎底层库及解码库 | 随初始分支导入，并含 F1C100s 像素格式补丁 |
| `libcedarx` | 主程序使用的 CedarX 多媒体中间层 | 固定到特定 Git 提交，依赖 OpenSSL 和 `libcedarc` |
| `tinyalsa` | 精简 ALSA 用户态库和工具 | Buildroot 通用包 |
| `libdrm` | DRM/KMS 用户态库 | 主程序渲染依赖 |
| `libpng`、`jpeg-turbo`、FreeType | 图片和字体处理 | 主程序资源依赖 |

`sunxi-cedarx` 是目录中另一个较旧的通用 CedarX 包，当前 `cra_epass_defconfig` 没有选择它。它与当前选择的 `libcedarc + libcedarx` 不是同一个包，不能混为一谈。

### 调试、存储和设备工具

当前还显式选择了：

- GDB 与部分 Host GDB 功能；
- MTD 分区工具，但关闭了若干直接擦写 NAND 的危险子工具；
- `fb-test-app`、`evtest`、`minicom`；
- eudev、`umtprd`；
- libgpiod 及其工具、mtdev、tslib、libevdev；
- Dropbear；
- genimage、mtd-utils、mtools、U-Boot tools 等 Host 镜像生成工具。

defconfig 未直接写出的依赖仍会由 Kconfig 和 `<PKG>_DEPENDENCIES` 自动加入构建。

## 主程序源码边界

当前 `package/epass_drm_app/epass_drm_app.mk` 默认定义：

```make
EPASS_DRM_APP_VERSION = a2.7.0
EPASS_DRM_APP_SITE = https://github.com/rhodesepass/drm_app_neo.git
EPASS_DRM_APP_SITE_METHOD = git
```

本地开发可通过 Buildroot 的 override 机制显式指定源码。例如在不提交的顶层 `local.mk` 中配置：

```make
EPASS_DRM_APP_OVERRIDE_SRCDIR = $(TOPDIR)/../drm_app_neo
```

## 常用包级构建目标

在已经加载正确 Buildroot 配置的前提下，可使用：

```sh
make <package>
make <package>-source
make <package>-rebuild
make <package>-reconfigure
make <package>-dirclean
```

含义：

| 目标 | 作用 |
| --- | --- |
| `<package>` | 构建该包及所需依赖 |
| `<package>-source` | 只获取该包源码 |
| `<package>-rebuild` | 重新执行编译和安装阶段 |
| `<package>-reconfigure` | 重新配置后再编译和安装 |
| `<package>-dirclean` | 删除该包的构建目录，下次从解压/同步阶段重来 |

这些目标仍可能改写 `output/`，但不会自动更新实体设备。构建完成与烧录是两个独立步骤。

## 语法与注释

| 文件 | 语言 | 注释和格式要求 |
| --- | --- | --- |
| `Config.in` | Kconfig | `# 注释`；`help` 内容保持 Tab/空格缩进 |
| `.mk` | GNU Make | `# 注释`；配方命令必须以 Tab 开头 |
| `.hash` | 空白分隔文本 | `# 注释`；哈希、文件名必须与下载产物严格一致 |
| `.patch` | Unified diff/邮件补丁 | 保留补丁头、上下文和行尾，不在补丁正文随意改缩进 |

---

<div align="center">

<sub><b>package/</b> · package metadata and build infrastructure for Buildroot</sub>

</div>
