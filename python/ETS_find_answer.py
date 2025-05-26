#!/usr/bin/env python
# Created:2025.05.18

import sys
from pathlib import Path
import json

ets_path = "/storage/emulated/0/Android/data/com.ets100.secondary/files/Download/ETS_SECONDARY/resource/"

"""
structure_type: type = collector.picture, collector.3q5a, collector.read
info: (read)
    value: text
    image: empty
    video: showing video file
    audio: playing audio file
info: (3q5a)
    value: ai saying before 3Q5A
    image: showing image
    audio: audio file
    question: []question list
        std: []answer list
            value: text of answer
            ai: same of value
            audio: answer audio file(may empty)
        sucai: answer audio file
        ask: printed chinese in 3Q | playing sound of 5A
        answer: reply need to record | nothing
        aswaudio: reply audio file in 3Q
        askaudio: question audio file in 5A
        xh: id
info: (picture)
    value: ai reading
    image: showing image
    audio: ai audio file
    analyze: analyze
    std: []answer list
        value: text of answer
        ai: same of value
        audio: answer audio file(may empty)
    topic: ...
"""

class Docum():
    def __init__(self, file : Path):
        if file.is_file() is False:
            print(f"`{str(file)}`不是文件")
            sys.exit(1)
        data = json.loads(file.read_text())
        self.content = []
        self.type = None
        if data.get("structure_type") ==  "collector.3q5a":
            self.type = "3q5a"
            self._get_3q5a(data)
        elif data.get("structure_type") ==  "collector.picture":
            self.type = "picture"
            self._get_picture(data)
    def _get_picture(self, data):
        info = data.get("info")
        ask = info.get("value")[:50]
        answer = info.get("std")[0].get("value")
        self.content.append({"tag":ask, "answer":answer})
    def _get_3q5a(self, data):
        questions = data.get("info").get("question")
        count = 0
        for i in questions:
            count+=1
            isasnw = i.get("answer") != ""
            typ = "三问" if isasnw else  "五答"
            ask = i.get("ask")
            answer = i.get("std")[0].get("value")
            self.content.append(
                    {"type":typ, "id":count, "tag":ask,
                     "answer":answer})
    @property
    def get_content(self):
        if self.type is None:
            return ""
        t = "====----====----====----====\n"
        if self.type == "picture":
            cont = self.content[0]
            t += f"RETELLING:\n -> {cont.get("answer")}\n"
            return t
        for i in self.content:
            t += f"{"3Q" if i.get("type") == "三问" else "5A"}:{i.get("tag")}\n -> {i.get("answer")}\n"
        return t

def run_main():
    input_f = list(Path(ets_path).glob("**/content.json"))
    if len(input_f) == 0:
        print("[!] no input file was found")
        return 0
    print(f"{len(input_f)} files were found")
    input_f = sorted(input_f, key=lambda x:x.stat().st_ctime, reverse=False)
    docs = []
    for i in input_f[-5:]:
        docum = Docum(i)
        if docum.type is not None:
            docs.append(docum)
    for i in docs:
        print(i.get_content)

if __name__ == "__main__":
    run_main()
