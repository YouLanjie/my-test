#!/usr/bin/python
"""python常用函数合集"""

import sys
from pathlib import Path

def print_err(s:str):
    """从stderr打印输出"""
    print(s, file=sys.stderr)

def _get_char_width(c:str) -> int:
    if ord(c) <= 127:
        return 1
    if c in "“”‘’❲❳…":
        # 非ASCII字符但是仍旧1字符宽度
        return 1
    return 2

def get_str_width(s:str) -> int:
    """计算字符串打印宽度"""
    width = 0
    for c in s:
        width += _get_char_width(c)
    return width

def split_str_by_width(s:str, obj_width:int) -> list[str]:
    """依据宽度为字符串分组"""
    if obj_width < 2:
        return []
    width = 0
    li = []
    current = ""
    for c in s:
        width += _get_char_width(c)
        if width > obj_width:
            width = _get_char_width(c)
            li.append(current)
            current = ""
        current += c
    if current:
        li.append(current)
    return li

def get_str_in_width(text:str, width:int, fill:str=' ', align:str="<c>"):
    """根据打印宽度截断字符串"""
    ret_text = text
    if width > get_str_width(ret_text):
        dwidth = width - get_str_width(ret_text)
        if align.lower() == "<l>":
            ret_text = f"{text}{fill[0]*(dwidth)}"
        elif align.lower() == "<c>":
            ret_text = f"{fill[0]*(dwidth//2)}{text}{fill[0]*(dwidth-dwidth//2)}"
        elif align.lower() == "<r>":
            ret_text = f"{fill[0]*(dwidth)}{text}"
    while len(ret_text) > 0 and width < get_str_width(ret_text):
        ret_text = ret_text[:-1]
    return ret_text

def calculate_relative(p1:Path, p2:Path) -> Path:
    """计算p2相对于p1的相对路径"""
    p1p = [p1.resolve()] + list(p1.resolve().parents)
    p2p = [p2.resolve()] + list(p2.resolve().parents)
    parents = list(set(p1p)&set(p2p))
    parents.sort()
    both_parent = parents[-1]
    relative = p1.resolve().relative_to(both_parent)
    depth = len(p2.resolve().relative_to(both_parent).parts)
    if not p2.is_dir():
        depth -= 1
    back_relative = "../"*depth
    return Path(f"./{back_relative}/{relative}")

def calculate_relative2(p1:Path, p2:Path) -> Path:
    """计算p2相对于p1的相对路径(使用absolute)"""
    p1p = [p1.absolute()] + list(p1.absolute().parents)
    p2p = [p2.absolute()] + list(p2.absolute().parents)
    parents = list(set(p1p)&set(p2p))
    parents.sort()
    both_parent = parents[-1]
    relative = p1.absolute().relative_to(both_parent)
    depth = len(p2.absolute().relative_to(both_parent).parts)
    if not p2.is_dir():
        depth -= 1
    back_relative = "../"*depth
    return Path(f"./{back_relative}/{relative}")
