'''
本程序为同名c程序用python写的仿制程序
'''
import os

if __name__ == '__main__':
    ctype = "mp3"
    i = 1

    os.system("clear")
    try:
        os.mkdir("data")
    except FileExistsError:
        print("文件夹已存在")
    try:
        os.mkdir("data/input")
    except FileExistsError:
        print("文件夹已存在")
    try:
        os.mkdir("data/output")
    except FileExistsError:
        print("文件夹已存在")
    while i == 1:
        os.system("clear")
        i = int(input("\033[1;32mWelocme\n\033[33minput '1' to start\n"))
        os.system("clear")
        if i == 1:
            fileName = os.listdir("data/input/")
            for filename in fileName:
                print(filename)
                filename_o = "ffmpeg -i \'" + filename + "\' /data/output"
                if filename[-4] == '.':
                    input()
                    #filename[-3] = 'm'
                    #filename[-2] = 'p'
                    #filename[-1] = '3'
                filename_o = filename_o + filename
                print(filename)
                print(filename_o)
                #os.access("data/output/"+filename)
                input()

