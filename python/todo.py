#!/usr/bin/env python
# 简陋的todo程序

from pathlib import Path
import json
import argparse
import time

def get_args() -> argparse.Namespace:
    """获取参数"""
    parser = argparse.ArgumentParser(description="简陋的todo程序")
    parser.add_argument("-d", "--delete", type=int, default=[], action="append", help="删除任务")
    parser.add_argument("-a", "--add", default=[], action="append", help="追加任务")
    parser.add_argument("-p", "--print", action="store_true", help="显示所有任务")
    return parser.parse_args()

def main():
    """主函数"""
    args = get_args()
    todof = Path.home()/".TODO.json"
    if todof.exists() and not todof.is_file():
        return
    text = "[]"
    if todof.is_file():
        text = todof.read_text(encoding="utf8")
        if text == "":
            text = "[]"
    data = json.loads(text)
    if not isinstance(data, list):
        return
    del_list = []
    for l in args.delete:
        if l < 0 or l >= len(data):
            continue
        if not Path().resolve().is_relative_to(Path(data[l]["path"]).resolve()):
            continue
        del_list.append(data[l])
    for l in del_list:
        data.remove(l)
    for l in args.add:
        data.append({"time":time.time(), "path":str(Path().resolve()), "event":l})
    if args.print:
        __import__("rich").print(data)
    else:
        for ind,item in enumerate(data):
            if not Path().resolve().is_relative_to(Path(item["path"]).resolve()):
                continue
            print(f"{ind:4}  {item["event"]}")
    todof.write_text(json.dumps(data, ensure_ascii=False), encoding="utf8")

if __name__ == "__main__":
    main()

