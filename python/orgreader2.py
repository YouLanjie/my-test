#!/usr/bin/python
"""解析org文件"""

# 标准库
from pathlib import Path
import datetime
import os
import time
import re
import gzip
import importlib
import threading
import subprocess
from html import escape
from abc import ABC, abstractmethod
from string import Template

try:
    from . import pytools
except ImportError:
    import pytools

class ExportVisitor(ABC):
    """导出基类"""
    def visit_comment(self, node: "Comment") -> str:
        del node
        return ""
    def visit_meta(self, node: "Meta") -> str:
        del node
        return ""
    def visit_footnotes(self, node: "Footnotes") -> str:
        del node
        return ""
    def visit_blockcomment(self, node: "BlockComment") -> str:
        del node
        return ""

    @abstractmethod
    def visit_strings(self, node: 'Strings', li: list) -> str:
        return ""
    @abstractmethod
    def visit_root(self, node: 'Root') -> str:
        return ""
    @abstractmethod
    def visit_text(self, node: "Text") -> str:
        return ""
    @abstractmethod
    def visit_title(self, node: "Title") -> str:
        return ""
    @abstractmethod
    def visit_titleoutline(self, node: "TitleOutline") -> str:
        return ""
    @abstractmethod
    def visit_footnote(self, node: "Footnote") -> str:
        return ""
    @abstractmethod
    def visit_listitem(self, node: "ListItem") -> str:
        return ""
    @abstractmethod
    def visit_list(self, node: "List") -> str:
        return ""
    @abstractmethod
    def visit_blockcode(self, node: "BlockCode") -> str:
        return ""
    @abstractmethod
    def visit_blockexport(self, node: "BlockExport") -> str:
        return ""
    @abstractmethod
    def visit_blockexample(self, node: "BlockExample") -> str:
        return ""
    @abstractmethod
    def visit_blockverse(self, node: "BlockVerse") -> str:
        return ""
    @abstractmethod
    def visit_blockquote(self, node: "BlockQuote") -> str:
        return ""
    @abstractmethod
    def visit_blockcenter(self, node: "BlockCenter") -> str:
        return ""
    @abstractmethod
    def visit_blockproperties(self, node: "BlockProperties") -> str:
        return ""
    @abstractmethod
    def visit_table(self, node: "Table") -> str:
        return ""
    @abstractmethod
    def visit_document(self, node: "Document") -> str:
        return ""

def _get_strings_pattern(s:str,blank_char=" )-,") -> re.Pattern:
    return re.compile(f"{s}([^ ].*?(?<! )){s}(?=[{blank_char}]|\n|$)",
                      re.DOTALL)

class Strings:
    """行内字符串类，提供行内格式转换"""
    pattern = [
            ("link", re.compile(r"^\[\[((?:[^\[\]\\]|\\.)*)(?:\]\[(.*?))?\]\]"), "["),
            ("timestamp",
             re.compile(r"([\[<])(\d{4})-(\d{2})-(\d{2})(?:\D*(?<= )(\d\d?):(\d{2}).*|.*)[\]>]"), "[<"),
            ("fn", re.compile(r"\[fn:([^]]*)\]"), "["),
            ("code", _get_strings_pattern("="), "="),
            ("code", _get_strings_pattern("~"), "~"),
            ("italic", _get_strings_pattern("/"), "/"),
            ("bold", _get_strings_pattern(r"\*"), "*"),
            ("del", _get_strings_pattern(r"\+"), "+"),
            ("underline", _get_strings_pattern("_"), "_"),
            ]
    img_exts=["png", "jpg", "jpeg", "gif", "webp", "svg", "avif"]
    rules = {"code":("<code>","</code>"),
             "italic":("<i>", "</i>"),
             "bold":("<b>", "</b>"),
             "del":("<del>", "</del>"),
             "underline":("<span class=\"underline\">", "</span>"),}
    def __init__(self, s:str, node, parse_able : bool = True) -> None:
        self.s = s
        self.parse_able = parse_able
        self.cache:dict[str,list[str]]= {}
        if isinstance(node, Root):
            self.upward = node
    def _parse_link_find_chapter(self, s:str) -> tuple[str|None,str]:
        doc = self.upward.document
        level = ""
        obj = None
        for i in doc.status["table_of_content"]:
            if i[-1]["title"].s != s:
                continue
            level = "-".join(str(j) for j in i[doc.status["lowest_title"]-1:-1])
            obj = f"#org-title-{level}"
        return (obj, level.replace("-","."))
    def _parse_link(self, ret, li, last):
        mode = "link"
        link = re.sub(r"\\([][])", r"\1", ret.group(1))
        match = re.match(r"((?:http[s]?:)?)(.*)",link)
        if not match:
            self.log(f"re模块出问题了?(匹配为空) - {link}")
            return
        alt = ret.group(2)
        original_link = link
        # 非`http[s]:`开头(非网络链接)
        if not match.group(1):
            obj_link, level = self._parse_link_find_chapter(link)
            if obj_link:
                link = obj_link
                if not alt:
                    alt = level
            elif not match.group(2).startswith("#"):
                # 非章节名且非#开头
                file_path = re.match(r"((?:file:)?)(.*)",link)
                file_path = file_path.group(2) if file_path else link
                base_path=Path(self.upward.document.setting["file_name"]).parent if\
                        self.upward.document.setting["file_name"] else Path()
                if not link.startswith("./") and not Path(file_path).is_file():
                    self.log(f"无法解决的链接: {link}", "ERROR")
                elif self.upward.document.setting.get("verbose_msg")\
                        and link.startswith("./") and not (base_path/file_path).is_file():
                    self.log(f"无法找到链接文件: {link}", "WARN")
                for j in "%[]\"\\ #?|<>\t":
                    if j not in link:
                        continue
                    link = link.replace(j, f"%{hex(ord(j))[2:].upper().zfill(2)}")
        ret = re.match(r".*/(.*\.(?:"+"|".join(self.img_exts)+r"))",link,re.I)
        if not alt and ret:
            mode,alt = self._parse_img(ret, last)
        elif not alt:
            alt = original_link
        if alt and mode != "img":
            alt = self.orgtext_to_list(alt , True)
        li.append([mode, link, alt])
    def _parse_img(self, ret, last):
        """处理为img的link，返还alt后再转list"""
        mode = "img"
        alt = ret.group(1)
        caption = self.upward.document.meta["caption"].get(self.upward.start-1)
        if not isinstance(self.upward, Title) and caption and last == 0:
            mode = "figure"
            alt = caption
        return mode, alt
    def _parse_timestamp(self, ret:re.Match, li:list):
        chr_typ, year, month, day, hours, minutes = ret.groups()
        enable_hm = bool(hours and minutes)
        year, month, day, hours, minutes = int(year), int(month), int(day), \
                int(hours or 0), int(minutes or 0)

        day -= 1
        year += (month-1) // 12
        month = ([12]+list(range(1,12)))[month % 12]
        timestamp = minutes*60 + hours*(60**2)
        timestamp += datetime.datetime(year,month,1).timestamp()+day*(60**2*24)
        s = pytools.get_strtime(datetime.datetime.fromtimestamp(timestamp),h=enable_hm,m=enable_hm,s=False)
        chr_typ2 = "]" if chr_typ == "[" else ">"
        s = f"{chr_typ}{s}{chr_typ2}"
        li.append(["timestamp", s])
    def orgtext_to_list(self, s:str, is_sub = False) -> list[str]:
        """解释行内字符串变成数组"""
        if not is_sub and self.cache.get(s):
            return self.cache.get(s) or []
        li = []
        i = 0
        last = i
        patterns = [j for j in self.pattern if \
                [k for k in j[2] if k in s]]
        if not patterns:
            i = len(s)
        while patterns and i < len(s):
            ret = None
            current_pattern = None
            for current_pattern in patterns:
                ret = current_pattern[1].match(s[i:])
                if ret:
                    break
            if not ret or not current_pattern:
                i+=1
                continue

            # 识别前一字符是否满足条件（前后条件还不一样）
            if current_pattern[0] not in ("link", "fn") and i-1 > 0 \
                    and s[i-1] not in " (-\n":
                i+=1
                continue
            if last != i and s[last:i].strip():
                # 补充增加前面的纯文本内容
                li.append(s[last:i])
            last = i
            i+=ret.span()[1]
            if current_pattern[0] == "link":
                self._parse_link(ret, li, last)
            elif current_pattern[0] == "timestamp":
                self._parse_timestamp(ret, li)
            elif current_pattern[0] == "code":
                li.append([current_pattern[0], [ret.group(1)]])
            elif current_pattern[0] == "fn":
                li.append([current_pattern[0], ret.group(1)])
            else:
                # 其他杂项并嵌套调用
                li.append([current_pattern[0], self.orgtext_to_list(ret.group(1), True)])
            last = i
        if last != i and re.findall(r"[^ ]", s[last:i]):
            # 补充增加前面的纯文本内容(若非空)
            li.append(s[last:i])
        if not is_sub:
            self.cache[s] = li
        return li
    def get_separator_between(self, ch1:str, ch2:str) -> str:
        """获取合并两行字符串的分隔符(空格是否)"""
        if not ch1:
            return ""
        if not ch2:
            return ""
        # 均为中文
        if ord(ch1[-1]) > 127 and ord(ch2[0]) > 127:
            return ""
        if ch1[-1] == "\n":
            return ""
        return " "
    def get_pre_text(self) -> str:
        """获取预处理文字"""
        lines = self.s.splitlines()
        text = ""
        for i in lines:
            match = re.match(r"(.*)(?<!\\)\\\\\s*$", i)
            if match:
                i = match.group(1) + "\n"
            text+=f"{self.get_separator_between(text, i)}{i}"
        return text
    def log(self, info = "", lv="WARN"):
        """打印日志"""
        self.upward.log(info, lv)
    def accept(self, visitor: ExportVisitor) -> str:
        """将给定的已经分好的列表转回"""
        method_name = f"visit_{self.__class__.__name__.lower()}"
        if not hasattr(visitor, method_name):
            return ""
        if not self.parse_able:
            return re.sub("\n", " ", self.s)
        text = self.get_pre_text()
        return getattr(visitor, method_name)(self, self.orgtext_to_list(text))


