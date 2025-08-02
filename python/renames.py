#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import re
import time
import shutil
import argparse
import hashlib
from pathlib import Path
import argcomplete

class Args:
    def __init__(self) -> None:
        self.format = r"%Y%m%d_%H%M%S"
        self.ignore_timestamp = False
        self.input = "."
        self.keep_name = False
        self.mode = "auto"
        self.no_apply = False
        self.no_follow_link = False
        self.no_skip = False
        self.postfix = ""
        self.prefix = ""
        self.print_recover = False
        self.recover = ["#!/usr/bin/env python","import shutil"]
        self.recover_file = None
        self.verbose = False
    def set_arg(self, arg:argparse.Namespace):
        self.format = arg.format
        self.ignore_timestamp = arg.ignore_timestamp
        self.input = arg.input
        self.keep_name = arg.keep_name
        self.mode = arg.mode
        self.no_apply = arg.no_apply
        self.no_follow_link = arg.no_follow_link
        self.no_skip = arg.no_skip
        self.postfix = arg.postfix
        self.prefix = arg.prefix
        self.print_recover = arg.print_recover
        self.recover_file = arg.recover_file
        self.verbose = arg.verbose

ARGS = Args()

def parse_arguments() -> None:
    parser = argparse.ArgumentParser(description='按照修改时间重命名指定目录的文件')
    parser.add_argument('-i', '--input', default='.', help='指定输入文件夹')
    parser.add_argument('-p', '--prefix', default='', help='指定前缀')
    parser.add_argument('-e', '--postfix', default='', help='指定后缀')
    parser.add_argument('-m', '--mode', default='auto', help='选择重命名模式',
                        choices=["time","md5","random","auto"])
    parser.add_argument('-f', '--format', default=r'%Y%m%d_%H%M%S',
                        help=r"指定时间格式 (默认: %%Y%%m%%d_%%H%%M%%S')")
    parser.add_argument('-L', '--no-follow-link', action='store_true', help='获取链接本身的修改时间')
    parser.add_argument('-k', '--keep-name', action="store_true", help='保存老名字')
    parser.add_argument('-N', '--no-skip', action="store_true", help='不跳过名字已具备类似条件的文件')
    parser.add_argument('-I', '--ignore-timestamp', action="store_true", help='忽略文件名中可能含有的时间戳')
    parser.add_argument('-o', '--recover-file', default=None, help='指定恢复脚本输出文件')
    parser.add_argument('-P', '--print-recover', action="store_true", help='打印恢复脚本')
    parser.add_argument('-n', '--no-apply', action='store_true', help='不进行任何更改')
    parser.add_argument('-v', '--verbose', action='store_true', help='执行时显示更多输出')
    argcomplete.autocomplete(parser)
    args = parser.parse_args()
    ARGS.set_arg(args)
    return

def print_verbose(s:str):
    if ARGS.verbose:
        print(s)

def save_recover():
    if not ARGS.recover_file:
        return
    log_file = Path(ARGS.recover_file)
    with log_file.open("a", encoding="utf8") as f:
        f.write("\n".join(ARGS.recover))
        f.close()

def calculate_md5(file: Path):
    hash_md5 = hashlib.md5()
    if not file.is_file():
        print(f"# WARN 要求计算非文件的md5: {file}")
        return hash_md5.hexdigest()
    with open(file, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def check_date(year:int, month:int, day:int) -> bool:
    return 1970 < year < 2050 and 0 < month < 13 and 0 < day < 32

def process_files():
    pattern_ymd = re.compile(r'.*?(\d{4})[-_年]*(\d{2})[-_月]*(\d{2}).*', re.I)
    pattern_stmp =  re.compile(r'.*?(\d{13}).*', re.I)
    processed = 0
    skipped = 0

    for file in Path(ARGS.input).iterdir():
        if not file.is_file():
            continue
        timestamp = file.stat(follow_symlinks=not ARGS.no_follow_link).st_mtime
        # 检查文件扩展名是否匹配
        year, month, day = ["0", "0", "0"]
        match = pattern_ymd.match(str(file))
        if match:
            # 提取年月日
            year, month, day = match.groups()[:3]
        if check_date(int(year), int(month), int(day)) and \
                not ARGS.no_skip and ARGS.mode not in ("random", "md5"):
            print_verbose(f"INFO 跳过:已具备类似条件'{file}'")
            skipped += 1
            continue
        if not ARGS.ignore_timestamp:
            match = pattern_stmp.match(str(file))
            if match:
                timestamp_ = float(match.group(1))/1000
                if 0 < timestamp_ < 2840111999:
                    timestamp = timestamp_
        # 文件全名: file.name
        # 文件名: file.stem
        # 后缀: file.suffix
        target_mdfiytime = time.strftime(ARGS.format,time.localtime(timestamp))
        target_format = (f"{file.parent}/{ARGS.prefix}", f"{f"_{file.stem}{ARGS.postfix}" if ARGS.keep_name else ARGS.postfix}{file.suffix}")
        if ARGS.mode == "time":
            target = Path(f"{target_format[0]}{target_mdfiytime}{target_format[1]}")
        elif ARGS.mode == "md5":
            target = Path(f"{target_format[0]}{calculate_md5(file)}{target_format[1]}")
        elif ARGS.mode == "random":
            target = Path(f"{target_format[0]}{hashlib.md5(os.urandom(32)).hexdigest()}{target_format[1]}")
        else:
            target = Path(f"{target_format[0]}{target_mdfiytime}{target_format[1]}")
            if target.exists() and file.resolve() != target.resolve():
                target = Path(f"{target_format[0]}{target_mdfiytime}_{calculate_md5(file)}{target_format[1]}")

        if file.resolve() == target.resolve():
            print_verbose(f"INFO 跳过:等价的路径'{file}' - '{target}'")
            skipped += 1
            continue

        if target.exists():
            print(f"WARN 跳过:重命名'{file}'时目标文件'{target}'已存在")
            skipped += 1
            continue

        # 执行文件操作
        try:
            if not ARGS.no_apply:
                shutil.move(file, target)
            print_verbose(f"INFO 移动: {file} -> {target}")
            ARGS.recover.append(f"shutil.move(\"{target.absolute()}\", \"{file.absolute()}\")")
            processed += 1
        except Exception as e:
            print(f"ERROR 处理文件失败:{file} - {str(e)}")

    print(f"操作完成: 重命名了 {processed} 个文件, 跳过了 {skipped} 个文件")
    if ARGS.no_apply:
        print_verbose("提示: 由于参数指定，并未进行任何操作")

def main():
    parse_arguments()

    if not Path(ARGS.input).is_dir():
        print(f"错误: 输入目录 '{ARGS.input}' 不存在或不是目录")
        sys.exit(1)
    process_files()
    save_recover()
    if ARGS.print_recover:
        print("---------以下是恢复脚本---------")
        print("\n".join(ARGS.recover))

if __name__ == '__main__':
    main()
