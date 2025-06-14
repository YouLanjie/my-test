#!/usr/bin/python

import pdb
import os
from pprint import pprint
import sys
from pathlib import Path
import re

def dp(s):
    if len(sys.argv) > 1 and sys.argv[-1] == "--debug":
        print(s)

class SignalBox:
    def __init__(self) -> None:
        self.current_line = 0
        self.is_in_src = []
        self.table_of_content = []

class Root:
    def __init__(self, lines:list[str], i:int) -> None:
        self.start = i
        self.childable = True
        self.line = lines[i]
        self.breakable = True
        self.end_offset = 0
        self.child = []
        self.current = self
    def checkend(self, lines:list[str], i:int) -> tuple[bool,int]:
        if not self.childable:
            return False, i
        match = re.match(RULES[type(self).__name__]["end"], lines[i], re.I)
        ret = i
        SIGNALBOX.current_line = i
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
            dp(f"ERR: {self.start}:{self}")
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
        ret = self.text_process(self.line, "self")
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
    def __init__(self, lines: list[str], i: int) -> None:
        super().__init__(lines, i)
        self.childable = False
        self.line = re.sub(r"^ *", "", self.line)
    def text_process(self, text:str, position:str) -> str :
        if len(text) > 0 and text[-1] != "\n":
            text+="\n"
        return f"{text}"

class Comment(Root):
    def __init__(self, lines: list[str], i: int) -> None:
        super().__init__(lines, i)
        self.childable = False
    def text_process(self, text:str, position:str) -> str :
        return f""

class Meta(Root):
    def __init__(self, lines: list[str], i: int) -> None:
        super().__init__(lines, i)
        self.childable = False
        match = re.match(RULES[type(self).__name__]["match"], self.line, re.I)
        if match is None:
            return
        self.key = match.group(1)
        self.value = match.group(2)
    def text_process(self, text:str, position:str) -> str :
        return ""

class Title(Root):
    def __init__(self, lines: list[str], i: int) -> None:
        super().__init__(lines, i)
        match = re.match(RULES[type(self).__name__]["match"], self.line, re.I)
        if match is None:
            return
        self.id = []
        self.level = len(match.group(1))
        self.comment = match.group(2) != ""
        self.line = match.group(3)

        last = SIGNALBOX.table_of_content[-1] if SIGNALBOX.table_of_content else []
        li = []
        num = 0
        for num in range(self.level):
            if num >= len(last)-1:
                li.append(0)
                continue
            li.append(last[num])
        li[num] += 1
        li.append(self.line)
        self.id = li
        SIGNALBOX.table_of_content.append(li)
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
            return f"{lv}: {text}"
        if position == "child":
            return f"|   {text}"
        return ""

class List(Root):
    def __init__(self, lines: list[str], i: int) -> None:
        super().__init__(lines, i)
        match = re.match(RULES[type(self).__name__]["match"], self.line, re.I)
        if match is None:
            return
        self.indent = len(match.group(1))
        self.level = 1
        self.line = match.group(2)
    def add(self, obj):
        if type(obj).__name__ == "List":
            obj.level = self.level+1
        return super().add(obj)
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        if len(match.group(1)) <= self.indent and match.group(2) != "":
            i = -1
            for i in SIGNALBOX.is_in_src:
                if i > self.start:
                    break
            if self.current == i:
                return True
            # 同侧退出，异侧不影响
            if (i-self.start)/(i-SIGNALBOX.current_line) < 0:
                return False
            return True
        return False
    def text_process(self, text: str, position: str) -> str:
        if position == "self":
            return f"- L{self.level} {text}"
        if position == "child":
            if self.child and isinstance(self.child[0], Text) and text==self.child[0].line:
                return f"{text}"
            if len(text) > 0 and text[0] != "\n":
                text=f"\n|   {text}"
            return f"{text}"
        return text

