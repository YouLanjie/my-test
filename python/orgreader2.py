#!/usr/bin/python
"""解析org文件"""

from pprint import pprint
from pathlib import Path
import sys
import time
import argparse
import re
import requests

def get_str_width(s:str):
    """计算字符串打印宽度"""
    width = 0
    count = 0
    for c in s:
        if ord(c) <= 127:
            width += 1
            count += 1
        else:
            width += 2
            count += 1
    return width

def get_str_in_width(text:str, width:int, fill:str=' ', align:str="<c>"):
    """根据打印宽度截断字符串"""
    ret_text = text
    if width > get_str_width(ret_text):
        dwidth = width - get_str_width(ret_text)
        if align.lower() == "<l>":
            ret_text = f"{text}{fill[0]*(dwidth)}"
        elif align.lower() == "<c>":
            ret_text = f"{fill[0]*(dwidth//2)}{text}{fill[0]*(dwidth-dwidth//2)}"
        elif align.lower() == "<r>":
            ret_text = f"{fill[0]*(dwidth)}{text}"
    while len(ret_text) > 0 and width < get_str_width(ret_text):
        ret_text = ret_text[:-1]
    return ret_text

class Strings:
    """行内字符串类，提供行内格式转换"""
    def __init__(self, s:str, node, parse_able : bool = True) -> None:
        self.s = s
        self.parse_able = parse_able
        if isinstance(node, Root):
            self.upward = node
    def parse_inline_text(self, s:str) -> list[str]:
        """解释行内字符串"""
        pattern = [
                ("link", re.compile(r"\[\[(.*?)(?<!\\)\](?:\[(.*?)(?<!\\)\])?\]")),
                ("code", re.compile(r"=([^ ].*?(?<! ))=(?=[ ()-]|$)")),
                ("code", re.compile(r"~([^ ].*?(?<! ))~(?=[ ()-]|$)")),
                ("italic", re.compile(r"/([^ ].*?(?<! ))/(?=[ ()-]|$)")),
                ("bold", re.compile(r"\*([^ ].*?(?<! ))\*(?=[ ()-]|$)")),
                ("del", re.compile(r"\+([^ ].*?(?<! ))\+(?=[ ()-]|$)")),
                ("fn", re.compile(r"\[fn:([^]]*)\]")),
                ]
        li = []
        i = 0
        last = i
        while i < len(s):
            for current_pattern in pattern:
                ret = current_pattern[1].match(s[i:])
                if not ret:
                    continue
                if current_pattern[0] not in ("link", "fn") and i-1 > 0 and s[i-1] not in " ()-":
                    continue
                if last != i and re.findall(r"[^ ]", s[last:i]):
                    li.append(s[last:i])
                last = i
                i+=ret.span()[1]
                if current_pattern[0] == "link":
                    mode = "link"
                    link = re.sub(r"\\([][])", r"\1", ret.group(1))
                    alt = ret.group(2)
                    ret = re.match(r".*/(.*\.(?:jpg|png|jpeg|gif))",link,re.I)
                    if not alt and ret:
                        mode = "img"
                        alt = ret.group(1)
                        caption = self.upward.document.meta["caption"]
                        if not isinstance(self.upward, Title) and\
                                caption[0] == self.upward.start-1 and last == 0:
                            mode = "figure"
                            alt = caption[1]
                    elif not alt:
                        alt = link
                    if alt and mode != "img":
                        alt = self.parse_inline_text(alt)
                    li.append([mode, link, alt])
                elif current_pattern[0] == "code":
                    li.append([current_pattern[0], [ret.group(1)]])
                elif current_pattern[0] == "fn":
                    li.append([current_pattern[0], ret.group(1)])
                else:
                    li.append([current_pattern[0], self.parse_inline_text(ret.group(1))])
                last = i
                i -= 1
                break
            i+=1
        if last != i and re.findall(r"[^ ]", s[last:i]):
            li.append(s[last:i])
        return li
    def list_to_text(self, li:list, mode:str="text") -> str:
        """将给定的已经分好的列表转回文字"""
        if not li:
            return ""
        rules = {"code":("<code>","</code>"),
                 "italic":("<i>", "</i>"),
                 "bold":("<b>", "</b>"),
                 "del":("<del>", "</del>")}
        ret = ""
        for i in li:
            if isinstance(i, str):
                ret+=i
                continue
            if mode == "text":
                if i[0] == "link":
                    ret += f""" [{self.list_to_text(i[2], mode)}]({i[1]})"""
                elif i[0] == "img":
                    ret += f""" ![{self.list_to_text(i[2], mode)}]({i[1]})"""
                elif i[0] == "figure":
                    ret += f""" ![{self.list_to_text(i[2], mode)}]({i[1]})"""
                elif i[0] in rules:
                    ret += f""" {rules[i[0]][0]}{self.list_to_text(i[1], mode)}{rules[i[0]][1]}"""
                else:
                    ret += str(i)
            else:
                if i[0] == "link":
                    ret += f"""\n<a href="{i[1]}">{self.list_to_text(i[2], mode)}</a>"""
                elif i[0] == "img":
                    ret += f"""\n<img src="{i[1]}" alt="{self.list_to_text(i[2], mode)}"></img>"""
                elif i[0] == "figure":
                    self.upward.document.counter["figure_count"]+=1
                    ret += f"""\n<div class="figure">\n<p><img src="{i[1]}" alt="{i[1]}"></img></p>\n"""
                    ret += f"""<p><span class="figure-number">Figure {self.upward.document.counter["figure_count"]}: </span>"""+\
                            """{self.list_to_text(i[2])}</p></div>"""
                elif i[0] == "fn":
                    self.upward.document.counter["footnote_count"]+=1
                    num = self.upward.document.counter["footnote_count"]
                    fns = self.upward.document.footnotes
                    fn = fns[i[1]]
                    fn.id = num
                    name = fn.name if fn.type == "str" else num
                    ret += f"""<sup><a id="fnr.{name}" class="footref" """
                    ret += f"""href="#fn.{name}" role="doc-backlink">{num}</a></sup>"""
                elif i[0] in rules:
                    ret += f"""\n{rules[i[0]][0]}{self.list_to_text(i[1], mode)}{rules[i[0]][1]}"""
                else:
                    ret += str(i)
        if ret and ret[0] == "\n":
            ret = ret[1:]
        return ret
    def split_cond(self, ch1:str, ch2:str) -> bool:
        """不分割的条件"""
        if not ch1:
            return True
        if not ch2:
            return False
        ch1 = ch1[-1]
        ch2 = ch2[-1]
        # 任一为中文
        if ord(ch1) > 127 or ord(ch2) > 127:
            return True
        # 任一为符号
        symbol = r"()[]{}@#$%&!^*+-=/"
        if ch1 in symbol or ch2 in symbol:
            return True
        return False
    def get_pre_text(self) -> str:
        lines = self.s.splitlines()
        text = ""
        for i in lines:
            match = re.match(r"(.*)\\\\$", i)
            if match:
                text+=f"{match.group(1)}\n"
            else:
                split = " "
                if self.split_cond(text, i):
                    split = ""
                text+=f"{split}{i}"
        return text
    def get_text(self, mode:str="text") -> str:
        """获取文字"""
        if not self.parse_able:
            return re.sub("\n", " ", self.s)
        text = self.get_pre_text()
        return self.list_to_text(self.parse_inline_text(text), mode)

