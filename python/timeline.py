#!/usr/bin/env python
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
import argcomplete
from pytools import print_err

class Events:
    """多个事件类"""
    def __init__(self, node:tuple, length:int) -> None:
        "timestamp:int|float|str|None, events:list[str]"
        timestamp = node[0]
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
        self.events : list[str] = [node[1][i] if i < len(node[1]) else "" for i in range(length)]
    def sort(self, convertable:list[int]):
        """排序"""
        if len(convertable) != len(self.events):
            print_err(f"ERROR 排序错误 - {len(convertable)} to {len(self.events)} {self.events}")
            return
        self.events = [self.events[i] for i in convertable]
    def get_filtered(self, head:list[int]) -> list[str]:
        """获取过滤后的列"""
        return [self.events[j] for j in head]
    def get_line(self, head:list[int], link:str="|") -> str:
        """获取单行文本输出"""
        return link.join(self.get_filtered(head))
    def get_str_time(self, time_format:str) -> str:
        """获取格式化的时间"""
        if not time_format:
            time_str = time.strftime("%Y-%m-%d", time.localtime(self.timestamp))
            time_str += " " + "一二三四五六日"[int(
                time.strftime("%w", time.localtime(self.timestamp)) )]
            time_str += " " + time.strftime("%H:%M:%S", time.localtime(self.timestamp))
        elif time_format == "timestamp":
            time_str = str(self.timestamp)
        else:
            time_str = time.strftime(time_format, time.localtime(self.timestamp))
        return time_str

class Timeline:
    """时间轴类"""
    def __init__(self, timeline:dict) -> None:
        head = timeline.get("head")
        if not isinstance(head, list):
            head = []
        self.head : list[list[str]] = head
        content = timeline.get("content")
        if not isinstance(content, dict):
            content = {}
        self.content : list[Events] = [Events(i, len(head)) for i in content.items()]
    def get_head_list(self, flit_list:list[int]|None = None) -> list[str]:
        """获取表头列表（已文本化）"""
        if not flit_list:
            return ["/".join(i) for i in self.head]
        return ["/".join(self.head[i]) for i in flit_list]
    def sort(self, no_sort:str = ""):
        """排序(横列和纵列)"""
        if "y" not in no_sort:
            self.content = sorted(self.content, key=lambda x:x.timestamp)
        if "x" in no_sort:
            return
        result = sorted(enumerate(self.head), key=lambda x:x[1])
        convertable = [i[0] for i in result]
        self.head = [i[1] for i in result]
        for i in self.content:
            i.sort(convertable)
    def merge(self, timeline):
        """合并其他时间轴"""
        if not isinstance(timeline, Timeline):
            return
        repeated = set(self.get_head_list()) & set(timeline.get_head_list())
        if repeated:
            print_err(f"WARN 重复的的表头 - {repeated}")
            return
        width = (len(self.head), len(timeline.head))
        for i in timeline.content:
            i.events = [""*i for i in range(width[0])]+i.events
        for i in self.content:
            i.events += [""*i for i in range(width[1])]
        merged = self.content+timeline.content
        result = sorted(merged, key=lambda x:x.timestamp)
        if not result:
            return
        self.head += timeline.head
        lastnode = result[0]
        remove_list = []
        for item in result[1:]:
            if item.timestamp == lastnode.timestamp:
                lastnode.events = [i or j for i,j in zip(lastnode.events,item.events)]
                remove_list.append(item)
                continue
            lastnode = item
        for i in remove_list:
            merged.remove(i)
        self.content = merged
        self.sort()
    def dump(self, match:re.Pattern|None) -> dict:
        """导出为json可保存格式"""
        if not match:
            match = re.compile("")
        head = [ind for ind,item in enumerate(self.head) if match.match("/".join(item))]
        return {"head":[self.head[j] for j in head],
                "content":{i.timestamp:i.get_filtered(head) for i in self.content}}
    def to_text(self, match:re.Pattern|None, time_format="", mode:str="text"):
        """转成文本(根据不同模式)"""
        if match:
            head = [ind for ind,item in enumerate(self.head) if match.match("/".join(item))]
        else:
            head = [i[0] for i in enumerate(self.head)]

        ret = ""
        if mode == "org":
            ret = "|Time|" + "|".join(self.get_head_list(head)) + "|\n|-|\n"
            for i in self.content:
                ret += f"|{i.get_str_time(time_format)}|"
                ret += i.get_line(head)
                ret += "|\n"
        elif mode == "csv":
            ret = "Time," + ",".join(self.get_head_list(head)) + "\n"
            for i in self.content:
                ret += f"{i.get_str_time(time_format)},"
                ret += i.get_line(head, ",")
                ret += ",\n"
        return ret

