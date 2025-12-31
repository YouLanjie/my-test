#!/usr/bin/python

import argparse
from pathlib import Path
from pathlib import PurePosixPath
import os
import re
import multiprocessing
import time
import orgreader2
import pytools

try:
    import natsort
except ModuleNotFoundError as e:
    INPUT = input("tyr: pip install natsort ? (y/N)")
    if INPUT in ("y", "yes"):
        import pip
        if not pip.main(["install","natsort"]):
            print("安装成功，请重新启动程序")
    raise e

IMG_EXTS=["png","jpg","jpeg","dng","gif","webp"]
VID_EXTS=["mp3","m4a","3ga","3gp","wav",
      "mp4","mkv","webm","mpeg","mov"]

def get_sort_key(s:str)->str:
    """尝试获取内容中的日期，否则返回原字符串"""
    result = re.match(r'.*?([1-2]\d{3})[-_年. ]*([0-1]\d)[-_月. ]*([0-3]\d)[-_日. ]*'
                      r'([0-2]\d)[-_小时. ]*([0-6]\d)[-_分钟. ]*([0-6]\d)[-_秒钟. ]*.*', s)
    if result:
        return "-".join(result.groups())
    return s

def safety_name(s:str)->str:
    """对中括号加上转义"""
    return re.sub(r"([][])", r"\\\1", s)

def get_output(files:list[Path], title:str, subtitle:str, link:tuple[Path|None, Path|None]|None, args:argparse.Namespace) -> str:
    """获得单个文件列表的输出"""
    navigation = ""
    if link:
        navigation = "|上一页|本页文件总数|下一页|\n|-|\n|<l>|<c>|<r>|\n"
        navigation += f"| [[./{safety_name(str(link[0]))}][{safety_name(link[0].stem)}]] |" \
                if link[0] else "| |"
        navigation += f" {len(files)} "
        navigation += f"| [[./{safety_name(str(link[1]))}][{safety_name(link[1].stem)}]] |" \
                if link[1] else "| |"
    fancybox_config = """Carousel:{formatCaption:(carouselRef,slide)=>{\
return `${slide.caption ? slide.caption + " | " : ""}${slide.alt}`},},""" if args.add_caption else ""
    ret = f"""\
#+title: {title}
# fancybox
{f"""
#+HTML_HEAD: <link rel="stylesheet" href={repr(args.fancybox_css)}>
#+HTML_HEAD: <script src={repr(args.fancybox_js)}></script>
#+HTML_HEAD: <script>window.onload = function() {"{"}Fancybox.bind('[data-fancybox]', {"{"+fancybox_config+"}"}){"}"}</script>
""" if args.enable_fancybox else ""}
# user's theme
#+HTML_HEAD: <link rel='stylesheet' type='text/css' href='{args.css_file}'/>
#+HTML_LINK_UP: ../
#+HTML_LINK_HOME: ./
#+HTML_HEAD: <style>li{"{margin: 0px}"}</style>
{f"* Header\n{navigation}" if link else ""}
* {subtitle}
"""
    li = []
    for i in files:
        i = PurePosixPath(i)
        safe_name = safety_name(str(i))
        prefix = "./" if safe_name[0] != "/" else "file://"
        if re.match(r".*\.(?:"+"|".join(VID_EXTS)+r")", i.name):
            li.append("-\n  #+begin_export html\n"
                      f"<video controls src={repr(prefix+str(i))} data-fancybox=\"gallery\">"
                      f"<source src={repr(prefix+str(i))}/></video>\n"
                      f"<a href={repr(prefix+str(i))}>原文件</a>\n"
                      "#+end_export")
        else:
            li.append(f"- [[{prefix}{safe_name}]]")
    if args.split:
        nl = []
        for index,item in enumerate(li):
            if index % args.split == 0:
                nl.append(f"** {index+1}-{index+10}")
            nl.append(item)
        li = nl
    ret += "\n".join(li)
    ret += f"\n* Tail\n{navigation}" if link else ""
    return ret