class Root:
    """根节点/节点父类"""
    def __init__(self, document) -> None:
        if not isinstance(document, Document):
            return
        self.opt = {"childable": True, "breakable": True}
        self.document = document
        self.start = document.current_line
        self.line = Strings("", self)
        if document.lines:
            self.line = Strings(document.lines[document.current_line], self)
        self.end_offset = 0
        self.child = []
        self.current = self
    def checkend(self, lines:list[str], i:int) -> tuple[bool,int]:
        """检查结构是否结束"""
        if not self.opt["childable"]:
            return False, i
        match = RULES[type(self).__name__]["end"].match(lines[i])
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
    def _debug_output(self):
        return f"('{self.line.s}')"
    def __str__(self, debug = False) -> str:
        ret = super().__str__()
        if not debug:
            return ret
        if self.line:
            ret += self._debug_output
        if self.child:
            ret += "\n"
        for i in self.child:
            for j in i.__str__(True).splitlines():
                ret += f"|   {j}\n"
        return ret
    def get_text(self, mode:str="text") -> str:
        """For Sub class"""
        text = self.line.get_text(mode) + "\n"
        for i in self.child:
            for j in i.get_text(mode).splitlines():
                if len(j) > 0 and j[-1] != "\n":
                    j+="\n"
                if mode == "text":
                    text += f"|   {j}"
                else:
                    text += j
        return text
    def end_condition(self, match: re.Match | None) -> bool:
        """For Sub class"""
        if match is None:
            return False
        if not isinstance(self.current, Title):
            return True
        return False