def csv_to_json(name:str, inp:list[str]) -> str:
    """将csv转成可用json"""
    ret = {}
    lastdate = 0.0
    laststrdate = ""
    tablehead = []
    spc = ";"
    if not inp:
        return ""
    li = [(len(inp[0].split(i)), i) for i in ",;:`"]
    spc = max(li)[1]
    tablehead = inp[0].split(spc)
    if len(tablehead) < 2:
        return ""
    names = name.split("/") if name.split("/")[0] else []
    tablehead = [names+[f"{ind+1:2}{item}"] for ind,item in enumerate(tablehead[1:])]
    pattern = re.compile(r".*[\[\(]([0-9:]*)[\]\)]")
    for i in inp[1:]:
        cols = i.split(spc)
        date = cols[0]
        possible_time = [pattern.match(j) \
                for j in cols[1:] if pattern.match(j)]
        if possible_time:
            possible_time = f" {possible_time[0].group(1)}" if possible_time[0] else ""
        else:
            possible_time = ""
        if date or possible_time:
            if not date:
                date = laststrdate
            try:
                timearry=time.localtime(
                        float(dateutil.parser.parse(
                            date+possible_time).strftime("%s")))
                laststrdate = date
                lastdate = float(time.mktime(timearry))
            except (ValueError, dateutil.parser.ParserError):
                lastdate = 0.0
        else:
            lastdate += 0.01
        while ret.get(lastdate):
            lastdate += 0.0001
        ret[lastdate] = cols[1:]
    ret = {"head":tablehead, "content":ret}
    return json.dumps(ret, ensure_ascii=False)

class Interactive:
    def __init__(self, tl:Timeline) -> None:
        self.tl = tl
        self.command_list = {
                "help":{"help":"显示帮助","func":self._print_help},
                "exit":{"help":"退出"},
                "quit":{"help":"退出"},
                "print":{"help":"打印值(x y)","func":self._print_value},
                "p":{"func":self._print_value},
                "list":{"help":"列出可用表头","func":self._list_head},
                "l":{"func":self._list_head},
                "q":{},
                }
        "命令列表"
    def _print_help(self):
        """打印帮助"""
        for key,value in self.command_list.items():
            if not value.get("help"):
                continue
            print(f"  {key:<20} {value.get("help")}")
    def _list_head(self):
        print("X:")
        for i in enumerate(self.tl.get_head_list()):
            print(f"{i[0]:<3} :{i[1]}")
    def _print_value(self):
        if len(self.cmd) == 1:
            print("! 全表:")
            print(orgreader2.Document(self.tl.to_text(None, mode="org").splitlines()).to_text())
            return
        if len(self.cmd) != 3:
            print("! 参数数量错误")
            return
        if not self.tl.content or not self.tl.content[0].events:
            print("! 表为空")
            return
        try:
            x = int(self.cmd[1])
            y = int(self.cmd[2])
        except ValueError:
            print("! 参数不为数字")
            return
        max_x = len(self.tl.content[0].events)
        max_y = len(self.tl.content)
        if x >= max_x or y >= max_y:
            print("! 超出索引范围")
            return
        print(self.tl.content[y].events[x])
    def run(self):
        """交互界面"""
        print("> 交互界面(`help` for help)")
        print("> 目前为测试阶段，不可用")
        print("> 输入`exit`回车退出")
        inp = ""
        while inp not in ("quit", "q", "exit"):
            inp = input(">>> ")
            if inp in self.command_list:
                func = self.command_list[inp].get("func")
                if func and type(func).__name__ in ("method", "function"):
                    func()
            else:
                print(f"Command not found '{inp}'")

