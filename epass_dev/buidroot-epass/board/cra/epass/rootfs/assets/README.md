# 主题素材目录

本目录在 Buildroot 合并 rootfs overlay 后对应设备上的：

```text
/assets/
```

它用于保存设备内部 NAND 中安装的主题素材，不是主程序源码目录，也不用于保存 `/root/res/` 中的固定界面组件。

## 主程序如何扫描

`drm_app_neo/src/config.h` 定义了两个主题位置：

```c
#define THEME_DIR "/assets/"
#define THEME_DIR_SD "/sd/assets/"
```

主程序只扫描这两个目录的**直接子目录**，不会把直接放在 `/assets/` 根目录下的单个 `epconfig.json` 当作一个主题。

基本结构如下：

```text
/assets/
└── example_theme/
    ├── epconfig.json
    ├── loop.mp4
    ├── icon.png
    └── 其他可选资源
```

每个主题至少需要：

- 可解析且版本匹配的 `epconfig.json`；
- 唯一且合法的 `uuid`；
- 与当前固件一致的 `screen`，本项目当前使用 `360x640`；
- `loop.file` 指向的可读循环视频。

图标、入场视频、过渡图像和叠加 UI 资源根据配置决定是否必需。主题解析失败信息会写入：

```text
/root/asset.log
```

## NAND、SD 卡与兜底主题

| 路径 | 含义 |
| --- | --- |
| `/assets/` | 内部 NAND 主题，来源于本 overlay 或后续写入 |
| `/sd/assets/` | SD 卡主题，只有挂载 SD 卡并以 SD 模式启动主程序时才扫描 |
| `/root/res/fallback/` | 没有有效主题时使用的固件内置兜底主题 |

`/assets/` 和 `/sd/assets/` 都没有有效主题时，程序才会载入 `/root/res/fallback/`。兜底主题不应移入本目录。

## 通过电脑访问

两个 uMTP Responder 配置都把本目录暴露为可读写 MTP 存储：

```text
storage "/assets" "assets" "rw"
```

因此设备进入 MTP 模式后，可以从电脑复制或删除主题。外部输入的主题配置和媒体文件不受数字签名保护，使用前应核对来源、尺寸、编码、容量和配置内容。
