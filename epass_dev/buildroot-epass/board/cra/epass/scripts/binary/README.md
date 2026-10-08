<div align="center">

# CRA Electric Pass Windows 辅助程序

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> 本目录保存配置、USB 诊断和固件烧录过程中可能使用的预编译 Windows 程序；这些文件属于主机端工具，不进入设备 rootfs。

<p align="center">
  <a href="#目录内容">目录内容</a> ·
  <a href="#epass_flasherexe"><code>epass_flasher.exe</code></a> ·
  <a href="#usbtreeviewexe"><code>UsbTreeView.exe</code></a> ·
  <a href="#zadig-29exe"><code>Zadig</code></a> ·
  <a href="#当前文件校验信息">文件校验</a> ·
  <a href="#安全与维护原则">安全维护</a>
</p>

---

## 目录内容

```text
binary/
├── epass_flasher.exe
├── UsbTreeView.exe
├── zadig-2.9.exe
└── README.md
```

| 文件 | 平台 | 当前大小 | 主要用途 |
| --- | --- | ---: | --- |
| `epass_flasher.exe` | Windows x86-64，控制台程序 | 9,916,293 字节 | 生成设备树配置并通过 FEL 等流程烧录设备 |
| `UsbTreeView.exe` | Windows x86-64，图形程序 | 977,664 字节 | 查看 USB 拓扑、描述符、VID/PID 和驱动状态 |
| `zadig-2.9.exe` | Windows x86，图形程序 | 5,334,088 字节 | 为指定 USB 设备安装或替换 WinUSB/libusb 类驱动 |

## `epass_flasher.exe`

这是电子通行证的 Windows 控制台烧录程序。

静态检查表明，该文件具有以下结构：

- 使用 Python 3.12 编写并通过 PyInstaller 打包；
- 主入口原始文件名为 `main.py`；
- 包含 `dt_patcher`、`interact` 和 `flasher` 等 Python 模块；
- 内置 `xfel.exe`、`dtc.exe`、`libfdt-1.dll`、`libusb-1.0.dll` 和 `libyaml-0-2.dll`；
- 支持读取烧录配置；
- 支持生成、修改和编译设备树；
- 支持检测并复用上一次生成的设备树及烧录配置；
- 支持通过 `--config_path` 进入配置文件驱动的批量烧录流程；
- 引导用户让设备进入 FEL 模式，然后执行固件下载。

其内部工作流程大致为：

```text
读取或交互生成配置
        │
        ├─生成设备树源码
        ├─应用设备树修改
        ├─调用 dtc 编译设备树
        └─生成烧录摘要
                 │
                 ▼
          等待设备进入 FEL
                 │
                 ▼
            调用 xfel 烧录
```

该程序会明确提示烧录操作可能清除设备上的全部数据。执行前必须确认硬件版本、屏幕配置、烧录文件和设备树选择均正确。

## `UsbTreeView.exe`

这是 Uwe Sieber 开发的 USB Device Tree Viewer，文件版本为 `4.5.1.0`。

它可以用于查看：

- USB 主控制器和集线器层级；
- 已连接设备的 VID、PID 和序列号；
- USB 描述符；
- 设备当前使用的 Windows 驱动；
- 设备连接速度和端点信息；
- 设备是否以 FEL、DFU、RNDIS 或其他预期模式枚举。

该工具只用于诊断和观察，不负责烧录设备，也不会自动修改 USB 驱动。

## `zadig-2.9.exe`

这是 Zadig `2.9.788`，由 Akeo Consulting 发布。

它用于为指定 USB 设备安装或替换 Windows 驱动，常见目标包括：

- WinUSB；
- libusbK；
- libusb-win32。

当设备能够在 USB 列表中出现，但 `xfel`、DFU 或其他用户态工具无法访问时，可用 Zadig 检查并调整对应接口的驱动。

使用时必须核对设备的 VID、PID、接口编号和名称。给错误的 USB 设备替换驱动，可能导致键盘、鼠标、网卡、调试器或其他设备暂时无法正常工作。该操作通常需要管理员权限。

当前文件包含有效的 Akeo Consulting Authenticode 签名。

## 当前文件校验信息

下列 SHA-256 对应当前仓库中的文件内容。任一文件发生变化后，都应重新计算并更新本节。

| 文件 | SHA-256 |
| --- | --- |
| `epass_flasher.exe` | `EDFB44E5558C772FA4E09B54628FE85901379359A5F6D9D953532FD58173DC3D` |
| `UsbTreeView.exe` | `1B22F76E90D824F2404B364174252E5F6A8AFA2FB34543AEADD91BE1AD86D2DF` |
| `zadig-2.9.exe` | `4ECAA95DF3DA3621486A043AEF8B3050B8BAFE7C901402871E816229EF82039B` |

在 PowerShell 中重新计算：

```powershell
Get-FileHash -Algorithm SHA256 .\epass_flasher.exe
Get-FileHash -Algorithm SHA256 .\UsbTreeView.exe
Get-FileHash -Algorithm SHA256 .\zadig-2.9.exe
```

## 安全与维护原则

- 不要直接运行来源不明、校验值不符或被意外修改的 EXE；
- 烧录前备份设备上的主题、应用、图片和其他用户数据；
- 不要把实体设备挂载目录当成普通构建输出目录进行批量覆盖；
- 不要将这些 Windows EXE 放入目标设备的 rootfs；
- 第三方工具升级后，应同步记录版本、来源、许可证和新的 SHA-256；
- 公开分发前，应补充第三方工具的来源链接及对应许可证说明；
- 项目自有烧录器应补齐完整源码、依赖版本和可复现构建方法；

---

<div align="center">

<sub><b>scripts/binary/</b> · Windows host utilities</sub>

</div>