class Text(Root):
    """文本类"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["childable"] = False
        self.line.s = re.sub(r"^ *", "", self.line.s)
    def get_text(self, mode:str="text") -> str:
        text = self.line.get_text(mode)
        if mode == "html":
            if text != "":
                need_ptag = self.line.parse_inline_text(self.line.get_pre_text())
                if len(need_ptag) > 1:
                    text = f"<p>{text}</p>"
                else:
                    text = f"{text}"
        return f"{text}"

class Comment(Root):
    """注释行"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["childable"] = False
    def get_text(self, mode:str="text") -> str:
        return ""

class Meta(Root):
    """元数据"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["childable"] = False
        match = RULES[type(self).__name__]["match"].match(self.line.s)
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
            if self.value in self.document.setupfiles:
                return
            content = ""
            try:
                req = requests.get(self.value, timeout=3)
                print(f"WARN 在文件中插入外部链接可能拖慢转译速度({req.elapsed})[{self.value}]")
                if req.status_code == 200:
                    req.encoding = req.apparent_encoding
                    content = req.text
            except Exception:
                setupfile=Path(f"{Path(self.document.file_name).parent}/{self.value}")
                if setupfile.is_file():
                    content = setupfile.read_text(encoding="utf8")
            self.document.setupfiles.append(self.value)
            doc = Document(content.splitlines(), setupfiles=self.document.setupfiles)
            self.document.setupfiles=list(set(self.document.setupfiles)&set(doc.setupfiles))
            for i in doc.meta:
                if isinstance(self.document.meta[i], list):
                    self.document.meta[i] += doc.meta[i]
                elif doc.meta[i] != "":
                    self.document.meta[i] = doc.meta[i]
        elif self.key == "seq_todo":
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
            return
    def get_text(self, mode:str="text") -> str:
        if self.key == "caption":
            self.document.meta["caption"] = (self.start, self.value)
        return ""

class Title(Root):
    """标题"""
    def __init__(self, document) -> None:
        super().__init__(document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.id = []
        self.level = len(match.group(1))
        self.comment = match.group(2) != ""
        self.line = Strings(match.group(3), self)
        self.tag = Strings(match.group(4), self)
        self.todo = []

        last = self.document.table_of_content[-1] if self.document.table_of_content else []
        li = []
        num = 0
        for num in range(self.level):
            if num >= len(last)-1:
                li.append(0)
                continue
            li.append(last[num])
        li[num] += 1
        text = self.line.get_text("text")
        li.append({"title":text, "tag":self.tag.s, "todo":None, "start":self.start})
        self.id = li
        if not self.comment:
            self.document.table_of_content.append(li)

        self.document.counter["lowest_title"] = min(self.document.counter["lowest_title"], self.level)
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
    def get_text(self, mode:str="text") -> str:
        if self.comment:
            return ""
        title = self.line.get_text(mode)

        lv = ""
        for i in self.id[self.document.counter["lowest_title"]-1:-1]:
            lv+=f"{i}."
        lv = lv[:-1]
        if mode == "text":
            text = f"{lv}: {title}"
            if self.tag.s:
                text = text[:-1]
                text += f"\t\t<:{self.tag.s}:>\n"
        else:
            text = f"""<div class="outline-{self.level+1}">\n<h{self.level+2-self.document.counter["lowest_title"]} id="org-title-{re.sub(r"\.","-",lv)}">"""
            text += f"""<span class="section-number-{self.level+2-self.document.counter["lowest_title"]}">{lv}.</span>"""
            if self.todo:
                text+=f" <span class=\"todo {self.todo[1]}\">{self.todo[1]}</span>"
            text += f" {title}"
            if self.tag.s:
                text += f"""{"&nbsp;"*3}<span class="tag">"""
                tags = self.tag.s.split(":")
                li = []
                for i in tags:
                    li.append(f"""<span class="{i}">{i}</span>""")
                text += "&nbsp;".join(li)
                text += """</span>"""
            text += f"</h{self.level+2-self.document.counter["lowest_title"]}>"

        for i in self.child:
            if mode == "text":
                if text and text[-1] != "\n":
                    text+="\n"
                for j in i.get_text(mode).splitlines():
                    text+=f"|   {j}\n"
            else:
                text += i.get_text(mode)
        if mode != "text":
            text += "</div>"
        return text

class Footnotes(Title):
    """脚注标题"""
    def __init__(self, document) -> None:
        Root.__init__(self, document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.id = []
        self.level = len(match.group(1))
        self.comment = match.group(2) != ""
        self.line = Strings(match.group(3), self)
    def get_text(self, mode:str="text") -> str:
        return ""

class Footnote(Text):
    """脚注(整行)"""
    def __init__(self, document) -> None:
        super().__init__(document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.name = match.group(1)
        self.type = "str" if re.findall("[^0-9]", self.name) else "int"
        self.line = Strings(match.group(2), self)
        self.id = -1
        self.document.footnotes[self.name] = self
    def get_text(self, mode:str="text") -> str:
        return ""

class ListItem(Root):
    """列表(单项)"""
    def __init__(self, document) -> None:
        super().__init__(document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.indent = len(match.group(1))
        self.line = Strings("", self)
        text = Text(document)
        text.line.s = match.group(3)
        self.add(text)
    @property
    def _debug_output(self):
        return ""
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if match.group(2) == "":
            return False
        if len(match.group(1)) <= self.indent:
            i = -1
            # 查找List后第一个block起始点
            for i in self.document.is_in_src:
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
    def get_text(self, mode:str="text") -> str:
        text = ""
        if mode == "text":
            ret = "\n|   ".join(self.child[0].get_text(mode).splitlines())
            text += f"{ret}\n"
            for i in self.child[1:]:
                # 下一级的东西
                for j in i.get_text(mode).splitlines():
                    if j and j[-1] != "\n":
                        j+="\n"
                    text += f"|   {j}"
            return text
        text += f"{self.child[0].get_text(mode)}\n"
        for i in self.child[1:]:
            # 下一级的东西
            for j in i.get_text(mode).splitlines():
                if j and j[-1] != "\n":
                    j+="\n"
                text += f"{j}"
        return text

class List(Root):
    """列表(多项集合)"""
    def __init__(self, document) -> None:
        super().__init__(document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
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
            if RULES[type(self).__name__]["match"].match(current_line):
                is_list = 1
        if len(match.group(1)) <= self.indent-is_list:
            i = -1
            # 查找List后第一个block起始点
            for i in self.document.is_in_src:
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
    def _debug_output(self):
        return f"(lv:{self.level}, t:{self.type})"
    def get_text(self, mode:str="text") -> str:
        if mode == "text":
            text = ""
            index = 0
            for i in self.child:
                index+=1
                if text and text[-1] != "\n":
                    text += "\n"
                # 每个列表组的每个项目
                level = f"{self.level}" if self.type == "ul" else f"{self.level}[{index}]"
                text += f"-L{level} {i.get_text(mode)}"
            return text

        text = f"<{self.type}>\n"
        for i in self.child:
            if text and text[-1] != "\n":
                text += "\n"
            # 每个列表组的每个项目
            text += f"<li>{i.get_text(mode)}</li>"
        text += f"</{self.type}>\n"
        return text

class Block(Root):
    """Block共同空类"""

class BlockCode(Block):
    """代码块"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["breakable"] = False
        self.line = self.document.lines
        self.number = 0
        self.end_offset = 1
        self.baise = -1
        self.document.is_in_src.append(self.start)
        match = RULES[type(self).__name__]["match"].match(self.line[self.start])
        if match is None:
            return
        self.lang = match.group(1)
    def add(self, obj):
        if isinstance(obj, Block):
            self.document.is_in_src.pop(-1)
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
            index = self.document.is_in_src.index(self.start)
            self.document.is_in_src=self.document.is_in_src[:index]
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
            i = re.sub(r"^( *),(?:[*,]|#\+)", r"\1", i)
            self.line += f"{i}\n"
        self.line = Strings(self.line, self)
        return True
    def get_text(self, mode:str="text") -> str:
        if not isinstance(self.line, Strings):
            return ""
        if mode == "text":
            ret = f",----{f" Lang:{self.lang}" if self.lang else ""}\n"
            for i in self.line.s.splitlines():
                ret += f"| {i}\n"
            ret += "`----"
            return ret
        ret = f"<pre class=\"src src-{self.lang.lower() if self.lang else "nil"}\">"
        for i in self.line.s.splitlines():
            ret += f"{i}\n"
        ret += "</pre>"
        return ret

