#!/usr/bin/env python
# Created:2025.11.29
"""
分割全一卷的txt小说文件（一般来自轻小说文库）
转自blogs
"""

import re
import argparse
import json
from pathlib import Path

import pytools

def parse_arg() -> argparse.Namespace:
    """解释参数"""
    parser=argparse.ArgumentParser(description="分割全一卷的txt小说文件")
    parser.add_argument("-c", "-i", "--config", type=Path, default=Path("config.json"),
                        help="指定配置文件")
    parser.add_argument("-C", "--print-config", action="store_true", help="打印配置文件模板")
    return parser.parse_args()

def get_same(li1:list, li2:list) -> list:
    limit = min(len(li1), len(li2))
    while li1[:limit] != li2[:limit]:
        limit -= 1
    return li1[:limit]

def process_unicode(s: str):
    def sub(match: re.Match):
        try:
            return chr(int(match.group(1)))
        except ValueError:
            return ""
    return re.sub(r'&#(\d+);', sub, s)

def main():
    args = parse_arg()
    cfg_f = args.config
    cfg = {"source_file":"",
           "title":"",
           "setupfile":"./setup.setup",
           "section_pattern":r"[　]*(第.*卷.*)",
           "comment":"这里用来记录点额外信息，不会对结果产生影响",
           "process_unicode":True,
           "words":[["身分","身份"], ["计画","计划"], ["徵","征"],
                    ["乾","干"]],}
    new_cfg = {}
    if cfg_f.is_file():
        try:
            new_cfg = json.loads(cfg_f.read_bytes() or "{}")
        except json.JSONDecodeError as e:
            pytools.print_err(str(e))
    if not isinstance(new_cfg, dict):
        pytools.print_err(f"Err type: {type(cfg)}")
        new_cfg = {}
    pytools.merge_dict(cfg, new_cfg, True)

    if args.print_config:
        print(json.dumps(cfg, ensure_ascii=False, indent='\t'))
        return

    inp = cfg_f.parent/cfg["source_file"]
    if not inp.is_file():
        pytools.print_err(f"[WARN] '{inp}' is not file")
        return
    content = pytools.read_text(inp)

    if cfg["process_unicode"]:
        content = process_unicode(content)

    for w1,w2 in cfg["words"]:
        print(f"替换 '{w1}' 为 '{w2}' : 共计{len(re.findall(w1, content))}处")
        content = re.sub(w1, w2, content)

    content = content.splitlines()
    groups : list[list[str]] = []
    now_group = []
    begin = False
    try:
        pattern = re.compile(cfg["section_pattern"], re.I)
    except re.error as e:
        pytools.print_err(f"[WARN] re compile error: {e}")
        pattern = re.compile(r"[　]*(第.*卷.*)", re.I)
    for line in content:
        if line == "":
            continue
        if pattern.match(line):
            if now_group:
                groups.append(now_group)
            now_group = []
            begin = True
        if begin:
            now_group.append(line)
    if now_group:
        groups.append(now_group)
    repeat = []
    ind = 1
    contents = []
    content = []
    same_len = 0
    while ind < len(groups):
        if not repeat:
            repeat = get_same(groups[ind-1][0].split(), groups[ind][0].split())
            if content:
                contents.append(content)
            content = []
            content.append(f"* {" ".join(repeat)}")
            if "插图" not in groups[ind-1][0]:
                content.append(f"** {" ".join(groups[ind-1][0].split()[len(repeat):])}")
            content+=groups[ind-1][1:]
            print(f"==> {repeat}")
            print(f"  --> {groups[ind-1][0].split()[len(repeat):]}")
        if len(get_same(groups[ind][0].split(), repeat)) < same_len:
            repeat = []
        else:
            if "插图" not in groups[ind][0]:
                content.append(f"** {" ".join(groups[ind][0].split()[len(repeat):])}")
            content+=groups[ind][1:]
            print(f"  --> {groups[ind][0].split()[len(repeat):]}")
        same_len = len(get_same(groups[ind][0].split(), repeat))
        ind += 1
    if content:
        contents.append(content)
    # import rich
    # rich.print(new_li)
    ind = 0
    for content in contents:
        ind += 1
        h1 = content[0][2:]
        outf = Path(f"{ind:03d}_{h1}.org")
        print((outf.is_file(), outf))
        if outf.is_dir():
            continue
        s = "\n\n".join([i for i in content if i])
        outf.write_text(f"""\
#+title: {cfg['title']} {h1}
#+setupfile: {cfg['setupfile']}
""" + s)

if __name__ == "__main__":
    main()
