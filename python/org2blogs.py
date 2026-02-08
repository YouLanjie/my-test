#!/usr/bin/env python
"""构建博客用脚本"""

from string import Template
from pathlib import Path
from dataclasses import dataclass
import re
import sys
import time
import subprocess
import argparse
import json

import socketserver
import http.server
import webbrowser

import orgreader2
import pytools

class EmacsBacken:
    """Emacs服务"""
    def __init__(self) -> None:
        self.avaliable = False
        ret = subprocess.run("type emacs", capture_output=True, shell=True, check=False)
        if ret.returncode == 0:
            self.avaliable = True
        self.started = False
    def _exec(self, cmd, capture_output = True):
        if not self.avaliable:
            return None
        ret= subprocess.run(cmd, capture_output=capture_output, check=False)
        if ret.returncode != 0:
            print(f"[Code:{ret.returncode}] Cmd:{cmd}")
            if ret.stderr:
                print("\033[31m"+ret.stderr.decode("utf-8")+"\033[0m")
        return ret
    def start(self, loop=False):
        """启动进程"""
        ret = self._exec(["emacs", "--bg-daemon", "ORG_TO_HTML_SERVER", "-Q"])
        if ret and ret.returncode == 0:
            self.started = True
            print("Emacs Deamond had running")
        elif ret and ret.returncode == 1:
            self.started = True
            self.stop()
            if not loop:
                self.start(True)
        else:
            print("[INFO] Emacs Deamond start FAILD")
            return
        ret = self._exec(["emacsclient", "-e", """(progn (require 'package) (package-initialize)
(setq-default make-backup-files nil auto-save-default nil)
(setq-default org-src-fontify-natively t org-export-with-sub-superscripts '{} org-use-sub-superscripts '{})
(require 'monokai-theme) (load-theme 'monokai t) (require 'htmlize))"""])
    def stop(self):
        """退出emacs"""
        if self.started:
            self._exec(["emacsclient", "-e", "(kill-emacs)"])
            self.started = False
    def update_file(self, filename):
        """更新文件（自动过滤css）"""
        if not self.started:
            self.start()
        filename = str(filename)
        ret = self._exec(["emacsclient", "-e",
                          f'(progn (find-file "{filename}")'\
                                  '(org-mode)(org-html-export-to-html)'\
                                  '(kill-buffer-and-window)())'])
        if ret and ret.returncode != 0:
            return
        file = Path(filename)
        if not file.is_file():
            return
        content = file.read_text(encoding="utf8").splitlines()
        i = 0
        flag = False
        while i < len(content):
            if content[i].startswith('<style type="text/css">'):
                flag = True
            if content[i].startswith('</style>'):
                del content[i]
                break
            if flag:
                del content[i]
            i += 1
        file.write_text("\n".join(content), encoding="utf8")

@dataclass
class Doc:
    date : str = ""
    link : str = ""
    title : str = ""
    desc : str = ""

class Args:
    """Arg class"""
    def __init__(self) -> None:
        cfgname = "BlogConfig.json"
        self.config = Path(cfgname)
        self.add : None|str = None
        self.ignore_time = False
        self.no_build_home = False
        self.touch = False
        self.limit = 50
        self.run = False
        self.verbose = False
        self.print_config = False
        self.watch = 0.0
        self.emacs = False
        if __name__ == "__main__":
            pytools.merge_dict(self.__dict__, self._get_args())
        if not self.config.is_file():
            for i in Path().resolve().parents:
                if (i/cfgname).is_file():
                    self.config = i/cfgname
                    break
        if not self.config.is_file():
            self.config = Path(cfgname)
        self.project_dir = self.config.parent
    def _get_args(self):
        parser=argparse.ArgumentParser(description="构建博客用脚本")
        parser.add_argument("-c", "--config", type=Path, default="BlogConfig.json",
                            help="配置文件")
        parser.add_argument("-a", "--add",type=str, help="模板建立一个指定的org文件")
        parser.add_argument("-I", "--ignore-time", action="store_true", help="忽略时间强制更新")
        parser.add_argument("-N", "--no-build-home", action="store_true", help="不构建主页")
        parser.add_argument("-t", "--touch", action="store_true", help="仅更新文件修改时间")
        parser.add_argument("-l", "--limit", type=int, default=50, help="对大文件的简略读取行数")
        parser.add_argument("-r", "--run", action="store_true", help="构建后运行http.server")
        parser.add_argument("-v", "--verbose", action="store_true", help="显示更详细的输出")
        parser.add_argument("-p", "--print-config", action="store_true", help="打印json配置")
        parser.add_argument("-w", "--watch", default=0.0, nargs="?", const=5.0, type=float,
                            help="自动循环运行相同命令")
        parser.add_argument("-e", "--emacs", action="store_true", help="Run Emacs as outputor")
        args = parser.parse_args()
        return vars(args) if args else {}

