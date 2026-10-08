<div align="center">

# Font Awesome 图标头文件生成工具

<sub>EEZ Selection · Unicode Metadata · C Macros</sub>

</div>

> [!NOTE]
> 本目录读取 EEZ 工程中为 `fontawesome` 字体选择的 Unicode 范围，并结合图标元数据生成设备 UI 使用的 C 宏。

---

## 文件职责

| 文件 | 用途 |
| :--- | :--- |
| `genheader.py` | 解析 EEZ 工程与图标元数据，生成头文件 |
| `icons.json` | Unicode 到 Font Awesome 图标名称的元数据映射 |
| `icons.h` | 自动生成的 `UI_ICON_*` 宏定义 |
| `../epass.eez-project` | 提供 `fontawesome` 的 `lvglRanges` 选择范围 |

---

## 使用方法

从本目录执行：

```bash
python genheader.py
```

脚本会展开 `lvglRanges` 中的单个编码和区间，在 `icons.json` 中查找名称，并重写 `icons.h`。找不到名称的编码会在终端输出警告。

> [!WARNING]
> `icons.h` 是生成文件，不应手工长期维护。更新 EEZ 图标选择或 `icons.json` 后，应重新运行脚本并审查生成差异。

---

<div align="center">

<sub><b>icon_header_gen/</b> · generated icon macros for the EEZ interface</sub>

</div>
