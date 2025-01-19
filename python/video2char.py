import threading
import time
import os
import cv2

def play_video():
    str = 'mqpka89045321@#$%^&*()_=||||}'    # 字符表
    video = cv2.VideoCapture(file_name)     # 读取视频
    ret, frame = video.read()    # 读取帧
    
    os.system('clear')
    start_time = time.time()
    fps = video.get(5)
    # time.sleep(1)
    while ret:    # 逐帧读取
        fp = video.get(1)
        str_img = ''    # 字符画
        grey = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)    # 灰度转换
        grey = cv2.resize(grey, (100, 40))    # 该表大小
        for i in grey:    # 遍历每个像素点
            for j in i:
                index = int(j / 256 * len(str))    # 获取字符坐标
                str_img += str[index]    # 将字符添加到字符画中
            str_img += '\n'
        # os.system('clear')    # 清除上一帧输出的内容
        print("\033[1;1H" + str_img, end="")    # 输出字符画
        ret, frame = video.read()    # 读取下一帧
        cv2.waitKey(1)
        sleep_time = fp/fps - time.time() + start_time
        if sleep_time > 0:
            time.sleep(sleep_time)

def play_audio():
    os.system("ffplay " + file_name)

if __name__ == "__main__":
    file_name="input.mp4"
    # vo = threading.Thread(target=play_video)
    # ao = threading.Thread(target=play_audio)
    # ao.start()
    # vo.start()
    play_video()
