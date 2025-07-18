#!/usr/bin/python
"""python常用函数合集"""

import sys

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
