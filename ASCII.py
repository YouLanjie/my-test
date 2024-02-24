#!/usr/bin/env python
# -*- coding: utf-8 -*-

i = 0
pch = 0

print("Num |  Hex  |    Bit    | Char")
while (i < 260):
    if i < ord('!'):
        pch = ' '
    else:
        pch = '%c' % i

    if i == ord('\r'):
        pch = '\\r'
    elif i == ord('\n'):
        pch = '\\n'
    elif i == ord('\t'):
        pch = '\\t'
    elif i == 0x1B:
        pch = '<ESC>'

    print("%03d | 0x%03x | %09d | %s" % (i, i, int(bin(i)[2:]), pch))
    i += 1
