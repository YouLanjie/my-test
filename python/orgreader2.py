#!/usr/bin/python
"""解析org文件"""

from pathlib import Path
import sys
import time
import argparse
import re
import requests

def print_err(s:str):
    """从stderr打印输出"""
    print(s, file=sys.stderr)

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
    pattern = [
            ("link", re.compile(r"\[\[(.*?)(?<!\\)\](?:\[(.*?)(?<!\\)\])?\]")),
            ("code", re.compile(r"=([^ ].*?(?<! ))=(?=[ ()-]|$)")),
            ("code", re.compile(r"~([^ ].*?(?<! ))~(?=[ ()-]|$)")),
            ("italic", re.compile(r"/([^ ].*?(?<! ))/(?=[ ()-]|$)")),
            ("bold", re.compile(r"\*([^ ].*?(?<! ))\*(?=[ ()-]|$)")),
            ("del", re.compile(r"\+([^ ].*?(?<! ))\+(?=[ ()-]|$)")),
            ("fn", re.compile(r"\[fn:([^]]*)\]")),
            ]
    img_exts=["png", "jpg", "jpeg", "gif", "webp"]
    rules = {"code":("<code>","</code>"),
             "italic":("<i>", "</i>"),
             "bold":("<b>", "</b>"),
             "del":("<del>", "</del>")}
    def __init__(self, s:str, node, parse_able : bool = True) -> None:
        self.s = s
        self.parse_able = parse_able
        if isinstance(node, Root):
            self.upward = node
    def _parse_link(self, ret, li, last):
        mode = "link"
        link = re.sub(r"\\([][])", r"\1", ret.group(1))
        for j in "[]":
            if j not in link:
                continue
            link = link.replace(j, f"%{hex(ord(j))[2:].upper()}")
        alt = ret.group(2)
        ret = re.match(r".*/(.*\.(?:"+"|".join(self.img_exts)+r"))",link,re.I)
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
            alt = self.orgtext_to_list(alt)
        li.append([mode, link, alt])
    def orgtext_to_list(self, s:str) -> list[str]:
        """解释行内字符串变成数组"""
        li = []
        i = 0
        last = i
        while i < len(s):
            for current_pattern in self.pattern:
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
                    self._parse_link(ret, li, last)
                elif current_pattern[0] == "code":
                    li.append([current_pattern[0], [ret.group(1)]])
                elif current_pattern[0] == "fn":
                    li.append([current_pattern[0], ret.group(1)])
                else:
                    li.append([current_pattern[0], self.orgtext_to_list(ret.group(1))])
                last = i
                i -= 1
                break
            i+=1
        if last != i and re.findall(r"[^ ]", s[last:i]):
            li.append(s[last:i])
        return li
    def list_to_html(self, li:list) -> str:
        """将给定的已经分好的列表转回html"""
        if not li:
            return ""
        ret = ""
        for i in li:
            if isinstance(i, str):
                ret+=i
                continue
            if i[0] == "link":
                ret += f"""\n<a href="{i[1]}">{self.list_to_html(i[2])}</a>"""
            elif i[0] == "img":
                ret += f"""\n<img src="{i[1]}" alt="{self.list_to_html(i[2])}" />"""
            elif i[0] == "figure":
                self.upward.document.status["figure_count"]+=1
                ret += f"""\n<div class="figure">\n<p><img src="{i[1]}" alt="{i[1]}" /></p>\n"""
                ret += """<p><span class="figure-number">Figure """+\
                        f"""{self.upward.document.status["figure_count"]}: </span>"""+\
                        f"""{self.list_to_text(i[2])}</p></div>"""
            elif i[0] == "fn":
                fns = self.upward.document.status["footnotes"]
                fn = fns[i[1]]
                num = self.upward.document.status["footnote_count"]
                if fn.id <= 0:
                    self.upward.document.status["footnote_count"]+=1
                    num+=1
                    fn.id = num
                name = fn.name if fn.type == "str" else num
                ret += f"""<sup><a id="fnr.{name}" class="footref" """
                ret += f"""href="#fn.{name}" role="doc-backlink">{num}</a></sup>"""
            elif i[0] in self.rules:
                ret += f"""\n{self.rules[i[0]][0]}{self.list_to_html(i[1])}{self.rules[i[0]][1]}"""
            else:
                ret += str(i)
        if ret and ret[0] == "\n":
            ret = ret[1:]
        return ret
    def list_to_text(self, li:list) -> str:
        """将给定的已经分好的列表转回文字"""
        if not li:
            return ""
        ret = ""
        for i in li:
            if isinstance(i, str):
                ret+=i
                continue
            if i[0] == "link":
                ret += f""" [{self.list_to_text(i[2])}]({i[1]})"""
            elif i[0] == "img":
                ret += f""" ![{self.list_to_text(i[2])}]({i[1]})"""
            elif i[0] == "figure":
                ret += f""" ![{self.list_to_text(i[2])}]({i[1]})"""
            elif i[0] in self.rules:
                ret += f""" {self.rules[i[0]][0]}{self.list_to_text(i[1])}{self.rules[i[0]][1]}"""
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
        symbol = r"()[]{}<>@#$%&!^*+-=/,.`~\"'"
        if ch1 in symbol or ch2 in symbol:
            return True
        return False
    def get_pre_text(self) -> str:
        """获取预处理文字"""
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
    def to_text(self) -> str:
        """输出行内文本"""
        if not self.parse_able:
            return re.sub("\n", " ", self.s)
        text = self.get_pre_text()
        return self.list_to_text(self.orgtext_to_list(text))
    def to_html(self) -> str:
        """输出行内html"""
        if not self.parse_able:
            return re.sub("\n", " ", self.s)
        text = self.get_pre_text()
        text = re.sub("<", r"&lt;", text)
        text = re.sub(">", r"&gt;", text)
        return self.list_to_html(self.orgtext_to_list(text))

