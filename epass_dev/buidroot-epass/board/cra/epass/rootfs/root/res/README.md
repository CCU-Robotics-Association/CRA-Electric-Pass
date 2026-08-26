# 主程序固定资源

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
