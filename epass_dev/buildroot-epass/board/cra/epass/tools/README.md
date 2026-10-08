<div align="center">

# CRA Electric Pass 辅助工具

<sub>Read this in other languages: [English](README_EN.md), [中文](README.md).</sub>

</div>

> [!NOTE]
> 本目录保存构建与资源生成过程中使用的主机端工具；它们在开发电脑或 Buildroot 环境中运行，不是设备端常驻程序，也不会自动安装到目标系统。

<p align="center">
  <a href="#目录结构">目录结构</a> ·
  <a href="#shutdown_variantspy"><code>shutdown_variants.py</code></a> ·
  <a href="#工作流程">工作流程</a> ·
  <a href="#使用限制与注意事项">使用限制</a> ·
  <a href="#代码规范">代码规范</a>
</p>

---

## 目录结构

```text
tools/
├── shutdown_variants.py
└── README.md
```

## `shutdown_variants.py`

该脚本用于生成三份带有不同中文提示的关机辅助程序。

### 图像布局

| 项目 | 数值 |
| --- | --- |
| 图像宽度 | 360 像素 |
| 图像高度 | 129 像素 |
| 像素格式 | RGB888 |
| 每像素字节数 | 3 |
| 图像数据长度 | 139320 字节 |
| 图像在程序中的偏移 | `0x0DDC` |

脚本会在黑色背景中央绘制一个白色矩形，再使用黑色文字绘制关机提示。生成的图像字节会写入模板程序的固定偏移位置。

### 生成内容

| 输出文件 | 提示文字 |
| --- | --- |
| `shutdown_message` | 要走了吗，不再看看 |
| `shutdown_message_2` | 再见，祝愿未来 |
| `shutdown_message_3` | 别忘记这里 |

指定预览目录后，还会生成以下 PNG 文件：

```text
shutdown_message_1.png
shutdown_message_2.png
shutdown_message_3.png
```

### 运行依赖

- Python 3；
- Pillow；
- 包含所需中文字符的 TrueType 或 OpenType 字体；
- 已经验证可用的 `shutdown_message` 模板程序。

安装 Pillow：

```sh
python3 -m pip install Pillow
```

### 使用方法

```sh
python3 board/cra/epass/tools/shutdown_variants.py \
    board/cra/epass/rootfs/bin \
    /path/to/chinese-font.ttf \
    --preview-dir /tmp/cra-shutdown-preview \
    --font-size 28
```

参数说明：

| 参数 | 是否必需 | 说明 |
| --- | --- | --- |
| `bin_dir` | 是 | 包含 `shutdown_message` 模板程序的目录 |
| `font` | 是 | 用于渲染中文提示的字体文件 |
| `--preview-dir` | 否 | PNG 预览图的输出目录 |
| `--font-size` | 否 | 提示文字字号，默认值为 `28` |

运行后，脚本会在 `bin_dir` 中写入三份程序，并将它们的权限设置为 `0755`。

查看完整命令行帮助：

```sh
python3 board/cra/epass/tools/shutdown_variants.py --help
```

## 工作流程

脚本按以下顺序处理文件：

1. 从 `bin_dir/shutdown_message` 读取模板程序；
2. 检查模板长度能否容纳固定尺寸的 RGB 图像；
3. 使用指定字体分别渲染三条提示；
4. 复制模板内容并替换固定偏移处的图像字节；
5. 写出三份可执行文件；
6. 在指定了 `--preview-dir` 时额外保存 PNG 预览图。

## 使用限制与注意事项

- 固定偏移 `0x0DDC` 与当前模板程序的二进制布局紧密相关。模板程序重新编译后，该偏移可能变化。
- 当前检查只能确认文件长度足够，不能验证模板的版本、符号或机器指令。使用不匹配的模板可能生成损坏的程序。
- 脚本会覆盖 `bin_dir` 中同名的 `shutdown_message*` 文件。建议在仓库的 rootfs 暂存目录中生成并检查结果，不要直接把实体设备的挂载目录作为 `bin_dir`。
- PNG 预览只能验证排版和文字渲染，不能替代目标设备上的帧缓冲显示测试。
- `chmod 0755` 以 Linux/WSL 等 POSIX 环境为准；在原生 Windows 文件系统中，权限行为可能不同。
- 修改尺寸、像素格式或偏移前，必须重新确认目标程序内嵌资源的实际布局。

## 代码规范

- 源码使用 UTF-8 和 LF 换行；
- 注释与模块说明使用中文；
- 函数使用类型标注；
- 函数文档字符串采用 Google 风格，通过 `Args`、`Returns` 和 `Raises` 描述接口与异常；
- 工具只负责生成文件，不负责烧录、上传或替换实体设备上的程序。

---

<div align="center">

<sub><b>tools/</b> · host-side resource generation utilities</sub>

</div>
