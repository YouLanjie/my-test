#!/usr/bin/env python
# Created:2026.02.09

import sys
import time

def main():
    begging = time.time()
    if not sys.stdin:
        return
    if sys.stdin.isatty():
        print("Not allow stdin is a tty")
        return
    count = 0
    diff = 0.0
    while diff < 5:
        c = sys.stdin.read(1)
        # print(c, end="")
        count += 1
        diff = time.time() - begging
        if count % 10000 == 0:
            print(f"COUNT: {count} [{diff:0.7f}s] ({count/diff:0.7f}tps)" + " "*10 + "\r", end="")
        if not c:
            break
    print(f"\nRESULT: {count} [{diff:0.7f}s] ({count/diff:0.7f}tps)")
    print("QUIT")

if __name__ == "__main__":
    try:
        main()
    except (EOFError, KeyboardInterrupt) as e:
        print(e)
