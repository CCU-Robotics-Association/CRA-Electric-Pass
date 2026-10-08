<div align="center">

# Linux 下载与烧录脚本

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> `flashutils/linux/` 保存 Linux 主机下使用 `sunxi-fel` 和 `dfu-util` 与 Allwinner F1C100S/F1C200S 设备通信的历史 Shell 脚本。

<p align="center">
  <a href="../README.md">返回 flashutils</a> ·
  <a href="#目录内容">目录内容</a> ·
  <a href="#两种通信方式">通信方式</a>
</p>

---

## 目录内容

```text
linux/
├── README.md
├── fel-uboot.sh
├── fel-linux.sh
├── dfu-kernel.sh
├── dfu-mmc-all.sh
├── dfu-nand-all.sh
├── dfu-nand-fast.sh
├── dfu-nor-all.sh
└── dfu-nor-fast.sh
```

## 两种通信方式

### FEL

FEL 是 Allwinner SoC Boot ROM 提供的 USB 恢复与下载模式。主机端使用 `sunxi-fel` 检测设备、向 RAM 写入数据，或将 U-Boot 上传到 RAM 后启动。

FEL 命令本身可以只操作 RAM，但上传并启动的 U-Boot 仍会执行自身环境中的启动逻辑。因此，启动 U-Boot 后是否会访问或擦除非易失存储，必须结合当前 `uboot.env` 判断。

### DFU

DFU 由设备端 U-Boot USB Gadget 提供。主机端使用 `dfu-util`，按 Alternate Setting 名称选择设备端的分区或区域，再下载镜像。

`dfu-util -a <名称> -D <镜像>` 会执行写入。`<名称>` 必须与当前设备运行的 U-Boot 通过 `dfu-util -l` 导出的名称完全一致。

> [!CAUTION]
> 本目录脚本面向历史镜像布局。执行任何写入命令前，先用 `sunxi-fel ver` 或 `dfu-util -l` 确认设备、连接模式和 Alternate Setting；不要仅凭脚本文件名判断其适用于当前 CRA 镜像。

---

<div align="center">

<sub><b>flashutils/linux/</b> · legacy Linux host scripts for Allwinner FEL and U-Boot DFU</sub>

</div>