class Root:
    """根节点/节点父类"""
    re_match = re.compile("")
    re_end = re.compile(r"^([*]+) +(.*)", re.I)
    def __init__(self, document, match: re.Match|None = None) -> None:
        if not isinstance(document, Document):
            return
        self.opt = {"childable": True, "breakable": True,
                    "printable":True}
        self.document = document
        self.start = document.current_line
        if document.lines:
            line = document.lines[document.current_line]
            self.line = Strings(line, self)
            if not match:
                match = self.re_match.match(line)
        else:
            self.line = Strings("", self)
        self.match = match
        self.end_offset = 0
        self.child :list[Root] = []
        self.current = self
    def checkend(self, lines:list[str], i:int) -> tuple[bool,int]:
        """检查结构是否结束"""
        if not self.opt["childable"] and self.re_end:
            return False, i
        match = self.re_end.match(lines[i])
        ret = i
        self.document.current_line = i
        if self.end_condition(match):
            # 正常(内部引发)关闭
            ret = self.end(i, True)
            if self.current != self and self.current.start == ret:
                self.remove(self.current)
            self.current = self
            return ret>=i,ret+(self.end_offset if ret>=i else 0)
        if self.current != self:
            stat,ret = self.current.checkend(lines, i)
            if stat:
                self.current = self
        return False,ret
    def end(self, i:int, is_normal_end:bool) -> int:
        """进行层级关闭通知，返回落点(<i则倒退)"""
        if hasattr(self, "match"):
            del self.match
        if not is_normal_end and not self.opt["breakable"]:
            return self.start
        # 由上级关闭引起的关闭
        return i if self.current == self else self.current.end(i, False)
    def add(self, obj):
        """将obj添加到当前操作末端"""
        if not self.current.opt["childable"]:
            return
        if self.current == self:
            self.child.append(obj)
            if obj.opt["childable"]:
                self.current = obj
        else:
            self.current.add(obj)
        return
    def remove(self, obj):
        """删除仅当前节点子项中的obj"""
        if not self.opt["childable"]:
            return
        if obj not in self.child:
            return
        self.child.remove(obj)
    @property
    def _summary(self):
        return f"('{self.line.s}')"
    def log(self, info = "", lv="WARN"):
        """打印日志"""
        warntext = f"{lv} [{self.document.setting["file_name"]}:{self.start+1}] "
        warntext += info
        pytools.print_err(warntext)
    def to_node_tree(self) -> str:
        """返回节点树"""
        ret = str(self)
        if self.line:
            ret += self._summary
        if self.child:
            ret += "\n"
        for i in self.child:
            for j in i.to_node_tree().splitlines():
                ret += f"{self.document.setting["indent_str"]}{j}\n"
        return ret
    def end_condition(self, match: re.Match | None) -> bool:
        """For Sub class"""
        if match is None:
            return False
        if not isinstance(self.current, TitleBase):
            return True
        return False
    def accept(self, visitor: ExportVisitor) -> str:
        """return str get from visitor"""
        # 根据节点类型自动匹配访问者方法
        method_name = f"visit_{self.__class__.__name__.lower()}"
        if hasattr(visitor, method_name):
            self.document.current_line = self.start
            return getattr(visitor, method_name)(self)
        # 默认返回空字符串（适配未实现的节点类型）
        return ""
    def search(self, key, opt="father"):
        """search for node"""
        if isinstance(key, int):
            if self.start > key:
                return None
            if self.start == key:
                return self
            i = 0
            while i < len(self.child):
                if self.child[i].start > key and i > 0:
                    return self.child[i-1].search(key, opt)
                i += 1
            if self.child:
                return self.child[i-1].search(key, opt)
            return None

        if isinstance(key, Root) and key in self.child:
            if opt == "father":
                return self
            if opt == "last":
                if self.child.index(key)-1 >= 0:
                    return self.child[self.child.index(key)-1]
                return None
            if opt == "next":
                if self.child.index(key)+1 < len(self.child):
                    return self.child[self.child.index(key)+1]
                return None
            return None
        for i in self.child:
            ret = i.search(key, opt)
            if ret:
                return ret
        return None

class TextBase(Root):
    """文本基类"""
    re_end = None
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.opt["childable"] = False
        self.line.s = re.sub(r"^ *", "", self.line.s)

class Text(TextBase):
    """文本类"""

class Comment(Root):
    """注释行"""
    re_match = re.compile(r"^[ ]*#[ ]+(.*)", re.I)
    re_end = None
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.opt["childable"] = False
        self.opt["printable"] = False

class Meta(Root):
    """元数据"""
    re_match = re.compile(r"^[ ]*#\+([^: ]*)(?! ):[ ]*(.*)", re.I)
    re_end = None
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.opt["childable"] = False
        self.opt["printable"] = False
        match = self.match
        if match is None:
            return
        self.key = match.group(1).lower()
        self.value = match.group(2)
        if self.key not in self.document.meta:
            return
        if isinstance(self.document.meta[self.key], list):
            self.document.meta[self.key].append(self.value)
        elif isinstance(self.document.meta[self.key], str):
            self.document.meta[self.key] = self.value

        if self.key == "setupfile":
            if self.value in self.document.status["setupfiles"]:
                return
            self._load_sub_setupfile()
        elif self.key == "seq_todo":
            self._process_seq_todo()
            return
        elif self.key == "options":
            self._process_options()
            return
        elif self.key == "include":
            self._load_include_file()
        elif self.key in ("caption", "name"):
            self.document.meta[self.key][self.start] = self.value
    def _load_include_file(self):
        incf=Path(self.document.setting["file_name"]).parent if\
                self.document.setting["file_name"] else Path()

        i = self.value
        li : list[tuple[bool,str]] = []
        s = ""
        is_str = False
        for j,k in enumerate(i):
            if is_str:
                s += k
            elif k != " ":
                s += k
            if (j <= 0 or i[j-1] == " ") and not is_str and k == '"':
                is_str = True
            if (j+1==len(i) or (j+1<len(i) and i[j+1] == " ")) and is_str and k == '"':
                is_str = False
                li.append((True, s[1:-1]))
                s = ""
                continue
            if s and (k == " " and not is_str) or j+1==len(i):
                li.append((False, s))
                s = ""
        if not li:
            return
        incf=incf/li[0][1]
        if not incf or not Path(incf).is_file():
            return

        args = {"search":"", "block":[],
                ":minlevel":"", ":lines":[]}
        li = li[1:]
        i = 0
        while i < len(li):
            if li[i][1] == ":lines" and i+1 < len(li):
                s = li[i+1][1].split("-")
                if not s or len(s) > 2:
                    i+=1
                    continue
                try:
                    s = [int(i or -1) for i in s]
                    if set(s) == {-1}:
                        s = []
                except ValueError:
                    s = []
                args[":lines"] = s
                i += 1
            elif li[i][1] == "src":
                args["block"] = ["src"]
                if i+1 < len(li) and (li[i+1][0] or not li[i+1][1].startswith(":")):
                    args["block"] += [li[i+1][1]]
            elif li[i][1] == "quote":
                args["block"] = ["quote"]
            i+=1
            
        s = Path(incf).read_text(encoding="utf8").splitlines()
        if len(args[":lines"]) == 1:
            s = [s[args[":lines"][0]]]
        elif len(args[":lines"]) == 2:
            s1 = args[":lines"][0]
            s1 = 0 if s1 < 0 else s1
            s2 = args[":lines"][1]
            if s2 > s1:
                s = s[s1:s2]
            else:
                s = s[s1:]
        if args["block"]:
            blk = args["block"][0]
            arg = "".join(args["block"][1:])
            if arg:
                arg = " "+arg
            if blk == "src":
                i = 0
                while i < len(s):
                    s[i] = re.sub(r"^( *)([*,]|#\+)", r"\1,\2", s[i])
                    i+=1
            s = ["",f"#+begin_{blk}{arg}"]+s+[f"#+end_{blk}",""]

        self.document.lines = self.document.lines[:self.start+1]+\
                s+\
                self.document.lines[self.start+1:]
    def _load_sub_setupfile(self):
        content = ""
        setupfile=Path(self.document.setting["file_name"]).parent if self.document.setting["file_name"] else Path()
        setupfile=setupfile/self.value
        setupfile_name = ""
        if setupfile.is_file():
            content = setupfile.read_text(encoding="utf8")
            setupfile_name = str(setupfile)
        elif re.match(r"http[s]?://.+", self.value):
            try:
                req = importlib.import_module("requests").get(self.value, timeout=3)
                self.log(f"在文件中插入外部链接可能拖慢转译速度({req.elapsed})[{self.value}]")
                if req.status_code == 200:
                    req.encoding = req.apparent_encoding
                    content = req.text
                else:
                    self.log(f"加载文件异常，状态码: {req.status_code}")
            except ModuleNotFoundError:
                self.log("模块requests不可用，无法调用网络setupfile")
            except (importlib.import_module("requests").exceptions.RequestException, IOError) as e:
                self.log(f"加载文件错误: {e}")
                return
        else:
            self.log(f"异常文件名提示: '{self.value}'")
        self.document.status["setupfiles"].append(self.value)
        content = str(content).splitlines()
        doc = Document(content,
                       file_name=setupfile_name,
                       setupfiles=self.document.status["setupfiles"])
        self.document.status["setupfiles"]=list(\
                set(self.document.status["setupfiles"])&\
                set(doc.status["setupfiles"])
            )
        for key,value in doc.meta.items():
            if isinstance(self.document.meta[key], list):
                self.document.meta[key] += value
            elif value != "":
                self.document.meta[key] = value
    def _process_seq_todo(self):
        value = self.value.split("|")
        todo, done = [], []
        if len(value) == 1:
            done = value[0].split(" ")
        elif len(value) == 2:
            todo, done = value[0].split(" "), value[1].split(" ")
        else:
            todo = value[0].split(" ")
            done = " ".join(value[1:]).split(" ")
        while "" in todo:
            todo.remove("")
        while "" in done:
            done.remove("")
        self.document.meta["seq_todo"]["todo"]+=[re.sub(r"\(.*\)", "", i) for i in todo]
        self.document.meta["seq_todo"]["done"]+=[re.sub(r"\(.*\)", "", i) for i in done]
    def _process_options(self):
        is_error = False
        keys = None
        options:list[str] = self.value.split()
        table = {"t":True, "nil":False}
        for option in options:
            if is_error:
                self.log(f"错误的OPTIONS: {keys}", "ERROR")
                is_error = False
            keys = option.lower().split(":")
            if len(keys) != 2:
                return
            key, value = keys
            try:
                value = int(value)
            except ValueError:
                if value not in table:
                    is_error = True
                    continue
                value = table[value]
            self.document.meta["options"][key] = value
            if is_error:
                self.log(f"错误的OPTIONS: {keys}", "ERROR")
                is_error = False
        if is_error:
            self.log(f"错误的OPTIONS: {keys}", "ERROR")

