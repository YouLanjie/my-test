#!/usr/bin/env python
# Created:2026.01.24

from pathlib import Path
import sys
import time
import threading
import argparse
import tempfile
import subprocess
from typing import List, Set

class Protect:
    def __init__(self) -> None:
        self.flag = True
        self.workdir = Path(sys.argv[0]).parent
        self.filelist = set()

        self.read_cfg()
        print("Filelist:")
        __import__('pprint').pprint(self.filelist)

        self.threadlist : List[threading.Thread] = []
        for i in self.filelist:
            pth = threading.Thread(
                    target=self.protect_file,
                    args=[i])
            self.threadlist.append(pth)
            pth.start()
    def read_cfg(self):
        args = parse_arguments(self.workdir)
        cfg = Path(args.input)
        filelist : Set[Path] = set()
        if cfg.is_file():
            print(f"Config: {cfg}")
            filelist |= set([cfg.resolve()])
            try:
                cont = cfg.read_text("utf8")
            except UnicodeDecodeError:
                cont = cfg.read_text("gbk")
            filelist |= {(self.workdir/i).resolve() for i in cont.splitlines() \
                    if (self.workdir/i).exists()}
        self.exe = Path(sys.argv[0]).resolve()
        newfl = set()
        for i in filelist:
            if i.is_dir():
                print(f"replace: {i}")
                newfl |= {j for j in i.glob("**/*") if j.is_file()}
            else:
                newfl.add(i)
        filelist = newfl
        filelist.add(self.exe)
        self.filelist = filelist
    def protect_file(self, file:Path):
        if not file.is_file():
            return
        parent = file.parent
        content = file.read_bytes()
        stat = file.stat()
        while self.flag:
            if file.is_file() and file.stat() == stat:
                time.sleep(1)
                continue
            if not parent.is_dir():
                parent.mkdir(parents=True,exist_ok=True)
            file.write_bytes(content)
            file.chmod(stat.st_mode)
            stat = file.stat()
            print(f"RECOVER '{file}'")
    def quit(self):
        self.flag = False
        for i in self.threadlist:
            i.join()

class Lock:
    inp_cfg = """MBTN_LEFT ignore
MBTN_LEFT_DBL set osc yes
MBTN_RIGHT ignore
WHEEL_UP ignore
WHEEL_DOWN ignore
MOUSE_MOVE ignore"""
    def __init__(self) -> None:
        self.avaliable = False
        try:
            subprocess.run(["mpv","-h"], check=True)
            self.avaliable = True
        except (FileNotFoundError, subprocess.CalledProcessError) as e:
            print(f"[WARN] {e}")
    def run(self, file:Path):
        if not self.avaliable:
            return
        if not file.is_file():
            return
        # subprocess.run(["nircmdc.exe", "mutesysvolume", "0"])
        # subprocess.run(["nircmdc.exe", "setsysvolume", "28000"])
        subprocess.run(["mpv", file, "--fs", "--no-osc", f"--input-conf={file}", "--input-doubleclick-time=50"], check=False)


def parse_arguments(workdir:Path) -> argparse.Namespace:
    """解释参数"""
    parser = argparse.ArgumentParser(description='python')
    parser.add_argument('-i', '--input', type=Path, default=workdir/"filelist.txt", help='config文件')
    args = parser.parse_args()
    return args

def main():
    protect = Protect()
    try:
        while True:
            time.sleep(10)
    except KeyboardInterrupt:
        print("quit now...")
        protect.quit()
        return
    protect.quit()

if __name__ == "__main__":
    main()