ARGS = Args()
formatter = orgreader2.HtmlExportVisitor()
emacs = EmacsBacken()

def update_file(file:Path) -> Doc:
    """更新文件"""
    if not file.is_file():
        pytools.print_err(f"[WARN] '{file}' 不是文件")
        return Doc()
    outputf = Path(re.sub(r"\.org$", ".html", str(file)))
    content = ""
    ret = Doc(link=str(outputf), title=str(outputf),
              desc="[NONUPDATE]")
    is_newer = ARGS.ignore_time or \
            (not outputf.exists() or outputf.stat().st_mtime < file.stat().st_mtime)
    if not is_newer and ARGS.no_build_home:
        return ret

    content = pytools.read_text(file).splitlines()
    if not is_newer and len(content) > (ARGS.limit or 100):
        content = content[:(ARGS.limit or 100)]    # 偷偷摸摸限制长度提高读取速度(实际上是在偷懒)
    if ARGS.verbose:
        print(f"[INFO] 正在处理文件 '{pytools.calculate_relative(file, ARGS.project_dir)}'")

    if ARGS.no_build_home and ARGS.emacs:
        doc = None
    else:
        doc = orgreader2.Document(content, str(file) )
        doc.setting["css_in_html"] = ""
        doc.setting["js_in_html"] = """\
<script>
window.MathJax = { tex: { ams: { multlineWidth: '85%' }, tags: 'ams', tagSide: 'right', tagIndent: '.8em' },
chtml: { scale: 1.0, displayAlign: 'center', displayIndent: '0em' },
svg: { scale: 1.0, displayAlign: 'center', displayIndent: '0em' },
output: { font: 'mathjax-modern', displayOverflow: 'overflow' } };
</script>
<script id="MathJax-script" async src="/theme/tex-mml-chtml.js"></script>"""
        date = re.sub(r"<(.*)>", r"\1", " ".join(doc.meta["date"]))
        date = re.sub(r"([^ ]*) [一|二|三|四|五|六|日]", r"\1", date)
        link = str(pytools.calculate_relative(outputf, ARGS.project_dir))
        ret = Doc(date, link,
                  " ".join(doc.meta["title"]) if doc.meta["title"] else file.stem,
                  " ".join(doc.meta["description"]))

    if outputf.is_dir():
        print(f"[ERROR] 输出文件名被文件夹占用 - {outputf}")
        return ret
    if is_newer:
        fname = pytools.calculate_relative(outputf, ARGS.project_dir)
        if not ARGS.touch:
            print(f"[INFO] 输出文件 - {fname}")
            if doc:
                outputf.write_text(doc.accept(formatter), encoding="utf8")
            else:
                emacs.update_file(file)
        elif outputf.is_file():
            print(f"[INFO] 更新文件时间 - {fname}")
            outputf.touch()
    return ret

def check_list(s:str, li:list|set) -> bool:
    """检查文件s是否在列表内"""
    for i in li:
        try:
            if re.search(i, s):
                return True
        except re.error as e:
            pytools.print_err(f"[WARN] regex: {e}")
    return False

