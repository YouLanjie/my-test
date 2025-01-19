#!/usr/bin/python
# Get time
import time

t = time.localtime()
print("%04d.%02d.%02d %02d:%02d:%02d" % (t[0], t[1], t[2], t[3], t[4], t[5]))
