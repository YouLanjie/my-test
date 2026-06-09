#!/usr/bin/env python
# Created:2026.06.09
# 手搓下吧。。。。
# Just for fun

import pickle
from pathlib import Path
from pprint import pprint
from collections import Counter
import sys
import random
import time
import argparse
try:
    import readline
    del readline
except ModuleNotFoundError:
    pass

#           当前字符  前一字符  后一字符 次数
WordCounter = dict[str,dict[str,dict[str,int]]]
Seperator = ",.?!-=()，。？！——……（）\r\n"

def load_data(file:Path):
    textdata = file.read_text()
    counter : WordCounter = {}
    total = len(textdata)
    t1 = time.monotonic()
    print("统计中",file=sys.stderr)
    for ind,c in enumerate(textdata):
        if not c or c.isspace() or not c.isprintable() or \
                ind >= len(textdata) or textdata[ind+1].isspace() or not textdata[ind+1].isprintable():
            continue
        if c not in counter:
            counter[c] = {}

        last = textdata[ind-1] if ind > 0 else ''
        if last in Seperator:
            last = ''
        if last not in counter[c]:
            counter[c][last] = {}

        nextc = textdata[ind+1]
        if nextc not in counter[c][last]:
            counter[c][last][nextc] = 0
        counter[c][last][nextc] += 1

        if ind % 100000 == 0:
            t2 = time.monotonic()
            progress = "="*int(35*(ind+1)/total)
            progress += " "*(35-len(progress))
            print(f"\r[{progress}] {(ind+1)/total*100:5.1f}% ({ind+1}/{total}) took {t2-t1:.0f}s ETA {(total/(ind+1)-1)*(t2-t1):.1f}s ",
                end="\r", file=sys.stderr)
    print(file=sys.stderr)
    return counter

def merge_data(counter1:WordCounter, counter2:WordCounter):
    for c,v in counter2.items():
        if c not in counter1:
            counter1[c] = v
            continue
        for lc,nc in v.items():
            if lc not in counter1[c]:
                counter1[c][lc] = nc
                continue
            for w,i in nc.items():
                if w not in counter1[c][lc]:
                    counter1[c][lc][w] = i
                    continue
                counter1[c][lc][w] += i
    return counter1

def counter_filter(counter:WordCounter, cut=10):
    for _,lw in counter.items():
        for _,v in lw.items():
            keys = [k for k in v]
            for k in keys:
                if v[k] < cut:
                    v.pop(k)
        keys = [k for k in lw]
        for k in keys:
            if k.isspace() or not k.isprintable():
                lw.pop(k)
    return counter

def get_possibile_next_words(counter:WordCounter, ch:str, lc='', maxlen=20):
    if ch not in counter:
        return None
    if not counter[ch]:
        return None
    lc = lc[:1]
    if lc not in counter[ch] and lc in Seperator:
        lc = ''
    if lc not in counter[ch]:
        lc = random.choice(list(counter[ch].keys()))
    li = sorted([w for w in counter[ch][lc]], key=lambda x:counter[ch][lc][x], reverse=True)
    if maxlen != 0:
        li = li[:maxlen]
    return li

def get_possibile_next_words_withnum(counter:WordCounter, ch:str, lc='', maxlen=20, cut=10):
    if ch not in counter:
        return None
    if not counter[ch]:
        return None
    lc = lc[:1]
    if lc not in counter[ch] and lc in Seperator:
        lc = ''
    if lc not in counter[ch] or (cut > 0 and sum(v for _,v in counter[ch][lc].items()) < cut):
        lc = random.choice(list(counter[ch].keys()))
    li = sorted([w for w in counter[ch][lc]], key=lambda x:counter[ch][lc][x], reverse=True)
    if not li:
        print((lc, ch, counter[ch]))
    if maxlen != 0:
        li = li[:maxlen]
    return {k:counter[ch][lc][k] for k in li}

def gen_sentence(counter:WordCounter, ch:str, rand=1, maxrepeat=20):
    lc = ch[-2:-1]
    ch = ch[-1:]
    if ch not in counter:
        return ""
    head = lc + ch
    s = lc+ch
    while (li := get_possibile_next_words_withnum(counter, ch, lc, 0)):
        wcount = Counter(s)
        wnearcount = Counter(s[-10:])
        nl = [w for w in li if w not in wcount or ((wcount[w] < maxrepeat or w in head) and wnearcount[w] < 5)]
        if not nl:
            break
        weights = [li[w] for w in nl]
        lc = ch
        if rand > 0 and len(s) > rand:
            if head[:1] in nl and random.random() < 0.05:
                if head[1:]:
                    s += head[0]
                    lc = head[0]
                ch = head[-1:]
            else:
                ch = random.choices(nl, weights)[0]
        else:
            ch =  nl[0]
        s += ch
    return s

def print_pairs(counter:WordCounter):
    pairs = []
    for src, dst_dict in counter.items():
        for dst, cnt in dst_dict.items():
            pairs.append((f"{src}{dst}", cnt))
    pairs.sort(key=lambda x: x[1], reverse=True)
    pprint(pairs[:100])   # 最常见的二元组，如 “我们”、“可以”、“但是”...

def parse_arg():
    parser = argparse.ArgumentParser(description="语言连续性测试(just for fun)")
    parser.add_argument("chr", help="字符（仅第一个有用）")
    parser.add_argument("-f", "--file", default="merged.org", help="应当读取用于训练的文本文件")
    parser.add_argument("-c", "--cache", default=".cache_Ngram.pickle", help="pickle缓存文件")
    parser.add_argument("-n", "--num", default=1, type=int, help="非交互模式下尝试输出次数")
    parser.add_argument("-a", "--append", action="store_true", help="追加模式（减少重训练时间）")
    args = parser.parse_args()
    return {"chr":args.chr, "num":args.num,
            "file":args.file, "cache":args.cache, "append":args.append}

def main():
    args = parse_arg()
    pickf = Path(args["cache"])
    textf = Path(args["file"])
    counter = None
    if pickf.is_file():
        print(f"加载pickle文件({pickf})", file=sys.stderr)
        counter = pickle.loads(pickf.read_bytes())
        print("加载完成", file=sys.stderr)
    elif not textf.is_file():
        print(f"pickle文件({pickle})或原文件({args["file"]})均不可用", file=sys.stderr)
        return

    if not counter or (textf.is_file() and args["append"]):
        if counter:
            print(f"追加载入文件数据({textf})")
            counter = merge_data(counter, load_data(textf))
        else:
            print(f"加载文件数据({textf})")
            counter = load_data(textf)
        pickf.write_bytes(pickle.dumps(counter))
        print(f"已保存到pickle文件({pickf})", file=sys.stderr)

    # counter_filter(counter)
    # pprint(counter)
    # print_pairs(counter)
    keys = [k for k in counter]
    if args["chr"] != "INPUT":
        for _ in range(args["num"] if args["num"] >= 0 else 1):
            ch = random.choice(keys) + (random.choice(keys) if random.random() < 0.2 else "")
            ch = (args["chr"] or ch)[-2:]
            # print(f"{repr(ch)}的可能词列表(<=20)" + str(get_possibile_next_words_withnum(counter, ch, '', 20)))
            print("> "+gen_sentence(counter, ch))
        return
    print("==交互输入模式==")
    print("键入候选词(最多两个字)，使用`QUIT`退出")
    while (ch := input("$ ")) != "QUIT":
        if ch == "RAND":
            ch = random.choice(keys) + (random.choice(keys) if random.random() < 0.2 else "")
        print("> "+gen_sentence(counter, ch))

if __name__ == "__main__":
    main()
