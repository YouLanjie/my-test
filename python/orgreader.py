#!/usr/bin/python

import sys
import pdb
import os
import time
from pathlib import Path
import re

debugmode = False

class Text:
    pattern = r"(.*)\n"
    rearg = re.M
    indent = "|   "
    def __init__(self, s:str) -> None:
        # if type(self).__name__ == "Text":
            # print([s])
        self.text = s
        if len(s) > 1 and s[-1] == "\n":
            s=s[:-1]
        self.content = [s]
    def match(self):
        ret = re.search(self.pattern, self.text, self.rearg)
        return ret
    def add_content(self, t):
        self.content.append(t)
    def to_text(self, level = 0):
        ret = ""
        for i  in self.content:
            # ret += self.indent*level
            ret += i
        # ret += "\n"
        return ret
    def __str__(self) -> str:
        ret = f"<{type(self).__name__}> {self.content}"
        return ret

class Newline(Text):
    def __init__(self, s: str = "\n") -> None:
        super().__init__(s)
    def to_text(self, level=0):
        # ret = self.indent*level
        ret = "\n"
        return ret

class Comment(Text):
    pattern = r"^[ ]*#[ ]+(.*)\n"
    def to_text(self, level = 0):
        return ""

class Meta(Text):
    pattern = r"^[ ]*#\+([^:\n]*):[ ]*(.*)\n"
    def __init__(self, s: str) -> None:
        super().__init__(s)
        ret = self.match()
        if ret is None:
            self.key = ""
            self.value = ""
            return
        self.key = ret.group(1).lower()
        self.value = ret.group(2)
    def __str__(self) -> str:
        ret = f"<{type(self).__name__}> [{self.key}:{self.value}]"
        return ret
    def to_text(self, level = 0):
        return ""

class Title(Text):
    pattern = r"^([*]+)[ ]+((?:COMMENT )?)[ ]*(.*)\n"
    def __init__(self, s:str) -> None:
        super().__init__(s)
        ret = self.match()
        if ret is None:
            return
        self.type = len(ret.group(1))
        self.attr = {"comment":not ret.group(2) == ""}
        self.name = [ret.group(3)]
        self.content = []
    def __str__(self) -> str:
        ret = f"<{type(self).__name__}> {self.to_text()}"
        if self.attr.get("comment"):
            ret += " <COMMENTED>"
        return ret
    def to_text(self, level=0):
        if self.attr.get("comment"):
            return ""
        ret = self.indent*(self.type-1)
        ret += f"H{self.type}: {self.name[0]}\n"
        for i in self.content:
            if debugmode:
                ret += f"{self.type}"+ "="*(self.type*2-1) + f"> {str(i)}"
                if ret[-1] != "\n":
                    ret += "\n"
            else:
                if ret[-1] == "\n" and type(i) in (Text,Newline):
                    ret += self.indent*self.type
                ret += f"{i.to_text(level = self.type)}"
        if ret[-1] != "\n":
            ret += "\n"
        return ret

class List(Text):
    rearg = re.M
    pattern = r"^( *)- +(.*)\n?((?:^\1 +.*\n?|^\n)*)"
    def __init__(self, s: str) -> None:
        super().__init__(s)
        ret = self.match()
        if ret is None:
            return
        self.level = len(ret.group(1))
        self.text = ret.group(2)
        self.content = []
        content = ret.group(3)
        if content != "":
            ret = get_document(content)
            li = []
            for i in ret:
                old = i
                if type(i) == Text:
                    i.content[0] = re.sub(r"(^|\n)[ ]*", r"\1", i.content[0], re.M)
                li.append(i)
            self.content += li
    def __str__(self) -> str:
        ret = f"<{type(self).__name__} lv={self.level}> {self.text}"
        for i in self.content:
            if ret[-1] != "\n":
                ret += "\n"
            ret += self.indent*(self.level+1)
            if issubclass(type(i), Text):
                ret += str(i)
            else:
                ret += f"{i}"
        return ret
    def to_text(self, level=0):
        ret = self.indent*level
        ret += f"- Lv.{level}: {self.text}"
        for i in self.content:
            if isinstance(i, List):
                if ret[-1] != "\n":
                    ret += "\n"
            if issubclass(type(i), Text):
                if ret[-1] == "\n":
                    lv = level+1
                    if type(i) == Text:
                        ret += self.indent*(level+1)
                else:
                    lv = 0
                ret += i.to_text(lv)
            else:
                if ret[-1] == "\n":
                    ret += self.indent*(level+1)
                ret += f"{i}"
        if ret[-1] != "\n":
            ret += "\n"
        return ret
    def update_text(self, level = 0):
        self.level = level
        nl = []
        for i in self.content:
            if isinstance(i, List):
                i.update_text(level+1)
            if isinstance(i, str):
                i = get_document(i)
            else:
                i = [i]
            nl+=i
        self.content = nl

