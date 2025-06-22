#!/usr/bin/python

from pprint import pprint
from pathlib import Path
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
    def __init__(self, s:str, parse_able : bool = True) -> None:
        self.s = s
        self.parse_able = parse_able
    def get_text(self, mode:str="text") -> str:
        if not self.parse_able:
            return self.s
        # links
        ret = re.sub(r"\[\[([^]]*)\](?:\[(.*)\])?\]",
                     lambda m: f"{f"[{m.group(2)}]" if m.group(2) else f"<{m.group(1)}>"}", self.s)
        return ret

class Root:
    def __init__(self, document) -> None:
        if not isinstance(document, Document):
            return
        self.document = document
        self.start = document.current_line
        self.childable = True
        self.line = Strings("")
        if document.lines:
            self.line = Strings(document.lines[document.current_line])
        self.breakable = True
        self.end_offset = 0
        self.child = []
        self.current = self
    def checkend(self, lines:list[str], i:int) -> tuple[bool,int]:
        if not self.childable:
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
        if not is_normal_end and not self.breakable:
            return self.start
        # 由上级关闭引起的关闭
        return i if self.current == self else self.current.end(i, False)
    def add(self, obj):
        if not self.current.childable:
            return
        if self.current == self:
            self.child.append(obj)
            if obj.childable:
                self.current = obj
        else:
            self.current.add(obj)
        return
    def remove(self, obj):
        if not self.childable:
            return
        self.child.remove(obj)
    def get_text(self, mode:str="text"):
        ret = self.text_process(self.line.get_text(), "self")
        for i in self.child:
            for j in i.get_text(mode).splitlines():
                ret += self.text_process(j, "child")
        ret += self.text_process("", "after")
        return ret
    def end_condition(self, match: re.Match | None) -> bool:
        """For Sub class"""
        if match is None:
            return False
        if not isinstance(self.current, Title):
            return True
        return False
    def text_process(self, text:str, position:str) -> str :
        r"""For Sub class"""
        if len(text) > 0 and text[-1] != "\n":
            text+="\n"
        if position == "child":
            text = f"|   {text}"
        return f"{text}"

class Text(Root):
    def __init__(self, document) -> None:
        super().__init__(document)
        self.childable = False
        self.line.s = re.sub(r"^ *", "", self.line.s)
    def text_process(self, text:str, position:str) -> str :
        if len(text) > 0 and text[-1] != "\n":
            text+="\n"
        return f"{text}"

class Comment(Root):
    def __init__(self, document) -> None:
        super().__init__(document)
        self.childable = False
    def text_process(self, text:str, position:str) -> str :
        return f""

class Meta(Root):
    def __init__(self, document) -> None:
        super().__init__(document)
        self.childable = False
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

        if self.key != "setupfile":
            return
        if self.value in self.document.setupfiles:
            return
        content = ""
        try:
            req = requests.get(self.value, timeout=3)
            print(f"WARN 在文件中插入外部链接可能拖慢转译速度({req.elapsed})[{self.value}]")
            if req.status_code == 200:
                req.encoding = req.apparent_encoding
                content = req.text
        except:
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
    def text_process(self, text:str, position:str) -> str :
        return ""