def main():
    """main function"""
    parser = argparse.ArgumentParser(description="时间轴")
    parser.add_argument("-i","--input", action="append",default=[],
                        help="输入文件(json,可用gzip压缩文件)")
    parser.add_argument("-o","--output", default="", help="输出到文件")
    parser.add_argument("-m","--mode", default="text",choices=["text","python","json","org","csv"],
                        help="输出格式")
    parser.add_argument("-z","--gzip", action="store_true", help="使用gzip压缩结果")
    parser.add_argument("-f","--filter", default="", help="匹配规则(正则表达式)")
    parser.add_argument("-l","--list", action="store_true", help="列出可用表头")
    parser.add_argument("--time-format", default="", help="时间格式(通用格式化字符串|timestamp)")
    parser.add_argument("--csv-input", default="", help="输入csv文件")
    parser.add_argument("--csv-name", default="", help="csv文件共用父类名")
    parser.add_argument("--no-sort", default="", nargs="?", const="xy", help="不进行排序(x/y)")
    parser.add_argument("--interactive", action="store_true", help="交互式(仅测试)")
    argcomplete.autocomplete(parser)
    args = parser.parse_args()

    timelines = []
    if args.csv_input and Path(args.csv_input).is_file():
        inp = json.loads(
                csv_to_json(args.csv_name,
                            Path(args.csv_input).read_text(encoding="utf8").splitlines()))
        args.input = []
        timelines.append(Timeline(inp))
    if not args.input:
        inp = {1752468032:["event","nothing 1", "path","a","b"],
               1752368032:["event","这玩意只是个示范哦", "path","a","c",
                           "event","nothing 2 but same time", "path","a","d"],
               1751468032:["event","nothing 4", "path","a","b"],
               1752496976:["event","现在的事情 呵呵"],
               "1949.10.1":["event","是国庆测试哦"],
               1752568032:["event","nothing 3", "path","a","b"]}
        inp = {"head":[["c"], ["c","1"],["c","2"], ["b"], ["a"],
                       ["w"], ["w","d"], ["n"], ["m"], ["d"], ["多余的"]],
               "content":inp}
        timelines.append(Timeline(inp))
    for input_f in args.input:
        if not Path(input_f).is_file():
            print_err(f"WARN 文件不存在: {input_f}")
            continue
        inp = Path(input_f).read_bytes()
        if subprocess.run(["file", "-b", input_f], capture_output=True, check=False,
                          text=True).stdout.startswith("gzip compressed data"):
            try:
                inp = gzip.decompress(inp).decode()
            except gzip.BadGzipFile:
                print_err("ERROR 解压gzip文件时发生错误")
                continue
        else:
            inp = inp.decode()
        try:
            inp = json.loads(inp)
        except json.JSONDecodeError:
            print_err("ERROR 读取JSON文件时发生错误")
            continue
        timelines.append(Timeline(inp))
    if not timelines:
        return
    timeline : Timeline = timelines[0]
    timeline.sort(args.no_sort)
    for i in timelines[1:]:
        i.sort(args.no_sort)
        timeline.merge(i)

    if args.interactive:
        Interactive(timeline).run()
        return

    if args.list:
        output = "\n".join(["/".join(i) for i in sorted(list(timeline.head))])
        print(output)
        return

    if args.mode == "python":
        output = str(timeline.dump(re.compile(args.filter)))
    elif args.mode == "json":
        output = json.dumps(timeline.dump(re.compile(args.filter)), ensure_ascii=False)
    elif args.mode == "org":
        output = timeline.to_text(re.compile(args.filter), args.time_format, "org")
    elif args.mode == "csv":
        output = timeline.to_text(re.compile(args.filter), args.time_format, "csv")
    else:
        doc = orgreader2.Document(
                timeline.to_text(
                    re.compile(args.filter), args.time_format, "org"
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
