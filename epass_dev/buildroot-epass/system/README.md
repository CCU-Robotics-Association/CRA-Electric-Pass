<div align="center">

# Buildroot 系统基础配置

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `system/` 是 Buildroot 2020.02.7 的通用系统配置层，负责定义根文件系统骨架、初始化系统、设备节点管理、账号与终端选项、文件权限以及构建后处理接口。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#本目录在构建中的位置">构建位置</a> ·
  <a href="#文件说明">文件说明</a> ·
  <a href="#skeleton-基础根文件系统">Skeleton</a> ·
  <a href="#cra-electric-pass-当前配置">CRA 配置</a> ·
  <a href="#哪些内容可以修改">修改边界</a> ·
  <a href="#二次开发原则">开发原则</a>
</p>

---

> [!IMPORTANT]
> 本目录主体来自上游 Buildroot，属于构建系统基础设施。CRA 专用内容通常应放在 `board/cra/epass/`、项目包或 rootfs Overlay 中，而不应直接写进这里的通用骨架。

## 目录结构

```text
system/
├── README.md
├── Config.in
├── system.mk
├── device_table.txt
├── device_table_dev.txt
└── skeleton/
    ├── dev/
    │   ├── .empty
    │   ├── fd      -> ../proc/self/fd
    │   ├── stdin   -> ../proc/self/fd/0
    │   ├── stdout  -> ../proc/self/fd/1
    │   └── stderr  -> ../proc/self/fd/2
    └── etc/
        ├── group
        ├── hosts
        ├── mtab        -> ../proc/self/mounts
        ├── passwd
        ├── profile
        ├── protocols
        ├── resolv.conf -> ../tmp/resolv.conf
        ├── services
        ├── shadow
        └── profile.d/
            └── umask.sh
```

箭头表示这些路径在 Git 中应当是符号链接，不是内容仅为目标路径的普通文本文件。

## 本目录在构建中的位置

默认根文件系统大致按以下顺序形成：

1. `package/skeleton-init-common` 把 `system/skeleton/` 复制到 `output/target/`；
2. 根据 BusyBox、System V、OpenRC 或 systemd 选择初始化系统专用骨架；
3. 各 Buildroot 软件包安装自己的目标文件；
4. `BR2_ROOTFS_OVERLAY` 中的目录依次覆盖到目标根文件系统；
5. 执行 Post-build 脚本；
6. 在 fakeroot 阶段应用用户表、权限表和设备表；
7. 打包选定的 rootfs 镜像；
8. 执行 Post-image 脚本，生成最终可部署镜像。

因此，`system/skeleton/` 只提供所有目标共享的最小基线。板级 Overlay 和软件包可以在后续阶段增加或覆盖文件。

## 文件说明

### `Config.in`

这是 Kconfig 菜单定义文件，对应 `make menuconfig` 中的 **System configuration** 页面。它定义的主要选项包括：

- 默认或自定义 Root FS skeleton；
- 主机名和登录提示文字；
- 密码哈希算法及 root 初始密码；
- BusyBox、System V、OpenRC、systemd 或无 init 系统；
- 静态 `/dev`、devtmpfs、mdev 或 eudev 设备管理；
- 权限表和静态设备表路径；
- merged `/usr` 布局；
- root 密码登录；
- `/bin/sh` 提供者；
- getty 端口、波特率、终端类型和附加参数；
- 根文件系统启动后是否重新挂载为可写；
- DHCP 接口和系统默认 `PATH`；
- Locale、NLS 和时区数据；
- 用户表、rootfs Overlay；
- Post-build、Post-fakeroot 和 Post-image 脚本及其参数。

`Config.in` 只定义选项、依赖关系、默认值和帮助文字。实际选择由 defconfig 或生成的 `.config` 决定。

### `system.mk`

这是 GNU Make 片段，定义系统层会被其他 Buildroot 组件调用的变量和宏。

| 宏或变量 | 作用 |
| --- | --- |
| `SYSTEM_USR_SYMLINKS_OR_DIRS` | 根据 merged `/usr` 选项，把 `/bin`、`/sbin`、`/lib` 创建为目录或指向 `/usr` 的符号链接 |
| `SYSTEM_RSYNC` | 用统一权限和排除规则复制 skeleton |
| `SYSTEM_LIB_SYMLINK` | 根据目标架构创建 `lib32` 或 `lib64` 链接 |
| `SYSTEM_GETTY_*` | 去除 Kconfig 字符串引号，提供 getty 配置值 |
| `SYSTEM_REMOUNT_ROOT_INITTAB` | 调整 `inittab`，控制启动时根文件系统是否重新挂载为读写 |

