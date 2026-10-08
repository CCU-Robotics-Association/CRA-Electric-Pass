<div align="center">

# Buildroot 主机构建支持工具

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `support/` 保存 Buildroot 2020.02.7 在构建主机上使用的依赖检查、源码下载、Kconfig、补丁、许可证收集、SDK 处理、统计分析与自动化测试工具。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#是否参与正常构建">构建关系</a> ·
  <a href="#dependencies">依赖检查</a> ·
  <a href="#download">下载框架</a> ·
  <a href="#kconfig">Kconfig</a> ·
  <a href="#scripts">脚本工具</a> ·
  <a href="#testing">自动测试</a> ·
  <a href="#语法与注释">语法说明</a>
</p>

---

## 目录结构

```text
support/
├── README.md
├── config-fragments/
├── dependencies/
├── docker/
├── download/
├── gnuconfig/
├── kconfig/
├── legal-info/
├── libtool/
├── misc/
├── scripts/
└── testing/
```

当前目录约有 125 个子目录和 459 个受版本控制的文件，其中包含 Shell、Python、C、Make、Kconfig、配置片段、补丁和测试资源。

## 是否参与正常构建

| 目录 | 正常构建是否可能调用 | 说明 |
| --- | --- | --- |
| `dependencies/` | 是 | 构建开始前检查主机环境和工具 |
| `download/` | 是 | 软件包下载、Git 导出和哈希校验 |
| `gnuconfig/` | 是 | 为部分 Autotools 软件包更新 `config.guess/config.sub` |
| `kconfig/` | 是 | 提供 `defconfig`、`menuconfig`、`nconfig` 等配置前端 |
| `libtool/` | 是 | 修补软件包自带的 Libtool 脚本 |
| `misc/` | 是或按配置调用 | CMake toolchain、SDK 重定位、目标目录警告等 |
| `scripts/` | 是或按目标调用 | 补丁、用户、RPATH、依赖图、体积统计等通用脚本 |
| `legal-info/` | 仅 `legal-info` 目标 | 许可证报告模板和 Buildroot 自身哈希 |
| `config-fragments/` | 通常否 | 官方自动构建和最小配置测试素材 |
| `docker/` | 否 | 历史 CI 容器定义 |
| `testing/` | 否 | Buildroot QEMU/运行时测试框架 |

## `config-fragments/`

该目录保存 Buildroot 维护和自动构建所用的配置片段。

### `minimal.config`

用于关闭 BusyBox、Shell、init 和 tar rootfs 等默认项，以获得最小的测试配置。它不是 CRA Electric Pass 的 defconfig。

### `autobuild/`

包含多种架构、C 库和外部工具链组合，例如 ARM、AArch64、MIPS、RISC-V、PowerPC、x86、uClibc、musl 和 glibc。`toolchain-configs.csv` 描述自动构建使用的工具链配置集合。

这些文件用于扩大 Buildroot 上游测试覆盖面，不会被当前 ARM926T 固件自动合并。

CRA 的实际配置位于：

```text
board/cra/epass/cra_epass_defconfig
```

## `dependencies/`

该目录负责检查构建主机，而不是检查电子通行证硬件。

### 主流程

`dependencies.mk` 定义顶层 `dependencies` 目标，并按需包含各种 `check-host-*.mk`。`dependencies.sh` 会检查：

- `PATH` 和 `LD_LIBRARY_PATH` 中是否错误包含当前目录；
- `grep`、`sed`、`which`、`file` 等基础命令；
- GNU Make、GCC、patch、tar、cpio、unzip、rsync 等版本和行为；
- 下载工具是否满足当前选包需要；
- 主机文件系统是否区分大小写并支持必要功能；
- 环境变量是否会污染 Perl 或其他构建流程。

### 专用检查

| 文件组 | 用途 |
| --- | --- |
| `check-host-make.*` | 判断系统 GNU Make 是否可用，否则构建 Host Make |
| `check-host-cmake.*` | 判断主机 CMake 是否满足要求 |
| `check-host-python3.*` | 检查 Python 3 |
| `check-host-tar.*` | 检查 tar 特性和兼容性 |
| `check-host-gzip.*`、`check-host-lzip.*`、`check-host-xzcat.*` | 检查解压工具 |
| `check-host-coreutils.*` | 检查基础 GNU 工具 |
| `check-host-bison-flex.mk` | 处理 Bison/Flex 需求 |
| `check-host-asciidoc.sh` | 生成文档时检查 AsciiDoc |

Buildroot 2020.02.7 的这些检查面向 Linux/Unix 主机。它们不是原生 PowerShell 或 CMD 脚本。