def get_input_dir(inp:Path, pattern:re.Pattern, fallback_path=None)->list[Path]:
    """获取可用输入文件夹列表并排序"""
    li = [i for i in inp.iterdir() if i.is_dir()]
    dir_list = []
    for i in li:
        for j in i.iterdir():
            if j.is_file() and pattern.match(j.name):
                dir_list.append(i)
                break
    if not dir_list and isinstance(fallback_path, Path):
        for j in fallback_path.iterdir():
            if j.is_file() and pattern.match(j.name):
                dir_list.append(fallback_path)
                break
    dir_list = natsort.natsorted(dir_list)
    return dir_list

def save_file(f:Path, s:str, progress:str = ""):
    """保存文件"""
    print(f"INFO 保存文件{progress} - {f}")
    if f.exists():
        if not f.is_file():
            pytools.print_err(f"ERROR 目标输出是已存在文件夹 - {f}")
            return
        pytools.print_err(f"WARN 覆盖文件 - {f}")
    f.write_text(s, encoding="utf8")

def calculate_relative(p1:Path, p2:Path) -> Path:
    """计算相对目录，根据长度选择reslove和absolute(可能存在bug)"""
    reslove = pytools.calculate_relative(p1, p2)
    absolute = pytools.calculate_relative2(p1, p2)
    if len(str(reslove)) <= len(str(absolute)):
        return reslove
    return absolute

class Sites:
    """转换网页临时数据结构"""
    def __init__(self, dir_list:list[Path], output_d:Path, title:str, args:argparse.Namespace) -> None:
        extra_exts=[i for i in args.extern.split("|") if i!=""]
        self.pattern = re.compile(r".*\.(?:"+"|".join(IMG_EXTS+VID_EXTS+extra_exts)+")", re.I)
        self.dir_list = dir_list
        self.output_d = output_d
        self.title = title
        self.args = args
        self.formatter = orgreader2.HtmlExportVisitor()
    def process_sigal_page(self, index:int, running, finish_count, lock):
        lock.acquire()
        running.value += 1
        lock.release()

        lastdir = Path(f"{self.dir_list[index-1]}.html") if index > 0 else None
        dirs = self.dir_list[index]
        nextdir = Path(f"{self.dir_list[index+1]}.html") if index+1 < len(self.dir_list) else None
        file_list = [calculate_relative(i, self.output_d)
                for i in dirs.iterdir()
                if i.is_file() and self.pattern.match(i.name)]
        file_list = natsort.natsorted(file_list, key=lambda x:get_sort_key(x.name))
        output = get_output(file_list, self.title,
                f"{self.dir_list.index(dirs)+1} / {dirs}" if len(self.dir_list) > 1 else f"{dirs}",
                (lastdir, nextdir), self.args)
        objname = dirs.name
        if len(self.dir_list) <= 1:
            objname = "index"
        if self.args.save_org or self.args.no_export:
            save_file(Path(f"{self.output_d}/{objname}.org"), output)
        if not self.args.no_export:
            doc = orgreader2.Document(output.splitlines(),
                                      setting={"pygments_css":False,"mathjax_script":False})
            doc.setting["css_in_html"] = ""
            lock.acquire()
            finish_count.value += 1
            lock.release()
            save_file(Path(f"{self.output_d}/{objname}.html"),
                      str(doc.accept(self.formatter)).\
                              replace("<img ", '<img data-fancybox="gallery" '),
                      f"({finish_count.value}/{len(self.dir_list)})")
        # print(output)
        lock.acquire()
        running.value -= 1
        lock.release()
        return

