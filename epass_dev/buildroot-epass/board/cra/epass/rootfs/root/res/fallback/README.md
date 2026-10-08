<div align="center">

# 内置兜底主题

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> 本目录提供设备找不到可用用户主题时加载的最小可运行主题，确保主界面仍能进入并展示 CRA 默认视觉资源。

<p align="center">
  <a href="#当前配置">当前配置</a> ·
  <a href="#加载逻辑">加载逻辑</a>
</p>

---

本目录在目标设备上对应：

```text
/root/res/fallback/
```

## 当前配置

`epconfig.json` 当前声明：

| 字段 | 值 | 含义 |
| --- | --- | --- |
| `version` | `1` | 主题配置格式版本 |
| `uuid` | `6752f6db-562e-42a7-819b-13a337208591` | CRA 主题标识 |
| `name` | `CRA Electric Pass` | 列表中的主题名称 |
| `screen` | `360x640` | 适用屏幕分辨率 |
| `loop.file` | `loop_1.mp4` | 循环视频 |
| `transition_in.type` | `none` | 不播放上游故障闪烁过渡 |
| `transition_loop.type` | `none` | 视频循环时不叠加额外过渡 |
| `overlay.type` | `cra_pass` | 使用 CRA E-PASS 原生叠加界面 |
| `overlay.options.appear_time` | `100000` | 主题加载后 0.1 秒调度叠加层；叠加代码内部继续保持 1.5 秒入场等待 |
| `overlay.options.top_left_text` | `CRA E-PASS` | 选择 CRA 自定义模板并显示左侧标记 |
| `overlay.options.top_right_bar_text` | `CCU Robotics Association` | 右侧组织名称 |

## 加载逻辑

主程序扫描正常主题后，如果 `theme_count == 0`，会调用主题加载器直接解析本目录。它仍会执行常规检查，包括：

- JSON 能否解析；
- `version` 是否为 `1`；
- UUID 是否有效；
- `screen` 是否为当前固件的 `360x640`；
- `loop_1.mp4` 是否存在且可读；
- CRA 叠加配置是否完整有效。

兜底主题自身配置损坏时，代码仍会把主题计数设为 1，因此不能依赖另一层自动兜底来恢复。

> [!IMPORTANT]
> 更新 `epconfig.json`、视频文件或叠加类型后，应同时在桌面模拟器与实体设备验证；配置能够解析不代表媒体解码和目标端渲染一定成功。

---

<div align="center">

<sub><b>rootfs/root/res/fallback/</b> · built-in recovery theme</sub>

</div>