它还会在正式构建时拒绝空的 `BR2_SYSTEM_DEFAULT_PATH`。

该文件会被 Buildroot 顶层 Makefile 提前包含。修改错误可能影响所有板卡配置，而不只是 CRA Electric Pass。

### `device_table.txt`

这是通用权限表，由 Buildroot 的 `makedevs` 在 fakeroot 阶段应用。它不负责创建字符设备或块设备，主要设定目标文件系统中的目录、普通文件、属主和权限。

当前表中包括：

- `/dev`、`/etc` 的 `0755`；
- `/tmp` 的 `01777`；
- `/root` 的 `0700`；
- `/var/www` 属于 UID/GID 33；
- `/etc/shadow` 的 `0600`；
- `/etc/passwd` 的 `0644`；
- ifupdown 钩子目录的 `0755`。

默认路径由 `BR2_ROOTFS_DEVICE_TABLE="system/device_table.txt"` 指定。板级项目若需要额外权限，通常应新增独立权限表，并在 defconfig 中追加路径，避免污染所有目标。

### `device_table_dev.txt`

这是静态 `/dev` 模式使用的设备节点表，包含控制台、TTY、Framebuffer、输入设备、MTD、块设备、I²C、V4L 等字符设备或块设备的主次设备号和权限。

表格格式为：

```text
<路径> <类型> <权限> <UID> <GID> <主设备号> <次设备号> <起始值> <增量> <数量>
```

常见类型：

| 类型 | 含义 |
| --- | --- |
| `d` | 目录 |
| `f` | 普通文件 |
| `c` | 字符设备 |
| `b` | 块设备 |

CRA 当前显式选择 `BR2_ROOTFS_DEVICE_CREATION_DYNAMIC_EUDEV=y`，因此不会使用该文件批量创建静态 `/dev` 节点。运行时设备节点主要由内核 devtmpfs 和 eudev 管理。

## `skeleton/` 基础根文件系统

### `skeleton/dev/`

该目录只保存最基本的标准输入输出链接：

| 路径 | 预期链接目标 | 用途 |
| --- | --- | --- |
| `/dev/fd` | `../proc/self/fd` | 访问当前进程打开的文件描述符 |
| `/dev/stdin` | `../proc/self/fd/0` | 标准输入 |
| `/dev/stdout` | `../proc/self/fd/1` | 标准输出 |
| `/dev/stderr` | `../proc/self/fd/2` | 标准错误 |

`.empty` 只用于让 Git 保留目录。`SYSTEM_RSYNC` 会排除它，不会复制到目标系统。

### 账号数据库

| 文件 | 内容 |
| --- | --- |
| `etc/passwd` | root、daemon、bin、sys、sync、mail、www-data、operator、nobody 等基础账号 |
| `etc/group` | root、tty、disk、video、audio、plugdev、netdev 等基础组 |
| `etc/shadow` | 基础账号的密码占位信息 |

骨架中的 root 密码字段只是构建前占位。`package/skeleton-init-common` 会在 Target finalize 阶段根据 `BR2_TARGET_GENERIC_ROOT_PASSWD` 重写 `/etc/shadow`。

### Shell 环境

`etc/profile` 完成以下工作：

- 将 `@PATH@` 替换为 `BR2_SYSTEM_DEFAULT_PATH`；
- 根据 UID 设置 root 或普通用户提示符；
- 将默认编辑器设为 `/bin/vi`；
- 依次加载 `/etc/profile.d/*.sh`。

`etc/profile.d/umask.sh` 将默认 `umask` 设为 `022`。

### 网络和系统数据库

| 文件 | 作用 |
| --- | --- |
| `etc/hosts` | 提供 `127.0.0.1 localhost` 基础映射，构建时还会追加目标主机名 |
| `etc/resolv.conf` | 应链接到 `/tmp/resolv.conf`，供运行时网络配置更新 DNS |
| `etc/protocols` | 常见 IP 协议号数据库 |
| `etc/services` | 常见网络服务名称与端口数据库 |
| `etc/mtab` | 应链接到 `/proc/self/mounts`，实时反映挂载状态 |