class Block(Text):
    rearg = re.M|re.DOTALL|re.I
    rules = {
            "default":{
                "left":"| ", "up_and_down":"-----",
                "inner" : False },
            "quote":{
                "left":"| ", "up_and_down":".....",
                "inner" : True },
            "src":{
                "left":"> ", "up_and_down":"-----",
                "inner" : False },
            }
    typ = ("src", "quote")
    pattern = r"^ *#\+begin_([^ \n]+)(?:[ ]+(.*?))?\n(.*?)^ *#\+end_\1\n?"
    def __init__(self, s: str) -> None:
        super().__init__(s)
        ret = self.match()
        if ret is None:
            return
        self.name = ret.group(1).lower()
        if self.name not in self.typ:
            self.name = "default"
            return
        self.value = ret.group(2) # language
        self.content = [ret.group(3)]
        if self.rules[self.name]["inner"]:
            self.content = get_document(self.content[0])
        else:
            base = min([len(i) for i in re.findall(r"^([ ]*)[^ \n]+", self.content[0], re.M)])
            self.content[0] = re.sub(r"(^|\n)[ ]{%d}"%base, r"\1", self.content[0], re.M)
            pass
    def match(self):
        # pat = self.pattern % (self.typ, self.typ)
        ret = re.search(self.pattern, self.text, self.rearg)
        return ret
    def __str__(self) -> str:
        ret = f"<{type(self).__name__} name={self.name} len={len(self.content)} key={self.value}>\n"
        if self.name == "src":
            ret += self.to_text()
            return ret
        for i in self.content:
            if isinstance(i, Text):
                ret += f"{i.to_text()}"
            else:
                ret += f"{[i]}"
        return ret
    def to_text(self, level = 0):
        ret = self.indent*(level) + ","+self.rules[self.name]["up_and_down"]*3+"\n"
        for i in self.content:
            inp = ""
            if isinstance(i, Text):
                inp = i.to_text()
            else:
                inp = i
            li = inp.splitlines()
            for j in li:
                ret += self.indent*(level) + f"{self.rules[self.name]["left"]}{j}\n"
        ret += self.indent*(level) + "`"+self.rules[self.name]["up_and_down"]*3+"\n"
        return ret

def getp(s, m):
    if m.pattern is None:
        return [s]
    ret = re.search(m.pattern, s, m.rearg)
    if ret is None:
        return [s]
    if ret.group() == "":
        return []
    obj = m(ret.group())
    if obj is None:
        return [s]
    # if m == Text:
        # print(f"{str(obj)}")
    before = s[:ret.span()[0]]
    after = s[ret.span()[1]:]
    li = []
    if before not in ("", "\n"):
        li.append(before)
    li.append(obj)
    if after != "":
        li += getp(after, m)
    return li

def get_document(s:str, mode=True):
    li = [s]
    li_type = (Meta, Title, Block, Comment, List, Text)
    for i in li_type:
        nl = []
        for j in li:
            if not isinstance(j, str):
                nl.append(j)
                continue
            l = getp(j, i)
            nl += l
        li = nl
    nl = []
    title = None
    last = None
    for i in li:
        if isinstance(i, Title):
            title = i
            nl.append(i)
            continue
        if type(i)==Text and i.content[0] == "\n":
            if type(last)==Newline:
                continue
            i = Newline()
        if type(last)==Text and type(i) not in (Text, Newline):
            tmp = Newline()
            if title is not None:
                title.add_content(tmp)
            else:
                nl.append(tmp)
        if isinstance(i, List):
            i.update_text()
        last = i
        if title is not None:
            title.add_content(i)
        else:
            nl.append(i)
    return nl

def readf(s:str, mode=True):
    li = get_document(s, mode)
    ret = ""
    for i in li:
        ret += i.to_text()
    return ret

class Document:
    def __init__(self, file:Path) -> None:
        self.text = file.read_text()
        self.meta={
            "title":"",
            "date":"",
            "description":"",
            "author":"",
            "setupfile":[],
            "html_link_home":"",
            "html_link_up":"",
            "html_head":"",
        }
        self.setupfiles = []
        self.content = ""
        self.output = ""
        self._get_meta()
        self._get_content()
        self._process_text()
    def _get_meta(self, content : None | str = None):
        if content is None:
            content = self.text
        li = re.findall(r"^[ ]*#\+([^:\n]*):[ ]*(.*)\n", content, flags=re.MULTILINE)
        if not isinstance(li, list):
            return
        for i,j in li:
            i = i.lower()
            if i not in self.meta:
                continue
            if isinstance(self.meta[i], list):
                self.meta[i].append(j)
                continue
            if self.meta[i] != "":
                self.meta[i] += "\n" if i in ("html_head") else " "
            self.meta[i] += j
        for i in self.meta["setupfile"]:
            file = Path(i).resolve()
            if not file.exists() or file in self.setupfiles:
                continue
            self.setupfiles.append(file)
            self._get_meta(file.read_text())
    def _get_content(self):
        li = re.findall(r"^([*]+)[ ]+(.*)\n", self.text, re.MULTILINE)
        content = [(len(i), j) for i,j in li]
        self.content = content
    def _process_text(self):
        self.output = readf(self.text)

def main():
    inpf = Path("./res/test.org")
    norg = Document(inpf)
    print(f"{norg.meta}\n\n{norg.content}\n")
    print(f"{norg.output}", end="")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "-d":
        debugmode = True
    main()
