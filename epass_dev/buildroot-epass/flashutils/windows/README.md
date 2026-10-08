<div align="center">

# Windows 下载与烧录工具

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `flashutils/windows/` 保存 Windows 主机下使用 Allwinner FEL 和 U-Boot DFU 的历史批处理脚本、第三方命令行工具、运行库以及驱动安装示意图。

<p align="center">
  <a href="../README.md">返回 flashutils</a> ·
  <a href="#目录结构">目录结构</a> ·
  <a href="#文件来源与性质">文件性质</a> ·
  <a href="#fel-与-dfu">FEL / DFU</a> ·
  <a href="#随附工具">随附工具</a>
</p>

---

## 目录结构

```text
windows/
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

## 文件来源与性质

| 类型 | 文件 | 性质 |
| --- | --- | --- |
| 项目辅助脚本 | `*.bat` | 针对旧镜像布局编写或继承的批处理，不参与 Buildroot 编译 |
| 第三方主机工具 | `sunxi-fel.exe`、`dfu-util.exe`、`zadig-2.5.exe` | 预编译 Windows 可执行文件，不是本项目业务代码 |
| 第三方运行库 | `libusb-1.0.dll`、`libwinpthread-1.dll` | 命令行工具所需动态链接库 |
| 文档素材 | `res/*.png` | 历史驱动安装步骤截图，不参与程序运行 |

## FEL 与 DFU

### FEL

FEL 是 Allwinner SoC Boot ROM 提供的 USB 恢复和下载模式。Windows 主机通过 `sunxi-fel.exe` 检测设备、向 RAM 写入内容，或上传并启动 U-Boot。

上传 U-Boot 通常从 RAM 开始运行，但 U-Boot 后续执行的环境命令仍可能访问或擦除非易失存储。因此 FEL 启动并不自动等于“不会改变设备内容”。

### DFU

DFU 由设备端 U-Boot USB Gadget 提供。Windows 主机通过 `dfu-util.exe` 选择 Alternate Setting 并下载镜像。

批处理中的 `-D` 参数会执行写入，`-R` 会在传输结束后请求设备复位。Alternate Setting 必须与目标设备当前运行的 U-Boot 通过 `dfu-util.exe -l` 导出的名称完全一致。

## 随附工具

| 文件 | 用途 | PE 元数据可读取的版本 |
| --- | --- | --- |
| `bin/sunxi-fel.exe` | Allwinner FEL 主机端工具 | 未提供版本字段 |
| `bin/dfu-util.exe` | U-Boot DFU 主机端工具 | 未提供版本字段 |
| `bin/libusb-1.0.dll` | USB 用户态通信库 | `1.0.20.11004` |
| `bin/libwinpthread-1.dll` | MinGW-w64 pthread 运行库 | `1.0.0.0` |
| `bin/zadig-2.5.exe` | Windows USB 驱动安装工具 | `2.5.730` |

当前文件 SHA-256：

| 文件 | SHA-256 |
| --- | --- |
| `dfu-util.exe` | `1F4687D0F11F0EEDE72D582FB5174537D5820DE515862A8041AC9506B4A6FA1E` |
| `sunxi-fel.exe` | `626A16C6FA8FF9638A52222452D7BED7F5922537F530494BE4BA0845BCA23203` |
| `libusb-1.0.dll` | `20E5CE87947C79C83624DEA087FE195CC8219BBD06E7EC2CD7EB025D9D16A73F` |
| `libwinpthread-1.dll` | `213B6ADDAB856FEB85DF1A22A75CDB9C010B2E3656322E1319D0DEF3E406531C` |
| `zadig-2.5.exe` | `78A1A26854FBC848284588A62C7FBEC9C652F6A3218BA543783D369265DF00D6` |

> [!CAUTION]
> 批处理中的 `-D` 会写入设备，Zadig 也会替换所选 USB 设备的驱动。操作前必须核对 VID/PID、设备模式、Alternate Setting 和镜像布局。

---

<div align="center">

<sub><b>flashutils/windows/</b> · legacy Windows host tools for Allwinner FEL and U-Boot DFU</sub>

</div>