class Title(Root):
    def __init__(self, document) -> None:
        super().__init__(document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.id = []
        self.level = len(match.group(1))
        self.comment = match.group(2) != ""
        self.line = Strings(match.group(3))
        self.TAG = Strings(match.group(4))

        last = self.document.table_of_content[-1] if self.document.table_of_content else []
        li = []
        num = 0
        for num in range(self.level):
            if num >= len(last)-1:
                li.append(0)
                continue
            li.append(last[num])
        li[num] += 1
        text = self.line.get_text()
        li.append(text + f"{get_str_in_width("",50-get_str_width(text), align="<r>")+":"+self.TAG.s+":" if self.TAG.s else ""}")
        self.id = li
        self.document.table_of_content.append(li)
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if len(match.group(1)) <= self.level:
            return True
        # 针对下级
        if self.current == self:
            return False
        if type(self.current).__name__ in ("BlockCode", "BlockQuote"):
            return True
        return False
    def text_process(self, text:str, position:str) -> str :
        if position == "self" and self.comment:
            text = f"{text}(COMMENTED)"
        if len(text) > 0 and text[-1] != "\n":
            text+="\n"
        if position == "self":
            lv = ""
            for i in self.id[:-1]:
                lv+=f"{i}."
            lv = lv[:-1]
            ret = f"{lv}: {text}"
            if self.TAG.s:
                ret = ret[:-1]
                ret += f"\t\t<:{self.TAG.s}:>\n"
            return ret
        if position == "child":
            return f"|   {text}"
        return ""

class Footnotes(Title):
    def __init__(self, document) -> None:
        Root.__init__(self, document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.id = []
        self.level = len(match.group(1))
        self.comment = match.group(2) != ""
        self.line = Strings(match.group(3))
    def text_process(self, text:str, position:str) -> str :
        return ""

class List(Root):
    def __init__(self, document) -> None:
        super().__init__(document)
        match = RULES[type(self).__name__]["match"].match(self.line.s)
        if match is None:
            return
        self.indent = len(match.group(1))
        self.level = 1
        ids = re.sub("[^0-9]*", "", match.group(2))
        self.id = int(ids) if ids != "" else -1
        self.line = Strings(match.group(3))
    def add(self, obj):
        if type(obj).__name__ == "List":
            obj.level = self.level+1
        return super().add(obj)
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if len(match.group(1)) <= self.indent and match.group(2) != "":
            i = -1
            for i in self.document.is_in_src:
                if i > self.start:
                    break
            if self.current == i:
                return True
            # 同侧退出，异侧不影响
            if (i-self.start)/(i-self.document.current_line) < 0:
                return False
            return True
        return False
    def text_process(self, text: str, position: str) -> str:
        if position == "self":
            ret = f"{self.level}" if self.id <= 0 else f"{self.level}[{self.id}]"
            return f"- L{ret} {text}"
        if position == "child":
            if self.child and isinstance(self.child[0], Text) and text==self.child[0].line:
                return f"{text}"
            if len(text) > 0 and text[0] != "\n":
                text=f"\n|   {text}"
            return f"{text}"
        return text

class BlockCode(Root):
    def __init__(self, document) -> None:
        super().__init__(document)
        self.breakable = False
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
        if type(obj).__name__ in ("BlockCode", "BlockQuote"):
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
        self.line = Strings(self.line)
        return True
    def text_process(self, text: str, position: str) -> str:
        if position == "self" and isinstance(self.line, Strings):
            ret = f",----{f" Lang:{self.lang}" if self.lang else ""}\n"
            for i in self.line.s.splitlines():
                ret += f"| {i}\n"
            ret += "`----"
            return ret
        return ""

class BlockExport(BlockCode):
    def text_process(self, text: str, position: str) -> str:
        if position == "self" and isinstance(self.line, Strings):
            ret = f",vvvv ExportMode:{self.lang}\n"
            for i in self.line.s.splitlines():
                ret += f"| {i}\n"
            ret += "`^^^^"
            return ret
        return ""

class BlockQuote(Root):
    def __init__(self, document) -> None:
        super().__init__(document)
        self.breakable = False
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
    def text_process(self, text: str, position: str) -> str:
        if len(text) > 0 and text[-1] != "\n":
            text+="\n"
        if position == "self":
            return ",========\n"
        if position == "child":
            return f"> {text}"
        if position == "after":
            return "`========"
        return ""

class Table(Text):
    def __init__(self, document) -> None:
        super().__init__(document)
        line = self.line.s
        split = re.match(r"\|[-+]+|", line)
        if split and split.group() != "":
            line = re.sub(r"\+","|",line)
        match = re.findall(r"\|([^|]*)", line)
        if len(match)>1 and match[-1] == "":
            match = match[:-1]
        self.col = len(match)
        self.lines : list[list[Strings]]= [[Strings(re.sub(r"^ *(.*?) *$", r"\1", i)) for i in match]]
        self.width = [get_str_width(i.get_text()) for i in self.lines[0]]
        self.control_line = []
        self.align = []
        self.check_control_line()
    def check_control_line(self):
        self.control_line = []
        index = 0
        for i in self.lines:
            is_break_split = False
            may_is_align = False
            is_break_align = False
            for j in i:
                result = re.findall(r"[^-]", j.s)
                if result:
                    is_break_split = True
                if j.s.lower() in ("<l>", "<c>", "<r>"):
                    may_is_align = True
                if j.s.lower() not in ("<l>", "<c>", "<r>", ""):
                    is_break_align = True
            if may_is_align and not is_break_align:
                self.control_line.append((index, "align"))
            if not is_break_split:
                self.control_line.append((index, "split"))
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
        for i in self.lines:
            if len(i) != len(self.width):
                continue
            for j in range(len(i)):
                if get_str_width(i[j].get_text()) > self.width[j]:
                    self.width[j] = get_str_width(i[j].get_text())
    def add_line(self, obj):
        if type(obj).__name__ != "Table":
            return
        line = obj.lines[0]
        li = []
        if obj.col > self.col:
            for i in self.lines:
                i += [Strings("") for i in range(obj.col - self.col)]
                li.append(li)
            self.col = obj.col
        else:
            li = self.lines
            t = [Strings("") for i in range(self.col - obj.col)]
            line += t
            li.append(line)
        self.lines = li
        self.reset_width()
        self.check_control_line()
    def text_process(self, text:str, position:str) -> str :
        width = self.col+1
        for i in self.width:
            width+=i
        if position == "self":
            text=f"{get_str_in_width("",width,".")}\n"
            index = 0
            skip_list = [-1 if i[1] != "align" else i[0] for i in self.control_line]
            for line in self.lines:
                if index in skip_list:
                    index+=1
                    continue
                for col,width,align in zip(line, self.width, self.align):
                    text+=f"|{get_str_in_width(col.get_text(), width, align=align)}"
                text+="|\n"
                index+=1
        if position == "after":
            text=f"{get_str_in_width("",width,"`")}\n"
        return f"{text}"

#^ *#\+begin_([^ \n]+)(?:[ ]+(.*?))?\n(.*?)^ *#\+end_\1\n?
RULES = {
        "Meta":       {"match":re.compile(r"^[ ]*#\+([^:]*):[ ]*(.*)", re.I), "class":Meta},
        "Footnotes":  {"match":re.compile(r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(Footnotes *.*)"),
                       "class":Footnotes,
                       "end":re.compile(r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)", re.I)},
        "Title":      {"match":re.compile(r"^([*]+)[ ]+((?:COMMENT )?) *(.*?(?= +:.*:|$))(?: +:(.*):)?", re.I),
                       "class":Title,
                       "end":re.compile(r"^([*]+)[ ]+((?:COMMENT )?) *(.*?(?= +:.*:|$))(?: +:(.*):)?", re.I)},
        "BlockCode":  {"match":re.compile(r"^ *#\+begin_src(?:[ ]+(.*))?", re.I),
                       "class":BlockCode,
                       "end":re.compile(r"^ *#\+end_src", re.I)},
        "BlockExport":{"match":re.compile(r"^ *#\+begin_export(?:[ ]+(.*))?", re.I),
                        "class":BlockExport,
                        "end":re.compile(r"^ *#\+end_export", re.I)},
        "BlockQuote": {"match":re.compile(r"^ *#\+begin_quote", re.I),
                       "class":BlockQuote,
                       "end":re.compile(r"^ *#\+end_quote", re.I)},
        "Comment":    {"match":re.compile(r"^[ ]*#[ ]+(.*)", re.I),"class":Comment},
        "List":       {"match":re.compile(r"^( *)(-|\+|\*|[0-9]+(?:\)|\.)) +(.*)", re.I),
                       "class":List,
                       "end":re.compile(r"^( *)(.*)", re.I)},
        "Table":      {"match":re.compile(r"^ *\|", re.I), "class":Table},
        "Text":       {"match":re.compile(""), "class":Text},
        "Root":       {"match":re.compile(""),
                       "class":Comment,
                       "end":re.compile(r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)", re.I)},
        }

class Document:
    def __init__(self, lines:list[str], file_name:str="", setupfiles:list[str]=[]) -> None:
        self.lines = lines
        self.file_name = file_name
        self.current_line = 0
        self.is_in_src = []
        self.table_of_content = []
        self.setupfiles = setupfiles
        self.meta={
            "title":[],
            "date":[],
            "description":[],
            "author":[],
            "setupfile":[],
            "html_link_home":"",
            "html_link_up":"",
            "html_head":[],
        }

        self.root = Root(self)
        self.root.line.s = self.file_name if self.file_name else "<DOCUMENT IN STRINGS>"
        self.build_tree()
        self.merge_text(self.root)
    def build_tree(self):
        last = -1
        self.current_line = 0
        while self.current_line < len(self.lines):
            if self.current_line <= last:
                t = Text(self)
                self.root.add(t)
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
        last = None
        remove_list = []
        for i in node.child:
            if i.childable:
                self.merge_text(i)
                last = None
                continue
            if last is None or last.line.s == "":
                if isinstance(i, Text):
                    last = i
                continue
            if type(i) == type(last):
                if type(i).__name__ == "Text" and i.line.s != "":
                    last.line.s += i.line.s
                    remove_list.append(i)
                elif isinstance(last, Table):
                    last.add_line(i)
                    remove_list.append(i)
                else:
                    last = None
            else:
                last = None
        for i in remove_list:
            node.remove(i)
        return
    def get_table_of_content(self) -> str:
        ret = ""
        for i in self.table_of_content:
            count = 0
            lastest = 0
            j = ""
            for j in i:
                if isinstance(j, int):
                    lastest = j
                    count+=1
                else:
                    break
            ret+=f"{"."*((count-1)*3-1)}{" " if count>1 else ""}{lastest}. {j}\n"
        return ret

def main():
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
        print(ret.root.get_text())
        print(ret.get_table_of_content())
        pprint(ret.meta)
        # pprint(ret.setupfiles)
        # pprint(vars(ret))
    return ret

def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='解析org文件')
    parser.add_argument('-d', '--diff', action="store_true", help='使用org-python导出')
    parser.add_argument('-i', '--input', default=None, help='指定输入文件')
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    main()