class Root:
    """根节点/节点父类"""
    def __init__(self, document) -> None:
        if not isinstance(document, Document):
            return
        self.opt = {"childable": True, "breakable": True,
                    "printable":True}
        self.document = document
        self.start = document.current_line
        self.line = Strings("", self)
        if document.lines:
            self.line = Strings(document.lines[document.current_line], self)
        self.end_offset = 0
        self.child :list[Root] = []
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
    def _summary(self):
        return f"('{self.line.s}')"
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
    def to_text(self) -> str:
        """输出成文本"""
        if not self.opt["printable"]:
            return ""
        text = self.line.to_text() + "\n"
        for i in self.child:
            for j in i.to_text().splitlines():
                if len(j) > 0 and j[-1] != "\n":
                    j+="\n"
                text += f"{self.document.setting["indent_str"]}{j}"
        return text
    def to_html(self) -> str:
        """输出成html"""
        if not self.opt["printable"]:
            return ""
        text = self.line.to_html() + "\n"
        for i in self.child:
            text += i.to_html()
        return text

class TextBase(Root):
    """文本基类"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["childable"] = False
        self.line.s = re.sub(r"^ *", "", self.line.s)

class Text(TextBase):
    """文本类"""
    def to_text(self) -> str:
        if not self.opt["printable"]:
            return ""
        return self.line.to_text()
    def to_html(self) -> str:
        if not self.opt["printable"]:
            return ""
        text = self.line.to_html()
        if not text:
            return text
        need_ptag = self.line.orgtext_to_list(self.line.get_pre_text())
        if len(need_ptag) > 1 or (need_ptag and isinstance(need_ptag[0], str)):
            text = f"<p>{text}</p>"
        return text

class Comment(Root):
    """注释行"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["childable"] = False
        self.opt["printable"] = False

