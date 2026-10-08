<div align="center">

# 主程序固定资源

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> 本目录保存所有主题共用的设备端固定资源，并提供无法加载用户主题时使用的内置兜底主题。

<p align="center">
  <a href="#程序加载位置">加载位置</a> ·
  <a href="#固定资源与主题包的边界">资源边界</a> ·
  <a href="fallback/README.md">兜底主题</a>
</p>

---

本目录在设备上对应：

```text
/root/res/
```

## 程序加载位置

`drm_app_neo/src/config.h` 定义固定路径：

```c
#define CACHED_ASSETS_ASSET_PATH "/root/res/"
```

主程序初始化时在 `src/main.c` 中把六个 PNG 加载到共享缓存，再由 `src/overlay/theme_info.c` 按固定资源 ID 绘制。文件名与 `CACHED_ASSETS_ASSET_PATH_*` 宏直接对应。

## 固定资源与主题包的边界

| 类型 | 固定资源 `/root/res/` | 主题包 `/assets/<theme>/` |
| --- | --- | --- |
| 所有主题共用的边框和标记 | 是 | 通常否 |
| 默认主题/应用图标 | 是 | 可在各包内提供自定义图标 |
| 循环视频 | 只在 `fallback/` 中提供兜底视频 | 每个主题各自提供 |
| `epconfig.json` | 只存在于 `fallback/` | 每个主题子目录一份 |
| 用户通过 MTP 安装和删除 | 不建议 | 是 |

> [!WARNING]
> 固定文件名与设备端缓存 ID 直接关联。重命名、删除或改变格式前，应同步修改 `drm_app_neo` 的路径宏和加载代码。

---

<div align="center">

<sub><b>rootfs/root/res/</b> · fixed runtime assets and fallback theme</sub>

</div>