class TitleBase(Root):
    """标题基类"""
    re_match = re.compile(r"^([*]+) +(.*?(?= +:.*:|$))(?: +:(.*):)?", re.I)
    re_end = re.compile(r"^([*]+) +(.*?(?= +:.*:|$))(?: +:(.*):)?", re.I)
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        match = self.match
        if match is None:
            return
        self.id = []
        self.level = len(match.group(1))
        self.comment = False
        self.line = Strings(match.group(2), self)
        self.todo = []
        self.tag = Strings("", self)
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if len(match.group(1)) <= self.level:
            return True
        # 针对下级
        if self.current == self:
            return False
        if isinstance(self.current, Block):
            return True
        return False
    @property
    def _summary(self):
        return f"(lv:{self.level}, hid:{self.comment}, todo:{self.todo}, '{self.line.s}')"

class Title(TitleBase):
    """标题"""
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        match = self.match
        if match is None:
            return
        self.tag = Strings(match.group(3), self)
        self.document.status["lowest_title"] = \
                min(self.document.status["lowest_title"], self.level)
    def set_status(self, comment:bool = False):
        """设置状态(TODO, COMMENT),更新目录"""
        last = (self.document.status["table_of_content"] or [[]])[-1]
        li = []
        num = 0
        for num in range(self.level):
            if num >= len(last)-1:
                li.append(0)
                continue
            li.append(last[num])
        li[num] += 1

        todo = None
        key = ""
        ind = 0
        self.comment = comment
        for ind,key in enumerate(self.line.s.split()):
            if key == "COMMENT":
                self.comment = True
                continue
            if todo:
                break
            if key in self.document.meta["seq_todo"]["todo"]:
                todo = ["todo", key]
            elif key in self.document.meta["seq_todo"]["done"]:
                todo = ["done", key]
            else:
                break
        if todo:
            self.line.s = " ".join(self.line.s.split()[ind:])
            self.todo = todo
        li.append({"title":self.line, "tag":self.tag.s,
                   "todo":todo, "start":self.start, "comment":self.comment})
        self.id = li
        if not self.comment:
            self.document.status["table_of_content"].append(li)

class TitleOutline(TitleBase):
    """标题（多项集合）"""
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        match = self.match
        if match is None:
            return
        self.line = Strings("", self)
        self.add(Title(document))
    def add(self, obj):
        if isinstance(obj, TitleOutline):
            if self.current == self and obj.child:
                obj = obj.child[0]
        return super().add(obj)
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if len(match.group(1)) < self.level:
            return True
        # 针对下级
        if self.current == self:
            return False
        if isinstance(self.current, Block):
            return True
        return False

class Footnotes(TitleBase):
    """脚注(大标题)"""
    re_match = re.compile(r"^([*]+) +(Footnotes *.*)(?: +:(.*):)?")
    re_end = re.compile(r"^([*]+) +(.*)", re.I)
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.opt["printable"] = False

class Footnote(Root):
    """脚注(整行甚至多行的定义)"""
    re_match = re.compile(r"^\[fn:([^]]*)\](.*)")
    re_end = re.compile(r"(?:^\[fn:([^]]*)\](.*))|(?:^([*]+) +[^ ]+.*)")
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        # self.opt["printable"] = False
        match = self.match
        if match is None:
            return
        self.line.s = ""
        self.name = match.group(1)
        self.type = "str" if re.findall("[^0-9]", self.name) else "int"
        # self.line = Strings(match.group(2), self)
        t = Text(document)
        t.line.s = match.group(2)
        self.add(t)
        self.id = -1
        self.document.status["footnotes"][self.name] = self
    def end_condition(self, match: re.Match | None) -> bool:
        if match:
            return True
        if self.document.lines[self.document.current_line] == "*** 阻断使用2":
            pytools.print_err(f"!: {self.document.lines[self.document.current_line]}")
            # import pdb; pdb.set_trace()
        return False

class ListItem(Root):
    """列表(单项)"""
    re_match = re.compile(r"^( *)(-|\+|\*|[0-9]+(?:\)|\.))( +(?:.*)|$)", re.I)
    re_end = re.compile(r"^( *)(.*)", re.I)
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        match = self.match
        if match is None:
            return
        self.indent = len(match.group(1))
        self.line = Strings("", self)
        text = Text(document)
        text.line.s = re.sub(r"^ *", "", match.group(3))
        text.opt["in_list"] = True
        self.add(text)
    @property
    def _summary(self):
        return ""
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if match.group(2) == "":
            return False
        if len(match.group(1)) <= self.indent:
            i = -1
            # 查找List后第一个block起始点
            for i in self.document.status["is_in_src"]:
                if i > self.start:
                    break
            if self.current == i:
                return True
            # start和current_line在i异侧(current_line在block内)
            if (i-self.start)/(i-self.document.current_line) < 0:
                return False
            # start和current_line在i同侧(非block)
            return True
        return False

class List(Root):
    """列表(多项集合)"""
    re_match = re.compile(r"^( *)(-|\+|(?<!^)\*|[0-9]+(?:\)|\.))( +(?:.*)|$)", re.I)
    re_end = re.compile(r"^( *)(.*)", re.I)
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        match = self.match
        if match is None:
            return
        self.indent = len(match.group(1))
        self.level = 1
        ids = re.sub("[^0-9]*", "", match.group(2))
        self.type = "ol" if ids else "ul"
        self.line = Strings("", self)
        self.add(ListItem(document))
    def add(self, obj):
        if isinstance(obj, List):
            obj.level = self.level+1
            if self.current == self and obj.child:
                obj = obj.child[0]
        return super().add(obj)
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if match.group(2) == "":
            return False
        is_list = 0
        if len(match.group(1)) == self.indent:
            current_line = self.document.lines[self.document.current_line]
            if self.re_match.match(current_line):
                is_list = 1
        if len(match.group(1)) <= self.indent-is_list:
            i = -1
            # 查找List后第一个block起始点
            for i in self.document.status["is_in_src"]:
                if i > self.start:
                    break
            if self.current == i:
                return True
            # start和current_line在i异侧(current_line在block内)
            if (i-self.start)/(i-self.document.current_line) < 0:
                return False
            # start和current_line在i同侧(非block)
            return True
        return False
    @property
    def _summary(self):
        return f"(lv:{self.level}, t:{self.type})"

class Block(Root):
    """Block共同空类"""
    re_match = re.compile("")
    re_end = None

class BlockCode(Block):
    """代码块"""
    re_match = re.compile(r"^ *#\+begin_src(?:[ ]+(.*))?", re.I)
    re_end = re.compile(r"^ *#\+end_src", re.I)
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.opt["breakable"] = False
        self.line = self.document.lines
        self.number = 0
        self.end_offset = 1
        self.baise = -1
        self.document.status["is_in_src"].append(self.start)
        if self.match is None:
            return
        self.lang = self.match.group(1)
    def add(self, obj):
        if isinstance(obj, Block):
            self.document.status["is_in_src"].pop(-1)
        if isinstance(self.line, Strings):
            return
        self.number+=1
        match = re.match(r"^( *)(.*)", self.line[obj.start])
        if match is None:
            return
        if match.group(2) == "":
            return
        if self.baise == -1:
            self.baise = len(match.group(1))
        elif self.baise > len(match.group(1)):
            self.baise = len(match.group(1))
        return
    def end(self, i: int, is_normal_end: bool) -> int:
        ret = super().end(i, is_normal_end)
        if ret == i or not is_normal_end:
            index = self.document.status["is_in_src"].index(self.start)
            self.document.status["is_in_src"]=self.document.status["is_in_src"][:index]
        return ret
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if isinstance(self.line, Strings):
            return False
        lines = self.line[self.start+1:self.start+1+self.number]
        lines = [re.sub(f"^{" "*self.baise}", "", i) for i in lines]
        self.line = ""
        for i in lines:
            i = re.sub(r"^( *),([*,]|#\+)", r"\1\2", i)
            self.line += f"{i}\n"
        self.line = Strings(self.line, self)
        return True

class BlockExport(BlockCode):
    """对应语言导出块"""
    re_match = re.compile(r"^ *#\+begin_export(?:[ ]+(.*))?", re.I)
    re_end = re.compile(r"^ *#\+end_export", re.I)

class BlockComment(BlockCode):
    """注释块"""
    re_match = re.compile(r"^ *#\+begin_comment(?:[ ]+(.*))?", re.I)
    re_end = re.compile(r"^ *#\+end_comment", re.I)
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.opt["printable"] = False

class BlockExample(BlockCode):
    """代码块"""
    re_match = re.compile(r"^ *#\+begin_example(?:[ ]+(.*))?", re.I)
    re_end = re.compile(r"^ *#\+end_example", re.I)