class Meta(Root):
    """元数据"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["childable"] = False
        self.opt["printable"] = False
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
            if self.value in self.document.status["setupfiles"]:
                return
            self._load_sub_setupfile()
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
    def _load_sub_setupfile(self):
        content = ""
        setupfile=Path(f"{Path(self.document.setting["file_name"]).parent}/{self.value}")
        if setupfile.is_file():
            content = setupfile.read_text(encoding="utf8")
        elif re.match(r"http[s]?://.+", self.value):
            try:
                req = requests.get(self.value, timeout=3)
                print_err(f"WARN 在文件中插入外部链接可能拖慢转译速度({req.elapsed})[{self.value}]")
                if req.status_code == 200:
                    req.encoding = req.apparent_encoding
                    content = req.text
            except (requests.exceptions.RequestException, IOError) as e:
                print_err(f"WARN 加载文件错误: {e}")
                return
        else:
            print_err(f"WARN 异常文件名提示: '{self.value}'")
        self.document.status["setupfiles"].append(self.value)
        doc = Document(content.splitlines(), setupfiles=self.document.status["setupfiles"])
        self.document.status["setupfiles"]=list(\
                set(self.document.status["setupfiles"])&\
                set(doc.status["setupfiles"])
            )
        for key,value in doc.meta.items():
            if isinstance(self.document.meta[key], list):
                self.document.meta[key] += value
            elif value != "":
                self.document.meta[key] = value
    def to_text(self) -> str:
        if self.key == "caption":
            self.document.meta["caption"] = (self.start, self.value)
        return ""
    def to_html(self) -> str:
        if self.key == "caption":
            self.document.meta["caption"] = (self.start, self.value)
        return ""

class TitleBase(Root):
    """标题基类"""
    def __init__(self, document) -> None:
        super().__init__(document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.id = []
        self.level = len(match.group(1))
        self.comment = match.group(2) != ""
        self.line = Strings(match.group(3), self)
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
        return f"(lv:{self.level}, '{self.line.s}')"

class Title(TitleBase):
    """标题"""
    def __init__(self, document) -> None:
        super().__init__(document)
        match = RULES[type(self).__name__]["match"]\
                .match(self.document.lines[self.document.current_line])
        if match is None:
            return
        self.tag = Strings(match.group(4), self)

        last = self.document.status["table_of_content"][-1] if \
                self.document.status["table_of_content"] else []
        li = []
        num = 0
        for num in range(self.level):
            if num >= len(last)-1:
                li.append(0)
                continue
            li.append(last[num])
        li[num] += 1
        text = self.line.to_text()
        li.append({"title":text, "tag":self.tag.s, "todo":None, "start":self.start})
        self.id = li
        if not self.comment:
            self.document.status["table_of_content"].append(li)

        self.document.status["lowest_title"] = \
                min(self.document.status["lowest_title"], self.level)
    def to_text(self) -> str:
        if self.comment or not self.opt["printable"]:
            return ""
        title = self.line.to_text()
        lv = ".".join([str(i) for i in self.id[self.document.status["lowest_title"]-1:-1]])
        text = f"{lv}: {title}"
        if self.tag.s:
            text = text[:-1]
            text += f"\t\t<:{self.tag.s}:>\n"
        for i in self.child:
            if text and text[-1] != "\n":
                text+="\n"
            for j in i.to_text().splitlines():
                text+=f"{self.document.setting["indent_str"]}{j}\n"
        return text
    def to_html(self) -> str:
        if self.comment or not self.opt["printable"]:
            return ""
        title = self.line.to_html()
        lv = ".".join([str(i) for i in self.id[self.document.status["lowest_title"]-1:-1]])

        text = f"""<div class="outline-{self.level+1}">\n"""
        text += f"""<h{self.level+2-self.document.status["lowest_title"]} """+\
                f"""id="org-title-{re.sub(r"\.","-",lv)}">"""
        text += """<span class="section-number-"""+\
                f"""{self.level+2-self.document.status["lowest_title"]}">{lv}.</span>"""
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
        text += f"</h{self.level+2-self.document.status["lowest_title"]}>"

        has_text_outline = False
        if self.child and not isinstance(self.child[0], Title):
            text += f"""<div class="outline-text-{self.level+1}" """+\
                    f"""id="text-{re.sub(r"\.","-",lv)}">"""
            has_text_outline = True

        for i in self.child:
            if has_text_outline and isinstance(i, Title):
                text += "</div>"
                has_text_outline = False
            text += i.to_html()
        if has_text_outline:
            text += "</div>"
        text += "</div>"
        return text

