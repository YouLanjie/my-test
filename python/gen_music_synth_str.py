#!/usr/bin/env python
# Created:2026.06.07
# 用来打谱（特别是五线谱）的辅助脚本

from pathlib import Path
import argparse
import math
import re
import sys
import pytools

def reverse_process(content: list[str]):
    """将txt谱转为简谱（统一改为0-9上下加点模式），用于检查"""
    ret : list[str] = []
    hint_up = ""
    hint_down = ""
    is_inconfig = False
    ktable = zip("cdefgabCDEFGAB1234567", "000000011111112222222", "123456712345671234567")
    ktable = {k:(int(typ), ch) for k,typ,ch in ktable}
    for line in content:
        nl = []
        for ind,c in enumerate(line):
            nl.append(c)
            if not is_inconfig and c == ':':
                is_inconfig = True
            elif is_inconfig and c == ';':
                is_inconfig = False
            if is_inconfig:
                continue
            if c in ktable:
                if ktable[c][0] == 0:
                    hint_down += " "*(ind-len(hint_down))
                    hint_down += "`"
                elif ktable[c][0] == 2:
                    hint_up += " "*(ind-len(hint_up))
                    hint_up += "."
                nl[ind] = ktable[c][1]
        if hint_up:
            ret.append(hint_up)
            hint_up = ""
        ret.append("".join(nl))
        if hint_down:
            ret.append(hint_down)
            hint_down = ""
    print("\n".join(ret))

def print_template(typ="high"):
    """打印绘制的五线谱基本模板"""
    note_char = ["cdefgab", "CDEFGAB", "1234567"]
    note_table = [i+"LL" for i in note_char[0]]
    note_table += [i+"L" for i in note_char[0]]
    note_table += note_char[0]
    ind_center_c = len(note_table)
    note_table += note_char[1]
    note_table += note_char[2]
    note_table += [i+"U" for i in note_char[2]]
    note_table += [i+"UU" for i in note_char[2]]

    if typ == "low":
        up, down, offset = 4, 16, -10
    else:
        up, down, offset = 16, 4, 2
    for i in range(ind_center_c+up, ind_center_c-down-1, -1):
        if i < 0 or i >= len(note_table):
            break
        # (默认)上下加线字符
        char = "."
        if (ind_center_c-i) % 2:
            # 空白字符
            char = " "
        elif ind_center_c+offset <= i < ind_center_c+offset+5+4:
            # 五线谱行字符
            char = "="
        print(f"{note_table[i]:3s} | {char*50} |")

def process(content: list[str]):
    """将五线谱转txt谱"""
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
        width = max(len(l) for l in lines)
        lines = [l+" "*(width-len(l)) for l in lines]
    lines = [(f":track={ind+1}; " if len(lines) > 1 else "")+("|"+l).strip()[1:] \
            for ind,l in enumerate(lines)]
    print("\n".join(lines))

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="用来打谱（特别是五线谱）的辅助脚本")
    parser.add_argument("--print-template", "-p", nargs="?", default="", const="high",
                        choices=("low", "high"), help="打印模板")
    parser.add_argument("--reverse", "-r", action="store_true", help="将txt曲谱反转成简谱(纯数字上下加点)")
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
    if args.reverse:
        reverse_process(content)
    else:
        process(content)

if __name__ == "__main__":
    main()
