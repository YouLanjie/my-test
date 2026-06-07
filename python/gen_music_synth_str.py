#!/usr/bin/env python
# Created:2026.06.07
# 用来打谱（特别是五线谱）的辅助脚本

import pytools
from pathlib import Path
import argparse
import math
import re
import sys

def print_template(typ="high"):
    note_char = ["cdefgab", "CDEFGAB", "1234567"]
    note_table = [i+"LL" for i in note_char[0]]
    note_table += [i+"L" for i in note_char[0]]
    note_table += [i for i in note_char[0]]
    ind_center_C = len(note_table)
    note_table += [i for i in note_char[1]]
    note_table += [i for i in note_char[2]]
    note_table += [i+"U" for i in note_char[2]]
    note_table += [i+"UU" for i in note_char[2]]

    if typ == "low":
        up, down, offset = 4, 16, -10
    else:
        up, down, offset = 16, 4, 2
    for i in range(ind_center_C+up, ind_center_C-down-1, -1):
        if i < 0 or i >= len(note_table):
            break
        line = (("=" if ind_center_C+offset <= i < ind_center_C+offset+5+4 else ".") if not (ind_center_C-i) % 2 else " ")*50
        print(f"{note_table[i]:3s} | {line} |")

def process(content: list[str]):
    notes : dict[int,list[str]] = {}
    for line in content:
        if not line or line[0] not in "cdefgabCDEFGAB1234567":
            continue

        name = line.split()[:1]
        if not name:
            continue
        name = name[0]

        line = line.split("|")[1:2]
        if not line:
            continue
        line = line[0]

        for match in re.finditer(r"(-?\d+\.?\d*)", line):
            ind = match.span()[0]
            if ind not in notes:
                notes[ind] = []
            value = float(match.group(0))
            tag = name if value >= 0 else "0"
            tail = ""
            if value != 1 and value % 2 != 0:
                tail += "."
                value+=1
            value = math.log2(abs(value)/4)
            tail = ("*" if value < 0 else "/")*int(abs(value)) + tail
            notes[ind].append(tag+tail)
    lines : list[str] = []
    width = 0
    for i in sorted(notes.keys()):
        for ind,note in enumerate(notes[i]):
            if len(lines) <= ind:
                lines += [" "*width]*(ind+1-len(lines))
            lines[ind] += note + " "
        width = max([len(l) for l in lines])
        lines = [l+" "*(width-len(l)) for l in lines]
    lines = [(f":track={ind+1}; " if len(lines) > 1 else "")+("|"+l).strip()[1:] \
            for ind,l in enumerate(lines)]
    print("\n".join(lines))

def main():
    parser = argparse.ArgumentParser(description="用来打谱（特别是五线谱）的辅助脚本")
    parser.add_argument("--print-template", "-p", nargs="?", default="", const="high",
                        choices=("low", "high"), help="打印模板")
    parser.add_argument("file", nargs="?", help="输入文件")
    args = parser.parse_args()
    if args.print_template:
        print_template(args.print_template)
        return

    content = []
    if args.file:
        input_file = Path(args.file)
        if not input_file.is_file():
            pytools.print_err(f"文件'{input_file}'不存在")
            return
        content = pytools.read_text(input_file).splitlines()
        if not content:
            pytools.print_err(f"文件'{input_file}'为空")
            return
    elif not sys.stdin.isatty():
        content = sys.stdin.read().splitlines()
    else:
        pytools.print_err("未提供任何内容来源")
        return
    process(content)

if __name__ == "__main__":
    main()