这些 `protocols` 和 `services` 条目属于通用兼容数据，不代表目标系统实际启动了对应服务。

## CRA Electric Pass 当前配置

当前配置入口是：

```text
board/cra/epass/cra_epass_defconfig
```

它与本目录相关的显式设置为：

```text
BR2_TARGET_GENERIC_HOSTNAME="epass"
BR2_TARGET_GENERIC_ISSUE="Welcome to CRA Electric Pass"
BR2_ROOTFS_DEVICE_CREATION_DYNAMIC_EUDEV=y
BR2_TARGET_GENERIC_ROOT_PASSWD="toor"
BR2_ROOTFS_OVERLAY="board/allwinner/generic/rootfs board/allwinner/suniv-f1c100s/rootfs board/cra/epass/rootfs"
BR2_ROOTFS_POST_IMAGE_SCRIPT="board/cra/epass/scripts/mknanduboot.sh board/cra/epass/scripts/mkdt.sh board/cra/epass/scripts/buildimage.sh"
```

结合 Buildroot 默认值，当前系统层主要表现为：

| 项目 | 当前行为 |
| --- | --- |
| 根文件系统骨架 | Buildroot 默认 skeleton |
| init 系统 | BusyBox init |
| `/dev` 管理 | devtmpfs + eudev |
| 主机名 | `epass` |
| 登录提示 | `Welcome to CRA Electric Pass` |
| rootfs Overlay | 通用 Allwinner → SUNIV → CRA 板级 Overlay，后者可覆盖前者 |
| Post-image | 依次生成 NAND U-Boot、设备树/FIT 和最终镜像 |

### 当前 root 密码配置

`cra_epass_defconfig` 目前以明文保存了简单 root 密码 `toor`。Buildroot 会在生成 rootfs 时按所选密码算法进行哈希，但明文仍存在于 defconfig 和可能的构建记录中。

这适合受控开发阶段，不适合直接作为公开或正式部署设备的安全配置。正式发布时应改为强哈希、禁用密码登录或采用密钥登录，并重新评估 Dropbear 的 root 登录策略。

## Windows Git 符号链接问题

Git 索引将以下文件记录为符号链接，模式为 `120000`：

```text
system/skeleton/dev/fd
system/skeleton/dev/stdin
system/skeleton/dev/stdout
system/skeleton/dev/stderr
system/skeleton/etc/mtab
system/skeleton/etc/resolv.conf
```

但在当前 Windows 工作树中，它们被检出为普通小文件，文件内容只是链接目标字符串。PowerShell 也未把它们识别为符号链接。

如果直接从这个工作树构建，`rsync -a` 可能把它们作为普通文件复制进 rootfs，导致 `/dev/stdin`、`/etc/mtab` 或 DNS 配置行为异常。

构建前应使用能够正确保留 Git 符号链接的 Linux 工作树，或在受控构建副本中恢复这些链接。不要直接编辑这些小文件中的目标字符串并把它们当普通配置文件使用。

## 开发

## 语法与注释

| 文件 | 语言 | 常用注释 |
| --- | --- | --- |
| `Config.in` | Kconfig | `# 注释`，`help` 后续说明必须保持缩进 |
| `system.mk` | GNU Make | `# 注释`，配方命令必须以 Tab 开头 |
| `device_table*.txt` | `makedevs` 表格 | `# 注释` |
| `skeleton/etc/profile*` | POSIX Shell | `# 注释` |
| `passwd/group/shadow` | 冒号分隔数据库 | 不应随意插入说明性注释 |
| `protocols/services` | 空白分隔数据库 | `# 注释` |

## 二次开发原则

- 优先通过 CRA defconfig、Overlay、板级表和软件包实现项目需求；
- 修改账号、密码、权限和设备节点时必须评估安全影响；
- 不要把运行时动态生成的 `/dev` 节点硬编码进静态设备表；
- 修改 Overlay 顺序时要检查同名文件的覆盖关系；
- 修改 Post-image 脚本时要区分 `output/target/` 和 `output/images/`；
- 保持 Kconfig、Make、Shell 和 `makedevs` 各自的语法与缩进；
- 在 Linux 构建环境中保留符号链接和可执行权限；
- 修改 Buildroot 上游通用文件前，应确认板级覆盖机制是否已经足够；

---

<div align="center">

<sub><b>system/</b> · generic root filesystem and system policy for Buildroot</sub>

</div>