class Footnotes(TitleBase):
    """脚注标题"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["printable"] = False
        match = RULES[type(self).__name__]["match"]\
                .match(self.document.lines[self.document.current_line])
        if match is None:
            return
        self.id = []
        self.level = len(match.group(1))
        self.comment = match.group(2) != ""
        self.line = Strings(match.group(3), self)

class Footnote(TextBase):
    """脚注(整行)"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["printable"] = False
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.name = match.group(1)
        self.type = "str" if re.findall("[^0-9]", self.name) else "int"
        self.line = Strings(match.group(2), self)
        self.id = -1
        self.document.status["footnotes"][self.name] = self

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
    def to_text(self) -> str:
        text = ""
        indent_str = self.document.setting["indent_str"]
        ret = f"\n{indent_str}".join(self.child[0].to_text().splitlines())
        text += f"{ret}\n"
        for i in self.child[1:]:
            # 下一级的东西
            for j in i.to_text().splitlines():
                if j and j[-1] != "\n":
                    j+="\n"
                text += f"{indent_str}{j}"
        return text
    def to_html(self) -> str:
        text = ""
        text += f"{self.child[0].to_html()}\n"
        for i in self.child[1:]:
            # 下一级的东西
            for j in i.to_html().splitlines():
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
    def to_text(self) -> str:
        text = ""
        index = 0
        for i in self.child:
            index+=1
            if text and text[-1] != "\n":
                text += "\n"
            # 每个列表组的每个项目
            level = f"{self.level}" if self.type == "ul" else f"{self.level}[{index}]"
            text += f"-L{level} {i.to_text()}"
        return text
    def to_html(self) -> str:
        text = f"<{self.type}>\n"
        for i in self.child:
            if text and text[-1] != "\n":
                text += "\n"
            # 每个列表组的每个项目
            text += f"<li>{i.to_html()}</li>"
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
        self.document.status["is_in_src"].append(self.start)
        match = RULES[type(self).__name__]["match"].match(self.line[self.start])
        if match is None:
            return
        self.lang = match.group(1)
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
            i = re.sub(r"^(?: *),([*,]|#\+)", r"\1", i)
            self.line += f"{i}\n"
        self.line = Strings(self.line, self)
        return True
    def to_text(self) -> str:
        if not isinstance(self.line, Strings) or not self.opt["printable"]:
            return ""
        ret = f",----{f" Lang:{self.lang}" if self.lang else ""}\n"
        for i in self.line.s.splitlines():
            ret += f"| {i}\n"
        ret += "`----"
        return ret
    def to_html(self) -> str:
        if not isinstance(self.line, Strings) or not self.opt["printable"]:
            return ""
        ret = f"<pre class=\"src src-{self.lang.lower() if self.lang else "nil"}\">"
        ret += self.line.s
        ret += "</pre>"
        return ret

class BlockExport(BlockCode):
    """对应语言导出块"""
    def to_text(self) -> str:
        if not isinstance(self.line, Strings) or not self.opt["printable"]:
            return ""
        ret = f",vvvv{f" Lang:{self.lang}" if self.lang else ""}\n"
        for i in self.line.s.splitlines():
            ret += f"| {i}\n"
        ret += "`^^^^"
        return ret
    def to_html(self) -> str:
        if not isinstance(self.line, Strings) or not self.opt["printable"]:
            return ""
        if self.lang == "html":
            return self.line.s
        return ""

class BlockComment(BlockCode):
    """注释块"""
    def __init__(self, document) -> None:
        super().__init__(document)
        self.opt["printable"] = False

class BlockExample(BlockCode):
    """代码块"""
    def to_text(self) -> str:
        if not isinstance(self.line, Strings) or not self.opt["printable"]:
            return ""
        ret = f",----{f" Lang:{self.lang}" if self.lang else ""} (EXAMPLE)\n"
        for i in self.line.s.splitlines():
            ret += f"| {i}\n"
        ret += "`----"
        return ret
    def to_html(self) -> str:
        if not isinstance(self.line, Strings) or not self.opt["printable"]:
            return ""
        ret = "<pre class=\"example\">"
        ret += self.line.s
        ret += "</pre>"
        return ret

