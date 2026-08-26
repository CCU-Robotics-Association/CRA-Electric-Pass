# CRA Electric Pass 扩展应用目录

其他语言版本：[English](README_EN.md)，[中文](README.md)。

本目录在 Buildroot 合并 rootfs overlay 后对应设备上的：

```text
/app/
```

它用于保存安装在设备内部 NAND 上的第三方扩展应用。

## 目录来源

| 文件 | 来源 | 说明 |
| --- | --- | --- |
| `.gitkeep` | 上游未改 | 上游用它在 Git 中保留空的 `/app` 目录 |
| `README.md` | CRA 新增 | 当前中文说明文档 |
| `README_EN.md` | CRA 新增 | 英文说明文档 |

上游 rootfs overlay 没有预装第三方应用。之后由使用者放入 `/app/` 的应用目录属于运行时或发行内容，不是 Buildroot 自动生成，也不能因为位于这个目录就视为上游可信程序。

CRA Electric Pass 主程序由 Buildroot 软件包安装到：

```text
/root/epass_drm_app
```

因此，主程序和 `/app` 扩展应用属于两个不同层级，不应把当前主项目复制或改造成 `/app` 下的第三方应用。

## 扫描位置

主程序启动时会扫描两个应用位置：

```text
/app/       内部 NAND 应用
/sd/app/    SD 卡应用
```

对应源码配置：

```c
#define APPS_MAX 64
#define APPS_CONFIG_VERSION 1
#define APPS_CONFIG_FILENAME "appconfig.json"
#define APPS_PARSE_LOG "/root/apps.log"
#define APPS_DIR "/app/"
#define APPS_DIR_SD "/sd/app/"
```

内部 NAND 和 SD 卡中的应用会加入同一个应用列表，当前最多保存 64 个有效应用条目。

## 应用目录结构

应用扫描器只检查 `/app` 下的子目录。直接放在 `/app` 根目录中的单个可执行文件不会被识别为应用。

一个应用的基本结构如下：

```text
/app/
└── example_app/
    ├── appconfig.json
    ├── example_app
    ├── icon.png
    └── 其他应用资源
```

其中：

- `appconfig.json` 保存应用元数据和启动配置；
- `example_app` 是设备可以执行的 ARM Linux 程序；
- `icon.png` 是可选的应用图标；
- 其他资源文件由应用自行读取和管理。

应用可执行文件必须适配当前 ARM926T EABI soft-float Linux 环境。Windows EXE、桌面电脑使用的 x86/x86-64 程序以及架构不匹配的 ARM 程序均无法在设备上运行。

## `appconfig.json`

应用配置文件名固定为：

```text
appconfig.json
```

当前配置格式版本为 `1`。完整示例：

```json
{
  "version": 1,
  "name": "Example App",
  "uuid": "12345678-1234-1234-1234-123456789abc",
  "executable": {
    "file": "example_app"
  },
  "type": "fg",
  "screens": [
    "360x640"
  ],
  "description": "示例应用",
  "icon": "icon.png",
  "extensions": [
    ".txt"
  ]
}
```

### 字段说明

| 字段 | 是否必需 | 说明 |
| --- | --- | --- |
| `version` | 是 | 配置格式版本，当前必须为 `1` |
| `name` | 否 | 应用显示名称；缺少或为空时使用应用文件夹名 |
| `uuid` | 是 | 应用唯一标识，必须是可解析的 UUID |
| `executable` | 是 | 应用目录内的可执行文件 |
| `type` | 是 | 应用启动类型 |
| `screens` | 是 | 应用支持的屏幕分辨率列表 |
| `description` | 否 | 应用描述；缺少时使用“无描述” |
| `icon` | 否 | 相对于应用目录的图标路径 |
| `extensions` | 否 | 应用可以处理的文件扩展名 |

`executable` 推荐使用对象格式：

```json
"executable": {
  "file": "example_app"
}
```

解析器也兼容旧的简单格式：

```json
"executable": "example_app"
```

## 应用类型

`type` 支持以下值：

| 值 | 类型 | 启动方式 |
| --- | --- | --- |
| `fg` | 普通前台应用 | 可以从应用列表直接启动，也可以处理关联文件 |
| `bg` | 后台应用 | 在主界面运行期间启动或停止 |
| `fg_ext` | 文件关联型前台应用 | 不能从应用列表直接启动，只能通过关联文件调用 |

