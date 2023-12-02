#!/usr/bin/python
# 计算质数

limit = 1000
list = [2]

def check(last):
    flag = True
    for i in list:
        if last % i == 0:
            flag = False
            break
    if flag:
        list.append(last)

def print_list():
    for i in list:
        print("-> " + str(i))

if __name__ == "__main__":
    for i in range(2,limit - 2):
        check(i)
    print_list()
    print("Total: " + str(len(list)))

