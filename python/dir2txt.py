#!/usr/bin/env python
# Created:2026.02.06

from pathlib import Path
import argparse
import hashlib
import pytools

def decode(b: bytes) -> str:
    """读取文件并尝试解码"""
    s = ""
    for i in ["utf8", "gbk", "utf32", "utf16"]:
        try:
            s = b.decode(i)
            break
        except UnicodeDecodeError:
            pass
    return s

def get_filelist(fl: set[Path]) -> set[Path]:
    nfl = set()
    for i in fl:
        if i.is_file():
            nfl.add(i)
            continue
        if i.name in [".git"]:
            continue
        nfl |= get_filelist(set(i.iterdir()))
    return nfl

def main():
    parser = argparse.ArgumentParser(description="将目录聚合成一个文本文件")
    parser.add_argument('dir', action="append", help='文件,文件夹')
    args = parser.parse_args()

    fl = [Path(i) for i in args.dir]
    fl = get_filelist({i.absolute() for i in fl if i.exists()})

    ret = ""
    for i in fl:
        b = i.read_bytes()
        s = decode(b)
        ret += f"+++ {pytools.calculate_relative(i, Path())}\n"
        ret += f"@@@ st_mtime: {i.stat().st_mtime}\n"
        if i.is_symlink():
            ret += "@@@ Symbol Link\n"
            ret += f"@@@ link_to: {pytools.calculate_relative(i.resolve(), Path())}"
        elif b and not s:
            s = hashlib.sha256(b).hexdigest()
            ret += "@@@ Binary File\n"
            ret += f"@@@ sha256: {s}"
        else:
            ret += "\n".join(["+"+i for i in s.splitlines()])
        ret += "\n"
    print(ret)
    return ret

if __name__ == "__main__":
    main()
