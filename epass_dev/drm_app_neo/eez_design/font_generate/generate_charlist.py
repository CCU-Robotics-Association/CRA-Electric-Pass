"""生成 EEZ/LVGL 字库使用的去重字符表。"""

from pathlib import Path
import sys
from typing import List


def read_character_sources(paths: List[Path]) -> str:
    """读取全部字符源文件并合并为一个字符串。"""
    return "".join(path.read_text(encoding="utf-8") for path in paths)


def unique_characters(text: str) -> str:
    """按首次出现顺序去除换行和重复字符。"""
    return "".join(dict.fromkeys(char for char in text if char not in "\r\n"))


base_dir = Path(__file__).resolve().parent
source_paths = [Path(arg) for arg in sys.argv[1:]]
if not source_paths:
    source_paths = [base_dir / "common_char.txt"]
    extra_path = base_dir / "extra_char.txt"
    if extra_path.exists():
        source_paths.append(extra_path)

result = unique_characters(read_character_sources(source_paths))
(base_dir / "result.txt").write_text(result, encoding="utf-8")
print(len(result))
