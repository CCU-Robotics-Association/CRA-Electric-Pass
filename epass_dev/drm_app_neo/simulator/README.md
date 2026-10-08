<div align="center">

# 桌面 UI 模拟器

<sub>LVGL · SDL2 · 360 × 640</sub>

</div>

> [!NOTE]
> 该目标复用设备程序的 EEZ 生成 UI、字体、图片和 LVGL 配置，在 SDL2 窗口中显示 360×640 界面；它不会加载 DRM、CedarX，也不会访问或改写实体设备文件。

<p align="center">
  <a href="#ubuntu--wsl-构建">Ubuntu / WSL 构建</a> ·
  <a href="#交互方式">交互方式</a> ·
  <a href="#代码边界">代码边界</a>
</p>

---

## Ubuntu / WSL 构建

先安装一次主机依赖：

```sh
sudo apt-get install build-essential cmake pkg-config libsdl2-dev libpng-dev
```

在 `drm_app_neo` 仓库根目录执行：

```sh
cmake -S simulator -B build/simulator -DCMAKE_BUILD_TYPE=Debug
cmake --build build/simulator --parallel
./build/simulator/epass_ui_simulator
```

在本项目当前的 Windows + WSL 环境中，也可以直接从 PowerShell 执行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\simulator\run.ps1
```

脚本会把 Windows 工作区中的 `eez_design/src/ui/`、`src/lv_conf.h` 和
`simulator/` 同步到 `/home/xcrane/epass_dev/drm_app_neo` 构建镜像，随后增量
编译并启动窗口。它不会访问实体设备。

---

## 交互方式

- 鼠标可直接点击控件。
- 键盘方向键和回车可操作当前页面焦点。
- 关闭 SDL 窗口即可退出。

> [!IMPORTANT]
> 模拟器用于验证页面结构和主机端交互桩，不等价于实体设备的 DRM 图层、视频解码、性能与输入设备测试。

---

## 代码边界

- `eez_design/src/ui/`：EEZ Studio 生成，模拟器只读取、编译，不手工修改。
- `simulator/sim_actions.c`：桌面端页面跳转和安全动作桩。
- `simulator/sim_vars.c`：桌面端示例数据与设置状态。
- `src/`：实体设备实现，桌面模拟器不参与编译。

---

<div align="center">

<sub><b>simulator/</b> · safe desktop preview for the device UI</sub>

</div>
