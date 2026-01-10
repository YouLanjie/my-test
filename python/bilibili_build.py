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
import shlex
import re
from functools import lru_cache

try:
    import readline
    del readline
except ModuleNotFoundError:
    pass

try:
    from pypinyin import lazy_pinyin
except ModuleNotFoundError:
    lazy_pinyin = None

@lru_cache(maxsize=1024)
def get_pinyin(s:str) -> str:
    if lazy_pinyin:
        return s + " " + "".join(lazy_pinyin(s))
    else:
        return s

import pytools

userlist = {}

class Video():
    """存储单个视频信息"""
    def __init__(self, file:Path):
        if file.is_file() is False:
            print(f"`{str(file)}`不是文件")
            sys.exit(1)
        self.file = str(file)
        # 针对分p视频的单个视频
        self.dir_self = str(file.parents[0])
        # 针对多p视频的总目录
        self.dir_parent = str(file.parents[1])
        content = file.read_text()
        data = json.loads(content)
        self.avid = str(data.get("avid")) #"AV"+
        self.bvid = str(data.get("bvid")) #[]
        # 时间戳设置
        self.timestamp = data.get("time_create_stamp")
        self.u_timestamp = data.get("time_update_stamp")
        self.f_timestamp = file.stat().st_mtime
        # 分P总标题/合集分标题
        self.maintitle = str(data.get("title"))
        self.owner_id = str(data.get("owner_id"))
        self.owner = data.get("owner_name")
        if not self.owner is None:
            userlist[self.owner_id] = self.owner
        self.is_ep = data.get("page_data") is None
        page_data = data.get("page_data") or data.get("ep") or {}
        # 分P排序
        self.page = page_data.get("page")
        # 分P分标题/合集总标题
        self.part = str(page_data.get("part"))
        # 分P总+分标题
        self.subtitle = str(page_data.get("download_subtitle"))
        # 未知键值对
        self.indextitle = str(page_data.get("index_title"))
        self.index = page_data.get("index")
        if self.index is None:
            self.index = 0
        try:
            self.index = int(self.index)
        except ValueError:
            self.index = 0
        self.danmaku = file.parent/"danmaku.xml"
        self.size = data.get("total_bytes")
        # set final title
        self.reset_title()
        # __import__("pprint").pprint(vars(self))
    def getlist(self):
        """返回视频的信息列表"""
        return [self.owner, self.title, "AV"+self.avid, self.bvid, self.dir_self]
    def reset_title(self, mode="auto"):
        """根据给出模式重新设置总标题"""
        if mode == "maintitle":
            self.title = self.maintitle
        elif mode == "part":
            self.title = self.part
        elif mode == "auto_reverse":
            self.title = self.maintitle
            if self.part not in ("None", "", self.maintitle) and \
                    not self.part.startswith(("Video_", "studio_video_")):
                self.title = f"{self.part} - {self.title}"
            if self.subtitle not in ("None", "", self.maintitle, f"{self.maintitle} {self.part}"):
                self.title = f"{self.subtitle} - {self.title}"
        else:
            self.title = self.maintitle
            if self.part not in ("None", "", self.maintitle) and \
                    not self.part.startswith(("Video_", "studio_video_")):
                self.title = f"{self.title} - {self.part}"
            if self.subtitle not in ("None", "", self.maintitle, f"{self.maintitle} {self.part}"):
                self.title = f"{self.title} - {self.subtitle}"

        if self.is_ep:
            self.title = f"{self.title} - {self.index:02d}"
            if self.indextitle not in ("None", ""):
                self.title = f"{self.title} {self.indextitle}"

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
        self.sets = curses.newwin(int(self.height/2), int(self.width/2), int(self.height/4), int(self.width/4))
        self.content : list[list[Video]] = content
        # self.height = len(self.content)
    def print_list(self, content):
        """打印视频列表"""
        self.list.erase()
        for i in range(self.position, self.position+self.height):
            if i >= len(content):
                break
            p = content[i]
            width = self.width - 2
            color = 0
            if isinstance(p, list):
                title = pytools.get_str_in_width(
                        (p[0].title if len(p)==1 else p[0].maintitle),
                        width-1, align="<l>")
                if len(p) != 1:
                    color+=curses.color_pair(2)
            else:
                title = pytools.get_str_in_width(p.title, width-1, align="<l>")
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
        self.info.erase()
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
    def gen_verbose_info(self, content) -> str:
        """显示光标处视频more详细信息"""
        p : Video|list[Video] = content[self.select]
        sets = True
        if isinstance(p, list):
            sets = len(p) == 1
            p = p[0]
        s = f"AV: AV{p.avid}\n"
        s += f"BV: {p.bvid:12}\n"
        s += f"DIR: {p.dir_self if sets else "<SET> "+p.dir_parent}\n"
        s += f"up主: {p.owner}\n"
        s += f"up主id: {p.owner_id}\n"
        s += f"分P总标题/合集分标题: {p.maintitle}\n"
        s += f"分P分标题/合集总标题: {p.part}\n"
        s += f"合并后标题: {p.title if sets else p.maintitle}\n"
        s += f"缓存更新日期: {pytools.get_strtime(p.u_timestamp/1000)}\n"
        s += f"缓存创建日期: {pytools.get_strtime(p.timestamp/1000)}\n"
        s += f"文件日期: {pytools.get_strtime(float(p.f_timestamp))}\n"
        if p.indextitle != "None":
            s += f"INDEX: {p.index}\n"
            s += f"INDEX_TITLE: {p.indextitle}\n"
        return s
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
        print("\n>>>>>>>> RUN:\n"+"\n".join(["$ "+i for i in cmd.splitlines()]))
        print("<<<<<<<<")
        if os.name == "posix":
            os.system(cmd)
        else:
            # 针对windows做的尝试性优化处理
            for i in cmd.splitlines():
                if i.startswith("#"):
                    continue
                c = shlex.split(i)
                if not c:
                    continue
                subprocess.run(shlex.split(i), check=False)
        input("按下回车返回")
        curses.reset_prog_mode()
    def setting(self):
        """设置界面"""
        scr = self.sets
        inp = ""
        select = 0
        edit_mode = False
        while inp.lower() != "q":
            scr.erase()
            if scr.getmaxyx()[0] <= 3:
                return
            title = "SETTING"
            if edit_mode:
                title += "(editing)"
            scr.addstr("\n"+pytools.get_str_in_width(title, scr.getmaxyx()[1]))
            offset = 0
            while select-offset >= scr.getmaxyx()[0] - 3:
                offset += 1
            i = 0
            while i+offset < len(CONFIG) and i < scr.getmaxyx()[0] - 3:
                key = list(CONFIG)[i+offset]
                pre = "  > " if select == i+offset else "    "
                scr.addstr(pre+pytools.get_str_in_width(f"({i}) {key}: {CONFIG[key]}",
                                                         scr.getmaxyx()[1]-8, align="<l>")+"\n")
                i+=1
            scr.box()
            scr.refresh()
            scr.keypad(True)
            inp = chr(scr.getch())
            if edit_mode:
                if inp == chr(27):
                    edit_mode = False
                    continue
                if inp == chr(curses.KEY_BACKSPACE):
                    if CONFIG[list(CONFIG)[select]]:
                        CONFIG[list(CONFIG)[select]] = CONFIG[list(CONFIG)[select]][:-1]
                    continue
                if inp in "\r\n"+chr(curses.KEY_ENTER):
                    continue
                CONFIG[list(CONFIG)[select]] += inp
                continue
            if inp in "lj"+chr(curses.KEY_DOWN)+chr(curses.KEY_RIGHT):
                select+=1
            elif inp in "hk"+chr(curses.KEY_UP)+chr(curses.KEY_LEFT):
                select-=1
            elif inp == "g":
                select=0
            elif inp == "G":
                select=-1
            elif inp == " ":
                if isinstance(CONFIG[list(CONFIG)[select]], bool):
                    CONFIG[list(CONFIG)[select]] = not CONFIG[list(CONFIG)[select]]
                else:
                    CONFIG[list(CONFIG)[select]] = ""
                    edit_mode = True
            select %= len(CONFIG)
        self.refresh_list()
    def refresh_list(self):
        def sort_key(x):
            if CONFIG["sort_type"] == "uf_time":
                return x[0].u_timestamp
            if CONFIG["sort_type"] == "f_time":
                return x[0].f_timestamp
            if CONFIG["sort_type"] == "owner":
                return x[0].owner
            return x[0].timestamp
        self.content = sorted(self.content, key=sort_key, reverse=CONFIG["sort_reverse"])
        title_mode = ("maintitle", "part", "auto", "auto_reverse")
        if CONFIG["name_format"] not in title_mode:
            CONFIG["name_format"] = "auto"
        for i in self.content:
            for j in i:
                j.reset_title(CONFIG["name_format"])
    def videos_filter(self, key:str) -> list[list[Video]]:
        """获取过滤后的视频列表"""
        key = key.lower()
        if key.startswith("owner:"):
            key = key[6:]
            flag = lambda x:get_pinyin(str(x[0].owner))
        else:
            flag = lambda x:get_pinyin(x[0].title)
        if not key:
            return self.content
        try:
            pattern = re.compile(key, re.I)
            li = [i for i in self.content if pattern.search(flag(i))]
        except re.error:
            li = self.content
        return li
    def main(self):
        """ui主程序"""
        self.check(self.content)
        self.refresh_list()
        self.stdscr.refresh()
        inp = "0"
        deep = 0
        position = 0
        select = 0
        collection = []
        search_key = ""
        key_modes = {"p":"play","P":"play_mp4","c":"copy",
                     "e":"mp3","E":"mp4","d":"mp4"}
        while inp != "q":
            cont = self.videos_filter(search_key)
            cont = [cont, cont[select]][deep]
            self.check(cont)
            obj = [cont[self.select]] \
                    if len(self.collection) == 0 else \
                    [cont[i] for i in self.collection]
            self.print_list(cont)
            self.print_info(cont)
            inp = chr(self.stdscr.getch())
            if inp in "j"+chr(curses.KEY_DOWN):
                self.select+=1
            elif inp in "k"+chr(curses.KEY_UP):
                self.select-=1
            elif inp == "g":
                self.select=0
            elif inp == "G":
                self.select=-1
            elif inp in "l"+chr(curses.KEY_RIGHT) and deep == 0 and len(cont[self.select]) != 1:
                position,select,self.position,self.select = self.position, self.select, 0, 0
                collection, self.collection = self.collection, []
                deep = 1
            elif inp in "h"+chr(curses.KEY_LEFT) and deep == 1:
                self.position,self.select = position,select
                position,select = 0, 0
                self.collection = collection
                deep = 0
            elif inp == "r":
                self.select = len(cont) - self.select - 1
                cont.reverse()
                CONFIG["sort_reverse"] = not CONFIG["sort_reverse"]
            elif inp in key_modes:
                self.cmd(cmd_genal(obj, key_modes[inp]))
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
            elif inp in "1234":
                title_mode = {"1":"maintitle", "2":"part", "3":"auto",
                              "4":"auto_reverse"}
                CONFIG["name_format"] = title_mode[inp]
                self.refresh_list()
            elif inp == "5":
                CONFIG["vfat_name"] = not CONFIG["vfat_name"]
            elif inp == "o":
                self.setting()
            elif inp == "i":
                self.cmd("#" + "\n#".join(self.gen_verbose_info(cont).splitlines()))
            elif inp == "/":
                curses.def_prog_mode()
                curses.endwin()
                print(">"*8+"搜索"+"<"*8)
                print("$ 输入`owner:xxx`以匹配up")
                print("$ 支持正则表达式")
                print("$ 留白表示清除搜索结果")
                if lazy_pinyin:
                    print("$ 支持简单的拼音搜索")
                _search_key = input("请输入搜索关键词:")
                if not self.videos_filter(_search_key):
                    print("$ WARN 无对应搜索结果")
                    input("按下回车返回")
                else:
                    search_key = _search_key
                curses.reset_prog_mode()
            elif inp == "O":
                curses.def_prog_mode()
                curses.endwin()
                outputf = Path(input("输入预期要保存的文件名:"))
                if outputf.exists():
                    print("文件已存在")
                    input("按下回车返回")
                else:
                    print("key list:")
                    __import__('pprint').pprint(key_modes)
                    k = input("输入类型:")
                    if k not in key_modes:
                        print("键不存在")
                        input("按下回车返回")
                    else:
                        Path(outputf).write_text(
                                cmd_genal(obj, key_modes[k]),
                                encoding="utf8")
                curses.reset_prog_mode()
        # curses.endwin()