class BlockCode(Root):
    def __init__(self, lines: list[str], i: int) -> None:
        super().__init__(lines, i)
        self.breakable = False
        self.line = lines
        self.number = 0
        self.end_offset = 1
        self.baise = -1
        SIGNALBOX.is_in_src.append(self.start)
        match = re.match(RULES[type(self).__name__]["match"], lines[i], re.I)
        if match is None:
            return
        self.lang = match.group(1) if match.group(1) else ""
        dp(f">>> APP_SRC:{SIGNALBOX.is_in_src}")
    def add(self, obj):
        if type(obj).__name__ in ("BlockCode", "BlockQuote"):
            dp(f"<<< POP_ERRINSRC:{SIGNALBOX.is_in_src}")
            SIGNALBOX.is_in_src.pop(-1)
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
            dp(f"<<< POP_SRC: {i}, {SIGNALBOX.is_in_src}")
            index = SIGNALBOX.is_in_src.index(self.start)
            SIGNALBOX.is_in_src=SIGNALBOX.is_in_src[:index]
        return ret
    def end_condition(self, match: re.Match | None) -> bool:
        if match is None:
            return False
        lines = self.line[self.start+1:self.start+1+self.number]
        lines = [re.sub(f"^{" "*self.baise}", "", i) for i in lines]
        self.line = ""
        for i in lines:
            if re.match(r"^,[*,]",i) or re.match(r"^,#\+",i):
                i=i[1:]
            self.line += f"{i}\n"
        return True
    def text_process(self, text: str, position: str) -> str:
        if position == "self" and isinstance(self.line, str):
            ret = f",---- Lang:{self.lang}\n"
            for i in self.line.splitlines():
                ret += f"| {i}\n"
            ret += "`----"
            return ret
        return ""

class BlockQuote(Root):
    def __init__(self, lines: list[str], i: int) -> None:
        super().__init__(lines, i)
        self.breakable = False
        self.end_offset = 1
        SIGNALBOX.is_in_src.append(self.start)
        # dp(f">>> APP_QUOT:{SIGNALBOX.is_in_src}")
    def end(self, i: int, is_normal_end: bool) -> int:
        ret = super().end(i, is_normal_end)
        if ret == i or not is_normal_end:
            dp(f"<<< POP_QUOT: {i}, {SIGNALBOX.is_in_src}")
            index = SIGNALBOX.is_in_src.index(self.start)
            SIGNALBOX.is_in_src=SIGNALBOX.is_in_src[:index]
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

#^ *#\+begin_([^ \n]+)(?:[ ]+(.*?))?\n(.*?)^ *#\+end_\1\n?
RULES = {
        "Meta":      {"match":r"^[ ]*#\+([^:]*):[ ]*(.*)", "class":Meta},
        "Title":     {"match":r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)",
                      "class":Title,
                      "end":r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)"},
        "BlockCode": {"match":r"^ *#\+begin_src(?:[ ]+(.*))?",
                      "class":BlockCode,
                      "end":r"^ *#\+end_src"},
        "BlockQuote":{"match":r"^ *#\+begin_quote",
                      "class":BlockQuote,
                      "end":r"^ *#\+end_quote"},
        "Comment":   {"match":r"^[ ]*#[ ]+(.*)","class":Comment},
        "List":      {"match":r"^( *)- +(.*)",
                      "class":List,
                      "end":r"^( *)(.*)"},
        "Text":      {"match":"", "class":Text},
        "Root":      {"match":r"","class":Comment,"end":r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)"},
        }
SIGNALBOX = SignalBox()

def merge_text(node:Root):
    last = None
    remove_list = []
    for i in node.child:
        if i.childable:
            merge_text(i)
            last = None
            continue
        dp(f"@-> i:{i.line}")
        if last is None or last.line == "":
            if isinstance(i, Text):
                last = i
                dp("@set last")
            continue
        if isinstance(i, Text) and i.line != "":
            last.line += i.line
            remove_list.append(i)
        else:
            dp("@clear last")
            last = None
    for i in remove_list:
        node.remove(i)

def build_tree(lines:list[str]):
    last = -1
    i = 0
    root = Root(lines, 0)
    root.line = "TEST DOCUMENT"
    while i < len(lines):
        dp(f"-> {i:2}, {last:3}, {str([lines[i]]):>25}")
        if i <= last:
            dp(f"  -> Force to text(call back)")
            t = Text(lines, i)
            root.add(t)
        else:
            for _,current in RULES.items():
                ret = re.match(current["match"], lines[i], re.I)
                if ret is None:
                    continue
                obj = current["class"](lines, i)
                root.add(obj)
                break
        last = i
        i+=1
        if i >= len(lines):
            break
        watch = lines[i]
        _,i=root.checkend(lines, i)
    merge_text(root)
    print(root.get_text())
    return root

def main():
    inpf = list(Path(".").glob("**/*.org"))
    if len(inpf) == 0:
        return
    if not inpf[-1].is_file():
        return
    inp = """\
- Lv1
  #+begin_quote
    普通文字1
    - Lv2
    #+begin_quote
      普通文字2
      - Lv3
      #+begin_quote
中途下坠极端测试
- Lv5(0缩进)
  - Lv6
    child内容
      #+end_quote
    #+end_quote
  #+end_quote
"""
    flag = False
    # flag = True
    if flag:
        build_tree(inp.splitlines())
    else:
        build_tree(inpf[-1].read_text().splitlines())
    # pprint(vars(SIGNALBOX))

if __name__ == "__main__":
    main()
