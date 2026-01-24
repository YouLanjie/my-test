#!/usr/bin/env python
# Created:2025.12.07
# 合并并导出一个或多个org文件为一个tex文件(套有缩印模板)
# 迁移自blogs

from string import Template
from pathlib import Path
import time
import json
import argparse
import re
import subprocess

import pytools
import orgreader2 as org2

try:
    from natsort import natsorted as mysorted
except ModuleNotFoundError:
    mysorted = sorted
    pytools.print_err("[WARN] no natsort")

class Template2(Template):
    """自定义模板类(变量名可含'.')"""
    delimiter = "#$"
    braceidpattern = "(?a:[._a-z][._a-z0-9]*)"

class TexExport(org2.TexExportVisitor):
    """导出 LaTeX with 缩印模板"""
    def visit_blockcode(self, node: org2.BlockCode) -> str:
        ret = r"\begin{lstlisting}"
        # 可能导致latex编译错误
        # if node.lang.lower() in ("c", "python", "java"):
            # ret += f"[language={node.lang.lower()}]"
        ret += "\n"
        if isinstance(node.line, org2.Strings):
            ret += node.line.s
        ret += "\\end{lstlisting}"
        return ret
    def visit_document(self, node: org2.Document) -> str:
        return node.root.accept(self)

class Config:
    """配置类"""
    cfg_template = {
            "title":"缩印文件默认标题",
            "author":"内容作者",
            "setting":{
                "gen_info":True,
                "mktitle":True,
                "mktoc":True,
                "ruler":False,    # 尺子，测试用
                "papersize":"a4paper",
                "fontsize":4.0,
                "fontsizes":{
                    "sections":{
                        "autocontrol":True,
                        "section":6.0,
                        "subsection":5.0,
                        "subsubsection":4.0,
                        },
                    "skip":0.1,
                    "footer":4.0,
                    },
                "fontname":"DengXian Light",
                "border":{
                    "left":0.7,
                    "right":0.7,
                    "top":0.5,
                    "bottom":0.9,  # 由于页码在该边距之外,所以要大点
                    "bindingoffset":.9,
                    "footskip":6.9, # pt
                    },
                "cols":9,
                "toc_cols":5,
                "col_gap":"1mm",
                },
            "filelist":{
                ".*":{
                    "ignore":[],
                    "add":[]
                    }
                },
            "output":"output_#${title}_#${setting.papersize}_#${setting.fontsize}pt_#${template.gtst}.tex",
            }
    latex_template = r"""% 生成于: #${template.generate_time}
\documentclass{article}
% 页边距需要考虑打印机实际情况
\usepackage[
	#${setting.papersize},
	left=#${setting.border.left}cm,
	right=#${setting.border.right}cm,
	top=#${setting.border.top}cm,
	bottom=#${setting.border.bottom}cm,
	footskip=#${setting.border.footskip}pt,  % 页码到正文的间距
	bindingoffset=#${setting.border.bindingoffset}cm,
	twoside,
]{geometry}
% \usepackage{footmisc}
% \usepackage{dblfnote}
% \usepackage{titling}   % 保留title等变量
\usepackage{fontspec}
\usepackage[hidelinks]{hyperref}
\usepackage{multicol}
\usepackage[UTF8]{ctex}
\usepackage{xcolor}
\usepackage{fancyhdr}  % 自定义脚注
\usepackage{titlesec}  % 控制标题格式
\usepackage{enumitem}  % 控制列表格式
\usepackage{tcolorbox} % 自定义quote环境格式用
\usepackage{listings}  % 自定义src环境格式用#${template.ruler}

% 多栏设置
\setlength{\columnsep}{#${setting.col_gap}}

% 缩小最大段落间距防止翻页时因为标题过度自动拉伸浪费空间
% 如果固定为0会导致每页底部无法对齐很难看
\setlength{\parskip}{0ex plus 0.00001ex}

% 脚注设置
\setlength{\footnotesep}{0.5\footnotesep} % 减少脚注之间的间距
\setlength{\skip\footins}{0.5\skip\footins} % 减少脚注与正文的间距
\renewcommand{\footnote}[1]{{【脚注：#1】}} % 替换原生脚注

% 字体大小设置
#${template.setfont}

% 使用 titlesec 重新定义标题格式，保持目录功能
\titleformat{\section}
  {\fontsize{#${setting.fontsizes.sections.section}}{0.1}\selectfont\bfseries}
  {【\arabic{section}.】}
  {0pt}
  {}
\titleformat{\subsection}
  {\fontsize{#${setting.fontsizes.sections.subsection}}{0.1}\selectfont\bfseries}
  {【\arabic{section}.\arabic{subsection}.】}
  {0pt}
  {}
\titleformat{\subsubsection}
  {\fontsize{#${setting.fontsizes.sections.subsubsection}}{0.1}\selectfont\bfseries}
  {【\arabic{section}.\arabic{subsection}.\arabic{subsubsection}.】}
  {0pt}
  {}
% 移除标题间距
\titlespacing*{\section}{0pt}{0pt}{0pt}
\titlespacing*{\subsection}{0pt}{0pt}{0pt}
\titlespacing*{\subsubsection}{0pt}{0pt}{0pt}

% 页脚设置
\pagestyle{fancy}
\fancyfoot[C]{\setsmallf{#${setting.fontsizes.footer}}\thepage}
\fancyfoot[RO]{\setsmallf{#${setting.fontsizes.footer}}【#${title}】|【\leftmark】}
\fancyfoot[LE]{\setsmallf{#${setting.fontsizes.footer}}【\leftmark】|【#${title}】}
\fancyfoot[LO]{\setsmallf{#${setting.fontsizes.footer}}【#${template.generate_time}】}
\fancyfoot[RE]{\setsmallf{#${setting.fontsizes.footer}}【#${template.generate_time}】}

% 设置列表的间距
\setlist{noitemsep,leftmargin=1em,labelsep=0.1em,topsep=0.1em,partopsep=0.1em}

% 自定义quote环境
\tcbuselibrary{skins,breakable}
% 定义新环境：左侧有竖线，整体左缩进0.5em，段首缩进2em
\renewtcolorbox{quote}[1][]{
	breakable,
	enhanced,
	frame hidden,
	colback=white,
	left=0.5em,
	right=0pt,
	top=0pt,
	bottom=0pt,
	sharp corners,
	before skip=0.5\baselineskip,
	after skip=0.5\baselineskip,
	overlay={
		\draw[line width=0.3pt, black]([shift={(0.5em, -0.5em)}]frame.north west) -- ([shift={(0.5em, 0.5em)}]frame.south west);
	},
	parbox=false,
	#1
}
% 设置环境内段落缩进
\usepackage{etoolbox}
\AtBeginEnvironment{quote}{\setlength{\parindent}{2em}}

% 自定义src环境
\lstset{
    breaklines=true,
    breakatwhitespace=false,     % 允许在任意位置换行
    breakindent=0em,             % 换行缩进（由于设置了箭头符号故这里设置为0）
    % prebreak=\mbox{\textcolor{red}{$\hookleftarrow$}\space},
    % postbreak=\mbox{\space\textcolor{red}{$\hookrightarrow$}},
    numbers=left,      % 行号位置
    numbersep=0.5em,   % 行号距左侧宽度
    xleftmargin=1em,   % 左侧整体偏移
    %numberstyle=\tiny\color{gray},
    %basicstyle=\ttfamily\small,
}

% 重定义verse环境
\renewenvironment{verse}
  {\begin{quote}【Verse】\par}
  {\end{quote}}

% 文档信息
\hypersetup{
  pdftitle={#${title}},
  pdfauthor={#${author}},
  hidelinks,
  pdfcreator={LaTeX via a python script}}
\title{#${title}}
\author{#${author}}
\date{#${template.generate_time}}

\begin{document}
% 分栏、字体设置
\setsmallf{#${setting.fontsize}}
#${template.mktitle}
#${template.mktoc}
\begin{multicols}{#${setting.cols}}

#${template.gen_info}

#${template.body}

\end{multicols}
\end{document}
"""
    latex_template_toc = r"""\begin{multicols}{#${setting.toc_cols}}
\tableofcontents
\end{multicols}
"""
    latex_template_info = "\n\n\\noindent\n".join(r"""\section{页面设置}
生成于:#${template.generate_time}
纸张类型：#${setting.papersize}
边距：上#${setting.border.top}cm,下#${setting.border.bottom}cm,左#${setting.border.left}cm,右#${setting.border.right}cm,装订偏移：#${setting.border.bindingoffset}cm,页脚：#${setting.border.footskip}pt,栏距#${setting.col_gap},
字体大小：题一:#${setting.fontsizes.sections.section}pt,题二:#${setting.fontsizes.sections.subsection}pt,题三:#${setting.fontsizes.sections.subsubsection}pt,页脚:#${setting.fontsizes.footer}pt,正文:#${setting.fontsize}pt
字词统计：#${counter.words}""".splitlines())
    latex_template_fgruler = r"""
\usepackage[type=user]{fgruler}
\fgrulerdefuser{
    \ifnum\value{page}<3\relax
        \fgrulertype{\fgrulerunit}{alledges}
    \fi
}
"""
    latex_template_setfont = [r"""
\setmainfont{#${setting.fontname}}
\newcommand{\setsmallf}[1]{\fontsize{#1pt}{#${setting.fontsizes.skip}}\selectfont\CJKfontspec{#${setting.fontname}}}
""",r"""
\newcommand{\setsmallf}[1]{\fontsize{#1pt}{#${setting.fontsizes.skip}}\selectfont}
"""]
    def __init__(self, cfg_f:Path):
        self.cfg_f = cfg_f
        self.cfg = self.cfg_template
        cfg = {}
        if cfg_f.is_file():
            try:
                cfg = json.loads(cfg_f.read_bytes() or "{}")
            except json.JSONDecodeError as e:
                pytools.print_err(str(e))
        if not isinstance(cfg, dict):
            pytools.print_err(f"Err type: {type(cfg)}")
            cfg = {}
        pytools.merge_dict(self.cfg, cfg, True)
        basefontsize = self.cfg["setting"]["fontsize"]
        fontsizes = self.cfg["setting"]["fontsizes"]
        if fontsizes["sections"]["autocontrol"]:
            fontsizes["sections"]["section"] = basefontsize + 2
            fontsizes["sections"]["subsection"] = basefontsize + 1
            fontsizes["sections"]["subsubsection"] = basefontsize
            # fontsizes["footer"] = basefontsize
    def print_config_template(self):
        """打印模板json"""
        print(json.dumps(self.cfg_template, ensure_ascii=False, indent='\t'))
    def get_filelist(self) -> tuple[list,dict[str,list[int]]]:
        """获取文件列表"""
        filelist = set()
        whitelist = {}
        home = self.cfg_f.parent
        for inp_dir in self.cfg["filelist"]:
            if (home/inp_dir).is_dir():
                print(f"[INFO] searching '{home/inp_dir}'")
                fl = set((home/inp_dir).glob("**/*.org"))
                wl = pytools.process_filelist(self.cfg["filelist"][inp_dir]["add"])
                bl = {home/inp_dir/i for i in self.cfg["filelist"][inp_dir]["ignore"]}
                whitelist.update({(home/inp_dir/i).resolve():j for i,j in wl.items()})
                wl = {home/inp_dir/i for i in wl}
            else:
                print(f"[INFO] adding '{home/inp_dir}'")
                wl = pytools.process_filelist([home/inp_dir])
                fl = set()
                bl = set()
                whitelist.update({((home/inp_dir).parent/i).resolve():j for i,j in wl.items()})
                wl = {(home/inp_dir).parent/i for i in wl}
            filelist |= {i.resolve() for i in ((fl-bl)|wl)}
        filelist = mysorted(filelist)
        return filelist, whitelist
    def get_merged(self) -> tuple[str,str]:
        """生成org文件内容"""
        filelist, whitelist = self.get_filelist()
        home = self.cfg_f.parent
        content = ""
        print("[INFO] filelist:")
        exportor = TexExport()
        for i in filelist:
            file = pytools.calculate_relative(i, home)
            filename = str(file)
            if i in whitelist:
                filename += f" {whitelist[i]}"
            print(f"       - {filename}")
            s = i.read_text(encoding="utf-8").splitlines()
            if i in whitelist:
                nums = whitelist[i]
                if len(nums) == 1:
                    s = s[nums[0]:]
                elif len(nums) == 2 and nums[0] < nums[1]:
                    s = s[nums[0]:nums[1]]
                print("         > "+"\n         > ".join(s[:5]))
            s = "\n".join(s)
            li = re.findall(r"^(\*+) (.*)", s, re.M)
            levels = sorted({len(i[0]) for i in li})
            if levels and min(levels) > 1 and max(levels) < 4:
                s = f"* {i.name}\n" + s
            s = s.splitlines()
            s = ["#+begin_verse", f"【FILE:{filename}】","#+end_verse",""] + s
            doc = org2.Document(
                    s,
                    file_name=str(file),
                    setting={"verbose_msg":True})
            content += doc.accept(exportor)
        return (f"字数：{len([i for i in content if i not in " \t\n\r\\{}[]"])/1000}k,\n"
                f"中文字符数：{len([i for i in content if len(i.encode()) > 1])/1000}k,\n"
                f"覆盖字符数：{len(set(content))},\n"
                f"行数：{len([i for i in content.splitlines() if i])},\n",
                content)
    def generate_template_dict(self, words = "None") -> dict:
        """生成用于模板的词典"""
        k : dict[str, str|int|float] = {
                "template.gtst": time.strftime("%Y%m%d_%H%M%S"),
                "template.generate_time": pytools.get_strtime(),
                }
        k.update(pytools.squash_dict(self.cfg))
        k["counter.words"] = words
        k["template.mktitle"] = r"\maketitle{}" if k["setting.mktitle"] else ""
        k["template.mktoc"] = Template2(self.latex_template_toc).safe_substitute(k) \
                if k["setting.mktoc"] else ""
        k["template.ruler"] = self.latex_template_fgruler if k["setting.ruler"] else ""
        k["template.gen_info"] = Template2(self.latex_template_info).safe_substitute(k) \
                if k["setting.gen_info"] else ""

        k["template.setfont"] = Template2(
                self.latex_template_setfont[0 if k["setting.fontname"] != "CTEX_DEFAULT" else 1]
                ).safe_substitute(k)
        return k
    def gen_toc_str(self, content:str) -> str:
        "手动生成填充空间用的目录文件（减少一次xelatex编译）"
        tocs = re.findall(r"\\((?:sub){0,2}section){(.*)}", content)
        if not tocs:
            return ""
        dic = ["section","subsection","subsubsection"]
        counter = [0, 0, 0]
        s = ""
        for i in tocs:
            counter[dic.index(i[0])] += 1
            for j in range(dic.index(i[0])+1,3):
                counter[j] = 0
            ind = ".".join([str(j) for j in counter if j])
            s += "\\contentsline {%s}{\\numberline {%s}%s}{1}{%s.%s}%%\n" % \
                    (i[0], ind, i[1], i[0], ind)
        return s