class BlockVerse(BlockCode):
    """参照原样输出(不合并行)"""
    re_match = re.compile(r"^ *#\+begin_verse(?:[ ]+(.*))?", re.I)
    re_end = re.compile(r"^ *#\+end_verse", re.I)
    def add(self, obj, flag=True):
        super().add(obj)
        if flag:
            text = Text(self.document)
            text.line.s = ""
            self.add(text, False)
            self.number-=1

class BlockQuote(Block):
    """引用块"""
    re_match = re.compile(r"^ *#\+begin_quote", re.I)
    re_end = re.compile(r"^ *#\+end_quote", re.I)
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.opt["breakable"] = False
        self.end_offset = 1
        self.document.status["is_in_src"].append(self.start)
    def end(self, i: int, is_normal_end: bool) -> int:
        ret = super().end(i, is_normal_end)
        if ret == i or not is_normal_end:
            index = self.document.status["is_in_src"].index(self.start)
            self.document.status["is_in_src"]=self.document.status["is_in_src"][:index]
        return ret
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        return True

class BlockCenter(BlockQuote):
    """居中块"""
    re_match = re.compile(r"^ *#\+begin_center", re.I)
    re_end = re.compile(r"^ *#\+end_center", re.I)

class BlockProperties(BlockQuote):
    """属性块"""
    re_match = re.compile(r"^ *:PROPERTIES:", re.I)
    re_end = re.compile(r"^ *:END:", re.I)
    is_printable = re.compile(r"^ *:([^:]*):(?:\s+(.*)|$)")
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.opt["printable"] = False
    def add(self, obj:Root):
        super().add(obj)
        line = self.document.lines[obj.start]
        if not self.is_printable.match(line):
            self.log(str((line)), "DEBUG")
            self.opt["printable"] = True
class Table(TextBase):
    """表格"""
    re_match = re.compile(r"^ *\|", re.I)
    def __init__(self, document, match: re.Match | None = None) -> None:
        super().__init__(document, match)
        self.col = 0
        self.lines : list[list[Strings]|str] = []
        self.control_line = []
        self.align = []

        line = self.line.s
        split = re.match(r"\|-", line)
        if split:
            self.lines = ["split"]
            self.check_control_line()
            return
        ret = re.findall(r"\|([^|]*)", line)
        if len(ret)>1 and ret[-1] == "":
            ret = ret[:-1]
        self.col = len(ret)
        self.lines : list[list[Strings]|str] = \
                [[Strings(re.sub(r"^ *(.*?) *$", r"\1", i), self) for i in ret]]
        self.check_control_line()
    def check_control_line(self):
        """检查控制行(分割线、对齐行)"""
        self.control_line = []
        index = 0
        for i in self.lines:
            if i == "split":
                self.control_line.append((index, "split"))
                index+=1
                continue
            may_is_align = False
            is_break_align = False
            for j in i:
                if not isinstance(j, Strings):
                    continue
                if j.s.lower() in ("<l>", "<c>", "<r>"):
                    may_is_align = True
                if j.s.lower() not in ("<l>", "<c>", "<r>", ""):
                    is_break_align = True
            if may_is_align and not is_break_align:
                self.control_line.append((index, "align"))
            index+=1
        self.align = ["<l>"]*len(self.lines[0])
        for i in self.control_line:
            if i[1] != "align":
                continue
            index = 0
            for col in self.lines[i[0]]:
                if col.s != "":
                    self.align[index] = col.s
                index+=1
    def add_line(self, obj):
        """添加表格的一行"""
        if type(obj).__name__ != "Table":
            return
        line = obj.lines[0]
        if isinstance(line, str):
            self.lines.append(line)
            return
        li = []
        if obj.col > self.col:
            for i in self.lines:
                if not isinstance(i, list):
                    continue
                i += [Strings("", self)] * (obj.col - self.col)
                li.append(i)
            self.col = obj.col
        else:
            li = self.lines
            t = [Strings("", self)] * (self.col - obj.col)
            line += t
        li.append(line)
        self.lines = li
    def accept(self, visitor: ExportVisitor) -> str:
        self.check_control_line()
        return super().accept(visitor)

class Document:
    """文档类，操作基本单位"""
    def __init__(self, lines:list[str]|str, file_name:str="", setupfiles:list[str]|None=None,
                 setting:dict|None=None) -> None:
        self.lines = lines.splitlines() if isinstance(lines, str) else lines
        self.current_line = 0
        self.setting = {"file_name":file_name, "indent_str":"|   ",
                        "footnote_style":("", ""),
                        "css_in_html":"", "js_in_html":"",
                        "pygments_css":True,"mathjax_script":True,
                        "progress":False,"id_prefix":"","verbose_msg":False}
        self.status = {"lowest_title":50, "clean_up":False, "is_in_src":[],
                       "table_of_content":[], "footnotes":{},
                       "setupfiles":setupfiles or []}
        self.meta={
            "title":[],
            "date":[],
            "description":[],
            "author":[],
            "setupfile":[],
            "html_link_home":"",
            "html_link_up":"",
            "include":[],
            "caption":{},
            "name":{},
            "language":"",
            "property":[],
            "html_head":[],
            "seq_todo":{"todo":["TODO"], "done":["DONE"]},
            # H:最大视为标题等级
            # toc:目录显示的标题等级(num/t/nil)
            # num:最大显示数字标号的标题等级
            # ^:上下角标
            "options":{"h":3, "toc":True, "num":True,
                       "^":True},
            "latex_compiler":"xelatex",
            "latex_header":[],
        }
        if isinstance(setting,dict):
            self.setting.update({i:setting[i] for i in setting if i in self.setting})

        self.root = Root(self)
        self.root.line.s = self.setting["file_name"] if self.setting["file_name"]\
                else "<DOCUMENT IN STRINGS>"
        pth = None
        if self.setting.get("progress"):
            pth = threading.Thread(target=self.print_progress, args=["READING"])
            pth.start()
        self.build_tree()
        self.merge_text(self.root)
        self.status["clean_up"] = True
        if pth:
            pth.join()

        if self.setting.get("pygments_css"):
            try:
                get_formatter = importlib.import_module("pygments.formatters").get_formatter_by_name
                css_style = get_formatter("html", style="monokai", nowrap=True).get_style_defs()
            except ModuleNotFoundError:
                css_style = ""
                self.root.log("模块pygments不可用，无法获取对应css")
            self.setting["css_in_html"] += css_style
        if self.setting.get("mathjax_script"):
            self.setting["js_in_html"] += """\
<script>
window.MathJax = { tex: { ams: { multlineWidth: '85%' }, tags: 'ams', tagSide: 'right', tagIndent: '.8em' },
chtml: { scale: 1.0, displayAlign: 'center', displayIndent: '0em' },
svg: { scale: 1.0, displayAlign: 'center', displayIndent: '0em' },
output: { font: 'mathjax-modern', displayOverflow: 'overflow' } };
</script>
<script id="MathJax-script" async src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"></script>"""
    def print_progress(self, typ = "PROGRESS"):
        """打印进度条"""
        if self.status.get("clean_up") is None:
            return
        total_line = len(self.lines)
        try:
            width = os.get_terminal_size().columns
            if width > 40 + len(str(typ)):
                width -= 40 + len(str(typ))
        except OSError:
            width = 40
        try:
            # import tqdm
            tqdm = importlib.import_module("tqdm")
            progress = tqdm.tqdm(total=total_line, desc=typ)
            last = 0
            while not self.status["clean_up"]:
                progress.update(self.current_line-last)
                last = self.current_line
                time.sleep(1/50)
            progress.close()
        except ModuleNotFoundError:
            while not self.status["clean_up"]:
                progress = self.current_line / total_line
                s = "#"*int(progress*width)+" "*(width-int(progress*width))
                print(f"{typ} [{s}] {progress*100:6.2f}% {self.current_line:10}", end="\r")
                time.sleep(1/50)
            print("")
    def build_tree(self):
        """构建节点树"""
        rules = [
                Meta, Footnotes, TitleOutline,
                BlockCode, BlockExport, BlockQuote, BlockCenter,
                BlockComment, BlockExample, BlockVerse, BlockProperties,
                Comment, List, Table, Footnote,
                Text, Root, ListItem, Title]
        last = -1
        self.current_line = 0
        while self.current_line < len(self.lines):
            if self.current_line <= last:
                t = Text(self)
                self.root.add(t)
            elif self.current_line - last > 1:
                self.current_line -= 1
            else:
                for current in rules:
                    ret = current.re_match.match(self.lines[self.current_line])
                    if ret is None:
                        continue
                    obj = current(self, ret)
                    self.root.add(obj)
                    break
            last = self.current_line
            self.current_line += 1
            if self.current_line >= len(self.lines):
                break
            self.current_line = self.root.checkend(self.lines, self.current_line)[1]
    def merge_text(self, node:Root):
        """合并多行文本、表格，以及进行其他后处理(todo)"""
        last = None
        remove_list = []
        insert_list = []
        index = -1
        while index+1 < len(node.child):
            index+=1
            i = node.child[index]
            if isinstance(i, Title):
                i.set_status(isinstance(node, Title) and node.comment)
            if i.opt["childable"]:
                self.merge_text(i)
                last = None
                continue
            if last is None or last.line.s == "":
                if isinstance(i, TextBase):
                    last = i
                continue
            if type(i) == type(last):
                if isinstance(i, Text) and i.line.s != "":
                    last.line.s += f"\n{i.line.s}"
                    remove_list.append(i)
                elif isinstance(last, Table):
                    last.add_line(i)
                    remove_list.append(i)
                else:
                    index-=1
                    last = None
            else:
                index-=1
                last = None
        for i in remove_list:
            node.remove(i)
        for i in insert_list:
            i[0].insert(i[0].index(i[1])+i[2], i[3])
    def accept(self, visitor: ExportVisitor) -> str:
        """return str get from visitor"""
        # 根据节点类型自动匹配访问者方法
        method_name = f"visit_{self.__class__.__name__.lower()}"
        if not hasattr(visitor, method_name):
            # 默认返回空字符串（适配未实现的节点类型）
            return ""
        self.status["clean_up"] = False
        self.current_line = 0
        pth = None
        if self.setting.get("progress"):
            pth = threading.Thread(target=self.print_progress, args=["PROCESSING"])
            pth.start()
        s = getattr(visitor, method_name)(self)
        self.status["clean_up"] = True
        if pth:
            pth.join()
        return s

