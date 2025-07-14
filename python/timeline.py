#!/usr/bin/python
"""制作时间轴"""

# 基本要求：
# 需要存储事件以及对应的时间，能够对事件进行分类（分类可以拥有子分类）
# 能够存储和读取存档文件
# 最终目标：
# 构建时间轴，能够以时间为序，体现等比时间关系（用空行或者生成图片）
# 支持范围事件
# 能够依照给出类型（事件分类）并列显示（同一条时间线多条时间轴进行对比）（分类查找）
# 能够导出易读的终端文本输出结果
# 能够导出成能够打印缩印的文件（docx或者latex等）
# 能够整合多个时间轴（文件之类的）并合并到同一条时间线上

from pathlib import Path
import argparse
import time
import re
import json
import subprocess
import gzip
import dateutil.parser
import orgreader2
from pytools import print_err

class Event:
    """单个事件类"""
    def __init__(self, event:dict) -> None:
        path = event.get("path")
        if path is None:
            self.path : list[str] = []
        else:
            self.path : list[str] = path if isinstance(path, list) else [path]
        self.event : str = event.get("event") or ""
    def get_path(self) -> str:
        """path的缝合文本"""
        return "/".join(self.path)
    def dump(self) -> dict:
        """导出为json可保存格式"""
        return {"event":self.event, "path":self.path}

class Events:
    """多个事件类"""
    def __init__(self, timestamp:int|float|str|None, events:list[dict[str, str]]) -> None:
        if isinstance(timestamp, str):
            try:
                timestamp = float(timestamp)
            except ValueError:
                try:
                    timearry=time.localtime(float(dateutil.parser.parse(timestamp).strftime("%s")))
                    timestamp = float(time.mktime(timearry))
                except (ValueError, dateutil.parser.ParserError):
                    timestamp = 0
        self.timestamp : float | int = timestamp or 0
        self.events : list[Event] = [Event(i) for i in events] or []
    def get_all_path(self) -> set[str]:
        """获取所有事件类型"""
        paths = set()
        for i in self.events:
            paths.add(i.get_path())
        return paths
    def dump(self) -> list:
        """导出为json可保存格式"""
        return [i.dump() for i in self.events]

class Timeline:
    """时间轴类"""
    def __init__(self, timeline:dict[int, list[dict[str,str]]]) -> None:
        self.time_nodes : list[Events] = [Events(i[0], i[1]) for i in timeline.items()]
        self.time_nodes = sorted(self.time_nodes, key=lambda x:x.timestamp)
    def get_all_path(self) -> set[str]:
        """获取所有事件类型"""
        paths = set()
        for i in self.time_nodes:
            paths |= i.get_all_path()
        return paths
    def dump(self) -> dict:
        """导出为json可保存格式"""
        return {i.timestamp:i.dump() for i in self.time_nodes}
    def to_text(self, match:re.Pattern, simple_time=False) -> str:
        """转成文本"""
        all_paths = set(filter(match.match, self.get_all_path()))
        ret = "|Time|" + "|".join(sorted(list(all_paths))) + "|\n|-|\n"
        for i in self.time_nodes:
            time_str = time.strftime("%Y-%m-%d", time.localtime(i.timestamp))
            if not simple_time:
                time_str += " " + "一二三四五六日"[int(
                    time.strftime("%w", time.localtime(i.timestamp)) )]
                time_str += " " + time.strftime("%H:%M:%S", time.localtime(i.timestamp))
            cols = {j:"" for j in all_paths}
            ret += f"|{time_str}"
            for j in i.events:
                if j.get_path() not in cols:
                    continue
                cols[j.get_path()] = j.event
            for j in sorted(list(cols.items()), key=lambda x:x[0]):
                ret += f"|{j[1]}"
            ret += "|\n"
        return ret

