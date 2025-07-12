#!/usr/bin/env python
# Created:2025.05.09

# Dependences: curses(tui),
#              ffmpeg(translate),
#              mpv(play media)
#              python-biliass

import os
import sys
import subprocess
import curses
import argparse
from pathlib import Path
import json

class Video():
    """存储单个视频信息"""
    def __init__(self, file:Path):
        if file.is_file() is False:
            print(f"`{str(file)}`不是文件")
            sys.exit(1)
        self.timestamp = file.stat().st_mtime
        self.file = str(file)
        # 针对分p视频的单个视频
        self.dir_self = str(file.parents[0])
        # 针对多p视频的总目录
        self.dir_parent = str(file.parents[1])
        content = file.read_text()
        data = json.loads(content)
        self.avid = str(data.get("avid")) #"AV"+
        self.bvid = str(data.get("bvid")) #[]
        # 分P总标题/合集分标题
        self.maintitle = str(data.get("title"))
        self.owner = str(data.get("owner_name"))
        page_data = data.get("page_data")
        self.page = page_data.get("page")
        # 分P分标题/合集总标题
        self.part = str(page_data.get("part"))
        # 分P总+分标题
        self.subtitle = str(page_data.get("download_subtitle"))
        # ???
        self.indextitle = str(page_data.get("index_title"))
        self.index = page_data.get("index")
        if self.index is None:
            self.index = 0
        self.danmaku = Path(file.parent/"danmaku.xml")
        # set final title
        self.reset_title()
    def getlist(self):
        """返回视频的信息列表"""
        return [self.owner, self.title, "AV"+self.avid, self.bvid, self.dir_self]
    def reset_title(self, mode="auto"):
        """根据给出模式重新设置总标题"""
        if mode == "maintitle":
            self.title = self.maintitle
        elif mode == "part":
            self.title = self.part
        else:
            self.title = self.maintitle
            if self.part not in ("None", "", self.maintitle):
                self.title = f"{self.title} - {self.part}"
            if self.subtitle not in ("None", "", self.maintitle, f"{self.maintitle} {self.part}"):
                self.title = f"{self.title} - {self.subtitle}"