class TextExportVisitor(ExportVisitor):
    """转成纯文本"""
    def visit_document(self, node: Document) -> str:
        def toc_to_text() -> str:
            """获取目录并转成文本"""
            options :dict = node.meta["options"]
            if not node.status["table_of_content"] or not options["toc"]:
                return ""
            ret = ""
            for i in node.status["table_of_content"]:
                i = i[node.status["lowest_title"]-1:]
                count = 0
                lastest = 0
                j = ""
                for j in i:
                    if isinstance(j, int):
                        lastest = j
                        count+=1
                    else:
                        break
                if not isinstance(options["toc"],bool) and count > options["toc"]:
                    continue
                if not isinstance(options["h"],bool) and count > options["h"]:
                    continue
                if isinstance(j, dict):
                    ret1 = f"{"."*((count-1)*3-1)}{" " if count>1 else ""}{lastest}. "
                    ret1+=f"{f"{j["todo"][1]} " if j["todo"] else ""}{j["title"].accept(self)}"
                    if j["tag"]:
                        ret1=pytools.get_str_in_width(f"{ret1}", 40, align="<l>")
                        ret1+=f":{j["tag"]}:"
                    ret+=ret1 + "\n"
            return ret
        s = " Table of Contents\n"
        s += "===================\n"
        s += toc_to_text()
        s += "\n"
        s += node.root.accept(self)
        return s
    def visit_root(self, node) -> str:
        """输出成文本"""
        if not node.opt["printable"]:
            return ""
        text = node.line.accept(self) + "\n"
        for i in node.child:
            for j in i.accept(self).splitlines():
                if len(j) > 0 and j[-1] != "\n":
                    j+="\n"
                text += f"{node.document.setting["indent_str"]}{j}"
        return text
    def visit_titleoutline(self, node) -> str:
        text = ""
        for i in node.child:
            text += i.accept(self)
        return text
    def visit_blockcode(self, node) -> str:
        if not isinstance(node.line, Strings) or not node.opt["printable"]:
            return ""
        ret = f",----{f" Lang:{node.lang}" if node.lang else ""}\n"
        for i in node.line.s.splitlines():
            ret += f"| {i}\n"
        ret += "`----"
        return ret
    def visit_blockexport(self, node) -> str:
        if not isinstance(node.line, Strings) or not node.opt["printable"]:
            return ""
        ret = f",vvvv{f" Lang:{node.lang}" if node.lang else ""}\n"
        for i in node.line.s.splitlines():
            ret += f"| {i}\n"
        ret += "`^^^^"
        return ret
    def visit_blockcenter(self, node) -> str:
        text = ",>>>>>>>>\n"
        for i in node.child:
            if text and text[-1] != "\n":
                text+="\n"
            ret = i.accept(self)
            rets = ret.splitlines()
            max_width = 0
            if rets:
                max_width = max(pytools.get_str_width(j) for j in rets)
            max_width += max_width%2
            ret = "\n".join([pytools.get_str_in_width(j, max_width, align="<l>") for j in rets])
            for j in ret.splitlines():
                text+=f"{node.document.setting["indent_str"]}{pytools.get_str_in_width(j, 60)}\n"
        text += "`>>>>>>>>"
        return text
    def visit_blockquote(self, node) -> str:
        if not node.opt["printable"]:
            return ""
        text = ",========\n"
        for i in node.child:
            if text and text[-1] != "\n":
                text+="\n"
            for j in i.accept(self).splitlines():
                text+=f"{node.document.setting["indent_str"]}{j}\n"
        text += "`========"
        return text
    def visit_blockexample(self, node) -> str:
        if not isinstance(node.line, Strings) or not node.opt["printable"]:
            return ""
        ret = f",----{f" Lang:{node.lang}" if node.lang else ""} (EXAMPLE)\n"
        for i in node.line.s.splitlines():
            ret += f"| {i}\n"
        ret += "`----"
        return ret
    def visit_blockverse(self, node) -> str:
        if not isinstance(node.line, Strings) or not node.opt["printable"]:
            return ""
        ret = ",==== (VERSE)\n"
        ret += "\n".join(f"| {i}" for i in node.line.accept(self).splitlines())
        ret += "`===="
        return ret
    def visit_blockproperties(self, node) -> str:
        return ""
    def visit_list(self, node) -> str:
        text = ""
        index = 0
        for i in node.child:
            index+=1
            if text and text[-1] != "\n":
                text += "\n"
            # 每个列表组的每个项目
            level = f"{node.level}" if node.type == "ul" else f"{node.level}[{index}]"
            text += f"-L{level} {i.accept(self)}"
        return text
    def visit_table(self, node) -> str:
        width = []
        for i in node.lines:
            if not isinstance(i, list):
                continue
            width = [width[i] if i < len(width) else 0 for i in range(len(i))]
            for index,item in enumerate(i):
                width[index] = max(pytools.get_str_width(item.accept(self)), width[index])

        node.check_control_line()
        skip_list = [i[0] for i in node.control_line if i[1] == "align"]
        total_width = node.col+1
        for i in width:
            total_width+=i
        text=f"{pytools.get_str_in_width("",total_width,".")}\n"
        index = 0
        for line in node.lines:
            if index in skip_list:
                index+=1
                continue
            if isinstance(line, str):
                line = [Strings("-"*max(width+[0]), node)]*node.col
            for col,w,align in zip(line, width, node.align):
                text+=f"|{pytools.get_str_in_width(col.accept(self), w, align=align)}"
            text+="|\n"
            index+=1
        text+=f"{pytools.get_str_in_width("",total_width,"`")}\n"
        return f"{text}"
    def visit_footnote(self, node, printable=False) -> str:
        if not printable:
            return ""
        return self.visit_root(node)
    def visit_text(self, node) -> str:
        if not node.opt["printable"]:
            return ""
        return node.line.accept(self)
    def visit_listitem(self, node) -> str:
        text = ""
        indent_str = node.document.setting["indent_str"]
        ret = f"\n{indent_str}".join(node.child[0].accept(self).splitlines())
        text += f"{ret}\n"
        for i in node.child[1:]:
            # 下一级的东西
            for j in i.accept(self).splitlines():
                if j and j[-1] != "\n":
                    j+="\n"
                text += f"{indent_str}{j}"
        return text
    def visit_title(self, node) -> str:
        if node.comment or not node.opt["printable"]:
            return ""
        title = node.line.accept(self)
        lv = ".".join([str(i) for i in node.id[node.document.status["lowest_title"]-1:-1]])
        text = f"{lv}: {title}"
        if node.tag.s:
            text = text[:-1]
            text += f"\t\t<:{node.tag.s}:>\n"
        for i in node.child:
            if text and text[-1] != "\n":
                text+="\n"
            for j in i.accept(self).splitlines():
                text+=f"{node.document.setting["indent_str"]}{j}\n"
        return text
    def visit_strings(self, node: Strings, li: list) -> str:
        """输出行内文本(Text)"""
        if not li:
            return ""
        ret = ""
        last_stat = ""
        for i in li:
            if isinstance(i, str):
                ret+=i
                continue
            if i[0] == "link":
                ret += f"""[{self.visit_strings(node, i[2])}]({i[1]})"""
            elif i[0] == "img":
                ret += f"""![{self.visit_strings(node, i[2])}]({i[1]})"""
            elif i[0] == "figure":
                ret += f"""![{self.visit_strings(node, i[2])}]({i[1]})"""
            elif i[0] == "timestamp":
                ret += '<span class="timestamp-wrapper"><span class="timestamp">'
                ret += f"{i[1]}"
                ret += '</span></span>'
                ret += last_stat
                last_stat = ""
            elif i[0] in node.rules:
                ret += f"""{node.rules[i[0]][0]}{self.visit_strings(node, i[1])}{node.rules[i[0]][1]} """
                ret += last_stat
                last_stat = ""
            else:
                last_stat = " "
                ret += str(i)
        if ret and ret[0] == "\n":
            ret = ret[1:]
        return ret

class TexExportVisitor(ExportVisitor):
    """导出为tex格式"""
    rules = {"code":(r"\texttt{","}"),
             "italic":(r"\emph{", "}"),
             "bold":(r"\textbf{", "}"),
             "del":(r"\sout{", "}"), # ulem
             "underline":(r"\uline{", "}"), # ulem
             "timestamp":(r"\textit{", "}")}
    def visit_strings(self, node: Strings, li: list) -> str:
        """输出行内文本(LaTex)"""
        def tex_escape(s:str):
            s = s.replace("\\",r"\textbackslash ")
            for i in "#$%&_{}":
                s = s.replace(i, "\\"+i)
            for i in "^~":
                s = s.replace(i, "\\"+i+"{}")
            s = re.sub("(.)\n", r"\1\\\\"+"\n", s)
            return s
        if not li:
            return ""
        ret = ""
        last_stat = ""
        i : str | list = ""
        for i in li:
            if isinstance(i, str):
                i = tex_escape(i)
                ret+=i
                continue
            i = [tex_escape(i) if isinstance(i, str) else i for i in i]
            if i[0] == "link":
                alt = self.visit_strings(node, i[2])
                if i[1] == alt:
                    ret += r"\url{%s}" % i[1]
                else:
                    ret += r"\href{%s}{%s}" % (i[1], alt)
            elif i[0] == "img":
                ret += r"""\begin{center}
\includegraphics[width=.9\linewidth]{%s}
\end{center}""" % i[1]
            elif i[0] == "figure":
                ret += r"""\begin{figure}[htbp]
\centering
\includegraphics[width=.9\linewidth]{%s}
\caption{%s}
\end{figure}""" % (i[1],self.visit_strings(node, i[2]))
            elif i[0] == "fn":
                fns : dict = node.upward.document.status["footnotes"]
                if not fns.get(i[1]):
                    node.log(f"引用没有定义的脚注'{i[1]}'", "ERROR")
                    continue
                fn : Footnote = fns[i[1]]
                ret += "\\footnote{%s}" % fn.accept(self)
            elif i[0] in self.rules:
                ret += f"""{self.rules[i[0]][0]}{self.visit_strings(node, i[1])}{self.rules[i[0]][1]}"""
                ret += last_stat
                last_stat = ""
            else:
                last_stat = " "
                ret += str(i)
        if ret and ret[0] == "\n":
            ret = ret[1:]
        return ret
    def visit_root(self, node:Root) -> str:
        return "\n".join([i.accept(self) for i in node.child])
    def visit_text(self, node: Text) -> str:
        return node.line.accept(self)
    def visit_title(self, node:Title) -> str:
        level = node.level-node.document.status["lowest_title"]
        if level < 3:
            tag = ["sub"*level+"section{", "}"]
        else:
            tag = ["item ", ""]
        title = f"\\{tag[0]}{node.line.accept(self)}{tag[1]}\n"
        return title+"\n".join([i.accept(self) for i in node.child])
    def visit_titleoutline(self, node) -> str:
        ret = ""
        ret = "\n".join([i.accept(self) for i in node.child])
        level = node.level-node.document.status["lowest_title"]
        if level >= 3:
            headline_num = node.document.meta["options"]["num"]
            li_type = "itemize"
            if (isinstance(headline_num,bool) and headline_num) or node.level <= headline_num:
                li_type = "enumerate"
            ret = "\\begin{%s}\n%s\n\\end{%s}" % (li_type, ret, li_type)
        return ret
    def visit_footnote(self, node) -> str:
        return "\n".join([i.accept(self) for i in node.child])
    def visit_listitem(self, node: ListItem) -> str:
        text = ""
        indent_str = ""
        ret = f"\n{indent_str}".join(node.child[0].accept(self).splitlines())
        text += f"{ret}\n"
        for i in node.child[1:]:
            # 下一级的东西
            for j in i.accept(self).splitlines():
                if j and j[-1] != "\n":
                    j+="\n"
                text += f"{indent_str}{j}"
        return text
    def visit_list(self, node: List) -> str:
        typ = "itemize" if node.type == "ul" else "enumerate"
        text = r"\begin{%s}" % typ
        for i in node.child:
            if text and text[-1] != "\n":
                text += "\n"
            # 每个列表组的每个项目
            text += r"\item %s" % i.accept(self)
        if text and text[-1] != "\n":
            text += "\n"
        text += r"\end{%s}" % typ
        return text
    def visit_blockcode(self, node) -> str:
        ret = "\\begin{verbatim}\n"
        if isinstance(node.line, Strings):
            ret += node.line.s
        ret += "\\end{verbatim}"
        return ret
    def visit_blockexport(self, node) -> str:
        if not isinstance(node.line, Strings) or not node.opt["printable"]:
            return ""
        if node.lang == "html":
            return node.line.s
        return ""
    def visit_blockexample(self, node) -> str:
        return self.visit_blockcode(node)
    def visit_blockverse(self, node: BlockVerse) -> str:
        if not node.opt["printable"]:
            return ""
        text = "\\begin{verse}\n"
        if isinstance(node.line, Strings):
            for i in node.line.s.splitlines():
                i = Strings(i, node).accept(self)
                text += i
                if i:
                    text += r"\\"
                text += "\n"
            text = text[:-1]
        text += "\n\\end{verse}"
        return text
    def visit_blockquote(self, node) -> str:
        if not node.opt["printable"]:
            return ""
        text = "\\begin{quote}\n"
        text +="\n".join([i.accept(self) for i in node.child])
        text += "\n\\end{quote}"
        return text
    def visit_blockcenter(self, node: "BlockCenter") -> str:
        if not node.opt["printable"]:
            return ""
        text = "\\begin{center}\n"
        text +="\n".join([i.accept(self) for i in node.child])
        text += "\n\\end{center}"
        return text
    def visit_blockproperties(self, node) -> str:
        cond = not node.opt["printable"]
        last = node.document.root.search(node, "last")
        cond = cond and not last
        if cond:
            return ""
        text = ""
        for i in node.child:
            text += i.accept(self)
        return text
    def visit_table(self, node) -> str:
        skip_list = [i[0] for i in node.control_line if i[1] == "align"]
        text = "\\begin{center}\n\\begin{tabular}{"
        text += "".join([i[1] for i in node.align])
        text += "}\n"
        index = 0
        for line in node.lines:
            if index in skip_list:
                index+=1
                continue
            if isinstance(line, str):
                text+="\\hline\n"
                index+=1
                continue
            text += " & ".join([i.accept(self) for i in line])
            text+="\\\\\n"
            index+=1
        text += "\\end{tabular}\n\\end{center}\n"
        return text
    def visit_document(self, node:Document) -> str:
        template = r"""% 创建于 ${created_time}
% 预期latex编译器: ${latex_compiler}
\documentclass{article}
\usepackage{graphicx}
\usepackage{longtable}
\usepackage{wrapfig}
\usepackage{rotating}
\usepackage[normalem]{ulem}
\usepackage{capt-of}
\usepackage{hyperref}
\usepackage[
	a4paper,
	left=1.7cm,
	right=1.7cm,
	top=1.5cm,
	bottom=1.9cm,
	footskip=1.2ex,  % 页码到正文的间距
	bindingoffset=0.9cm,
	twoside,
]{geometry}
\usepackage{listings}
\usepackage{xcolor}
\usepackage{ctex}${latex_header}${author}${date}${title}
\hypersetup{
 pdfauthor={${pdf_author}},
 pdftitle={${pdf_author}},
 pdfkeywords={},
 pdfsubject={},
 pdfcreator={第三方org解释器}, 
 pdflang={Chinese Simplified}}
\begin{document}
${maketitle}${toc}
${body}
\end{document}"""
        data = {"latex_compiler":node.meta["latex_compiler"],
                "latex_header":"\n"+"\n".join(node.meta["latex_header"])\
                        if node.meta["latex_header"] else "",
                "created_time":pytools.get_strtime(),
                "maketitle":"", "toc":"",
                "body":node.root.accept(self)
                }
        for i in ("title","author","date",):
            s = " ".join(node.meta[i]) if node.meta.get(i) else ""
            if not s and i == "date":
                s = r"\today"
            if not s and i != "title":
                data[i] = ""
                data["pdf_"+i] = ""
                continue
            data[i] = "\n\\%s{%s}" % (i, s)
            data["pdf_"+i] = s
        if data["title"]:
            data["maketitle"] = "\n\\maketitle"
        if node.meta["options"]["toc"]:
            data["toc"] = "\n\\tableofcontents\n"
        return Template(template).safe_substitute(data)

class HtmlExportVisitor(ExportVisitor):
    """输出成html"""
    def __init__(self) -> None:
        super().__init__()
        self.text_backend = TextExportVisitor()
        self.figure_count = 0
        self.footnote_count = 0
        self.call_footnotes = []
    def toc_to_html(self, node: Document) -> str:
        """获取目录并转成html"""
        options :dict = node.meta["options"]
        if not node.status["table_of_content"] or not options["toc"]:
            return ""
        ret = """\n<div id="table-of-contents" role="doc-toc">\n<h2>Table of Contents</h2>\n"""
        ret += """<div id="text-table-of-contents" role="doc-toc">\n"""
        last = node.status["lowest_title"]-1
        for i in node.status["table_of_content"]:
            if not isinstance(options["toc"],bool) and len(i[:-1]) > options["toc"]:
                continue
            if not isinstance(options["h"],bool) and len(i[:-1]) > options["h"]:
                continue
            level = ".".join(str(j) for j in i[node.status["lowest_title"]-1:-1])+"."
            if not isinstance(options["num"],bool) and len(i[:-1]) > options["num"]:
                text = ""
            else:
                text = f"{level} "
            if i[-1]["todo"]:
                text+=f"<span class=\"todo {i[-1]["todo"][0]}\">{i[-1]["todo"][1]}</span> "
            text+=f"{i[-1]["title"].accept(self.text_backend)}"
            if i[-1]["tag"]:
                text+=f"""{"&nbsp;"*3}<span class="tag">"""
                tags = i[-1]["tag"].split(":")
                li = []
                for j in tags:
                    li.append(f"""<span class="{j}">{j}</span>""")
                text += "&nbsp;".join(li)
                text += """</span>"""

            if len(i[:-1]) > last:
                ret+="<ul><li>"*(len(i[:-1])-last)
            elif len(i[:-1]) == last:
                ret+="</li><li>"
            else:
                ret+="</li></ul>"*(last-len(i[:-1]))
                ret+="</li>\n<li>"
            levels = node.setting["id_prefix"] + re.sub(r"\.","-",level[:-1])
            ret+=f"""<a href="#org-title-{levels}">{text}</a>"""
            last = len(i[:-1])

        ret+="</li></ul>"*(last)
        ret += "</div></div>"
        return ret
    def fns_to_html(self, node: Document) -> str:
        """获取footnotes并转成html"""
        template = {
        "footnotes": Template("""\
<div id="footnotes">
<h2 class="footnotes">Footnotes: </h2>
<div id="text-footnotes">
${footnotes}</div></div>"""),
                    "footnote": Template("""\
<div class="footdef">
<sup>\
<a id="fn.${href_id}" class="footnum" href="#fnr.${href_id}" role="doc-backlink">${s}${id}${e}</a>\
</sup>
<div class="footpara" role="doc-footnote">${content}</div>
</div>"""),}
        li = sorted([node.status["footnotes"][i] for i in self.call_footnotes],
                    key=lambda x:x.id)
        footnotes = []
        for i in li:
            href_id = i.name if i.type == "str" else i.id
            footnotes.append(template["footnote"].safe_substitute(
                href_id = node.setting["id_prefix"]+str(href_id),
                s = node.setting["footnote_style"][0],
                id = i.id,
                e = node.setting["footnote_style"][1],
                content = self.visit_footnote(i, printable=True).\
                        replace("<p>","<p class=\"footpara\">"),
                    ))
        return template["footnotes"].safe_substitute(footnotes = "".join(footnotes))
    def visit_document(self, node: Document) -> str:
        self.figure_count = 0
        self.footnote_count = 0
        self.call_footnotes = []

        template = {"body": Template("""\
<!DOCTYPE html>
<html lang="zh">
<head>
<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />${meta}
<meta name="generator" content="Org Mode (third party program by python)" />
<title>${metatitle}</title>${html_head}
</head>
<body>${home_and_up}
<div id="content" class="content">${bodytitle}${toc}\
${body}${footnotes}\
</div>
${postamble}
</body>
</html>"""),
                    "home_and_up": Template("""
<div id="org-div-home-and-up">
 <a accesskey="h" href="${up}"> UP </a>
 |
 <a accesskey="H" href="${home}"> HOME </a>
</div>"""),
                    "postamble" : f"""\
<div id="postamble" class="status">\
{f"\n<p class=\"date\">时间: {" ".join(node.meta["date"])}</p>\n"
  if node.meta["date"] else ""}\
{f"<p class=\"author\">作者: {" ".join(node.meta["author"])}</p>\n"
  if node.meta["author"] else ""}\
{f"<p class=\"description\">描述: {" ".join(node.meta["description"])}</p>\n"
  if node.meta["description"] else ""}\
<p class="date">生成于: {pytools.get_strtime()}</p>
</div>"""
                    }
        line, node.root.line.s = node.root.line.s, ""
        data = {
                "meta":"\n".join([f"""<meta name="{i}" content="{" ".join(node.meta[i])}" />"""
                                  for i in ("author", "description") if node.meta[i]]),
                "metatitle":" ".join(node.meta["title"]),
                "html_head":"\n".join([i for i in [
                    f"<style>\n{node.setting["css_in_html"]}\n</style>" \
                            if node.setting["css_in_html"] else "",
                    "\n".join(node.meta["html_head"]) if node.meta["html_head"] else "",
                    node.setting["js_in_html"],
                    ] if i]),
                "home_and_up" : template["home_and_up"].safe_substitute(
                    up = node.meta["html_link_up"] or "#",
                    home = node.meta["html_link_home"] or "#",
                    ) if node.meta["html_link_home"] or node.meta["html_link_up"] else "",
                "bodytitle": f"\n<h1 class=\"title\">{" ".join(node.meta["title"])}</h1>" \
                        if node.meta["title"] else "",
                "toc": self.toc_to_html(node),
                "body": node.root.accept(self),
                "footnotes" : "",
                "postamble" : template["postamble"],
                }
        node.root.line.s = line
        del line
        for i in ("meta", "html_head"):
            if data[i]:
                data[i] = "\n" + data[i]

        if node.status["footnotes"]:
            data["footnotes"] = self.fns_to_html(node)

        html = template["body"].safe_substitute(data)
        return html
    def visit_root(self, node: Root) -> str:
        if not node.opt["printable"]:
            return ""
        text = node.line.accept(self) + "\n"
        for i in node.child:
            text += i.accept(self)
        return text
    def visit_meta(self, node) -> str:
        return ""
    def visit_titleoutline(self, node) -> str:
        text = ""
        for i in node.child:
            if text and text[-1] != "\n":
                text += "\n"
            # 每个列表组的每个项目
            text += f"{i.accept(self)}"
        if node.level > node.document.meta["options"]["h"] and text != "":
            headline_num = node.document.meta["options"]["num"]
            li_type = "ul"
            if (isinstance(headline_num,bool) and headline_num) or node.level <= headline_num:
                li_type = "ol"
            text = f"""<{li_type} class="org-ol">{text}</{li_type}>\n"""
        return text
    def visit_blockcode(self, node) -> str:
        if not isinstance(node.line, Strings) or not node.opt["printable"]:
            return ""
        ret = "<div class=\"org-src-container\">"
        ret += f"<pre class=\"src src-{node.lang.lower() if node.lang else "nil"}\">"
        try:
            pygments = importlib.import_module("pygments")
            get_lexer_by_name = importlib.import_module("pygments.lexers").get_lexer_by_name
            fmt = importlib.import_module("pygments.formatters").get_formatter_by_name("html", style="monokai", nowrap=True)
            try:
                lang = node.lang
                if not isinstance(lang, str):
                    lang = ""
                if lang.lower().startswith("conf"):
                    lang = "cfg"
                elif lang.lower() == "vimrc":
                    lang = "vim"
                compiled = pygments.highlight(
                        node.line.s,
                        get_lexer_by_name(lang),
                        fmt)
            except pygments.util.ClassNotFound:
                compiled = pygments.highlight(
                        node.line.s,
                        get_lexer_by_name("text"),
                        fmt)
        except ModuleNotFoundError:
            node.log("模块pygments不可用，使用基础fallback")
            compiled = escape(node.line.s)
        ret += compiled
        ret += "</pre></div>"
        return ret
    def visit_blockquote(self, node) -> str:
        if not node.opt["printable"]:
            return ""
        text = "<blockquote>\n"
        for i in node.child:
            text += i.accept(self)
        text += "</blockquote>"
        return text
    def visit_blockexport(self, node) -> str:
        if not isinstance(node.line, Strings) or not node.opt["printable"]:
            return ""
        if node.lang == "html":
            return node.line.s
        return ""
    def visit_blockcenter(self, node) -> str:
        text = "<div class=\"org-center\">\n"
        for i in node.child:
            text += i.accept(self)
        text += "</div>"
        return text
    def visit_blockexample(self, node) -> str:
        if not isinstance(node.line, Strings) or not node.opt["printable"]:
            return ""
        ret = "<pre class=\"example\">"
        ret += node.line.s
        ret += "</pre>"
        return ret
    def visit_blockverse(self, node) -> str:
        if not isinstance(node.line, Strings):
            return ""
        ret = "<p class=\"verse\">"
        for i in node.line.s.splitlines():
            ret += f"{Strings(i, node).accept(self)}<br/>"
        ret += "</p>"
        return ret
    def visit_blockproperties(self, node: BlockProperties) -> str:
        cond = not node.opt["printable"]
        last = node.document.root.search(node, "last")
        cond = cond and not last
        if cond:
            return ""
        text = ""
        for i in node.child:
            text += i.accept(self)
        return text
    def visit_list(self, node) -> str:
        text = f"<{node.type} class=\"org-{node.type}\">\n"
        for i in node.child:
            if text and text[-1] != "\n":
                text += "\n"
            # 每个列表组的每个项目
            text += f"<li>{i.accept(self)}</li>"
        text += f"</{node.type}>\n"
        return text
    def visit_table(self, node:Table) -> str:
        skip_list = [i[0] for i in node.control_line if i[1] == "align"]
        split_list = [i[0] for i in node.control_line if i[1] == "split"]
        text = """\n<table border="2" cellspacing="0" cellpadding="6" """+\
                """rules="groups" frame="hsides">"""
        if len(node.lines) - len(node.control_line) <= 0:
            text += "</table>"
            return text
        rule = {"<l>":"left", "<c>":"center", "<r>":"right"}
        text += "<colgroup>"
        text += "\n".join([f"""<col class="org-{rule[i]}" />""" for i in node.align])
        text += "</colgroup>"

        # 一线分头尾
        if split_list:
            text += "<thead>"
        else:
            text += "<tbody>"
        index = 0
        for line in node.lines:
            if index in skip_list:
                index+=1
                continue
            if isinstance(line, str):
                if index == split_list[0]:
                    text += "\n</thead><tbody>\n"
                else:
                    text += "\n</tbody><tbody>\n"
                index+=1
                continue
            text += "<tr>"
            for col,align in zip(line, node.align):
                if split_list and index < split_list[0]:
                    text+=f"<th scope=\"col\" class=\"org-{rule[align]}\">{col.accept(self)}</th>"
                else:
                    text+=f"<td class=\"org-{rule[align]}\">"+\
                            f"{col.accept(self) if col.accept(self) else "&nbsp;"}</td>"
            index+=1
            text += "</tr>"
        text += "</table>"
        return text
    def visit_footnote(self, node: Footnote, printable=False) -> str:
        if not printable:
            return ""
        return self.visit_root(node)
    def visit_text(self, node: Text) -> str:
        if not node.opt["printable"]:
            return ""
        text = node.line.accept(self)
        if not text:
            return text
        need_ptag = node.line.orgtext_to_list(node.line.get_pre_text())
        if len(need_ptag) > 1 or \
                (need_ptag and isinstance(need_ptag[0], str)) or \
                (need_ptag and need_ptag[0][0] not in ("img","figure")):
            text = f"<p>{text}</p>"
        return text
    def visit_listitem(self, node: ListItem) -> str:
        text = ""
        flag = True
        for i in node.child[1:]:
            if not isinstance(i, (Comment, List)):
                flag = False
                break
        if isinstance(node.child[0], Text) and flag:
            text += f"{node.child[0].line.accept(self)}\n"
        else:
            text += f"{node.child[0].accept(self)}\n"
        for i in node.child[1:]:
            # 下一级的东西
            j = i.accept(self)
            if j and j[-1] != '\n':
                j += "\n"
            text +=  j
        return text
    def visit_title(self, node: Title) -> str:
        if node.comment or not node.opt["printable"]:
            return ""
        title = node.line.accept(self)
        ids = node.id[node.document.status["lowest_title"]-1:-1]
        lv = ".".join([str(i) for i in ids])

        text = ""
        levels = node.document.setting["id_prefix"] + re.sub(r"\.","-",lv)
        if node.level <= node.document.meta["options"]["h"]:
            text += f"""<div id="outline-container-{levels}" class=\"outline-{node.level+1}\">\n"""
            text += f"""<h{node.level+2-node.document.status["lowest_title"]} """+\
                f"""id="org-title-{levels}">"""
            headline_num = node.document.meta["options"]["num"]
            if (isinstance(headline_num,bool) and headline_num) or len(ids) <= headline_num:
                text += """<span class="section-number-"""+\
                        f"""{node.level+2-node.document.status["lowest_title"]}">{lv}.</span>"""
        else:
            text += f"""<li>\n<a id="org-title-{levels}"></a>"""

        if node.todo:
            text+=f" <span class=\"{node.todo[0]} {node.todo[1]}\">{node.todo[1]}</span>"
        text += f" {title}"
        if node.tag.s:
            text += f"""{"&nbsp;"*3}<span class="tag">"""
            tags = node.tag.s.split(":")
            li = []
            for i in tags:
                li.append(f"""<span class="{i}">{i}</span>""")
            text += "&nbsp;".join(li)
            text += """</span>"""

        if node.level <= node.document.meta["options"]["h"]:
            text += f"</h{node.level+2-node.document.status["lowest_title"]}>\n"
        else:
            text += "<br/>\n"

        has_text_outline = False
        if node.child and not isinstance(node.child[0], Title):
            text += f"""<div class="outline-text-{node.level+1}" """+\
                    f"""id="text-{levels}">"""
            has_text_outline = True

        for i in node.child:
            if has_text_outline and isinstance(i, TitleOutline):
                text += "</div>"
                has_text_outline = False
            text += i.accept(self)
        if has_text_outline:
            text += "</div>"

        if node.level <= node.document.meta["options"]["h"]:
            text += "</div>"
        else:
            text += "</li>\n"
        return text
    def visit_strings(self, node: Strings, li: list) -> str:
        """输出行内html"""
        if not li:
            return ""
        ret = ""
        last_stat = ""
        i : str | list = ""
        for i in li:
            if isinstance(i, str):
                i = escape(i).replace("\n", "<br/>")
                ret+=i
                continue
            i = [escape(i) if isinstance(i, str) else i for i in i]
            if i[0] == "link":
                ret += f"""<a href="{i[1]}">{self.visit_strings(node, i[2])}</a>"""
            elif i[0] == "img":
                ret += "<div class=\"figure\"><p>" \
                        if isinstance(node.upward, Text) and \
                        not node.upward.opt.get("in_list") and len(li) == 1 else ""
                ret += f"""\n<img src="{i[1]}" alt="{self.visit_strings(node, i[2])}" />\n"""
                ret += "</p></div>\n" \
                        if isinstance(node.upward, Text) and \
                        not node.upward.opt.get("in_list") and len(li) == 1 else ""
            elif i[0] == "figure":
                self.figure_count += 1
                ret += f"""\n<div class="figure">\n<p><img src="{i[1]}" alt="{i[1]}" /></p>\n"""
                ret += """<p><span class="figure-number">Figure """+\
                        f"""{self.figure_count}: </span>"""+\
                        f"""{self.text_backend.visit_strings(node, i[2])}</p></div>"""
            elif i[0] == "fn":
                fns : dict = node.upward.document.status["footnotes"]
                if not fns.get(i[1]):
                    node.log(f"引用没有定义的脚注'{i[1]}'", "ERROR")
                    continue
                fn : Footnote = fns[i[1]]
                num = self .footnote_count
                if fn.id <= 0:
                    self .footnote_count += 1
                    num+=1
                    fn.id = num
                    name = fn.name if fn.type == "str" else num
                    self.call_footnotes.append(i[1])
                else:
                    num = fn.id
                name = fn.name if fn.type == "str" else num
                name = node.upward.document.setting["id_prefix"] + str(name)
                fn.line.accept(self)
                ret += f"""<sup><a id="fnr.{name}" class="footref" """
                ret += f"""href="#fn.{name}" role="doc-backlink">"""
                outline = node.upward.document.setting["footnote_style"]
                ret += f"""{outline[0]}{num}{outline[1]}</a></sup>"""
            elif i[0] == "timestamp":
                ret += '<span class="timestamp-wrapper"><span class="timestamp">'
                ret += f"{i[1]}"
                ret += '</span></span>'
                ret += last_stat
                last_stat = ""
            elif i[0] in node.rules:
                ret += f"""{node.rules[i[0]][0]}{self.visit_strings(node, i[1])}{node.rules[i[0]][1]}"""
                ret += last_stat
                last_stat = ""
            else:
                last_stat = " "
                ret += str(i)
        if ret and ret[0] == "\n":
            ret = ret[1:]
        ret.replace("\\", "<span>\\</span>")
        return ret

def _set_css(args, doc:Document):
    if not args.pygments_css:
        doc.setting["css_in_html"] = ""
    if not args.emacs_css:
        return
    ret = subprocess.run("type emacs", capture_output=True, shell=True, env={"LANG":"en_US.UTF-8"},
                         check=False)
    emacs_home = None
    if ret.returncode == 0:
        emacs_home = list(Path(ret.stdout.decode()[9:]).resolve().parents)
        if len(emacs_home) < 2:
            emacs_home = None
        else:
            emacs_home = emacs_home[1]/"share/emacs/"
    if not emacs_home or not emacs_home.is_dir():
        doc.root.log("内嵌emacs内置css失败 - 找不到Emacs")
        return
    li = list(emacs_home.glob("**/ox-html.el.gz"))
    if not li:
        doc.root.log("内嵌emacs内置css失败 - 找不到文件")
        return
    css_file = li[0]
    if not css_file.is_file():
        doc.root.log(f"内嵌emacs内置css失败 - 不是文件:{css_file}")
        return
    css_content = gzip.decompress(css_file.read_bytes()).decode()
    match = re.search(
            r"<style type=\\\"text/css\\\">(.*?)</style>",
            css_content, re.DOTALL)
    if match:
        doc.setting["css_in_html"] += match.group(1).replace("\\", "")

def print_feature():
    """打印相较于emacs的特性"""
    print("""\
相较于Emacs原生org的html导出，本程序:
- 暂不支持: org-list-agenda语法
- 暂不支持: 公式输出
- 暂不支持: 比较合理的html结构
- 暂不支持: 依照Emacs主题进行代码高亮(受限于pygments)
- 暂不支持: 自动标注链接（非显式指定的链接）
- 暂不支持: 自动将指向org文件的链接改为指向其html导出文件
- 暂不支持: 上下角标
- 暂不支持: <<引用>>,<<<全局>>>链接
- 去除了: 严格的链接限制(允许非`./`开头的文件引用)
- 增加了: 链接中允许定位`#`的使用
- 增加了: 对含有特殊字符文件的自动转义(非网络链接)
- 增加了: 在合并多行字符串时如果两侧边界均为非ASCII字符则不会添加空格
- 修改了: 各级标题的id格式，使其与节点顺序绑定（减少重新生成文件产生的变动）
- 修改了: 代码高亮的方式，需要额外指定css（或者通过`--pygments-css`选项内嵌）
- 修改了: 文件末尾的信息`postamble`的文字为中文
- 修改了: 脚注引用错误响应方式（仅报错，文件照样生成）
- 修改了: 错误链接响应方式（仅报错，文件照样生成）
- 保持了: Emacs原生导出的html大体框架，
          css和js文件基本上能够无缝切换（除了代码高亮）