class BlockVerse(BlockCode):
    """参照原样输出(不合并行)"""
    def add(self, obj, flag=True):
        super().add(obj)
        if flag:
            text = Text(self.document)
            text.line.s = ""
            self.add(text, False)
            self.number-=1
    def to_text(self) -> str:
        if not isinstance(self.line, Strings) or not self.opt["printable"]:
            return ""
        ret = ",==== (VERSE)\n"
        ret += "\n".join(f"| {i}" for i in self.line.to_text().splitlines())
        ret += "`===="
        return ret
    def to_html(self) -> str:
        if not isinstance(self.line, Strings):
            return ""
        ret = "<p class=\"verse\">"
        for i in self.line.s.splitlines():
            ret += f"{Strings(i, self).to_html()}\n"
        ret += "</p>"
        return ret

class BlockQuote(Block):
    """引用块"""
    def __init__(self, document) -> None:
        super().__init__(document)
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
    def to_text(self) -> str:
        if not self.opt["printable"]:
            return ""
        text = ",========\n"
        for i in self.child:
            if text and text[-1] != "\n":
                text+="\n"
            for j in i.to_text().splitlines():
                text+=f"{self.document.setting["indent_str"]}{j}\n"
        text += "`========"
        return text
    def to_html(self) -> str:
        if not self.opt["printable"]:
            return ""
        text = "<blockquote>\n"
        for i in self.child:
            text += i.to_html()
        text += "</blockquote>"
        return text

class BlockCenter(BlockQuote):
    """居中块"""
    def to_text(self) -> str:
        text = ",>>>>>>>>\n"
        for i in self.child:
            if text and text[-1] != "\n":
                text+="\n"
            ret = i.to_text()
            rets = ret.splitlines()
            max_width = 0
            if rets:
                max_width = max(get_str_width(j) for j in rets)
            max_width += max_width%2
            ret = "\n".join([get_str_in_width(j, max_width, align="<l>") for j in rets])
            for j in ret.splitlines():
                text+=f"{self.document.setting["indent_str"]}{get_str_in_width(j, 60)}\n"
        text += "`>>>>>>>>"
        return text
    def to_html(self) -> str:
        text = "<div class=\"org-center\">\n"
        for i in self.child:
            text += i.to_html()
        text += "</div>"
        return text

