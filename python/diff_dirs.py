#!/usr/bin/env python
"""计算两个文件夹之间的差异"""

import argparse
from pathlib import Path
import hashlib
import os
import argcomplete

source_file_black_list = [Path(i).resolve() for i in [
    ".local/state/konsolestaterc",
    ".config/fcitx5/conf/cached_layouts",
    "README.org",
    ".gitmodules",
    "diff.py"]]
source_black_list = [Path(i).resolve() for i in [
    ".local/share/aurorae",
    ".local/share/icons",
    ".local/share/color-schemes",
    ".local/share/plasma",
    ".local/share/themes",
    ".config/latte",
    ".config/polybar",
    ".config/bspwm",
    ".config/sxhkd",
    ".config/i3",
    ".git"]]

stat = {"diff":0, "remove":0, "change_type":0, "total":0, "input_width":0}

def diff_dir(obj_dir:Path):
    if not obj_dir.is_dir():
        return
    if obj_dir.resolve() in source_black_list:
        return
    for i in obj_dir.iterdir():
        if i.is_dir():
            diff_dir(i)
            continue
        checkfile(i)

def print_splitline():
    if ARGS.diff or ARGS.copy:
        print("-------------------------------->")

def print_debug(s):
    if ARGS.debug:
        print(s)

def checkfile(file:Path):
    if not file.is_file():
        return
    if file.resolve() in source_file_black_list:
        return
    origin_path = Path(ARGS.origin).absolute() if ARGS.origin else Path.home()
    source_file = Path(f"{origin_path}/{str(file.absolute())[stat["input_width"]:]}")
    print_debug(f"CHECKFILE: {str(file.absolute())}  <===== {source_file}")
    stat["total"]+=1
    if not source_file.exists():
        print_splitline()
        print(f"[{source_file}] non exists")
        stat["remove"]+=1
        return
    if not source_file.is_file():
        print_splitline()
        print(f"[{source_file}] not file")
        stat["change_type"]+=1
        return
    if source_file.stat().st_mtime <= file.stat().st_mtime:
        return
    if hashlib.md5(source_file.read_bytes()).hexdigest() == hashlib.md5(file.read_bytes()).hexdigest():
        return
    stat["diff"]+=1
    print_splitline()
    print(f">>> FILE CHANGED: `{file}`")
    if ARGS.diff:
        os.system(f"diff --color=always {file} {source_file}")
    if ARGS.copy:
        os.system(f"cp {source_file} {file}")
        print(f"[!] copyed:`cp {source_file} {file}`")

def main():
    stat["input_width"] = len(str(Path(ARGS.input).absolute()))
    print_debug(f"INPUT_WIDTH: {stat["input_width"]}")
    diff_dir(Path(ARGS.input))
    print(f"共{stat['diff']}个文件修改,{stat['remove']}个文件被移除,{stat['change_type']}个文件类型改变")
    total_change = stat['change_type']+stat['diff']+stat['remove']
    print(f"共{total_change}个变化文件占{stat['total']}个文件的"
          f"{(total_change/stat['total']*100) if stat['total'] else 0}%")
    if ARGS.black_list:
        print("由于设置了额外黑名单，以下文件夹下可能会有被略过的文件:")
        for i in ARGS.black_list.split(r"\n"):
            print(f" - {ARGS.input}/{i}")

if __name__ == "__main__":
    arg_parse = argparse.ArgumentParser(description="对比当前备份文件夹和家目录的变化")
    arg_parse.add_argument("-d","--diff",action="store_true",help="对比前后变化")
    arg_parse.add_argument("-c","--copy",action="store_true",help="复制更新文件")
    arg_parse.add_argument("-i","--input",default=".",help="目标（旧的）文件夹设置（对比基准文件树）")
    arg_parse.add_argument("-o","--origin",default=None,help="对比（更新的）文件夹设置（默认为$HOME）")
    arg_parse.add_argument("-b","--black-list",default=None,help="文件夹黑名单")
    arg_parse.add_argument("-x","--debug",action="store_true",help="调试")
    argcomplete.autocomplete(arg_parse)
    ARGS = arg_parse.parse_args()
    if ARGS.black_list:
        source_black_list += [Path(f"{ARGS.input}/{i}").resolve() for i in ARGS.black_list.split(r"\n")]
    main()
