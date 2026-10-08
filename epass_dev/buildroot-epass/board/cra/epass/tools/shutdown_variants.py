#!/usr/bin/env python3
"""
生成三份带有不同 CRA 关机提示的帧缓冲辅助程序。
"""

from __future__ import annotations
import argparse
import os
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

WIDTH = 360
HEIGHT = 129
RGB_OFFSET = 0x0DDC
RGB_SIZE = WIDTH * HEIGHT * 3
MESSAGES = (
    "要走了吗，不再看看",
    "再见，祝愿未来",
    "别忘记这里",
)


def render_message(message: str, font: ImageFont.FreeTypeFont) -> Image.Image:
    """
    将一条关机提示渲染为 RGB 位图

    Args:
        message: 要显示在白色矩形中的提示文字
        font: 用于绘制提示文字的 Pillow 字体对象

    Returns:
        尺寸为 360×129、可直接转换为 RGB888 字节流的图像
    """

    image = Image.new("RGB", (WIDTH, HEIGHT), "black")
    draw = ImageDraw.Draw(image)
    draw.rectangle((8, 20, WIDTH - 9, HEIGHT - 21), fill="white")
    box = draw.textbbox((0, 0), message, font=font)
    text_width = box[2] - box[0]
    text_height = box[3] - box[1]
    x = (WIDTH - text_width) // 2 - box[0]
    y = (HEIGHT - text_height) // 2 - box[1]
    draw.text((x, y), message, font=font, fill="black")
    return image


def main() -> None:
    """
    解析命令行参数并生成三份关机提示程序

    Raises:
        FileNotFoundError: 模板程序或字体文件不存在
        ValueError: 模板程序不足以容纳固定尺寸的 RGB 图像
        OSError: 字体加载、文件写入或权限设置失败
    """

    parser = argparse.ArgumentParser(
        description="生成三份带有不同中文提示的 CRA 关机辅助程序"
    )
    parser.add_argument(
        "bin_dir",
        type=Path,
        help="包含 shutdown_message 模板程序的目录",
    )
    parser.add_argument(
        "font",
        type=Path,
        help="用于渲染中文提示文字的字体文件",
    )
    parser.add_argument(
        "--preview-dir",
        type=Path,
        help="可选的 PNG 预览图输出目录",
    )
    parser.add_argument(
        "--font-size",
        type=int,
        default=28,
        help="提示文字字号（默认值：28）",
    )
    args = parser.parse_args()

    template_path = args.bin_dir / "shutdown_message"
    template = bytearray(template_path.read_bytes())
    if len(template) < RGB_OFFSET + RGB_SIZE:
        raise ValueError(f"{template_path} 太小，无法容纳内嵌的 RGB 图像")

    font = ImageFont.truetype(str(args.font), args.font_size)
    output_names = ("shutdown_message", "shutdown_message_2", "shutdown_message_3")

    if args.preview_dir:
        args.preview_dir.mkdir(parents=True, exist_ok=True)

    for index, (message, output_name) in enumerate(
        zip(MESSAGES, output_names), start=1
    ):
        image = render_message(message, font)
        executable = bytearray(template)
        executable[RGB_OFFSET : RGB_OFFSET + RGB_SIZE] = image.tobytes()
        output_path = args.bin_dir / output_name
        output_path.write_bytes(executable)
        os.chmod(output_path, 0o755)
        if args.preview_dir:
            image.save(args.preview_dir / f"shutdown_message_{index}.png")


if __name__ == "__main__":
    main()
