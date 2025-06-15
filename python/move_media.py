#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re
import sys
import shutil
import argparse
import hashlib
from pathlib import Path

VERBOSE = [0]

def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='将媒体文件按照文件名的时间排列进入文件夹')
    parser.add_argument('-i', '--input', default='.', help='指定输入文件夹')
    parser.add_argument('-o', '--output', default='.', help='指定输出根文件夹')
    parser.add_argument('-c', '--copy', action='store_true', help='使用复制操作')
    parser.add_argument('-n', '--no-apply', action='store_true', help='不进行任何更改')
    parser.add_argument('-p', '--path', default=None, help='指定沿用目录树的目录')
    parser.add_argument('-f', '--format', default=r'%Y/%Y_%m/%Y_%m_%d',
                        help=r"指定输出路径格式 (默认: %%Y/%%Y_%%m/%%Y_%%m_%%d)")
    parser.add_argument('-v', '--verbose', nargs="?", default=0, const=1, type=int, help='执行时显示更多输出(指定等级)')
    args = parser.parse_args()
    VERBOSE[0] = args.verbose
    return args

def print_verbose(level:int,s:str):
    if VERBOSE[0] >= level:
        print(s)

def diff_file(file1:Path, file2:Path) -> bool:
    if not file1.is_file() or not file2.is_file():
        print_verbose(3, f"警告: 比较非文件之间是否相同 - {file1}")
        return False
    if file1.stat().st_size == file2.stat().st_size:
        # 计算文件哈希
        hash1 = calculate_md5(file1)
        hash2 = calculate_md5(file2)
        return hash1 == hash2
    return False

def calculate_md5(file: Path):
    hash_md5 = hashlib.md5()
    if not file.is_file():
        print_verbose(3, f"警告: 要求计算非文件的md5 - {file}")
        return hash_md5.hexdigest()
    with open(file, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def process_files(args:argparse.Namespace) -> tuple[int, int]:
    pattern = re.compile(
        r'.*?(\d{4})[-_年]*(\d{2})[-_月]*(\d{2}).*' + \
        r'\.(mp4|mkv|png|jpg|jpeg|dng|gif|3gp|m4a|webp)$',
        re.I
    )

    processed_dirs = {}
    processed = 0
    skipped = 0

    for file in Path(args.input).iterdir():
        # 跳过目录和非文件项
        if not file.is_file():
            continue
        # 检查文件扩展名是否匹配
        year, month, day = [0, 0, 0]
        match = pattern.match(str(file))
        if match:
            # 提取年月日
            year, month, day = match.groups()[:3]
        if not( int(year) > 1949 and \
                int(month) > 0 and int(month) < 13 and \
                int(day) > 0 and int(day) < 32):
            print_verbose(2, f"跳过: 文件名不符合模式 - {file}")
            skipped += 1
            continue

        # 构建目标路径
        try:
            target_relative_path = args.format \
                    .replace(r"%Y", year) \
                    .replace(r"%m", month) \
                    .replace(r"%d", day)
        except Exception as e:
            print(f"错误: 无效的格式字符串 - {str(e)}")
            sys.exit(1)
        if target_relative_path in processed_dirs:
            # 使用缓存
            target_dir = processed_dirs[target_relative_path]
        else:
            target_dirs = list(Path(args.output).glob(f"{target_relative_path}*"))
            if args.path:
                target_dirs = [Path(f"./{str(i)[len(args.path):]}") for i in Path(args.path).glob(f"{target_relative_path}*")] + target_dirs
            target_dir = Path(f"{args.output}/{target_relative_path}")
            for target_dir in target_dirs:
                if target_dir.is_dir() or not target_dir.exists():
                    break
            if len(target_dirs) > 1 and target_relative_path not in processed_dirs:
                print(target_dirs)
                processed_dirs[target_relative_path] = target_dir
                print_verbose(3, f"警告: 出现多个相同的匹配目录 - {target_relative_path}*")
        target = Path(f"{target_dir}/{file.name}")

        if target_dir.is_file():
            print_verbose(2, f"跳过: 输出目录与现存文件重名 - {file}")
            skipped += 1
            continue

        if not target_dir.exists():
            print_verbose(3, f"创建: 创建输出文件夹 - {target_dir}")
            if not args.no_apply:
                target_dir.mkdir(parents=True)

        # 检查目标文件是否已存在
        if target.exists():
            # 文件内容相同
            if diff_file(file, target):
                print_verbose(2,f"跳过: 文件已存在且内容相同 - {file}")
                backup = Path(f"{file}.bak")
                if not backup.exists():
                    if not args.no_apply:
                        shutil.move(file, backup)
                    print_verbose(2,f"重命名: {file} -> {backup}")
                else:
                    print_verbose(3, f"警告: 重复文件重名 - {file}")

                skipped += 1
                continue
            else:
                print(f"警告: 目标文件已存在但内容不同 - {file}")
                skipped += 1
                continue

        # 执行文件操作
        try:
            if args.copy:
                if not args.no_apply:
                    shutil.copy2(file, target)
                action = "复制"
            else:
                if not args.no_apply:
                    shutil.move(file, target)
                action = "移动"

            print_verbose(2, f"{action}: {file} -> {target}")

            processed += 1
        except Exception as e:
            print(f"错误: 处理文件 {file} 失败 - {str(e)}")
    return processed, skipped

def main():
    args = parse_arguments()

    input_dir = Path(args.input)
    output_dir = Path(args.output)
    if not input_dir.is_dir() or not output_dir.is_dir():
        hit = ("输入目录", args.input)
        if not output_dir.is_dir():
            hit = ("输出目录", args.output)
        print(f"错误: {hit[0]} '{hit[1]}' 不存在或不是目录")
        sys.exit(1)
    if len(str(output_dir.resolve())) < len(str(output_dir)):
        args.output = str(output_dir.resolve())
    if args.path:
        path_dir = Path(args.path).resolve()
        if path_dir == output_dir.resolve() or not path_dir.is_dir():
            args.path = None

    # 处理文件
    processed, skipped = process_files(args)

    # 显示摘要
    action = "移动" if not args.copy else "复制"
    print_verbose(1,f"操作完成: {action}了 {processed} 个文件, 跳过了 {skipped} 个文件")
    if args.no_apply:
        print_verbose(3, "提示: 由于参数指定，并未进行任何操作")

if __name__ == '__main__':
    main()