class Ui():
    """TUI,需要curses"""
    width=80
    position=0
    select=0
    collection = []
    def __init__(self, stdscr, content : list) -> None:
        if "curses" not in sys.modules:
            print("模块curses未导入，curses界面不可用")
            sys.exit(1)
        info_height = 9
        try:
            self.width=os.get_terminal_size().columns
            self.height=os.get_terminal_size().lines - info_height - 1
        except OSError:
            self.width = 80
            self.height = 24
        self.stdscr = stdscr
        curses.curs_set(0)
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLUE)
        curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_WHITE, curses.COLOR_BLACK)
        self.list = curses.newwin(self.height + 1, self.width, 0, 0)
        self.info = curses.newwin(info_height, self.width, self.height + 1, 0)
        self.content = content
        # self.height = len(self.content)
    def gsl(self, s, lim, start=0):
        """根据打印宽度截断字符串"""
        width = 0
        count = 0
        for c in s[start:]:
            if ord(c) <= 127:
                width += 1
                count += 1
            else:
                width += 2
                count += 1
            if lim > 0 and width+1 >= lim - 2:
                break
        return s[start:count]
    def print_list(self, content):
        """打印视频列表"""
        self.list.clear()
        for i in range(self.position, self.position+self.height):
            if i >= len(content):
                break
            p = content[i]
            width = self.width - 2
            color = 0
            if isinstance(p, list):
                title = self.gsl((p[0].title if len(p)==1 else p[0].maintitle)+" "*width, width)
                if len(p) != 1:
                    color+=curses.color_pair(2)
            else:
                title = self.gsl(p.title+" "*width, width)
            if i in self.collection:
                color+=curses.A_BOLD
            if i == self.select:
                color+=curses.A_UNDERLINE
                self.list.addstr(f"> {title}\n", color)
                continue
            if color:
                self.list.addstr(f"  {title}\n", color)
            else:
                self.list.addstr(f"  {title}\n")
            # print(p.getlist())
        self.list.refresh()
    def print_info(self, content):
        """打印光标处视频详细信息"""
        p = content[self.select]
        sets = True
        if isinstance(p, list):
            sets = len(p) == 1
            p = p[0]
        self.info.clear()
        self.info.addstr(f"BV:    {p.bvid:12} | AV: AV{p.avid}\n")
        self.info.addstr("DIR:   ")
        self.info.addstr(f"{p.dir_self if sets else p.dir_parent+" <SET>"}\n",
                         curses.color_pair(3 if sets else 2))
        self.info.addstr(f"OWNER: {p.owner}\n")
        self.info.addstr(f"FULL_TITLE: {p.title if sets else p.maintitle}")
        if p.indextitle != "None":
            self.info.addstr(
                f"\nINDEX: {p.index}\n"
                f"INDEX_TITLE: {p.indextitle}"
            )
        self.info.refresh()
    def check(self, content):
        """越界、上下滑动检查"""
        if self.select < 0:
            self.select = len(content)-1
        elif self.select >= len(content):
            self.select = 0
        while self.position + self.height <= self.select:
            self.position += 1
        while self.position > self.select:
            self.position -= 1
    def cmd(self, cmd:str):
        """暂时脱离curses环境执行命令"""
        curses.def_prog_mode()
        curses.endwin()
        os.system(cmd)
        curses.reset_prog_mode()
    def main(self):
        """ui主程序"""
        self.check(self.content)
        self.stdscr.refresh()
        inp = "0"
        deep = 0
        position = 0
        select = 0
        collection = []
        while inp != "q":
            cont = [self.content, self.content[select]][deep]
            obj = [cont[self.select]] \
                    if len(self.collection) == 0 else \
                    [cont[i] for i in self.collection]
            self.print_list(cont)
            self.print_info(cont)
            inp = chr(self.stdscr.getch())
            if inp == "j":
                self.select+=1
            elif inp == "k":
                self.select-=1
            elif inp == "g":
                self.select=0
            elif inp == "G":
                self.select=-1
            elif inp == "l" and len(self.content[self.select]) != 1 and deep == 0:
                position,select,self.position,self.select = self.position, self.select, 0, 0
                collection, self.collection = self.collection, []
                deep = 1
            elif inp == "h" and deep == 1:
                self.position,self.select = position,select
                self.collection = collection
                deep = 0
            elif inp == "r":
                self.select = len(cont) - self.select - 1
                cont.reverse()
            elif inp == "p":
                self.cmd(cmd_genal(obj, "play"))
            elif inp == "P":
                self.cmd(cmd_genal(obj, "play_mp4"))
            elif inp == "c":
                self.cmd(cmd_genal(obj, "copy"))
            elif inp == "e":
                self.cmd(cmd_genal(obj, "mp3"))
            elif inp == "E":
                self.cmd(cmd_genal(obj, "mp4"))
            elif inp == "d":
                self.cmd(cmd_genal(obj, "mp4"))
            elif inp == " ":
                if self.select in self.collection:
                    self.collection.remove(self.select)
                else:
                    self.collection.append(self.select)
                self.select+=1
            elif inp == "A":
                self.collection = []
            elif inp == "a":
                self.collection = list(range(len(cont)))
            elif inp in ("1", "2", "3"):
                if not isinstance(obj[0], list):
                    obj = [obj]
                title_mode = {"1":"maintitle", "2":"part", "3":"auto"}
                for i in obj:
                    for j in i:
                        j.reset_title(title_mode[inp])
            elif inp == "4":
                CONFIG["vfat_name"] = not CONFIG["vfat_name"]
            self.check([self.content, self.content[select]][deep])
        # curses.endwin()