def get_video_dimensions(file_path:Path) -> tuple[int, int]:
    cmd = ["ffprobe", "-v", "error", "-select_streams", "v:0",
           "-show_entries", "stream=width,height", "-of", "json", str(file_path)]
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    data = json.loads(result.stdout)
    width = data["streams"][0]["width"]
    height = data["streams"][0]["height"]
    return width, height

def cmd_genal(li, mode) -> str:
    """生成shell命令用于保存或执行"""
    has_subprocess = subprocess.run(["biliass","-v"],check=False,
            capture_output=True).returncode == 0
    if CONFIG["cover"] and Path(CONFIG["cover"]).is_file():
        cover_f = Path(CONFIG["cover"])
    else:
        cover_f = None
    t = ""
    if not isinstance(li[0], list):
        li = [li]
    t+=f"echo \"Running mode:{mode}\"\n"
    for i in li:
        if len(i) != 1:
            t+="# "+">"*25+"\n"
            t+=f"# {"番剧" if i[0].is_ep else "分P视频"}: '{i[0].maintitle}'\n"
            t+=f"# 视频AV号: AV{i[0].avid}\n"
            t+="# "+">"*25+"\n\n"
        for j in i:
            if not isinstance(j, Video):
                continue
            danmaku : Path = j.danmaku
            # 替换vfat系统不允许出现的字符（包括部分正常文件系统特殊字符）
            output = j.title
            replacement = ["/", "／"]
            if CONFIG["vfat_name"]:
                replacement = ["\\/:*?\"<>", "＼／∶＊？＂〈〉｜"]
            for k,l in zip(list(replacement[0]), list(replacement[1])):
                output=output.replace(k, l)
            output = CONFIG["outputd"]+CONFIG["prefix"]+output
            if output[0] == "-":
                output = "./"+output
            base_output = output
            title = j.title if len(i) == 1 else \
                    (j.title[len(j.maintitle)+3:] if j.is_ep else j.part)
            t+=f"# {"" if len(i) == 1 else "分P"}标题: '{title}'\n"
            t+=f"# 目录: '{j.dir_self}'\n"

            vid = list(Path(j.dir_self).glob("**/video.m4s"))
            aud = list(Path(j.dir_self).glob("**/audio.m4s"))
            if len(vid) == 0 or len(aud) == 0:
                t+="# [ERROR] 未找到符合预期的视频或音频文件\n\n"
                continue

            vid = vid[0]
            aud = aud[0]
            # if vid.stat().st_size != j.size:
                # t+="# [WARN] 视频文件大小对不上，文件可能不完整\n"

            # -v warning -progress pipe:1
            ffmpeg = "ffmpeg -hide_banner"
            if mode == "3gp":
                t+=f'{ffmpeg} -i {repr(str(vid))} -i {repr(str(aud))}'\
                    +' -r 12 -b:v 400k -s 352x288 -ab 12.2k -ac 1 -ar 8000 -c copy '
                output+=".3gp"
            elif mode == "mp4":
                t+=f"{ffmpeg} -i {repr(str(vid))} -i {repr(str(aud))} -c copy "
                output+=".mp4"
            elif mode in ("m4a" "copy"):
                t+=f"cp {repr(str(aud))} "
                output+=".m4a"
            elif mode == "play":
                t+=f'echo "Now Playing:{j.title}"\n{CONFIG["player"]} {repr(str(aud))}\n'
                continue
            elif mode == "play_mp4":
                t+=f'echo "Now Playing:{j.title}"\n{CONFIG["player"]} {repr(str(vid))} &\n'+\
                        f'{CONFIG["player"]} "{repr(str(aud))}"\n'
                continue
            else:
                t+=f'{ffmpeg} -i {repr(str(aud))} '
                if cover_f:
                    t+=f'-i {repr(str(cover_f))} '
                    t+='-map 0:0 -map 1:0 -id3v2_version 3 '
                    t+='-metadata:s:v title="Album cover" -metadata:s:v comment="Cover (Front)" '
                t+=f'-metadata album={repr(CONFIG["album"])} '
                if j.owner != "None":
                    t+=f'-metadata artist={repr(j.owner)} '
                t+=f'-metadata title={repr(j.title)} '
                comment = f"AV{j.avid}({j.bvid}) || MAINTITLE: {j.maintitle} || PART: {j.part}"
                t+=f'-metadata comment={repr(comment)} '
                output+=".mp3"
            t+=f'{repr(str(output))}\n'
            if CONFIG["genass"] and has_subprocess and mode not in ("play", "play_mp4"):
                width, height = get_video_dimensions(vid)
                t+=f'biliass {repr(str(danmaku))} -s {width}x{height} -o '\
                    f'{repr(str(base_output)+".ass")}\n'
            t += "\n"
        if len(i) != 1:
            t+="# "+"<"*25+"\n"
            t+=f"# {"番剧" if i[0].is_ep else "分P视频"}结束: {i[0].maintitle}\n"
            t+="# "+"<"*25+"\n\n"
    t+='echo "Done!"\n'
    return t

