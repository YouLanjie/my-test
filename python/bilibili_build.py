#!/usr/bin/env python
# Created:2025.05.09

# Dependences: curses(tui),
#              ffmpeg(translate),
#              mpv(play media)

import os
import sys
import pathlib
import getopt
import json

class Video():
    def __init__(self, file):
        if file.is_file() is False:
            return None
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
        # 分P分标题/合集总标题
        self.part = str(page_data.get("part"))
        # 分P总+分标题
        self.subtitle = str(page_data.get("download_subtitle"))
        # ???
        self.indextitle = str(page_data.get("index_title"))
        self.index = page_data.get("index")
        if self.index is None:
            self.index = 0
        # set final title
        self.title = self.maintitle
        if self.part not in ("None", "", self.maintitle):
            self.title = f"{self.title} - {self.part}"
        if self.subtitle not in ("None", "", self.maintitle, f"{self.maintitle} {self.part}"):
            self.title = f"{self.title} - {self.subtitle}"
    def getlist(self):
        return [self.owner, self.title, "AV"+self.avid, self.bvid, self.dir_self]

class Ui():
    width=os.get_terminal_size().columns
    position=0
    select=0
    def __init__(self, stdscr, content : list) -> None:
        info_height = 9
        self.height=os.get_terminal_size().lines - info_height - 1
        self.stdscr = stdscr
        # self.stdscr = curses.initscr()
        # curses.noecho()
        # curses.cbreak()
        curses.curs_set(0)
        # curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLUE)
        curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_WHITE, curses.COLOR_BLACK)
        self.list = curses.newwin(self.height + 1, self.width, 0, 0)
        self.info = curses.newwin(info_height, self.width, self.height + 1, 0)
        self.content = content
        # self.height = len(self.content)
    def gsl(self, s, lim, start=0):
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
        self.list.clear()
        for i in range(self.position, self.position+self.height):
            if i >= len(content):
                break
            p = content[i]
            width = self.width - 2
            color = None
            if type(p) is list:
                title = self.gsl((p[0].title if len(p)==1 else p[0].maintitle)+" "*width, width)
                color = curses.color_pair(2) if len(p) != 1 else None
            else:
                title = self.gsl(p.title+" "*width, width)
            if i == self.select:
                self.list.addstr(f"> {title}\n", curses.color_pair(1))
                continue
            if color:
                self.list.addstr(f"  {title}\n", color)
            else:
                self.list.addstr(f"  {title}\n")
            # print(p.getlist())
        self.list.refresh()
    def print_info(self, content):
        p = content[self.select]
        sets = True
        if type(p) is list:
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
        # if self.height > len(self.content):
            # self.height = len(self.content)
        if self.select < 0:
            self.select = len(content)-1
        elif self.select >= len(content):
            self.select = 0
        while self.position + self.height <= self.select:
            self.position += 1
        while self.position > self.select:
            self.position -= 1
    def cmd(self, cmd:str):
        curses.def_prog_mode()
        curses.endwin()
        os.system(cmd)
        curses.reset_prog_mode()
    def main(self):
        self.check(self.content)
        # print("hit 'q' to quit,jk to move")
        self.stdscr.refresh()
        # curses.endwin()
        # return None
        inp = "0"
        deep = 0
        position = 0
        select = 0
        while inp != "q":
            cont = [self.content, self.content[select]][deep]
            obj = cont[self.select]
            self.print_list(cont)
            self.print_info(cont)
            inp = "%c" % self.stdscr.getch()
            if inp == "j":
                self.select+=1
            elif inp == "k":
                self.select-=1
            elif inp == "g":
                self.select=0
            elif inp == "G":
                self.select=-1
            elif inp == "p":
                t = obj[0] if type(obj) == list else obj
                # vid = list(pathlib.Path(t.dir_self).glob("**/video.m4s"))
                aud = list(pathlib.Path(t.dir_self).glob("**/audio.m4s"))
                self.cmd(f"echo \"Playing Now: {t.title}\" ; mpv \"{str(aud[0])}\"")
            elif inp == "e":
                self.cmd(save([obj], "mp3"))
            elif inp == "E":
                self.cmd(save([obj], "mp4"))
            elif inp == "l" and len(self.content[self.select]) != 1 and deep == 0:
                position,select,self.position,self.select = self.position, self.select, 0, 0
                deep = 1
            elif inp == "h" and deep == 1:
                self.position,self.select = position,select
                deep = 0
            self.check([self.content, self.content[select]][deep])
        # curses.endwin()
        return None

def save(li, mode) -> str:
    t = ""
    if type(li[0]) != list:
        li = [li]
    for i in li:
        for j in i:
            vid = list(pathlib.Path(j.dir_self).glob("**/video.m4s"))
            aud = list(pathlib.Path(j.dir_self).glob("**/audio.m4s"))
            if len(vid) == 0 or len(aud) == 0:
                print(f"{j.dir_self}:no media file({j.title})")
                continue
            t+=f"# {j.title} <Dir: {j.dir_self}>\n"
            if mode == "3gp":
                t+=f"ffmpeg -i \"{str(vid[0])}\" -i \"{str(aud[0])}\""\
                    +f" -r 12 -b:v 400k -s 352x288 -ab 12.2k -ac 1 -ar 8000 -c copy \"{j.title}.3gp\"\n"
            elif mode == "mp4":
                t+=f"ffmpeg -i \"{str(vid[0])}\" -i \"{str(aud[0])}\""\
                    +f" -c copy \"{j.title}.mp4\"\n"
            else:
                t+=f"ffmpeg -i \"{str(aud[0])}\" \"{j.title}.mp3\"\n"
    return t

def main(argv):
    list_mode = False
    output = None
    mode = "mp3"
    try:
        opts, args = getopt.getopt(argv, "hlo:m:", ["help", "list", "output=","mode="])
    except getopt.GetoptError:
        mhelp(1, msg="[!] Err: getopt error")
        exit(-1)
    for option, argument in opts:
        if option in ("-h", "--help"):
            mhelp()
        elif option in ("-l", "--list"):
            list_mode = True
        elif option in ("-m", "--mode"):
            mode = argument
        elif option in ("-o"):
            output = argument

    input_f = list(pathlib.Path("./").glob("**/entry.json"))
    if len(input_f) == 0:
        mhelp(msg="[!] no input file was found")
    print(f"{len(input_f)} files were found")
    li = []
    for i in range(len(input_f)):
        v = Video(input_f[i])
        if len(li) > 0 and li[-1][0].dir_parent == v.dir_parent:
            li[-1].append(v)
        else:
            li.append([v])

    if list_mode:
        for i in li:
            for j in i:
                print(j.getlist(), end=" ")
            print()
        exit(0)
    if output is not None:
        file = open(output, "w")
        file.write(save(li, mode))
        file.close()
        exit(0)
    return li

def mhelp(ret=0, msg=""):
    if msg != "":
        print(str(msg))
    print("Usage bilibili_build.py [OPTION]")
    print("OPTION:")
    print("    -l         --list          list media")
    print("    -o <file>  --output=file   set output file")
    print("    -H <num>   --height=num    set the height of printing")
    print("    -m <mode>  --mode=mode     set output format(mp3|mp4|3gp)")
    print("    -h         --help          show this help")
    print("[*] make sure there are file `entry.json`")
    print("[*] in the tui,press `hjkl` to move,`eE` to export media")
    exit(ret)

if __name__ == "__main__":
    li = main(sys.argv[1:])
    import curses
    curses.wrapper(lambda stdscr: Ui(stdscr, li).main())