class BlockExport(BlockCode):
    """对应语言导出块"""
    def get_text(self, mode:str="text") -> str:
        if not isinstance(self.line, Strings):
            return ""
        if mode == "text":
            ret = f",vvvv{f" Lang:{self.lang}" if self.lang else ""}\n"
            for i in self.line.s.splitlines():
                ret += f"| {i}\n"
            ret += "`^^^^"
            return ret
        if mode==self.lang:
            return self.line.s
        return ""

class BlockQuote(Block):
    """引用块"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["breakable"] = False
        self.end_offset = 1
        self.document.is_in_src.append(self.start)
    def end(self, i: int, is_normal_end: bool) -> int:
        ret = super().end(i, is_normal_end)
        if ret == i or not is_normal_end:
            index = self.document.is_in_src.index(self.start)
            self.document.is_in_src=self.document.is_in_src[:index]
        return ret
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        return True
    def get_text(self, mode:str="text") -> str:
        if mode == "text":
            text = ",========\n"
            for i in self.child:
                if text and text[-1] != "\n":
                    text+="\n"
                for j in i.get_text(mode).splitlines():
                    text+=f"|   {j}\n"
            text += "`========"
            return text
        text = "<blockquote>\n"
        for i in self.child:
            text += i.get_text(mode)
        text += "</blockquote>"
        return text

class BlockCenter(BlockQuote):
    """居中块"""
    def get_text(self, mode:str="text") -> str:
        if mode == "text":
            text = ",>>>>>>>>\n"
            for i in self.child:
                if text and text[-1] != "\n":
                    text+="\n"
                ret = i.get_text(mode)
                rets = ret.splitlines()
                max_width = max([get_str_width(j) for j in rets])
                max_width += max_width%2
                ret = "\n".join([get_str_in_width(j, max_width, align="<l>") for j in rets])
                for j in ret.splitlines():
                    text+=f"|   {get_str_in_width(j, 60)}\n"
            text += "`>>>>>>>>"
            return text
        text = "<div class=\"org-center\">\n"
        for i in self.child:
            text += i.get_text(mode)
        text += "</div>"
        return text

class Table(Text):
    """表格"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.col = 0
        self.lines : list[list[Strings]|str] = []
        self.width = []
        self.control_line = []
        self.align = []

        line = self.line.s
        split = re.match(r"\|-", line)
        if split:
            self.lines = ["split"]
            self.reset_width()
            self.check_control_line()
            return
        match = re.findall(r"\|([^|]*)", line)
        if len(match)>1 and match[-1] == "":
            match = match[:-1]
        self.col = len(match)
        self.lines : list[list[Strings]|str]= [[Strings(re.sub(r"^ *(.*?) *$", r"\1", i), self) for i in match]]
        self.reset_width()
        self.check_control_line()
    def check_control_line(self):
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
        self.align = ["<l>" for i in self.lines[0]]
        for i in self.control_line:
            if i[1] != "align":
                continue
            index = 0
            for col in self.lines[i[0]]:
                if col.s != "":
                    self.align[index] = col.s
                index+=1
    def reset_width(self):
        """重设每列的最大宽度"""
        for i in self.lines:
            if not isinstance(i, list):
                continue
            self.width = [self.width[i] if i < len(self.width) else 0 for i in range(len(i))]
            for j in range(len(i)):
                if get_str_width(i[j].get_text()) > self.width[j]:
                    self.width[j] = get_str_width(i[j].get_text())
    def add_line(self, obj):
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
                i += [Strings("", self) for i in range(obj.col - self.col)]
                li.append(i)
            self.col = obj.col
        else:
            li = self.lines
            t = [Strings("", self) for i in range(self.col - obj.col)]
            line += t
        li.append(line)
        self.lines = li
        self.reset_width()
        self.check_control_line()
    def get_text(self, mode:str="text") -> str:
        skip_list = [-1 if i[1] != "align" else i[0] for i in self.control_line]
        split_list = [-1 if i[1] == "align" else i[0] for i in self.control_line]
        if mode == "text":
            total_width = self.col+1
            for i in self.width:
                total_width+=i
            text=f"{get_str_in_width("",total_width,".")}\n"
            index = 0
            for line in self.lines:
                if index in skip_list:
                    index+=1
                    continue
                if isinstance(line, str):
                    line = [Strings("-"*max(self.width), self) for i in range(self.col)]
                for col,width,align in zip(line, self.width, self.align):
                    text+=f"|{get_str_in_width(col.get_text(mode), width, align=align)}"
                text+="|\n"
                index+=1
            text+=f"{get_str_in_width("",total_width,"`")}\n"
            return f"{text}"
        text = """\n<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">"""
        rule = {"<l>":"left", "<c>":"center", "<r>":"right"}
        text += "<colgroup>"
        text += "\n".join([f"""<col class="org-{rule[i]}" />""" for i in self.align])
        text += "</colgroup>"
        # 一线分头尾
        if split_list:
            text += "<thead>"
        else:
            text += "<tbody>"
        index = 0
        for line in self.lines:
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
            for col,width,align in zip(line, self.width, self.align):
                if split_list and index < split_list[0]:
                    text+=f"<th scope=\"col\" class=\"org-{rule[align]}\">{col.get_text(mode)}</td>"
                else:
                    text+=f"<td class=\"org-{rule[align]}\">{col.get_text(mode)}</td>"
            index+=1
            text += "</tr>"
        text += "</table>"
        return text
    def text_process(self, text:str, mode:tuple[str, str]) -> str :
        width = self.col+1
        for i in self.width:
            width+=i
        if mode[0] == "self":
            text=f"{get_str_in_width("",width,".")}\n"
            index = 0
            skip_list = [-1 if i[1] != "align" else i[0] for i in self.control_line]
            for line in self.lines:
                if index in skip_list or not isinstance(line, list):
                    index+=1
                    continue
                for col,width,align in zip(line, self.width, self.align):
                    text+=f"|{get_str_in_width(col.get_text(), width, align=align)}"
                text+="|\n"
                index+=1
        if mode[0] == "after":
            text=f"{get_str_in_width("",width,"`")}\n"
        return f"{text}"