def csv_to_json(name:str, inp:list[str]) -> str:
    """将csv转成可用json"""
    ret = {}
    lastdate = 0.0
    tablehead = []
    spc = ";"
    if not inp:
        return ""
    li = [(len(inp[0].split(i)), i) for i in ",;:`"]
    spc = max(li)[1]
    tablehead = inp[0].split(spc)
    if len(tablehead) < 2:
        return ""
    tablehead = [f"{index+1:2}{item}" for index,item in enumerate(tablehead[1:])]
    for i in inp[1:]:
        cols = i.split(spc)
        if cols[0]:
            try:
                timearry=time.localtime(float(dateutil.parser.parse(cols[0]).strftime("%s")))
                lastdate = float(time.mktime(timearry))
            except (ValueError, dateutil.parser.ParserError):
                lastdate = 0.0
        else:
            lastdate += 0.01
        nodes = [{"event":j,"path":[name,k]} for j,k in zip(
            cols[1:],
            tablehead
            )]
        while ret.get(lastdate):
            lastdate += 0.001
        ret[lastdate] = nodes
    return json.dumps(ret, ensure_ascii=False)

def main():
    """main function"""
    parser = argparse.ArgumentParser(description="时间轴")
    parser.add_argument("-i","--input", default="", help="输入文件(json,可用gzip压缩文件)")
    parser.add_argument("-I","--csv-input", default="", help="输入csv文件")
    parser.add_argument("--csv-name", default="", help="csv文件共用父类名")
    parser.add_argument("-o","--output", default="", help="输出到文件")
    parser.add_argument("-m","--mode", default="text", help="输出格式(text,python,json)")
    parser.add_argument("-z","--gzip", action="store_true", help="使用gzip压缩结果")
    parser.add_argument("-s","--simple-time", action="store_true", help="输出更简单的日期")
    parser.add_argument("-f","--filter", default="", help="匹配规则(正则表达式)")
    parser.add_argument("-l","--list", action="store_true", help="列出可用表头")
    args = parser.parse_args()
    if args.input and Path(args.input).is_file():
        inp = Path(args.input).read_bytes()
        if subprocess.run(["file", "-b", args.input], capture_output=True, check=False,
                          text=True).stdout.startswith("gzip compressed data"):
            try:
                inp = gzip.decompress(inp).decode()
            except gzip.BadGzipFile as e:
                print_err("ERROR 解压gzip文件时发生错误")
                raise e
        else:
            inp = inp.decode()
        try:
            inp = json.loads(inp)
        except json.JSONDecodeError as e:
            print_err("ERROR 读取JSON文件时发生错误")
            raise e
    else:
        inp = {1752468032:[{"event":"nothing 1", "path":["a","b"]}],
               1752368032:[{"event":"这玩意只是个示范哦", "path":["a","c"]},
                           {"event":"nothing 2 but same time", "path":["a","d"]}],
               1751468032:[{"event":"nothing 4", "path":["a","b"]}],
               1752496976:[{"event":"现在的事情 呵呵"}],
               "1949.10.1":[{"event":"是国庆测试哦"}],
               1752568032:[{"event":"nothing 3", "path":["a","b"]}]}
    if args.csv_input and Path(args.csv_input).is_file():
        inp = json.loads(
                csv_to_json(args.csv_name,
                            Path(args.csv_input).read_text(encoding="utf8").splitlines()))
    timeline = Timeline(inp)

    if args.list:
        output = str(sorted(list(timeline.get_all_path())))
        print(output)
        return

    if args.mode == "python":
        output = str(timeline.dump())
    elif args.mode == "json":
        output = json.dumps(timeline.dump(), ensure_ascii=False)
    else:
        doc = orgreader2.Document(
                timeline.to_text(
                    re.compile(args.filter), args.simple_time
                    ).splitlines())
        output = doc.to_text()

    outf = Path(args.output)
    if args.output and not outf.is_dir():
        output = output.encode()
        if args.gzip:
            output = gzip.compress(output)
        outf.write_bytes(output)
        return
    print(output)


if __name__ == "__main__":
    main()