class Blog:
    """basic class"""
    cfg_template = {
            "article_dir" : "post/",
            "static_sites" : [
                "about.org",
                "404.org"
                ],
            "generate_sites" : [
                "index.org",
                "timeline.org"
                ],
            "blacklist" : [],
            "whitelist" : [],
            "setupfile" : "setup.setup",
            "settings" : {
                "follow_link" : False,
                "tree_base_toc": 2,
                "tree_toc": 2,
                "tree_time": False,
                "save_generated_org_file": False,
                }
            }
    def __init__(self) -> None:
        self.cfg = self.cfg_template.copy()
        self.tree = {}
        if not pytools.load_json_cfg(self.cfg, ARGS.config):
            pytools.print_err("[WARN] 配置错误,退出")
            sys.exit()
    def fordir(self, d:Path) -> dict[str,dict|Doc]:
        """遍历文件夹并更新文件"""
        if not d.is_dir():
            return {}
        if not self.cfg["settings"]["follow_link"] and d.is_symlink():
            return {}
        filelist = {}
        for file in sorted(d.iterdir()):
            if file.is_dir():
                sublist = self.fordir(file)
                if sublist:
                    filelist[file.name] = sublist
                continue
            if file.suffix != ".org":
                continue
            ret = update_file(file)
            link = ret.link
            is_avaliable = not check_list(ret.link, self.cfg["blacklist"]) or\
                    check_list(ret.link, self.cfg["whitelist"])
            if link and is_avaliable:
                filelist[file.name] = ret
        return filelist
    def tree2str(self, tree:dict[str,dict|Doc], titlv=0, level=1, hide_time:bool=False) -> str:
        """将文件列表转为org字符串"""
        ret = ""
        for dirname,obj in tree.items():
            if isinstance(obj, dict):
                if titlv >= level:
                    ret += "*"*level+f" {dirname}\n"
                else:
                    ret += "  "*(level-1)+f"- {dirname}\n"
                ret += self.tree2str(obj, titlv, level+1, hide_time)+"\n"
                continue
            time_format = f" /[{obj.date or "NULL"}]/" if not hide_time else ""
            spacing = "  "*level
            ret += spacing+f"- [[./{obj.link}][*{obj.title}*]]{time_format}"
            ret += ("\\\\\n"+spacing+f"  {obj.desc}\n") if obj.desc else "\n"
        return ret
    def build_generated_page(self):
        """构建半动态生成界面"""
        s_tree = self.tree2str(self.tree,
                               titlv=self.cfg["settings"]["tree_toc"],
                               level=self.cfg["settings"]["tree_base_toc"],
                               hide_time=not self.cfg["settings"]["tree_time"])
        # content_index = index_template+re.sub(r"^\- ", "** ", list_to_str(tree,True), flags=re.M)
        timeline = pytools.squash_dict(self.tree, split='/')
        timeline = {k:v for k,v in sorted(timeline.items(), key=lambda x:x[1].date, reverse=True)}
        s_timeline = self.tree2str(timeline)
        content_timeline = [i[2:] for i in s_timeline.splitlines()]
        for i in range(2000, 2100):
            ind = 0
            has = False
            for ind,l in enumerate(content_timeline):
                if f"[{i}-" in l:
                    has = True
                    break
            if not has:
                continue
            content_timeline.insert(ind, f"* {i}")
        s_timeline = "\n".join(content_timeline)

        key = {"tree":s_tree,
               "timeline":s_timeline}
        for i in self.cfg["generate_sites"]:
            file : Path = ARGS.project_dir/i
            template_file = file.parent/(file.stem+".template"+file.suffix)
            if not template_file.is_file():
                f1 = pytools.calculate_relative(file, Path())
                f2 = pytools.calculate_relative(template_file, Path())
                pytools.print_err(f"[WARN] 文件'{f1}'找不到对应的模板文件'{f2}'")
                continue
            template = Template(pytools.read_text(template_file))
            if set(template.get_identifiers()) & set(["tree","timeline"]) and not self.tree:
                continue
            print(f"[INFO] 构建页面 {pytools.calculate_relative(file, ARGS.project_dir)}")
            file.write_text(template.safe_substitute(key), encoding="utf8")
            update_file(file)
            if not self.cfg["settings"]["save_generated_org_file"]:
                file.unlink(True)
    def build_static_page(self):
        """构建静态界面"""
        for i in self.cfg["static_sites"]:
            update_file(ARGS.project_dir/i)
    def run_once(self):
        """运行"""
        self.tree = self.fordir(ARGS.project_dir/self.cfg["article_dir"])
        if not ARGS.no_build_home:
            self.build_generated_page()
        self.build_static_page()

def main():
    """主函数"""
    blog = Blog()
    if not ARGS.config.is_file():
        pytools.print_err(f"[ERROR] 无法找到有效配置文件 - {ARGS.config}")
        return
    if ARGS.print_config:
        print(json.dumps(blog.cfg, indent='\t'))
        return
    if ARGS.add:
        p1 = ARGS.project_dir/blog.cfg["setupfile"]
        p2 = Path(ARGS.add)
        if p2.exists():
            print(f"文件或目录'{p2}'已存在")
            return
        setupf = pytools.calculate_relative(p1, p2)
        t = "#+TITLE: \n"
        t += "#+DESCRIPTION: \n"
        t += f"#+DATE: <{pytools.get_strtime(s=False)}>\n"
        t += f"#+SETUPFILE: {setupf}\n\n请输入文本\n"
        p2.write_text(t, encoding="utf8")
        print(f"已创建文件'{p2}'")
        return

    blog.run_once()
    if ARGS.watch > 1:
        print("[Watching...]")
    while ARGS.watch > 1:
        try:
            time.sleep(ARGS.watch)
            blog.run_once()
        except (EOFError, KeyboardInterrupt):
            print("[Quit Watching]")
            break

def run_server():
    """运行服务器"""
    port = 8080
    for i in range(port, port+100):
        try:
            with socketserver.TCPServer(("", i), http.server.SimpleHTTPRequestHandler) as httpd:
                print(f"[INFO] 服务器(WebUI)运行在 http://localhost:{i}/")
                print("[INFO] 浏览器或将自动打开")
                try:
                    webbrowser.open(f"http://localhost:{i}/")
                except ModuleNotFoundError:
                    print("[INFO] 无法自动打开浏览器")
                # print("按 Ctrl+C 停止服务器")
                try:
                    httpd.serve_forever()
                except KeyboardInterrupt:
                    print("\n[INFO] 服务器已停止")
        except OSError:
            continue
        break

if __name__ == "__main__":
    try:
        main()
    except (EOFError, KeyboardInterrupt):
        print("Stoppping...")
    emacs.stop()
    if ARGS.run:
        run_server()