""",end="")

def run_main() -> Document|str|None:
    """运行主函数"""
    args = parse_arguments()
    if args.feature_info:
        print_feature()
        return None
    inp_fname = ""
    inp = ""
    ret = None
    if args.input and Path(args.input).is_file():
        inp_fname = args.input
        inp = Path(args.input).read_text(encoding="utf8")
    else:
        inpf = list(Path(".").glob("**/*.org"))
        if len(inpf) == 0:
            return None
        if not inpf[-1].is_file():
            return None
        inp_fname = str(inpf[-1])
        inp = inpf[-1].read_text(encoding="utf8")

    ret = Document(inp.splitlines(), file_name=inp_fname,
                   setting={"pygments_css":args.pygments_css,
                            "progress":args.auto_output and args.progress,
                            "verbose_msg":args.verbose})
    _set_css(args, ret)
    configs = {
            "html":["html", lambda:ret.accept(HtmlExportVisitor())],
            "text":["txt", lambda:ret.accept(TextExportVisitor())],
            "latex":["tex", lambda:ret.accept(TexExportVisitor())],
            "nodetree":["nodetree.txt", ret.root.to_node_tree],
            }
    if args.mode not in configs:
        return ret
    if args.auto_output:
        output_f = Path(Path(inp_fname).stem+"."+configs[args.mode][0])
        print(f"INFO output file: '{output_f}'")
        output_f.write_text(configs[args.mode][1](), encoding="utf8")
    else:
        print(configs[args.mode][1]())
    return ret

def parse_arguments():
    """解释参数"""
    argparse=importlib.import_module("argparse")
    parser = argparse.ArgumentParser(description='解析org文件')
    parser.add_argument('-i', '--input', default=None, help='指定输入文件')
    parser.add_argument('-m', '--mode', default="html", help='指定输出模式',
                        choices=["html", "text", "latex", "nodetree"])
    parser.add_argument('-E', '--emacs-css', action="store_true", help='从安装好的emacs获取内置css文件')
    parser.add_argument('-O', '--auto-output', action="store_true", help='自动输出到同名的.org文件')
    parser.add_argument('-p', '--progress', action="store_true", help='显示进度条')
    parser.add_argument('-v', '--verbose', action="store_true", help='显示更verbose msg')
    parser.add_argument('--pygments-css', action="store_true", help='内置pygments生产的css文件')
    parser.add_argument('--feature-info', action="store_true", help='查看特性信息')
    try:
        importlib.import_module("argcomplete").autocomplete(parser)
    except ModuleNotFoundError:
        pass
    args = parser.parse_args()
    return args

def main():
    """对外提供主函数"""
    run_main()

if __name__ == "__main__":
    main()
