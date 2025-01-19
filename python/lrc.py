#!/usr/bin/python

"""
用于给歌词上时间，很没用但有用
"""

import time
import os

def print_lrc():
    line = 1
    print("==> LRC:")
    print("      vvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
    for i in lrc:
        if (line >= count - 2 and line <= count + 2):
            print("  --> " + i, end="")
        line += 1
    print("      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^")

if __name__ == "__main__":
    start = time.time()
    fp = open("output.txt", "w")

    lrc = os.popen("cat *.lrc")
    lrc = lrc.readlines()

    inp = ""
    minum = 0
    count = 5
    while inp != "q":
        print_lrc()
        inp = str(input("==> 按下回车标记，q退出:"))
        if inp == "q":
            break
        mark = time.time()
        t = mark - start
        minum = t // 60
        t -= 60 * minum
        count += 1
        print("  --> Time:[%02d:%05.2f]" % (minum, t))
        fp.write("[%02d:%05.2f]\n" % (minum, t))

    fp.close()
