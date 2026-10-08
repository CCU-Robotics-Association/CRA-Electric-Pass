<div align="center">

# EEZ / LVGL 字符表生成工具

<sub>UTF-8 Character Sources · Stable Deduplication</sub>

</div>

> [!NOTE]
> 本目录生成 EEZ Studio 与 LVGL 字库所需的去重字符表，避免把界面未使用的字符全部打入目标程序。

---

## 文件职责

| 文件 | 用途 |
| :--- | :--- |
| `generate_charlist.py` | 读取 UTF-8 字符源，去除换行与重复字符，并生成 `result.txt` |
| `common_char.txt` | 默认公共字符集 |
| `extra_char.txt` | 可选的界面专用字符集，不存在时会自动跳过 |
| `result.txt` | 运行后生成的字符表，不作为手写输入维护 |

---

## 使用方法

不传参数时，脚本默认读取 `common_char.txt`；若同目录存在 `extra_char.txt`，也会一并读取：

```bash
python generate_charlist.py
```

也可以显式传入一个或多个 UTF-8 文本文件：

```bash
python generate_charlist.py common_char.txt extra_char.txt
```

输出字符按首次出现顺序保留。脚本完成后会把结果写入同目录的 `result.txt`，并在终端打印字符数量。

> [!IMPORTANT]
> 更新字体前应确认页面、动态文本和错误提示所需字符均已包含；桌面预览正常不代表目标固件中的裁剪字库一定完整。

---

<div align="center">

<sub><b>font_generate/</b> · character-set input for generated UI fonts</sub>

</div>