def main():
    """主函数"""
    args = parse_arg()
    config = Config(args.config)
    if args.print_config:
        config.print_config_template()
        return
    if args.print_template:
        print(Template2(config.latex_template).safe_substitute(
            config.generate_template_dict()))
        return
    if not config.cfg_f.is_file():
        pytools.print_err(f"[WARN] 配置文件 '{config.cfg_f}' 不存在")
        if args.title:
            config.cfg["title"] = args.title
        if args.author:
            config.cfg["author"] = args.author
        for i in args.filelist:
            config.cfg["filelist"][i] = {}
        return
    print("[INFO] Config:")
    __import__('pprint').pprint(config.cfg)
    w,c = config.get_merged()
    temp_dict = config.generate_template_dict(w)
    outputf = config.cfg_f.parent/Template2(config.cfg["output"]).safe_substitute(temp_dict)

    temp_dict["template.body"] = c
    c = Template2(config.latex_template).safe_substitute(temp_dict)
    outputf.write_text(c, encoding="utf8")
    print(f"[INFO] 输出文件为'{outputf}'")
    if not args.run:
        return
    print(f"[INFO] 自动构建'{outputf}'")
    try:
        if config.cfg["setting"]["mktoc"]:
            toc_outputf = outputf.parent/(outputf.stem+".toc")
            toc_outputf.write_text(config.gen_toc_str(c))
            subprocess.run(["xelatex", outputf], check=True)
        subprocess.run(["xelatex", outputf], check=True)
    except KeyError as e:
        pytools.print_err(f"[WARN] KeyError: {e}")
    except (FileNotFoundError, subprocess.CalledProcessError) as e:
        pytools.print_err(f"[WARN] 执行命令出现错误：{e}")

def parse_arg() -> argparse.Namespace:
    """解释参数"""
    parser=argparse.ArgumentParser(description="合并org文件并生成缩印用的tex文件")
    parser.add_argument("-c", "-i", "--config", type=Path, default=Path("config.json"),
                        help="指定配置文件(不可用时命令行配置才有用)")
    parser.add_argument("-C", "--print-config", action="store_true", help="打印配置文件模板")
    parser.add_argument("-p", "--print-template", action="store_true", help="打印latex模板")
    parser.add_argument("-r", "--run", action="store_true", help="auto run command")
    parser.add_argument("-t", "--title", type=str, help="指定标题")
    parser.add_argument("-a", "--author", type=str, help="指定作者")
    parser.add_argument("-f", "--filelist", action="append", help="增设文件列表")
    return parser.parse_args()

if __name__ == "__main__":
    main()