#^ *#\+begin_([^ \n]+)(?:[ ]+(.*?))?\n(.*?)^ *#\+end_\1\n?
RULES = {
        "Meta":        {"match":re.compile(r"^[ ]*#\+([^:]*):[ ]*(.*)", re.I), "class":Meta},
        "Footnotes":   {"match":re.compile(r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(Footnotes *.*)"),
                        "class":Footnotes,
                        "end":re.compile(r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)", re.I)},
        "Title":       {"match":re.compile(r"^([*]+)[ ]+((?:COMMENT )?) *(.*?(?= +:.*:|$))(?: +:(.*):)?", re.I),
                        "class":Title,
                        "end":re.compile(r"^([*]+)[ ]+((?:COMMENT )?) *(.*?(?= +:.*:|$))(?: +:(.*):)?", re.I)},
        "BlockCode":   {"match":re.compile(r"^ *#\+begin_src(?:[ ]+(.*))?", re.I),
                        "class":BlockCode,
                        "end":re.compile(r"^ *#\+end_src", re.I)},
        "BlockExport": {"match":re.compile(r"^ *#\+begin_export(?:[ ]+(.*))?", re.I),
                         "class":BlockExport,
                         "end":re.compile(r"^ *#\+end_export", re.I)},
        "BlockQuote":  {"match":re.compile(r"^ *#\+begin_quote", re.I),
                        "class":BlockQuote,
                        "end":re.compile(r"^ *#\+end_quote", re.I)},
        "BlockCenter": {"match":re.compile(r"^ *#\+begin_center", re.I),
                        "class":BlockCenter,
                        "end":re.compile(r"^ *#\+end_center", re.I)},
        "Comment":     {"match":re.compile(r"^[ ]*#[ ]+(.*)", re.I),"class":Comment},
        "List":        {"match":re.compile(r"^( *)(-|\+|\*|[0-9]+(?:\)|\.)) +(.*)", re.I),
                        "class":List,
                        "end":re.compile(r"^( *)(.*)", re.I)},
        "Table":       {"match":re.compile(r"^ *\|", re.I), "class":Table},
        "Footnote":    {"match":re.compile(r"^\[fn:([^]]*)\](.*)"), "class":Footnote},
        "Text":        {"match":re.compile(""), "class":Text},
        "Root":        {"match":re.compile(""),
                        "class":Comment,
                        "end":re.compile(r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)", re.I)},
        "ListItem":    {"match":re.compile(r"^( *)(-|\+|\*|[0-9]+(?:\)|\.)) +(.*)", re.I),
                        "class":List,
                        "end":re.compile(r"^( *)(.*)", re.I)},
        }

class Document:
    """文档类，操作基本单位"""
    def __init__(self, lines:list[str], file_name:str="", setupfiles:list[str]=[]) -> None:
        self.lines = lines
        self.file_name = file_name
        self.current_line = 0
        self.is_in_src = []
        self.table_of_content = []
        self.footnotes = {}
        self.setupfiles = setupfiles
        self.meta={
            "title":[],
            "date":[],
            "description":[],
            "author":[],
            "setupfile":[],
            "html_link_home":"",
            "html_link_up":"",
            "caption":(-2, ""),
            "html_head":[],
            "seq_todo":{"todo":[], "done":[]},
        }
        self.counter = {"figure_count":0, "footnote_count":0, "lowest_title":50, "clean_up":False}

        self.root = Root(self)
        self.root.line.s = self.file_name if self.file_name else "<DOCUMENT IN STRINGS>"
        self.build_tree()
        self.merge_text(self.root)
    def build_tree(self):
        """构建节点树"""
        last = -1
        self.current_line = 0
        while self.current_line < len(self.lines):
            if self.current_line <= last:
                t = Text(self)
                self.root.add(t)
            elif self.current_line - last > 1:
                self.current_line -= 1
            else:
                for _,current in RULES.items():
                    ret = current["match"].match(self.lines[self.current_line])
                    if ret is None:
                        continue
                    obj = current["class"](self)
                    self.root.add(obj)
                    break
            last = self.current_line
            self.current_line += 1
            if self.current_line >= len(self.lines):
                break
            _,self.current_line = self.root.checkend(self.lines, self.current_line)
        return
    def merge_text(self, node:Root):
        """合并多行文本、表格，以及进行其他后处理"""
        last = None
        remove_list = []
        insert_list = []
        count_title = 0
        for i in node.child:
            if isinstance(i, Title):
                key = i.line.s.split(" ")[0]
                todo = None
                if key in self.meta["seq_todo"]["todo"]:
                    todo = ["todo", key]
                elif key in self.meta["seq_todo"]["done"]:
                    todo = ["done", key]
                if todo:
                    i.line.s = i.line.s[len(key):]
                    i.todo = todo
                    current_title = [j[-1] for j in self.table_of_content if j[-1]["start"] == i.start]
                    if current_title:
                        current_title[0]["todo"] = todo
                        current_title[0]["title"] = i.line.get_text()
                if not i.comment:
                    count_title += 1
            if i.opt["childable"]:
                self.merge_text(i)
                last = None
                continue
            if last is None or last.line.s == "":
                if isinstance(i, Text):
                    last = i
                continue
            if type(i) == type(last):
                if type(i) == Text and i.line.s != "":
                    last.line.s += f"\n{i.line.s}"
                    remove_list.append(i)
                elif isinstance(last, Table):
                    last.add_line(i)
                    remove_list.append(i)
                else:
                    last = None
            elif type(last) == Footnote and type(i) == Text:
                last.line.s += f" {i.line.s}"
                remove_list.append(i)
            else:
                last = None
        for i in remove_list:
            node.remove(i)
        for i in insert_list:
            i[0].insert(i[0].index(i[1])+i[2], i[3])
        return
    def get_table_of_content(self, mode = "text") -> str:
        """获取目录"""
        if not self.table_of_content:
            return ""
        if mode == "text":
            ret = ""
            for i in self.table_of_content:
                i = i[self.counter["lowest_title"]-1:]
                count = 0
                lastest = 0
                j = ""
                for j in i:
                    if isinstance(j, int):
                        lastest = j
                        count+=1
                    else:
                        break
                if isinstance(j, dict):
                    ret1 = f"{"."*((count-1)*3-1)}{" " if count>1 else ""}{lastest}. "
                    ret1+=f"{f"{j["todo"][1]} " if j["todo"] else ""}{j["title"]}"
                    if j["tag"]:
                        ret1=get_str_in_width(f"{ret1}", 40, align="<l>")
                        ret1+=f":{j["tag"]}:"
                    ret+=ret1 + "\n"
            return ret
        ret = """\n<div id="table-of-contents" role="doc-toc">\n<h2>Table of Contents</h2>\n"""
        ret += """<div id="text-table-of-contents" role="doc-toc">\n"""
        last = self.counter["lowest_title"]-1
        for i in self.table_of_content:
            level = ""
            for j in i[self.counter["lowest_title"]-1:-1]:
                level += f"{j}."
            text = f"{level} "
            if i[-1]["todo"]:
                text+=f"<span class=\"todo {i[-1]["todo"][0]}\">{i[-1]["todo"][1]}</span>"
            text+=f"{i[-1]["title"]}"
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
            ret+=f"""<a href="#org-title-{re.sub(r"\.","-",level[:-1])}">{text}</a>"""
            last = len(i[:-1])

        ret += "</div></div>"
        return ret
    def to_text(self) -> str:
        """转成纯文本"""
        self.counter["figure_count"] = 0
        self.counter["footnote_count"] = 0
        return self.root.get_text()
    def to_html(self) -> str:
        """转成html"""
        self.counter["figure_count"] = 0
        self.counter["footnote_count"] = 0
        meta = f"""\n{"\n".join(\
                [f"""<meta name="{i}" content="{" ".join(self.meta[i])}" />"""\
                for i in ("author", "description")])}"""
        html_head = f"\n{"\n".join(self.meta["html_head"])}" if self.meta["html_head"] else ""
        title = f"\n<h1 class=\"title\">{" ".join(self.meta["title"])}</h1>" if self.meta["title"] else ""
        html = f"""\
<!DOCTYPE html>
<html lang="zh">
<head>{meta}
<title>{" ".join(self.meta["title"])}</title>{html_head}
</head>
<body>
"""
        if self.meta["html_link_home"] or self.meta["html_link_up"]:
            html += f"""\
<div id="org-div-home-and-up">
 <a accesskey="h" href="{self.meta["html_link_up"]}"> UP </a>
 |
 <a accesskey="H" href="{self.meta["html_link_home"]}"> HOME </a>
</div>
"""
        html += f"""<div id="content" class="content">{title}{self.get_table_of_content("html")}"""
        line = self.root.line.s
        self.root.line.s = ""
        html += self.root.get_text("html")
        self.root.line.s = line
        if self.footnotes:
            html += """\
<div id="footnotes">
<h2 class="footnotes">Footnotes: </h2>
<div id="text-footnotes">
"""
            for i in sorted([self.footnotes[i] for i in self.footnotes], key=lambda x:x.id):
                i.line.get_text("html")
            for i in sorted([self.footnotes[i] for i in self.footnotes], key=lambda x:x.id):
                html += f"""\
<div class="footdef">
<sup><a id="fn.{i.name if i.type == "str" else i.id}" class="footnum" href="#fnr.{i.name if i.type == "str" else i.id}" role="doc-backlink">{i.id}</a></sup>
<div class="footpara" role="doc-footnote"><p class="footpara"> {i.line.get_text("html")} </p></div>
</div>"""
            html += "</div></div>"
        html += "</div>\n"

        html += f"""\
<div id="postamble" class="status">\
{f"<p class=\"date\">标记时间: {" ".join(self.meta["date"])}</p>\n" if self.meta["date"] else ""}\
{f"<p class=\"author\">作者: {" ".join(self.meta["author"])}</p>\n" if self.meta["author"] else ""}\
{f"<p class=\"description\">描述: {" ".join(self.meta["description"])}</p>\n" if self.meta["description"] else ""}\
<p class="date">文件生成时间: {time.strftime("%Y-%m-%d")}{"一二三四五六日"[int(time.strftime("%w"))]}\
{time.strftime("%H:%M:%S")}</p>
</div>
"""
        html += "</body>\n</html>"
        return html

def main():
    """运行主函数"""
    args = parse_arguments()
    inp_f = ""
    inp = ""
    ret = None
    if args.input and Path(args.input).is_file():
        inp_f = args.input
        inp = Path(args.input).read_text(encoding="utf8")
    else:
        inpf = list(Path(".").glob("**/*.org"))
        if len(inpf) == 0:
            return
        if not inpf[-1].is_file():
            return
        inp_f = str(inpf[-1])
        inp = inpf[-1].read_text(encoding="utf8")
    if args.diff:
        # pip install org-python
        import orgpython
        ret = orgpython.to_html(inp)
        print(ret)
    else:
        ret = Document(inp.splitlines(), file_name=inp_f)
        if args.debug:
            print(ret.root.__str__(True))
        elif args.text_mode:
            print(" Table of Contents")
            print("===================")
            print(ret.get_table_of_content())
            print(ret.to_text())
            # pprint(ret.meta)
            # pprint(ret.table_of_content)
            # pprint(ret.setupfiles)
            # pprint(vars(ret))
        else:
            print(ret.to_html())
    return ret

def parse_arguments() -> argparse.Namespace:
    """解释参数"""
    parser = argparse.ArgumentParser(description='解析org文件')
    parser.add_argument('-d', '--diff', action="store_true", help='使用org-python导出')
    parser.add_argument('-i', '--input', default=None, help='指定输入文件')
    parser.add_argument('-t', '--text-mode', action="store_true", help='以text形式输出')
    parser.add_argument('-x', '--debug', action="store_true", help='调试输出')
    # parser.add_argument('-i', '--input', default="/home/Chglish/WorkSpace/simp.org", help='指定输入文件')
    # parser.add_argument('-t', '--text-mode', action="store_false", help='以text形式输出')
    # parser.add_argument('-x', '--debug', action="store_false", help='调试输出')
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    main()
