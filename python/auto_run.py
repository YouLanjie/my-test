#!/usr/bin/env python
# Created:2026.04.19

import sys
import time
import subprocess
from datetime import datetime
from dataclasses import dataclass
from pathlib import Path
import urllib.request
import urllib.error

SOURCE_URLS=[i+"post/Programs/actionslist.txt" for i in [
        "https://youlanjie.github.io/",
        "https//raw.githubusercontent.com/youlanjie/my-test/refs/heads/main/",
        ]]

EXE = Path(sys.argv[0]).resolve()
CWD = EXE.parent
CFGFILE = CWD/"actionslist.txt"

def read_text(file: Path) -> str:
    """读取文件并尝试解码"""
    if not file.is_file():
        return ""
    try:
        b = file.read_bytes()
    except PermissionError:
        return ""
    s = ""
    for i in ["utf8", "gbk", "utf32", "utf16"]:
        try:
            s = b.decode(i)
            break
        except UnicodeDecodeError:
            pass
    return "\n".join(s.splitlines())

def download_file(urls:list[str], savefile=None) -> bytes:
    req = urllib.request
    errors = urllib.error
    ret = None
    url = ""
    for url in urls:
        try:
            ret = req.urlopen(url)
            if ret.getcode() == 200:
                break
            ret = None
        except errors.HTTPError as e:
            print(f"[WARN] 链接不可用: {url} ({e})")
        except errors.URLError as e:
            print(f"[WARN] 域名无法访问: {url} ({e})")
    if not ret:
        print(f"[INFO] 下载失败 - '{url}'")
        return b""
    content = ret.read()
    if savefile:
        try:
            Path(savefile).write_bytes(content)
        except (OSError, PermissionError) as e:
            print(f"[ERROR] Saving '{savefile}': {e}")
    return content

@dataclass
class Event:
    time: float
    deps: list[str]
    cmd: str
    def run(self):
        print(f"[EXEC] {self.cmd}")
        try:
            subprocess.Popen(self.cmd, shell=True, cwd=CWD)
        except (FileNotFoundError,
                subprocess.CalledProcessError) as e:
            print(f"[Err] {e}")

def parse_cfg(s: str):
    ret : list[Event] = []
    lines = s.splitlines()
    for i in lines:
        opts = i.split("::")
        if len(opts) < 2:
            continue
        try:
            t = datetime.fromisoformat(opts[0])
        except ValueError as e:
            print(f"[Err] {e}")
            continue
        deps = []
        if len(opts) == 3:
            deps = opts[1].split(",")
            deps = [i for i in deps if i]
        ret.append(Event(t.timestamp(), deps, opts[-1]))
        print(ret[-1])
    return ret

def main():
    while download_file(SOURCE_URLS, CFGFILE) == b"" and not CFGFILE.is_file():
        print("[WARN] 等待60秒后重试")
        time.sleep(60)
    li = parse_cfg(read_text(CFGFILE))
    while True:
        last_t = time.time()
        time.sleep(1)
        for i in li:
            if last_t < i.time < time.time():
                i.run()

if __name__ == "__main__":
    if len(sys.argv) == 2 and Path(sys.argv[-1]).is_file():
        CFGFILE = Path(sys.argv[-1]).resolve()
        print(f"[INFO] 切换配置文件为 {CFGFILE}")
    try:
        main()
    except KeyboardInterrupt:
        print("[INFO] 退出")