def main():
    """主函数"""
    args = parse_arguments()
    extra_exts=[i for i in args.extern.split("|") if i!=""]
    pattern = re.compile(r".*\.(?:"+"|".join(IMG_EXTS+VID_EXTS+extra_exts)+")", re.I)
    input_d = Path(args.input)
    output_d = Path(args.output)
    if not input_d.is_dir() or not output_d.is_dir():
        pytools.print_err("ERROR 指定的输入或输出文件夹不是文件夹")
        return

    title = input_d.absolute().name
    if args.title:
        title = args.title

    print("INFO 获取合法目录列表")
    dir_list : list[Path] = get_input_dir(input_d, pattern, input_d)

    if len(dir_list) > 1:
        print("INFO 创建首页")
        output = get_output([calculate_relative(Path(f"{i}.html"), output_d)
                             for i in dir_list],
                            f"INDEX:{title}", title, None, args)
        if args.save_org or args.no_export:
            save_file(Path(f"{output_d}/index.org"), output)
        if not args.no_export:
            doc = orgreader2.Document(output.splitlines(),
                                      setting={"pygments_css":False,"mathjax_script":False})
            save_file(Path(f"{output_d}/index.html"),
                      str(doc.accept(orgreader2.HtmlExportVisitor())))
        # print(output)
    elif dir_list:
        print("INFO 单文件(无章节)")
    else:
        pytools.print_err("WARN 好像没有找到识别范围内文件呢喵")
    sites = Sites(dir_list, output_d, title, args)
    index = 0
    running = multiprocessing.Value('i', 0)
    finish_count = multiprocessing.Value('i', 0)
    lock = multiprocessing.Lock()
    max_pth = (os.cpu_count() or 4) / 2
    for index,_ in enumerate(dir_list):
        multiprocessing.Process(target=sites.process_sigal_page,
                                args=(index, running,finish_count,lock)).start()
        index+=1
        while running.value > max_pth:
            time.sleep(0.01)

def parse_arguments() -> argparse.Namespace:
    """解释参数"""
    parser = argparse.ArgumentParser(description='用于生成一堆org文件然后导出成html（看漫画用的）')
    parser.add_argument('-i', '--input', default="./", help='设置输入目录(默认./)')
    parser.add_argument('-o', '--output', default="./", help='设置输出目录(默认./)')
    parser.add_argument('-c', '--css-file', default="../main.css", help='设置css文件(默认../main.css)')
    parser.add_argument('-e', '--extern', default="", help='设置额外查找文件后缀(`|`分割)')
    parser.add_argument('-t', '--title', default=None, help='设置标题')
    parser.add_argument('-s', '--split', type=int, nargs="?", default=0, const=10,
            help='在图片间插入标题用于快速跳转(指定分组)')
    parser.add_argument('-N', '--no-export', action="store_true", help='保存org文件但不导出')
    parser.add_argument('-S', '--save-org', action="store_true", help='保存org文件并导出')
    parser.add_argument('--enable-fancybox', action="store_true", help='启用fancybox')
    parser.add_argument('--enable-old-fancybox', action="store_true", help='启用旧版fancybox(需要未指定fancybox链接)')
    parser.add_argument('--add-caption', action="store_true", help='为文件添加caption')
    parser.add_argument('--fancybox-js', default=None, help='设置fancybox的js文件地址')
    parser.add_argument('--fancybox-css',default=None, help='设置fancybox的css文件地址')
    try:
        __import__("argcomplete").autocomplete(parser)
    except ModuleNotFoundError:
        pass
    args = parser.parse_args()
    urls = [{"js":"https://cdn.jsdelivr.net/npm/@fancyapps/ui/dist/fancybox/fancybox.umd.js",
             "css":"https://cdn.jsdelivr.net/npm/@fancyapps/ui/dist/fancybox/fancybox.css"},
            {"js":"https://cdn.jsdelivr.net/npm/@fancyapps/ui/dist/fancybox.umd.js",
             "css":"https://cdn.jsdelivr.net/npm/@fancyapps/ui/dist/fancybox.css"}]
    if args.fancybox_js is None:
        args.fancybox_js = urls[args.enable_old_fancybox]["js"]
    if args.fancybox_css is None:
        args.fancybox_css = urls[args.enable_old_fancybox]["css"]
    return args

if __name__ == "__main__":
    main()
