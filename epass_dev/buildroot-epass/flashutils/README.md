<div align="center">

# CRA Electric Pass 下载与烧录工具

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `flashutils/` 保存通过 Allwinner FEL 和 U-Boot DFU 与设备通信的历史辅助脚本，以及 Windows 下随仓库提供的下载工具。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#fel-与-dfu">FEL / DFU</a> ·
  <a href="#windows-脚本">Windows 脚本</a> ·
  <a href="#windows-随附工具">随附工具</a> ·
  <a href="#usb-驱动">USB 驱动</a> ·
  <a href="#只读检查">只读检查</a>
</p>

---

目录入口：

```text
flashutils/
```

## 目录结构

```text
flashutils/
├── README.md
├── linux/
│   ├── README.md
│   ├── fel-uboot.sh
│   ├── fel-linux.sh
│   ├── dfu-kernel.sh
│   ├── dfu-mmc-all.sh
│   ├── dfu-nand-all.sh
│   ├── dfu-nand-fast.sh
│   ├── dfu-nor-all.sh
│   └── dfu-nor-fast.sh
└── windows/
    ├── README.md
    ├── fel-uboot.bat
    ├── fel-linux.bat
    ├── dfu-mmc-all.bat
    ├── dfu-nand-all.bat
    ├── dfu-nor-all.bat
    ├── bin/
    │   ├── sunxi-fel.exe
    │   ├── dfu-util.exe
    │   ├── libusb-1.0.dll
    │   ├── libwinpthread-1.dll
    │   └── zadig-2.5.exe
    └── res/
        ├── Create New Device.png
        ├── FEL Driver.png
        └── DFU Driver.png
```

## FEL 与 DFU

### FEL

FEL 是 Allwinner SoC Boot ROM 提供的 USB 恢复和下载模式。主机端通过 `sunxi-fel` 与 SoC ROM 通信，可以检查设备、向 RAM 写入数据，或把 U-Boot 上传到 RAM 并启动。

### DFU

DFU 是由 U-Boot USB Gadget 提供的下载接口。主机端通过 `dfu-util` 按 Alternate Setting 名称选择 U-Boot、Boot 分区或 rootfs 等设备端区域。

| 参数 | 含义 |
| --- | --- |
| `-l` | 只列出当前 DFU 设备和 Alternate Settings |
| `-a <名称>` | 选择 U-Boot 导出的目标区域 |
| `-D <文件>` | 把指定文件下载到目标区域 |
| `-R` | 传输完成后请求设备复位 |

## Windows 脚本

| 脚本 | 实际行为 | 当前状态 |
| --- | --- | --- |
| `fel-uboot.bat` | 通过 FEL 在 RAM 中运行 U-Boot | 文件名仍存在，但必须核对启动后行为 |
| `fel-linux.bat` | FEL 启动旧式 `zImage + DTB + initramfs` | 不兼容 |
| `dfu-mmc-all.bat` | 写入旧 `sysimage-sdcard.img` | 不兼容 |
| `dfu-nand-all.bat` | 写入旧 `sysimage-nand.img` | 不兼容 |
| `dfu-nor-all.bat` | 写入旧 `sysimage-nor.img` | 不兼容 |

批处理通过 `%~dp0` 定位 `windows/bin/`，但镜像仍使用相对于当前工作目录的 `output\images\...`。即使后续修复脚本，也必须从 Buildroot 仓库根目录启动，或改用可靠的绝对/仓库相对路径。

## Windows 随附工具

| 文件 | 作用 | 可读取的版本 |
| --- | --- | --- |
| `dfu-util.exe` | U-Boot DFU 主机端工具 | PE 元数据未提供 |
| `sunxi-fel.exe` | Allwinner FEL 主机端工具 | PE 元数据未提供 |
| `libusb-1.0.dll` | USB 用户态通信库 | `1.0.20.11004` |
| `libwinpthread-1.dll` | MinGW-w64 pthread 运行库 | `1.0.0.0` |
| `zadig-2.5.exe` | Windows USB 驱动安装工具 | `2.5.730` |

当前文件 SHA-256：

| 文件 | SHA-256 |
| --- | --- |
| `dfu-util.exe` | `1F4687D0F11F0EEDE72D582FB5174537D5820DE515862A8041AC9506B4A6FA1E` |
| `sunxi-fel.exe` | `626A16C6FA8FF9638A52222452D7BED7F5922537F530494BE4BA0845BCA23203` |
| `libusb-1.0.dll` | `20E5CE87947C79C83624DEA087FE195CC8219BBD06E7EC2CD7EB025D9D16A73F` |
| `libwinpthread-1.dll` | `213B6ADDAB856FEB85DF1A22A75CDB9C010B2E3656322E1319D0DEF3E406531C` |
| `zadig-2.5.exe` | `78A1A26854FBC848284588A62C7FBEC9C652F6A3218BA543783D369265DF00D6` |

## USB 驱动

### Linux

`linux/README.md` 说明了 `sunxi-tools`、`dfu-util` 和 udev 规则的安装方式，并使用以下 VID/PID：

| 模式 | VID:PID |
| --- | --- |
| Allwinner FEL | `1f3a:efe8` |
| U-Boot DFU | `1f3a:1010` |

现有规则使用 `SUBSYSTEM!="usb_device"`，在部分现代 udev 环境中可能无法正确匹配。正式使用前应通过 `udevadm` 和 `lsusb` 验证。

### Windows

`windows/README.md` 使用 Zadig 手动建立以下设备：

| 名称 | VID:PID |
| --- | --- |
| Allwinner FEL Device | `1F3A:EFE8` |
| Allwinner DFU Device | `1F3A:1010` |

必须确认选择的是处于 FEL 或 U-Boot DFU 模式的目标硬件。不要把 Linux 主系统的 RNDIS、存储设备或其他 USB 设备误换成 WinUSB/libusb 驱动。`windows/res/` 中的图片只用于历史操作示意。

## 只读检查

在重写烧录脚本前，可先使用只读命令确认连接：

```sh
sunxi-fel ver
dfu-util -l
```

应记录 USB VID/PID、序列号、Alternate Settings、设备当前模式和连接设备数量。这些查询本身不下载镜像，但启动 U-Boot、进入 `rundfu` 或执行任何带 `-D` 的命令都可能改变设备状态。

> [!CAUTION]
> 任何包含 `-D` 的 DFU 命令、启动后自动执行写入逻辑的 U-Boot 环境，或面向错误 USB 设备安装驱动的操作，都可能造成数据损坏。写入前必须核对设备模式、目标介质和镜像名称。

---

<div align="center">

<sub><b>flashutils/</b> · host-side FEL and DFU utilities for legacy flashing workflows</sub>

</div>
