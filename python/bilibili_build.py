#!/usr/bin/env python
# Created:2025.05.09

import os
import sys
import pathlib
import getopt
import json

def help(ret=0, msg=""):
    if msg != "":
        print(str(msg))
    print("Usage bilibili_build.py [OPTION]")
    print("OPTION:")
    print("    -l         --list          list media")
    print("    -o <file>  --output=file   set output file")
    print("    -H <num>   --height=num    set the height of printing")
    print("    -m <mode>  --mode=mode     set output format(mp3|mp4|3gp)")
    print("    -h         --help          show this help")
    print("Tip: make sure there are file `entry.json`")
    exit(ret)
def gsl(s, lim, start=0):
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

# Code of Getch From:https://blog.csdn.net/su_cicada/article/details/81166214
class Getch():
    def __init__(self):
        try:
            self.impl = self._GetchWindows()
        except ImportError:
            self.impl = self._GetchUnix()

    def __str__(self):
        return str(self.impl)

    def _GetchUnix(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

    def _GetchWindows(self):
        import msvcrt
        return msvcrt.getch()

class Video():
    file="FILE"
    dir1="DIR1"           # 针对分p视频的单个视频
    dir2="DIR2"           # 针对多p视频的总目录
    maintitle="MAINTITLE" # 分P总标题/合集分标题
    part="PART"           # 分P分标题/合集总标题
    subtitle="SUBTITLE"   # 分P总+分标题
    index=0
    indextitle="INDEXTITLE"
    owner="OWNER"
    def __init__(self, file):
        if file.is_file() == False:
            return None
        self.file = str(file)
        self.dir1 = str(file.parents[0])
        self.dir2 = str(file.parents[1])
        content = file.read_text()
        data = json.loads(content)
        self.maintitle = str(data.get("title"))
        self.owner = str(data.get("owner_name"))
        page_data = data.get("page_data")
        self.part = str(page_data.get("part"))
        self.subtitle = str(page_data.get("download_subtitle"))
        self.indextitle = str(page_data.get("index_title"))
        self.index = page_data.get("index")
        if self.index == None:
            self.index = 0
    @property
    def title(self):
        t = self.maintitle
        if self.part != "None" and self.part != "" and self.part != self.maintitle:
            t = f"{t} - {self.part}"
        if self.subtitle != "None" and self.subtitle != "" and self.subtitle != self.maintitle and self.subtitle != self.maintitle + " " + self.part:
            t = f"{t} - {self.subtitle}"
        return t
    def getlist(self):
        return [self.owner, self.title, self.index, self.indextitle, self.dir1]

class Ui():
    content : list
    height=5
    width=os.get_terminal_size().columns
    positin=0
    select=0
    cours=0
    def __init__(self, content) -> None:
        self.content = content
        # self.height = len(self.content)
    def print_list(self):
        print(f"\033[{self.cours}A", end="")
        for i in range(self.positin, self.positin+self.height):
            if i >= len(self.content):
                break
            p = self.content[i]
            width = self.width - 2
            title = gsl(p.title+" "*width, width)
            if i == self.select:
                print(f"> {title}")
                continue
            print(f"  {title}")
            # print(p.getlist())
        self.cours=self.height
    def print_info(self):
        print("\033[s", end="")
        for i in range(7):
            print(" "*self.width)
        print("\033[u", end="")
        print("-----------")
        p = self.content[self.select]
        print(f"FULL_TITLE: {p.title}")
        print(f"OWNER: {p.owner}")
        print(f"DIR: {p.dir1}")
        if p.indextitle != "None":
            print(f"INDEX: {p.index}")
            print(f"INDEX_TITLE: {p.indextitle}")
        print("-----------")
        print("\033[u", end="")
    def check(self):
        # if self.height > len(self.content):
            # self.height = len(self.content)
        if self.select < 0:
            self.select = len(self.content)-1
        elif self.select >= len(self.content):
            self.select = 0
        while self.positin + self.height <= self.select:
            self.positin += 1
        while self.positin > self.select:
            self.positin -= 1
    def main(self):
        self.check()
        print("hit 'q' to quit,jk to move")
        print("\n"*(self.height+6))
        print(f"\033[{(self.height+6)}A", end="")
        inp = "0"
        while inp != "q":
            self.print_list()
            self.print_info()
            inp = str(Getch())
            if inp == "j":
                self.select+=1
            elif inp == "k":
                self.select-=1
            elif inp == "g":
                self.select=0
            elif inp == "G":
                self.select=-1
            elif inp == " ":
                return self.select
            self.check()
        print("\n"*5)
        return None

def main(argv):
    list_mode = False
    height = -1
    output = None
    mode = "mp3"
    try:
        opts, args = getopt.getopt(argv, "hlH:o:m:", ["help", "list", "height=", "output=","mode="])
    except getopt.GetoptError:
        help(1, msg="Err: getopt error")
        exit(-1)
    for option, argument in opts:
        if option in ("-h", "--help"):
            help()
        elif option in ("-l", "--list"):
            list_mode = True
        elif option in ("-m", "--mode"):
            mode = argument
        elif option in ("-H" "--height"):
            try:
                height = int(argument)
            except ValueError as e:
                height = -1
                # raise e
        elif option in ("-o"):
            output = argument
        # elif option in ("-v", "--verbose"):
            # verbose = True
    input_f = list(pathlib.Path("./").glob("**/entry.json"))
    if len(input_f) == 0:
        help(msg="no input file was found")
    print(f"{len(input_f)} files were found")
    li = [Video(input_f[i]) for i in range(len(input_f))]
    if list_mode:
        for i in li:
            print(i.getlist())
        return 0
    if output is not None:
        file = open(output, "w")
        for i in li:
            vid = list(pathlib.Path(i.dir1).glob("**/video.m4s"))
            aud = list(pathlib.Path(i.dir1).glob("**/audio.m4s"))
            if len(vid) == 0 or len(aud) == 0:
                print(f"{i.dir1}:no media file({i.title})")
                continue
            file.write(f"# {i.title} <Dir: {i.dir1}>\n")
            if mode == "3gp":
                file.write(f"ffmpeg -i \"{str(vid[0])}\" -i \"{str(aud[0])}\" -r 12 -b:v 400k -s 352x288 -ab 12.2k -ac 1 -ar 8000  -c copy \"{i.title}.3gp\"\n")
            elif mode == "mp4":
                file.write(f"ffmpeg -i \"{str(vid[0])}\" -i \"{str(aud[0])}\" -c copy \"{i.title}.mp4\"\n")
            else:
                file.write(f"ffmpeg -i \"{str(vid[0])}\" \"{i.title}.mp3\"\n")
        file.close()
        return 0
    ui = Ui(li)
    if height > 5 and height <= os.get_terminal_size().lines - 10:
        ui.height = height
    ui.main()

if __name__ == "__main__":
    main(sys.argv[1:])