### 前台应用

启动前台应用时，主程序会生成：

```text
/tmp/appstart
```

该临时 Shell 脚本会：

1. 为应用程序添加执行权限；
2. 切换到应用目录；
3. 启动对应可执行文件；
4. 在通过文件关联启动时传入完整文件路径。

主程序随后退出当前 UI，由系统启动流程执行该临时脚本。应用结束后，系统可以重新启动 CRA Electric Pass 主界面。

### 后台应用

后台应用会在独立进程组中运行。主程序记录其 PID，并可以：

1. 发送 `SIGTERM` 请求正常退出；
2. 等待指定超时时间；
3. 仍未退出时发送 `SIGKILL` 强制结束。

同一个后台应用已经运行时，主程序不会重复启动第二份实例。

## 屏幕兼容性

当前主程序使用：

```text
360x640
```

应用配置的 `screens` 数组至少需要包含：

```json
"screens": [
  "360x640"
]
```

解析器还保留以下屏幕值：

```text
480x854
720x1280
```

但只有主程序编译时启用的分辨率会被认为与当前固件兼容。如果没有任何匹配项，应用不会被加入应用列表。

## 图标

`icon` 是相对于应用目录的路径，例如：

```json
"icon": "icon.png"
```

主程序会检查图标文件是否存在且可读。缺少 `icon`、路径无效或文件不可读时，会使用：

```text
/root/res/defaulticon.png
```

## 文件扩展名关联

`extensions` 用于声明应用可以处理的文件类型：

```json
"extensions": [
  ".txt",
  ".json"
]
```

扩展名应包含开头的点。文件管理器打开匹配文件时，会把文件的完整路径作为单个命令行参数传给对应应用。

扩展名映射最多保存 128 个条目。多个应用注册同一扩展名时，应检查当前映射实现和实际加载顺序，避免产生不明确的处理程序。

## 加载检查

主程序加载应用时会依次检查：

1. 应用子目录能否访问；
2. `appconfig.json` 是否存在且可读取；
3. JSON 是否能正常解析；
4. `version` 是否等于 `1`；
5. `uuid` 是否存在且格式有效；
6. 可执行文件字段是否存在；
7. 可执行文件是否存在且可读；
8. 是否能把可执行权限设置为 `0755`；
9. `type` 是否为有效值；
10. `screens` 是否存在且兼容当前固件；
11. 图标和扩展名信息是否有效。

加载失败的应用不会进入应用列表。解析信息写入：

```text
/root/apps.log
```

## 通过电脑访问

设备的 uMTP Responder 配置把 `/app` 暴露为可读写存储：

```text
storage "/app" "app" "rw"
```

设备进入 MTP 模式后，电脑上会出现名为 `app` 的存储入口，可以用来复制、更新或删除扩展应用目录。

MTP 传输完成后，应让主程序重新扫描应用列表或重新启动主程序，避免继续使用旧的解析结果。

## 安全边界

当前应用加载机制会检查配置格式和文件可读性，但不会验证：

- 应用数字签名；
- SHA-256 校验值；
- 发布者身份；
- 程序来源；
- 应用调用的系统接口；
- 应用是否包含危险或破坏性行为。

加载成功后，主程序会为可执行文件设置 `0755` 权限并启动它。因此：

- 不要安装来源不明的二进制程序；
- 不要把 Windows 或桌面 Linux 程序直接复制到该目录；
- 不要在未验证硬件访问范围的情况下运行第三方程序；
- 更新应用前应备份应用数据和设备资源；
- 应用包应记录源码、构建环境、目标架构、版本和校验值；
- 不应把 CRA Electric Pass 主程序本身迁移到该目录。

## 相关源码

应用目录和配置常量：

```text
drm_app_neo/src/config.h
```

应用扫描和配置解析：

```text
drm_app_neo/src/apps/apps_cfg_parse.c
```

应用启动和进程管理：

```text
drm_app_neo/src/apps/apps.c
```

应用数据结构：

```text
drm_app_neo/src/apps/apps_types.h
```

MTP 存储入口：

```text
buildroot-epass/board/cra/epass/rootfs/etc/umtprd/
```
