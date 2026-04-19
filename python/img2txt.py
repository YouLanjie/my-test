#!/usr/bin/env python
# Created:2026.04.18
# ai生成（比libcaca效果好）

"""
img2txt - 将图片转换为终端彩色字符画（使用半块字符 ▀）
依赖: Pillow (pip install Pillow)
用法: python img2txt.py <图片路径> [输出宽度(字符列数)]
"""

import sys
from PIL import Image

def ansi_truecolor(fg_rgb=None, bg_rgb=None):
    """生成 ANSI 真彩色转义序列 (前景/背景)"""
    codes = []
    if fg_rgb:
        codes.append(f"\033[38;2;{fg_rgb[0]};{fg_rgb[1]};{fg_rgb[2]}m")
    if bg_rgb:
        codes.append(f"\033[48;2;{bg_rgb[0]};{bg_rgb[1]};{bg_rgb[2]}m")
    return "".join(codes)

def img2txt(image_path, width=80):
    """主转换函数"""
    # 1. 加载图片并转为 RGB
    img = Image.open(image_path).convert("RGB")
    orig_w, orig_h = img.size

    # 2. 计算缩放尺寸（保持宽高比，每个字符对应 2 个像素行）
    # 终端字符的宽高比近似 1:2，因此目标像素高度 = 字符行数 * 2
    target_w = width
    target_h_px = int(target_w * orig_h / orig_w)   # 按原比例计算像素高度
    # 确保高度为偶数，因为每个字符显示 2 行像素
    if target_h_px % 2 != 0:
        target_h_px += 1
    target_h = target_h_px // 2   # 字符行数

    # 3. 缩放图片
    img = img.resize((target_w, target_h_px), Image.Resampling.LANCZOS)
    pixels = img.load()

    # 4. 逐字符生成输出
    out_lines = []
    for row in range(target_h):          # 每个字符行
        line_parts = []
        for col in range(target_w):
            # 上像素 (y = row*2), 下像素 (y = row*2+1)
            r1, g1, b1 = pixels[col, row * 2]
            r2, g2, b2 = pixels[col, row * 2 + 1]

            # 生成 ANSI 控制码：上像素为前景色，下像素为背景色
            ansi = ansi_truecolor(fg_rgb=(r1, g1, b1), bg_rgb=(r2, g2, b2))
            # 使用半块字符 '▀' (上半块)
            line_parts.append(f"{ansi}▀")
        # 行末重置样式并换行
        line_parts.append("\033[0m")
        out_lines.append("".join(line_parts))

    # 5. 输出到终端
    print("\n".join(out_lines))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("用法: python img2txt.py <图片文件> [输出宽度]")
        sys.exit(1)

    img_path = sys.argv[1]
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 80
    img2txt(img_path, width)