def run_main() -> list[list[Video]]:
    """运行主程序(需要)"""
    list_mode = False
    inputd = ["/sdcard/Android/data/tv.danmaku.bili/download/",
              "/sdcard/Android/data/com.a10miaomiao.bilimiao/download/",
              "./"]
    parser = argparse.ArgumentParser(description="导出B站手机端缓存")
    parser.add_argument('-l', '--list', action="store_true", help='列出视频')
    parser.add_argument('-n', '--no-vfat-name', action="store_true", help='不替换特殊字符')
    parser.add_argument('-A', '--gen-ass', action="store_true", help='生成弹幕(会拖慢脚本生成速度)')
    parser.add_argument('-i', '--input-dir', action="append", help='设置输入文件夹')
    parser.add_argument('-o', '--output-dir', default="", help='设置输出文件夹')
    parser.add_argument('-O', '--output-file', default=None, help='设置输出文件')
    parser.add_argument('-p', '--player', default="mpv", help='设置默认播放器')
    parser.add_argument('-m', '--mode', default="mp3", choices=["mp3", "mp4", "m4a", "3gp", "play"],
                        help='设置脚本输出类型(mp3默认,m4a仅复制)')
    parser.add_argument('-s', '--sort', default="auto", choices=["cf_time", "uf_time", "f_time", "owner"],
                        help='设置排序方式')
    parser.add_argument('-H', '--help-key', action="store_true", help='内部按键帮助')
    parser.add_argument('--album', default="b站", help='设置专辑')
    parser.add_argument('--cover', default="", help='设置封面')
    parser.add_argument('--title', action="store_true", help='是否设置标题')
    parser.add_argument('--prefix', default="V_", help='输出文件prefix')
    try:
        __import__("argcomplete").autocomplete(parser)
    except ModuleNotFoundError:
        pass
    args = parser.parse_args()
    if args.help_key:
        mhelp()
    list_mode = args.list
    mode = args.mode
    CONFIG["outputd"] = args.output_dir
    outputf = args.output_file
    CONFIG["vfat_name"] = not args.no_vfat_name
    CONFIG["player"] = args.player
    CONFIG["genass"] = args.gen_ass
    CONFIG["sort_type"] = args.sort
    CONFIG["album"] = args.album
    CONFIG["cover"] = args.cover
    CONFIG["prefix"] = args.prefix
    if args.input_dir:
        inputd=args.input_dir

    input_f = []
    for i in (list(Path(i).glob("**/entry.json")) for i in inputd):
        input_f += i
    if len(input_f) == 0:
        mhelp(msg="[!] no input file was found")
    print(f"{len(input_f)} files were found")
    li : list[list[Video]] = []
    for i in enumerate(input_f):
        v = Video(input_f[i[0]])
        if len(li) > 0 and li[-1][0].dir_parent == v.dir_parent:
            li[-1].append(v)
        else:
            li.append([v])
        li[-1] = sorted(li[-1], key=lambda x:x.page, reverse=not CONFIG["sort_reverse"])
    for i in li:
        for j in i:
            if not j.owner is None or j.owner_id not in userlist:
                continue
            j.owner = userlist[j.owner_id]
    li = sorted(li, key=lambda x:x[0].timestamp, reverse=CONFIG["sort_reverse"])

    if list_mode:
        for i in li:
            for j in i:
                print(j.getlist(), end=" ")
            print()
        sys.exit(0)
    if outputf is not None:
        if not args.gen_ass:
            print("Tips: 没有弹幕？尝试使用`--gen-ass`选项")
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
          "genass":False, "sort_type":"cf_time",
          "sort_reverse":True,
          "name_format":"auto",
          "album":"", "cover":"", "prefix":""}
if __name__ == "__main__":
    video_list = run_main()
    curses.wrapper(lambda stdscr: Ui(stdscr, video_list).main())
