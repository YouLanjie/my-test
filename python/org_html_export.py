#!/usr/bin/python

from pathlib import Path
from pathlib import PurePosixPath
from string import Template
import re
import hashlib
import argparse
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

class Node:
    """节点"""
    template = Template(
"""\
#+title: ${title}
# fancybox
${fancybox}
# user's theme
#+HTML_HEAD: <link rel='stylesheet' type='text/css' href='${css_file}'/>
#+HTML_LINK_UP: ${up}
#+HTML_LINK_HOME: ${home}
#+HTML_HEAD: <style>li{margin: 0px}</style>

${nav_h}
${tree}
* ${subtitle}
${filelist}
${nav_t}
""")
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
    def _proc_name(self, f:Path, start:Path|None=None, add_prefix=True) -> str:
        if not start:
            start = self.output_d
        relpath = pytools.calculate_relative2(f, start)
        name = safety_name(str(relpath))
        if not add_prefix:
            return name
        if name.startswith("/"):
            name = "file://"+name
        elif not name.startswith("../"):
            name = "./"+name
        return name
    def len(self) -> int:
        """计算子节点总数"""
        l = 0
        if len(self.filelist) > 0:
            l = 1
        for i in self.tree:
            l += i.len()
        return l
    def save_file(self, f:Path, s:str, num:int=0, total:int=0):
        """保存文件"""
        progress = ""
        if num > 0 and total > 0:
            progress = f"({num}/{total})"
        name = f.name
        if len(name) > 15:
            name = name[:5]+"..."+name[-6:]
        action = "保存"
        if f.exists():
            if not f.is_file():
                pytools.print_err(f"ERROR 目标输出是已存在文件夹 - {f}")
                return
            action = "覆盖"
        print(f"INFO {action}文件{progress} - {name}({self.name})")
        f.write_text(s, encoding="utf8")
    def get_tree_text(self, is_top=False) -> list[str]:
        """生成目录树文本"""
        s = []
        for i in self.tree:
            tree = i.get_tree_text()
            if tree:
                s += tree
        s = ["  "+i for i in s]
        if len(self.filelist) > 0 and not is_top:
            s = [f"- [[{self._proc_name(self.output_f)}][{safety_name(self.name.name)}]]"]+s
        return s
    def get_navigation_text(self, prevp:Node|None, nextp:Node|None) -> str:
        """生成导航栏文本"""
        if not (prevp or nextp or len(self.filelist)!=0):
            return ""
        data = {"prev":"", "hint_prev":"上一页",
                "next":"", "hint_next":"下一页",
                "num":str(len(self.filelist))}
        if prevp:
            url = self._proc_name(prevp.output_f)
            data["prev"] = f"[[{url}][{safety_name(prevp.name.name)}]]"
            data["hint_prev"] = f"[[{url}][上一页]]"
        if nextp:
            url = self._proc_name(nextp.output_f)
            data["next"] = f"[[{url}][{safety_name(nextp.name.name)}]]"
            data["hint_next"] = f"[[{url}][下一页]]"
        return Template("|${hint_prev}|本页文件总数|${hint_next}|\n"
                        "|-|\n"
                        "|${prev}|${num}|${next}|").safe_substitute(data)
    def get_filelist_text(self, split:int|str|None = 0) -> str:
        """生成文件列表文本"""
        li = []
        for i in self.filelist:
            pattern = r".*\.(?:"+ "|".join(VID_EXTS) +r")"
            if not re.match(pattern, i.name):
                li.append(f"- [[{self._proc_name(i)}]]")
                continue
            prefix = "./" if not self._proc_name(i).startswith("/") else "file://"
            i = PurePosixPath(i)
            li.append("-\n  #+begin_export html\n"
                      f"<video controls src={repr(prefix+str(i))} data-fancybox=\"gallery\">"
                      f"<source src={repr(prefix+str(i))}/></video>\n"
                      f"<a href={repr(prefix+str(i))}>原文件</a>\n"
                      "#+end_export")

        if not isinstance(split, int):
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
                "up":"../", "home":"./", "css_file":"",
                "navigation":"", "nav_h":"", "nav_t":"",
                "tree":"", "filelist":"",
                "fancybox":""}
        for k,v in args.items():
            data[k] = v

        if self.father:
            data["up"] = self._proc_name(self.father.output_f)
        data["home"] = self._proc_name(self.root.output_f)
        data["tree"] = "\n".join(self.get_tree_text(True))
        if data["tree"]:
            data["tree"] = f"* Tree of {data["subtitle"]}\n"+data["tree"]
        data["navigation"] = self.get_navigation_text(prevp, nextp)
        if data["navigation"]:
            data["nav_h"] = "* Header\n" + data["navigation"]
            data["nav_t"] = "* Tail\n" + data["navigation"]
        data["filelist"] = self.get_filelist_text(data.get("split"))
        return self.template.safe_substitute(data)
    def dump2html(self, args:dict,
                  done=0, total=0):
        """导出总调用接口(递归调用)"""
        done+=1
        if total <= 0:
            total = self.root.len()
        prevp = nextp = None
        if self.father:
            idx = self.father.tree.index(self)
            if idx > 0:
                prevp = self.father.tree[idx-1]
            if idx+1 < len(self.father.tree):
                nextp = self.father.tree[idx+1]
        s = self.gen_page({
            "title": str(args.get("title") or ""),
            "subtitle":f"{done} / {pytools.calculate_relative3(self.name, self.root.name)}" \
                    if total > 0 else self.name.name,
            "fancybox": str(args.get("fancybox") or ""),
            "split": args.get("split"),
            "css_file": args.get("css_file")}, prevp, nextp)
        if args.get("save_org"):
            suffix = self.output_f.parent
            self.save_file(suffix/(self.output_f.stem+".org"), s, done, total)
        if not args.get("no_export"):
            doc = orgreader2.Document(s.splitlines(),
                                      setting={"pygments_css":False,"mathjax_script":False})
            doc.setting["css_in_html"] = ""
            formatter = orgreader2.HtmlExportVisitor()
            s = str(doc.accept(formatter)).replace("<img ", '<img data-fancybox="gallery" ')
            self.save_file(self.output_f, s, done, total)
        for i in self.tree:
            done = i.dump2html(args, done, total)
        return done

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

    print("INFO 构建节点树")
    node = Node(input_d, output_d, pattern)

    fancybox_config = """\
Carousel:{formatCaption:(carouselRef,slide)=>{\
return `${slide.caption ? slide.caption + " | " : ""}\
${slide.alt}`},},""" if args.add_caption else ""

    fancybox = """
#+HTML_HEAD: <link rel="stylesheet" href=%s>
#+HTML_HEAD: <script src=%s></script>
#+HTML_HEAD: <script>window.onload = function() {Fancybox.bind('[data-fancybox]', %s)}</script>
""" % (repr(args.fancybox_css), repr(args.fancybox_js), "{"+fancybox_config+"}") \
        if args.enable_fancybox else ""

    if node.len() <= 0:
        pytools.print_err("WARN 好像没有找到识别范围内文件呢喵")
        return
    node.dump2html({
        "title":title,
        "fancybox": fancybox,
        "save_org": args.save_org,
        "no_export": args.no_export,
        "split": args.split,
        "css_file": args.css_file,
        })

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
    parser.add_argument('--enable-old-fancybox', action="store_true",
                        help='启用旧版fancybox(需要未指定fancybox链接)')
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
