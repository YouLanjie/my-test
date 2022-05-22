#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
i = 0
os.system("clear")
print("Number | Hex | Char")
while (i < 260):
    print("%03d | " % i + "0x%03x" % i + " | " + "%c" % i)
    i += 1
