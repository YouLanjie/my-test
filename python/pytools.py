#!/usr/bin/python
"""python常用函数合集"""

import sys
from pathlib import Path
import datetime
import copy

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

def calculate_relative(path_to:Path, path_from:Path) -> Path:
    """计算path_to相对于path_from的相对路径"""
    p1 = path_to
    p2 = path_from
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

def calculate_relative2(path_to:Path, path_from:Path) -> Path:
    """计算path_to相对于path_from的相对路径(使用absolute)"""
    p1 = path_to
    p2 = path_from
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

def get_strtime(dt :float|int|datetime.datetime = datetime.datetime.now(), h=True,m=True,s=True) -> str:
    """返回格式化的字符串（时分秒可选）"""
    if isinstance(dt, (float,int)):
        dt = datetime.datetime.fromtimestamp(dt)
    t = dt.strftime("%Y-%m-%d ")
    t += "一二三四五六日"[dt.weekday()]
    l = [i[1] for i in ((h,"%H"),(m,"%M"),(s,"%S")) if i[0]]
    t += dt.strftime(" "+":".join(l) if l else "")
    return t

def get_filename_test_str(touch=False, skip="") -> str:
    """生成用于文件名测试的字符串"""
    spec = r"`~!@#$%^&*()-=_+[]{}\|,.<>?;:' "+'"\n\t'
    spec = list(set(spec)-set(skip))
    table = list(range(ord('A'),ord('Z')+1))+list(range(ord('a'),ord('z')))
    f = "".join(["%c%c" % (i,j) for i,j in zip(table[:len(spec)],spec)]) + ".txt"
    if touch and not Path(f).exists():
        Path(f).touch()
    return f

def merge_dict(old:dict, new:dict, warn=False, prefix=""):
    """将new词典的内容依照old的结构和类型合并"""
    if prefix:
        prefix+="/"
    for k in new:
        if ".*" in old:
            old[k] = copy.deepcopy(old[".*"])
        if k not in old:
            if warn:
                print_err(f"[WARN] 词典'/{prefix}'不存在关键词'{k}'")
            continue
        if not isinstance(new[k], type(old[k])):
            if warn:
                print_err(f"[WARN] 路径'/{prefix}{k}'的类型应该是"
                        f"{type(old[k])}而非{type(new[k])}")
            continue
        if isinstance(old[k], dict):
            merge_dict(old[k], new[k], warn=warn, prefix=prefix+k)
            continue
        old[k] = new[k]
    if ".*" in old:
        old.pop(".*")

def squash_dict(data:dict, prefix="", split=".") -> dict:
    """将分级词典以'.'为分割字符合并为同级词典"""
    new = {}
    if prefix:
        prefix+=split
    for k in data:
        if isinstance(data[k], dict):
            new.update(squash_dict(data[k], prefix=f"{prefix}{k}", split=split))
            continue
        new[f"{prefix}{k}"] = data[k]
    return new

def process_filelist(li:list[str]) -> dict[str,list[int]]:
    """识别文件列表"""
    ret = {}
    for s in li:
        s = str(s)
        l = s.split("::")
        if len(l) != 2:
            ret[s] = []
            continue
        nums = []
        try:
            nums = [int(i or -1) for i in l[1].split("-")]
        except ValueError:
            ret[s] = (-1, -1)
            continue
        ret[l[0]] = nums
    return ret

def read_text(file: Path) -> str:
    if not file.is_file():
        return ""
    try:
        b = file.read_bytes()
    except (PermissionError):
        return ""
    s = ""
    for i in ["utf8", "gbk", "utf32", "utf16"]:
        try:
            s = b.decode(i)
            break
        except UnicodeDecodeError:
            pass
    return s
