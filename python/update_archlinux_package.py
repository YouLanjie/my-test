#!/usr/bin/python
import os
import re
import argparse
from pathlib import Path
import shutil
from natsort import natsorted

ARGS = []
pattern = re.compile(r"(.*)-([0-9a-zA-Z.:_]+-[0-9]+)-.*\.pkg\.tar\.zst")
# pattern = re.compile(r"^(.+?)-((?:\d[^-]*?)-(?:\d[^-]+?))-(?:[^-]+?)\.pkg\.tar\.(?:zst|xz|gz|bz2)$")

def get_package_list(inp_file_list:list[Path]) -> dict:
    inp_list = {}
    for file in inp_file_list:
        match = pattern.match(file.name)
        if not match:
            continue
        name, version = match.groups()
        if not inp_list.get(name):
            inp_list[name] = [(version, file)]
        else:
            inp_list[name].append((version, file))
    for name in inp_list:
        inp_list[name] = natsorted(inp_list[name])
    return inp_list

def update_packge(old:Path|None, new:Path, out_path:Path, repo:Path, use_link:bool):
    if old:
        print(f"Updated: {new.name}")
    else:
        print(f"Add: {new.name}")
    if ARGS and ARGS[-1].print:
        if old and old.exists(follow_symlinks=False):
            print(f"rm '{old}'")
        output_file = out_path/new.name
        if use_link:
            print(f"ln -s '{new}' '{output_file}'")
        else:
            print(f"cp '{new}' '{output_file}'")
        print(f"repo-add '{repo}' '{output_file}'")
    if ARGS and ARGS[-1].no_apply:
        return
    if old and old.exists(follow_symlinks=False):
        old.unlink()
    output_file = out_path/new.name
    if use_link:
        output_file.symlink_to(new)
    else:
        shutil.copy(new, output_file)
    os.system(f"repo-add '{repo}' '{output_file}'")

def copy_package(inp_path:Path, out_path:Path, use_link=True):
    if not inp_path.exists():
        print(f"WARN input path `{out_path}` not exists")
        return
    if not out_path.exists():
        print(f"WARN out path `{out_path}` not exists")
        return
    repo_list = list(out_path.glob("*.db.tar.gz"))
    if not repo_list and (ARGS and not ARGS[-1].no_apply):
        print("WARN repo file `*.db.tar.gz` not exists")
        return
    count_update = 0
    count_add = 0
    if repo_list:
        repo = repo_list[-1]
    else:
        repo = Path(".")
    print(f"INFO set searching dir `{inp_path}`")
    print(f"INFO set output dir `{out_path}`")
    print(f"INFO set repo file `{repo}`")
    inp_file_list = list(inp_path.glob("**/*.pkg.tar.zst"))
    out_file_list = list(out_path.glob("**/*.pkg.tar.zst"))
    inp_dict = get_package_list(inp_file_list+out_file_list)

    # 更新文件
    for file in out_file_list:
        match = pattern.match(file.name)
        if not match:
            continue
        name, version = match.groups()
        if file == inp_dict[name][-1][1]:
            continue
        update_packge(file, inp_dict[name][-1][1], out_path, repo, use_link)
        count_update+=1

    # 添加新文件
    for file_name in list(set(inp_dict)-set(get_package_list(out_file_list))):
        if re.match(r".*-debug$", file_name):
            # print(f"INFO 跳过调试包 - {file_name}")
            continue
        update_packge(None, inp_dict[file_name][-1][1], out_path, repo, use_link)
        count_add+=1

    print(f"成功更新了{count_update}个文件，添加了{count_add}个文件")

def arg_parser():
    arg = argparse.ArgumentParser(description="自动添加archlinux的包文件到库")
    arg.add_argument("-n", "--no-apply", action="store_true", help="不进行文件操作")
    arg.add_argument("-p", "--print", action="store_true", help="打印操作命令")
    ARGS.append(arg.parse_args())

if __name__ == "__main__":
    print("WARN 这是一个模块文件，直接执行没有任何功能")
