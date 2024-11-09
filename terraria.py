#!/usr/bin/env manim -pqh -s
# 泰拉瑞亚刷怪范围绘图

from manim import *

def add_box(left, right, up, down, col, k2=1, fb=[0,0,0,0], title=""):
    k = 0.047*k2
    rect = Rectangle(width=(left+right)*k, height=(up+down)*k, color=col)
    rect.set_fill(color=col, opacity=0.9)
    rect.shift(RIGHT*((left+right)/2-left)*k + DOWN*((up+down)/2-up)*k)
    dot=Dot().next_to(rect, LEFT+UP)
    tit=Text(title, color=BLACK).scale(k*12).next_to(dot, RIGHT+DOWN)
    f=[]
    for i,j,fbi in zip([left, right, up, down], [LEFT, RIGHT, UP, DOWN], fb):
        f.append(Text(str(i), color=col).scale(k*10).next_to(rect, 4*k*j).shift(fbi))
    shape = Group(rect,tit)
    text = Group(f[0], f[1], f[2], f[3])
    return shape,text

class terraria(Scene):
    def construct(self):
        # plane = NumberPlane()
        # self.add(plane)
        # ax = Axes([-250, 220], [-140, 120])
        # self.add(plane, ax)
        r1,t1 = add_box(250, 220, 120, 140, ORANGE, k2=0.60, title="此范围外的怪物会忽视计时器直接清除")
        r2,t2 = add_box(120, 121, 68, 72, YELLOW, title="NPC检测判定区域")
        r3,t3 = add_box(83, 85, 61, 64, GREEN, fb=[DOWN*2.25,DOWN*2.25,0,0], title="环境判定区")
        r4,t4 = add_box(84, 83, 46, 45, RED, title="刷怪区")
        r5,t5 = add_box(63, 65, 36, 38, BLUE, title="安全区")
        dot=Dot(color=WHITE)
        player=Text("以玩家左上角方块为原点计算",color=WHITE).scale(0.45).next_to(dot, DOWN)
        g1 = Group(r1, r2, r3, r4, r5)
        g2 = Group(t1, t2, t3, t4, t5)
        g3 = Group(dot, player)
        g1.shift(0.475*RIGHT+0.25*UP)
        g2.shift(0.475*RIGHT+0.25*UP)
        g3.shift(0.475*RIGHT+0.25*UP)
        self.add(g1, g2, g3)

