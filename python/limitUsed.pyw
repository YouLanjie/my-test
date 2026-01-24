#!/usr/bin/env python
# Created:2026.01.24

from pathlib import Path
import sys
import time
import threading
import argparse
from typing import List, Set

FLAG = [True]

def protect_file(file:Path):
    if not file.is_file():
        return
    content = file.read_bytes()
    stat = file.stat()
    while FLAG[0]:
        if file.is_file() and file.stat() == stat:
            time.sleep(1)
            continue
        file.write_bytes(content)
        file.chmod(stat.st_mode)
        stat = file.stat()
        print(f"RECOVER '{file}'")

def parse_arguments(workdir:Path) -> argparse.Namespace:
    """解释参数"""
    parser = argparse.ArgumentParser(description='python')
    parser.add_argument('-i', '--input', type=Path, default=workdir/"filelist.txt", help='config文件')
    args = parser.parse_args()
    return args

def main():
    workdir = Path(sys.argv[0]).parent
    args = parse_arguments(workdir)
    cfg = Path(args.input)
    filelist : Set[Path] = set()
    if cfg.is_file():
        print(f"Config: {cfg}")
        filelist |= set([cfg.resolve()])
        try:
            filelist |= {(workdir/i).resolve() for i in cfg.read_text("utf8").splitlines() if (workdir/i).is_file()}
        except UnicodeDecodeError:
            filelist |= {(workdir/i).resolve() for i in cfg.read_text("gbk").splitlines() if (workdir/i).is_file()}
    print("Filelist:")
    __import__('pprint').pprint(filelist)
    self = Path(sys.argv[0]).resolve()
    filelist -= set([self])
    threadlist : List[threading.Thread] = []
    for i in filelist:
        pth = threading.Thread(target=protect_file, args=[i])
        threadlist.append(pth)
        pth.start()
    try:
        protect_file(self)
    except KeyboardInterrupt:
        print("quit now...")
        FLAG[0] = False
        for i in threadlist:
            i.join()
        return

if __name__ == "__main__":
    main()