## `download/`

这是 Buildroot 的统一下载层，由 `package/pkg-download.mk` 调用。

### `dl-wrapper`

`dl-wrapper` 负责：

1. 在临时目录中下载；
2. 尝试主站、备用地址和镜像；
3. 调用后端导出源码；
4. 通过 `.hash` 文件验证；
5. 成功后原子地移入 `dl/` 缓存；
6. 失败时清理不完整文件。

这样可以避免半截下载直接污染长期缓存。

### 下载后端

| 文件 | 获取方式 |
| --- | --- |
| `wget` | HTTP/HTTPS/FTP 等普通文件下载 |
| `git` | Git 仓库导出，可处理子模块 |
| `svn` | Subversion |
| `hg` | Mercurial |
| `cvs` | CVS |
| `bzr` | Bazaar |
| `scp` | SSH/SCP 文件复制 |
| `file` | 本地文件或目录 |

### `check-hash`

按软件包 `.hash` 文件校验 MD5、SHA-1、SHA-224、SHA-256、SHA-384 或 SHA-512，并区分以下情况：

- 哈希匹配；
- 哈希不匹配；
- `.hash` 文件存在但缺少目标文件条目；
- 使用了不支持的哈希类型；
- 软件包完全没有 `.hash` 文件。

下载工具会访问网络和 `dl/` 缓存，但不会连接或刷写实体电子通行证。

## `gnuconfig/`

包含 GNU Config 的：

```text
config.guess
config.sub
```

它们用于识别构建主机和目标 triplet。Autotools 软件包中的旧副本可能无法识别新架构，`pkg-autotools.mk` 会在适当阶段用这里的版本替换。

`README.buildroot` 记录该副本基于 GNU config Git 提交：

```text
104ee6463c4bfaac3f3029d9be9bdd6e93879323
```

## `kconfig/`

该目录是从 Linux 内核 Kconfig 工具复制并为 Buildroot 修改的配置系统。`README.buildroot` 说明当前基础来自 Linux 4.17-rc2。

### 主要前端

| 文件 | 对应功能 |
| --- | --- |
| `conf.c` | 非交互式 `config/oldconfig/defconfig` 处理 |
| `mconf.c` + `lxdialog/` | `make menuconfig` |
| `nconf.c` | `make nconfig` |
| `qconf.cc` | Qt `make xconfig` |
| `gconf.c`、`gconf.glade` | GTK `make gconfig` |
| `confdata.c`、`symbol.c`、`expr.c`、`menu.c` | Kconfig 解析和符号求值核心 |
| `merge_config.sh` | 合并配置片段 |

`patches/` 中的补丁将内核版 Kconfig 改造成 Buildroot 可用版本，包括 BR2 前缀、外部树、输出目录和菜单界面调整。

## `legal-info/`

该目录为 `make legal-info` 提供报告模板：

| 文件 | 作用 |
| --- | --- |
| `README.header` | 说明收集的源码、配置、清单和许可证材料 |
| `README.warnings-header` | 引出无法收集或元数据不完整的警告 |
| `buildroot.hash` | 校验 Buildroot 自身许可证文件 |

legal-info 结果通常包含：

- 目标包和 Host 包清单；
- 软件包版本及许可证；
- 可再分发源码归档；
- 已应用补丁；
- 许可证正文；
- 无法自动收集项目的警告。

## `libtool/`

包含针对不同 Libtool 版本的 Buildroot 补丁：

```text
buildroot-libtool-v1.5.patch
buildroot-libtool-v2.2.patch
buildroot-libtool-v2.4.patch
buildroot-libtool-v2.4.4.patch
```

`pkg-autotools.mk` 会识别软件包中的 Libtool 脚本并应用相应补丁，以修复交叉编译、静态链接、sysroot 和安装路径问题。

## `misc/`

| 文件 | 用途 |
| --- | --- |
| `Buildroot.cmake` | 为 CMake 软件包提供 Buildroot 辅助模块 |
| `toolchainfile.cmake.in` | 生成交叉编译 CMake Toolchain 文件的模板 |
| `relocate-sdk.sh` | 导出 SDK 后修正路径，使工具链可重定位 |
| `target-dir-warning.txt` | 提醒 `output/target/` 不能直接当作完整 rootfs 部署 |
| `utils.mk` | Buildroot 顶层 Make 使用的通用函数 |
| `Vagrantfile` | 历史虚拟机构建环境 |