class Table(TextBase):
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
        self.lines : list[list[Strings]|str] = \
                [[Strings(re.sub(r"^ *(.*?) *$", r"\1", i), self) for i in match]]
        self.reset_width()
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
    def reset_width(self):
        """重设每列的最大宽度"""
        for i in self.lines:
            if not isinstance(i, list):
                continue
            self.width = [self.width[i] if i < len(self.width) else 0 for i in range(len(i))]
            for index,item in enumerate(i):
                self.width[index] = max(get_str_width(item.to_text()), self.width[index])
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
        self.reset_width()
        self.check_control_line()
    def to_text(self) -> str:
        skip_list = [i[0] for i in self.control_line if i[1] == "align"]
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
                line = [Strings("-"*max(self.width+[0]), self)]*self.col
            for col,width,align in zip(line, self.width, self.align):
                text+=f"|{get_str_in_width(col.to_text(), width, align=align)}"
            text+="|\n"
            index+=1
        text+=f"{get_str_in_width("",total_width,"`")}\n"
        return f"{text}"
    def to_html(self) -> str:
        skip_list = [i[0] for i in self.control_line if i[1] == "align"]
        split_list = [i[0] for i in self.control_line if i[1] == "split"]
        text = """\n<table border="2" cellspacing="0" cellpadding="6" """+\
                """rules="groups" frame="hsides">"""
        if len(self.lines) - len(self.control_line) <= 0:
            text += "</table>"
            return text
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
            for col,align in zip(line, self.align):
                if split_list and index < split_list[0]:
                    text+=f"<th scope=\"col\" class=\"org-{rule[align]}\">{col.to_html()}</th>"
                else:
                    text+=f"<td class=\"org-{rule[align]}\">"+\
                            f"{col.to_html() if col.to_html() else "&nbsp;"}</td>"
            index+=1
            text += "</tr>"
        text += "</table>"
        return text

#^ *#\+begin_([^ \n]+)(?:[ ]+(.*?))?\n(.*?)^ *#\+end_\1\n?
RULES = {
    "Meta":         {"match":re.compile(r"^[ ]*#\+([^:]*):[ ]*(.*)", re.I), "class":Meta},
    "Footnotes":    {"match":re.compile(
                        r"^([*]+)[ ]+((?:COMMENT )?) *(Footnotes *.*)(?: +:(.*):)?"),
                     "class":Footnotes,
                     "end":re.compile(r"^([*]+)[ ]+((?:COMMENT )?) *(.*)", re.I)},
    "Title":        {"match":re.compile(
                            r"^([*]+)[ ]+((?:COMMENT )?) *(.*?(?= +:.*:|$))(?: +:(.*):)?", re.I),
                     "class":Title,
                     "end":re.compile(
                            r"^([*]+)[ ]+((?:COMMENT )?) *(.*?(?= +:.*:|$))(?: +:(.*):)?", re.I)},
    "BlockCode":    {"match":re.compile(r"^ *#\+begin_src(?:[ ]+(.*))?", re.I),
                     "class":BlockCode,
                     "end":re.compile(r"^ *#\+end_src", re.I)},
    "BlockExport":  {"match":re.compile(r"^ *#\+begin_export(?:[ ]+(.*))?", re.I),
                      "class":BlockExport,
                      "end":re.compile(r"^ *#\+end_export", re.I)},
    "BlockQuote":   {"match":re.compile(r"^ *#\+begin_quote", re.I),
                     "class":BlockQuote,
                     "end":re.compile(r"^ *#\+end_quote", re.I)},
    "BlockCenter":  {"match":re.compile(r"^ *#\+begin_center", re.I),
                     "class":BlockCenter,
                     "end":re.compile(r"^ *#\+end_center", re.I)},
    "BlockComment": {"match":re.compile(r"^ *#\+begin_comment(?:[ ]+(.*))?", re.I),
                     "class":BlockComment,
                     "end":re.compile(r"^ *#\+end_comment", re.I)},
    "BlockExample": {"match":re.compile(r"^ *#\+begin_example(?:[ ]+(.*))?", re.I),
                     "class":BlockExample,
                     "end":re.compile(r"^ *#\+end_example", re.I)},
    "BlockVerse":   {"match":re.compile(r"^ *#\+begin_verse(?:[ ]+(.*))?", re.I),
                     "class":BlockVerse,
                     "end":re.compile(r"^ *#\+end_verse", re.I)},
    "Comment":      {"match":re.compile(r"^[ ]*#[ ]+(.*)", re.I),"class":Comment},
    "List":         {"match":re.compile(r"^( *)(-|\+|\*|[0-9]+(?:\)|\.)) +(.*)", re.I),
                     "class":List,
                     "end":re.compile(r"^( *)(.*)", re.I)},
    "Table":        {"match":re.compile(r"^ *\|", re.I), "class":Table},
    "Footnote":     {"match":re.compile(r"^\[fn:([^]]*)\](.*)"), "class":Footnote},
    "Text":         {"match":re.compile(""), "class":Text},
    "Root":         {"match":re.compile(""),
                     "class":Comment,
                     "end":re.compile(r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)", re.I)},
    "ListItem":     {"match":re.compile(r"^( *)(-|\+|\*|[0-9]+(?:\)|\.)) +(.*)", re.I),
                     "class":List,
                     "end":re.compile(r"^( *)(.*)", re.I)},
    }

class Document:
    """文档类，操作基本单位"""
    def __init__(self, lines:list[str], file_name:str="", setupfiles:list[str]|None=None) -> None:
        self.lines = lines
        self.current_line = 0
        self.setting = {"file_name":file_name,"indent_str":"|   "}
        self.status = {"figure_count":0, "footnote_count":0, "lowest_title":50, "clean_up":False,
                       "is_in_src":[], "table_of_content":[], "footnotes":{},
                       "setupfiles":setupfiles if setupfiles else []}
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

        self.root = Root(self)
        self.root.line.s = self.setting["file_name"] if self.setting["file_name"]\
                else "<DOCUMENT IN STRINGS>"
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
            self.current_line = self.root.checkend(self.lines, self.current_line)[1]
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
                    current_title = [j[-1] for j in self.status["table_of_content"] \
                            if j[-1]["start"] == i.start]
                    if current_title:
                        current_title[0]["todo"] = todo
                        current_title[0]["title"] = i.line.to_text()
                if not i.comment:
                    count_title += 1
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
                    last = None
            elif isinstance(last, Footnote) and isinstance(i, Text):
                last.line.s += f" {i.line.s}"
                remove_list.append(i)
            else:
                last = None
        for i in remove_list:
            node.remove(i)
        for i in insert_list:
            i[0].insert(i[0].index(i[1])+i[2], i[3])
        return
    def table_of_content_to_text(self) -> str:
        """获取目录并转成文本"""
        if not self.status["table_of_content"]:
            return ""
        ret = ""
        for i in self.status["table_of_content"]:
            i = i[self.status["lowest_title"]-1:]
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
    def table_of_content_to_html(self) -> str:
        """获取目录并转成html"""
        if not self.status["table_of_content"]:
            return ""
        ret = """\n<div id="table-of-contents" role="doc-toc">\n<h2>Table of Contents</h2>\n"""
        ret += """<div id="text-table-of-contents" role="doc-toc">\n"""
        last = self.status["lowest_title"]-1
        for i in self.status["table_of_content"]:
            level = ".".join(str(j) for j in i[self.status["lowest_title"]-1:-1])+"."
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

        ret+="</li></ul>"*(last)
        ret += "</div></div>"
        return ret
    def to_text(self) -> str:
        """转成纯文本"""
        self.status["figure_count"] = 0
        self.status["footnote_count"] = 0
        return self.root.to_text()
    def to_html(self) -> str:
        """转成html"""
        self.status["figure_count"] = 0
        self.status["footnote_count"] = 0
        meta = f"""\n{"\n".join(\
                [f"""<meta name="{i}" content="{" ".join(self.meta[i])}" />"""\
                for i in ("author", "description")])}"""
        html_head = f"\n{"\n".join(self.meta["html_head"])}" if self.meta["html_head"] else ""
        title = f"\n<h1 class=\"title\">{" ".join(self.meta["title"])}</h1>" \
                if self.meta["title"] else ""
        html = f"""\
<!DOCTYPE html>
<html lang="zh">
<head>{meta}
<title>{" ".join(self.meta["title"])}</title>{html_head}
</head>
<body>
"""
        if self.meta["html_link_home"] or self.meta["html_link_up"]:
            href_up = self.meta["html_link_up"] if self.meta["html_link_up"] else "#"
            href_home = self.meta["html_link_home"] if self.meta["html_link_home"] else "#"
            html += f"""\
<div id="org-div-home-and-up">
 <a accesskey="h" href="{href_up}"> UP </a>
 |
 <a accesskey="H" href="{href_home}"> HOME </a>
</div>
"""
        html += f"""<div id="content" class="content">{title}{self.table_of_content_to_html()}"""
        line = self.root.line.s
        self.root.line.s = ""
        html += self.root.to_html()
        self.root.line.s = line
        if self.status["footnotes"]:
            html += """\
<div id="footnotes">
<h2 class="footnotes">Footnotes: </h2>
<div id="text-footnotes">
"""
            for i in sorted([self.status["footnotes"][i] for i in self.status["footnotes"]],
                            key=lambda x:x.id):
                i.line.to_html()
            for i in sorted([self.status["footnotes"][i] for i in self.status["footnotes"]],
                            key=lambda x:x.id):
                href_id = i.name if i.type == "str" else i.id
                html += f"""\
<div class="footdef">
<sup>\
<a id="fn.{href_id}" class="footnum" href="#fnr.{href_id}" role="doc-backlink">{i.id}</a>\
</sup>
<div class="footpara" role="doc-footnote"><p class="footpara"> {i.line.to_html()} </p></div>
</div>"""
            html += "</div></div>"
        html += "</div>\n"

        html += f"""\
<div id="postamble" class="status">\
{f"<p class=\"date\">标记时间: {" ".join(self.meta["date"])}</p>\n" if self.meta["date"] else ""}\
{f"<p class=\"author\">作者: {" ".join(self.meta["author"])}</p>\n" if self.meta["author"] else ""}\
{f"<p class=\"description\">描述: {" ".join(self.meta["description"])}</p>\n" \
if self.meta["description"] else ""}\
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
            print(ret.root.to_node_tree())
        elif args.text_mode:
            print(" Table of Contents")
            print("===================")
            print(ret.table_of_content_to_text())
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
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    main()
