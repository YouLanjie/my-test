#!/usr/bin/python
# Get time
import time

t = time.localtime()
t_str = str(t[0])+"."+str(t[1])+"."+str(t[2])+" "+str(t[3])+":"+str(t[4])+":"+str(t[5])
print(t_str)
