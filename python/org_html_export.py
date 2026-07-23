#!/usr/bin/python

import argparse
from pathlib import Path
from pathlib import PurePosixPath
from string import Template
import re
import hashlib
# from sys import exception
import orgreader2
import pytools

try:
    import natsort
    mysorted = natsort.natsorted
except ModuleNotFoundError as e:
    pytools.print_err("[WARN] natsort不可用，使用sort作为替代")
    mysorted = sorted

IMG_EXTS=["png","jpg","jpeg","dng","gif","webp"]
VID_EXTS=["mp3","m4a","3ga","3gp","wav",
      "mp4","mkv","webm","mpeg","mov"]

def get_sort_key(s:str)->str:
    """尝试获取内容中的日期，否则返回原字符串"""
    result = re.match(r'.*?([1-2]\d{3})[-_年. ]*([0-1]\d)[-_月. ]*([0-3]\d)[-_日. ]*'
                      r'([0-2]\d)[-_小时. ]*([0-6]\d)[-_分钟. ]*([0-6]\d)[-_秒钟. ]*.*', s)
    if not result:
        return s
    try:
        y,m,d,th,tm,ts = [int(i) for i in result.groups()]
        return f"{y:04d}-{m:02d}-{d:02d} {th:02d}:{tm:02d}:{ts:02d}"
    except ValueError:
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

class Node:
    """节点"""
    template = Template(""" """)
    def __init__(self, input_d:Path, output_d:Path, pattern:re.Pattern, father = None) -> None:
        if not input_d.is_dir():
            return
        self.name = input_d
        self.output_d = output_d
        self.father : Node|None = father
        self.root : Node = father.root if father else self

        md5 = hashlib.md5(self._proc_name(self.name).encode("utf8")).hexdigest()
        self.output_f = output_d/(md5+".html") if father else output_d/"index.html"

        self.filelist : list[Path] = []
        self.tree : list[Node] = []
        for i in input_d.iterdir():
            if i.is_dir() and not i.is_symlink():
                node = Node(i, output_d, pattern, father=self)
                if len(node.filelist)+len(node.tree) == 0:
                    continue
                self.tree.append(node)
            elif pattern.match(i.name):
                self.filelist.append(i)
        self._sort()
    def _sort(self):
        self.tree = mysorted(self.tree, key=lambda x:get_sort_key(x.name.name))
        self.filelist = mysorted(self.filelist, key=lambda x:get_sort_key(x.name))
    def _proc_name(self, f:Path) -> str:
        relpath = pytools.calculate_relative2(f, self.output_d)
        relpath = PurePosixPath(relpath)
        name = safety_name(relpath.name)
        if name.startswith("/"):
            name = "file://"+name
        elif not name.startswith("../"):
            name = "./"+name
        return name
    def get_tree_text(self) -> list[str]:
        """生成目录树文本"""
        s = []
        for i in self.tree:
            tree = i.get_tree_text()
            if tree:
                s.append(f"- [[{self._proc_name(i.output_f)}]]")
                s += tree
        s = ["  "+i for i in s]
        if s:
            s = [f"- [[{self._proc_name(self.output_f)}]]"]+s
        return s
    def get_navigation_text(self, prevp:Node|None, nextp:Node|None) -> str:
        """生成导航栏文本"""
        if not (prevp or nextp or len(self.filelist)!=0):
            return ""
        data = {"prev":self._proc_name(prevp.output_f) if prevp else "",
                "next":self._proc_name(nextp.output_f) if nextp else "",
                "num":str(len(self.filelist))}
        return Template("|上一页|本页文件总数|下一页|\n"
                        "|-|\n"
                        "|[[${prev}]]|${num}|${next}|").safe_substitute(data)
    def get_filelist_text(self, split:int|str|None = 0) -> str:
        """生成文件列表文本"""
        li = []
        for i in self.filelist:
            safe_name = safety_name(str(i))
            pattern = r".*\.(?:"+ "|".join(VID_EXTS) +r")"
            if not re.match(pattern, i.name):
                li.append(f"- [[{self._proc_name(i)}]]")
                continue
            i = PurePosixPath(i)
            prefix = "./" if not safe_name.startswith("/") else "file://"
            li.append("-\n  #+begin_export html\n"
                      f"<video controls src={repr(prefix+str(i))} data-fancybox=\"gallery\">"
                      f"<source src={repr(prefix+str(i))}/></video>\n"
                      f"<a href={repr(prefix+str(i))}>原文件</a>\n"
                      "#+end_export")

        if isinstance(split, str|None):
            try:
                split = int(str(split))
            except ValueError:
                split = 0
        if split > 0:
            nl = []
            for index,item in enumerate(li):
                if index % split == 0:
                    nl.append(f"** {index+1}-{index+10}")
                nl.append(item)
            li = nl
        return "\n".join(li)
    def gen_page(self, args:dict, prevp:Node|None=None, nextp:Node|None=None) -> str:
        """生成单页的html文本"""
        data = {"title":"", "subtitle":"",
                "up":"../", "home":"./",
                "navigation":"", "nav_h":"", "nav_t":"",
                "tree":"", "filelist":"",
                "fancybox":""}
        for k,v in args.items():
            data[k] = v

        if self.father:
            data["up"] = self._proc_name(self.father.output_f)
        data["home"] = self._proc_name(self.root.output_f)
        data["tree"] = "\n".join(self.get_tree_text())
        data["navigation"] = self.get_navigation_text(prevp, nextp)
        if data["navigation"]:
            data["nav_h"] = "* Header\n" + data["navigation"]
            data["nav_t"] = "* Tail\n" + data["navigation"]
        data["filelist"] = self.get_filelist_text(data.get("split"))
        return self.template.safe_substitute(data)

class SiteRoot:
    """根节点"""
    def __init__(self, source_d:Path, output_d:Path, title:str) -> None:
        pass
        # extra_exts=[i for i in args.extern.split("|") if i!=""]
        # self.pattern = re.compile(r".*\.(?:"+"|".join(IMG_EXTS+VID_EXTS+extra_exts)+")", re.I)

class OldSites:
    """转换网页临时数据结构"""
    def __init__(self, dir_list:list[Path], output_d:Path, title:str, args:argparse.Namespace) -> None:
        extra_exts=[i for i in args.extern.split("|") if i!=""]
        self.pattern = re.compile(r".*\.(?:"+"|".join(IMG_EXTS+VID_EXTS+extra_exts)+")", re.I)
        self.dir_list = dir_list
        self.output_d = output_d
        self.title = title
        self.args = args
        self.formatter = orgreader2.HtmlExportVisitor()
        self.finish_count = 0
    def process_sigal_page(self, index:int):
        lastdir = Path(f"{self.dir_list[index-1]}.html") if index > 0 else None
        dirs = self.dir_list[index]
        nextdir = Path(f"{self.dir_list[index+1]}.html") if index+1 < len(self.dir_list) else None
        file_list = [pytools.calculate_relative2(i, self.output_d)
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
            self.finish_count += 1
            save_file(Path(f"{self.output_d}/{objname}.html"),
                      str(doc.accept(self.formatter)).\
                              replace("<img ", '<img data-fancybox="gallery" '),
                      f"({self.finish_count}/{len(self.dir_list)})")
        # print(output)
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
        output = get_output([pytools.calculate_relative2(Path(f"{i}.html"), output_d)
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
    sites = OldSites(dir_list, output_d, title, args)
    index = 0

    for index,_ in enumerate(dir_list):
        sites.process_sigal_page(index)
        index+=1

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