def get_video_dimensions(file_path:Path) -> tuple[int, int]:
    cmd = ["ffprobe", "-v", "error", "-select_streams", "v:0",
           "-show_entries", "stream=width,height", "-of", "json", str(file_path)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    data = json.loads(result.stdout)
    width = data["streams"][0]["width"]
    height = data["streams"][0]["height"]
    return width, height

def cmd_genal(li, mode) -> str:
    """生成shell命令用于保存或执行"""
    has_subprocess = subprocess.run("which biliass",check=False,
            shell=True ,capture_output=True).returncode == 0
    t = ""
    if not isinstance(li[0], list):
        li = [li]
    t+=f"echo \"Running mode:{mode}\"\n"
    for i in li:
        if len(i) != 1:
            t+="# =========================\n"
            t+=f"# SETS: {i[0].maintitle} <AV{i[0].avid}>\n"
        for j in i:
            vid = list(Path(j.dir_self).glob("**/video.m4s"))
            aud = list(Path(j.dir_self).glob("**/audio.m4s"))
            if len(vid) == 0 or len(aud) == 0:
                t+=f"# {j.dir_self}:no media file({j.title})"
                continue
            danmaku : Path = j.danmaku
            vid = vid[0]
            aud = aud[0]
            # 替换vfat系统不允许出现的字符（包括部分正常文件系统特殊字符）
            output = j.title
            replacement = ["/", "／"]
            if CONFIG["vfat_name"]:
                replacement = ["\\/:*?\"<>", "＼／∶＊？＂〈〉｜"]
            for k,l in zip(list(replacement[0]), list(replacement[1])):
                output=output.replace(k, l)
            output = CONFIG["outputd"]+output
            if output[0] == "-":
                output = "./"+output
            base_output = output
            t+=f"# {j.title if len(i) == 1 else j.part} <Dir: {j.dir_self}>\n"
            if mode == "3gp":
                t+=f"ffmpeg -i \"{str(vid)}\" -i \"{str(aud)}\""\
                    +" -r 12 -b:v 400k -s 352x288 -ab 12.2k -ac 1 -ar 8000 -c copy "
                output+=".3gp"
            elif mode == "mp4":
                t+=f"ffmpeg -i \"{str(vid)}\" -i \"{str(aud)}\" -c copy "
                output+=".mp4"
            elif mode in ("m4a" "copy"):
                t+=f"cp \"{str(aud)}\" "
                output+=".m4a"
            elif mode == "play":
                t+=f"echo \"Now Playing:{j.title}\"\n{CONFIG["player"]} \"{str(aud)}\"\n"
                continue
            elif mode == "play_mp4":
                t+=f"echo \"Now Playing:{j.title}\"\n{CONFIG["player"]} \"{str(vid)}\" &\n"+\
                        f"{CONFIG["player"]} \"{str(aud)}\"\n"
                continue
            else:
                t+=f"ffmpeg -i \"{str(aud)}\" "
                output+=".mp3"
            t+=f"\"{output}\"\n"
            if not CONFIG["noass"] and has_subprocess and mode not in ("play", "play_mp4"):
                width, height = get_video_dimensions(vid)
                t+=f"""biliass "{danmaku}" -s {width}x{height} -o "{base_output}.ass"\n"""
        if len(i) != 1:
            t+=f"# END OF SETS: {i[0].maintitle} <AV{i[0].avid}>\n"
            t+="# =========================\n\n"
    t+="echo \"Done!\\n\"\n"
    return t

def run_main():
    """运行主程序(需要)"""
    list_mode = False
    inputd = ["/sdcard/Android/data/tv.danmaku.bili/download/",
              "/sdcard/Android/data/com.a10miaomiao.bilimiao/download/",
              "./"]
    parser = argparse.ArgumentParser(description="导出B站手机端缓存")
    parser.add_argument('-l', '--list', action="store_true", help='列出视频')
    parser.add_argument('-n', '--no-vfat-name', action="store_false", help='不替换特殊字符')
    parser.add_argument('-N', '--no-ass', action="store_true", help='不生成弹幕(加快脚本生成速度)')
    parser.add_argument('-i', '--input-dir', action="append", help='设置输入文件夹')
    parser.add_argument('-o', '--output-dir', default="", help='设置输出文件夹')
    parser.add_argument('-O', '--output-file', default=None, help='设置输出文件')
    parser.add_argument('-p', '--player', default="mpv", help='设置默认播放器')
    parser.add_argument('-m', '--mode', default="mp3", help='设置脚本输出类型(mp3(默认)|mp4|m4a(仅复制)|3gp|play)')
    parser.add_argument('-H', '--help-key', action="store_true", help='内部按键帮助')
    args = parser.parse_args()
    if args.help_key:
        mhelp()
    list_mode = args.list
    mode = args.mode
    CONFIG["outputd"] = args.output_dir
    outputf = args.output_file
    CONFIG["vfat_name"] = args.no_vfat_name
    CONFIG["player"] = args.player
    CONFIG["noass"] = args.no_ass
    if args.input_dir:
        inputd=args.input_dir

    input_f = []
    for i in (list(Path(i).glob("**/entry.json")) for i in inputd):
        input_f += i
    if len(input_f) == 0:
        mhelp(msg="[!] no input file was found")
    print(f"{len(input_f)} files were found")
    li = []
    for i in enumerate(input_f):
        v = Video(input_f[i[0]])
        if len(li) > 0 and li[-1][0].dir_parent == v.dir_parent:
            li[-1].append(v)
        else:
            li.append([v])
        li[-1] = sorted(li[-1], key=lambda x:x.page, reverse=False)
    li = sorted(li, key=lambda x:x[0].timestamp, reverse=False)

    if list_mode:
        for i in li:
            for j in i:
                print(j.getlist(), end=" ")
            print()
        sys.exit(0)
    if outputf is not None:
        Path(outputf).write_text(cmd_genal(li, mode), encoding="utf8")
        sys.exit(0)
    return li

def mhelp(ret=0, msg=""):
    """显示帮助信息"""
    if msg != "":
        print(str(msg))
    print("[*] 请确保`entry.json`文件存在")
    print("[*] 在TUI中`jk`上下移动 `l`查看多p视频 `h`返回")
    print("    `gG`快速跳转到列表头部或尾部 `r`反转列表排序")
    print("    ` `选中或取消选中光标项 `aA`全选或取消全选")
    print("    `e`导出为mp3 `E`导出为mp4 `c`复制出m4a文件")
    print("    `p`播放音频文件 `123`切换总标题命名格式")
    print("    `4`切换是否替换输出文件名特殊字符")
    sys.exit(ret)

CONFIG = {"player":"mpv", "outputd":"", "vfat_name":True,
          "noass":False}
if __name__ == "__main__":
    video_list = run_main()
    video_list.reverse()
    curses.wrapper(lambda stdscr: Ui(stdscr, video_list).main())