`target-dir-warning.txt` 很重要：普通用户权限下的 `output/target/` 还没有 fakeroot 阶段最终设备节点、属主和权限，不能直接复制到 SD 卡或 NFS 作为正式根文件系统。应使用 `output/images/` 中完成打包的镜像。

## `docker/`

这里的 Dockerfile 原本用于 Buildroot GitLab CI，基础镜像为 2017 年的 Debian Stretch，并安装构建、下载、QEMU 和 Python 测试依赖。

它具有以下特点：

- 只构建主机环境，不生成设备容器运行时；
- 不会自动挂载本项目或构建 CRA 固件；
- 依赖历史 Debian 软件源；
- 未针对当前 Windows、WSL 或 ARM926T 项目重新验证。
## `scripts/`

### 正常构建辅助脚本

| 脚本 | 作用 |
| --- | --- |
| `apply-patches.sh` | 按 `series` 或文件名顺序递归应用软件包补丁 |
| `check-bin-arch` | 检查目标目录 ELF 架构，避免把 Host 二进制装进 rootfs |
| `check-host-rpath` | 检查 Host 工具是否带有正确 RPATH |
| `check-kernel-headers.sh` | 核对工具链内核头文件版本 |
| `check-merged-usr.sh` | 检查 skeleton/Overlay 是否符合 merged `/usr` 布局 |
| `fix-rpath` | 清理 Host、Staging 和 Target 中不应保留的构建路径 |
| `hardlink-or-copy` | 优先硬链接文件，失败时退回复制 |
| `mkmakefile` | 在输出目录生成转发到源码树的 Makefile |
| `mkusers` | 根据 users table 生成用户、组、密码和 fakeroot 操作 |
| `pycompile.py` | 为交叉构建的目标 Python 模块生成 `.pyc` |
| `setlocalversion` | 根据 Git/Hg/SVN 状态生成本地版本后缀 |

### 条件性或维护工具

| 脚本 | 作用 |
| --- | --- |
| `br2-external` | 验证并注册外部 Buildroot 树 |
| `eclipse-register-toolchain` | 将 Buildroot 工具链登记到历史 Eclipse 插件 |
| `expunge-gconv-modules` | 精简 glibc 字符集转换模块清单 |
| `fix-configure-powerpc64.sh` | 修复旧 Autoconf 对 PowerPC64 的特定判断问题 |
| `generate-gitlab-ci-yml` | 根据 defconfig 和运行时测试生成 CI 条目 |
| `genimage.sh` | 用 Host genimage 和指定配置组装通用磁盘镜像 |
| `graph-build-time` | 生成软件包或阶段构建时间图 |
| `graph-depends` | 生成当前配置的软件包依赖图 |
| `pkg-stats` | 统计软件包版本、许可证、补丁及已知安全信息 |
| `size-stats` | 分析目标文件与软件包体积占用 |
| `brpkgutil.py` | 为依赖图和包统计提供公共 Python 函数 |

## `testing/`

该目录是 Buildroot 的 Python 运行时测试框架。

### 结构

| 路径 | 作用 |
| --- | --- |
| `run-tests` | nose2 测试入口 |
| `infra/builder.py` | 为每个用例生成配置并执行 Buildroot |
| `infra/emulator.py` | 控制 QEMU |
| `infra/basetest.py` | 测试基类、日志和超时 |
| `conf/` | 测试使用的内核片段、引导配置和容器配置 |
| `tests/core/` | rootfs、密码、时区、Overlay 等核心测试 |
| `tests/fs/` | ext、UBI、JFFS2、SquashFS、YAFFS2 等文件系统测试 |
| `tests/init/` | BusyBox、无 init、systemd 测试 |
| `tests/package/` | 各类软件包运行测试 |
| `tests/download/` | Git 引用、哈希、子模块和下载测试夹具 |
| `tests/toolchain/` | 外部工具链测试 |

## 语法与注释

| 文件类型 | 语言 | 常用注释 |
| --- | --- | --- |
| `.sh` 或无扩展名脚本 | POSIX Shell/Bash | `# 注释`，保留 LF 和 shebang |
| `.py` | Python | `# 注释` 或 docstring |
| `.mk` | GNU Make | `# 注释`，配方命令使用 Tab |
| `.c/.h` | C/C++ | `/* ... */` 或 `//`，遵循原文件风格 |
| `.config` | Kconfig 片段 | `# 注释` 或 `# CONFIG_x is not set` |
| `.patch` | Unified diff | 不破坏补丁头、上下文和原始行尾 |

---

<div align="center">

<sub><b>support/</b> · host-side helpers, infrastructure and tests for Buildroot</sub>

</div>
