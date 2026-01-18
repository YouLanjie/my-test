#!/usr/bin/manim -sqk
# 无职人物关系图

# from typing import Any, Iterable
from pathlib import Path
import python.family as family
import manim as m

class People(family.People):
    cfg_template = {".*":{
        "male":True,
        "comment":"",
        "child":[],
        "father":None,
        "mother":None,
        "partner":set(),
        "level":1,
        "fixed":False,

        "fgcolor":m.WHITE,
        "bgcolor":m.BLACK,
        "position":m.ORIGIN
        }}
    graph_size = 0.7
    def __init__(self, source: Path | str | bytes | dict):
        super().__init__(source)
        self.size = max([len(v) for _,v in self.to_levels().items()] or [0])
        self.graph = m.VGroup()
        level = 0
        families = {}
        for _,i in self.to_levels().items():
            count = 0
            group = m.VGroup()
            for k,v in i.items():
                person = m.VGroup()
                if v["male"]:
                    person += m.Rectangle(color=v["fgcolor"], height=self.graph_size, width=self.graph_size)
                else:
                    person += m.Circle(self.graph_size/2, color=v["fgcolor"])
                person[0].set_fill(v["bgcolor"], 0.7)
                person += m.Text(k, color=m.GREEN).scale(0.3)
                # v["position"] = (3.5-1.1*level)*m.UP + (count-1)*m.RIGHT
                # if v["father"] or v["mother"]:
                    # offset = ((self.cfg[v["father"]]["position"] if v["father"] else m.ORIGIN)+\
                            # (self.cfg[v["mother"]]["position"] if v["mother"] else m.ORIGIN))/2
                    # offset *= m.RIGHT
                    # v["position"] += offset
                # else:
                count += 1
                person.shift(v["position"])

                group += person
            group.arrange()
            self.graph += group.shift((3.5-1.1*level)*m.UP)
            level += 1

SOURCE = {
        "鲁迪": {"male":True, "child":["露西","齐格","克莉丝","亚尔斯","菈菈","莉莉"]},

        "保罗": {"male":True, "child":["鲁迪","诺伦","爱夏"]},
        "塞妮丝": {"male":False, "child":["鲁迪","诺伦"]},
        "莉莉雅": {"male":False, "child":["爱夏"]},
        "诺伦": {"male":False, "child":["瑞雪莉亚"]},
        "瑞杰路德": {"male":True, "child":["瑞雪莉亚"]},
        "爱夏": {"male":False, "child":["鲁洛伊"]},

        "p1": {"male": True, "child": [ "p3", "罗绍斯" ], "comment":"格雷拉特家" },
        "p2": {"male": False, "child": [ "p3", "罗绍斯" ] },

        "p3": {"male": True, "child": [ "保罗", "皮列蒙" ], "comment":"诺托斯家" },
        "p5": {"male": False, "child": [ "保罗", "皮列蒙" ] },
        "皮列蒙": {"male":True, "child": ["路克"]},
        "路克": {"male":True},

        "罗绍斯":{"male": True, "child": [ "菲利普", "詹姆士" ], "comment":"伯雷亚斯家" },
        "p6": {"male": False, "child": [ "菲利普", "詹姆士" ] },
        "菲利普": {"male":True, "child": ["艾莉丝"]},
        "希尔达": {"male":False, "child": ["艾莉丝"]},
        "艾莉丝": {"male":False, "child": ["克莉丝","亚尔斯"]},

        "艾莉娜丽洁": {"male":False, "child": ["罗尔兹", "克莱夫"]},
        "克里夫": {"male":True, "child": ["克莱夫"]},
        "p8": {"male":True, "child": ["罗尔兹"]},
        "罗尔兹": {"male":True, "child": ["希露菲"]},
        "p7": {"male":False, "child": ["希露菲"]},
        "希露菲": {"male":False, "child": ["露西","齐格"]},

        "洛琪希": {"male":False, "child":["菈菈","莉莉"]},
        "露西": {"male":False},
        "齐格": {"male":True},
        "克莉丝": {"male":False},
        "亚尔斯": {"male":True, "child":["鲁洛伊"]},
        "菈菈": {"male":False},
        "莉莉": {"male":False},
        "鲁洛伊": {"male":True, "child":["菲莉斯"]},
        "p9": {"male":False, "child":["菲莉斯"]},
        "菲莉斯": {"male": False},
}

# def add_box(left, right, up, down, col, k2=1, fb=[0,0,0,0], title=""):
    # k = 0.047*k2
    # rect = Rectangle(width=(left+right)*k, height=(up+down)*k, color=col)
    # rect.set_fill(color=col, opacity=0.9)
    # rect.shift(RIGHT*((left+right)/2-left)*k + DOWN*((up+down)/2-up)*k)
    # dot=Dot().next_to(rect, LEFT+UP)
    # tit=Text(title, color=BLACK).scale(k*12).next_to(dot, RIGHT+DOWN)
    # f=[]
    # for i,j,fbi in zip([left, right, up, down], [LEFT, RIGHT, UP, DOWN], fb):
        # f.append(Text(str(i), color=col).scale(k*10).next_to(rect, 4*k*j).shift(fbi))
    # shape = Group(rect,tit)
    # text = Group(f[0], f[1], f[2], f[3])
    # return shape,text

class Pic(m.Scene):
    def construct(self):
        plane = m.NumberPlane()
        self.add(plane)
        people = People(SOURCE)
        self.add(people.graph)
        __import__('pprint').pprint(people.to_levels())

        # ax = Axes([-250, 220], [-140, 120])
        # self.add(plane, ax)
        # r1,t1 = add_box(250, 220, 120, 140, ORANGE, k2=0.60, title="此范围外\n怪物会忽视\n计时器直接\n清除")
        # r2,t2 = add_box(120, 121, 68, 72, YELLOW, title="NPC检测\n判定区域")
        # r3,t3 = add_box(83, 85, 61, 64, GREEN, fb=[DOWN*2.25,DOWN*2.25,0,0], title="环境判定区")
        # r4,t4 = add_box(84, 83, 46, 45, RED, title="刷怪区")
        # r5,t5 = add_box(63, 65, 36, 38, BLUE, title="安全区")
        # dot=Dot(color=WHITE)
        # player=Text("以玩家左上角方块为原点计算",color=WHITE).scale(0.45).next_to(dot, DOWN)
        # g1 = Group(r1, r2, r3, r4, r5)
        # g2 = Group(t1, t2, t3, t4, t5)
        # g3 = Group(dot, player)
        # g1.shift(0.475*RIGHT+0.25*UP)
        # g2.shift(0.475*RIGHT+0.25*UP)
        # g3.shift(0.475*RIGHT+0.25*UP)
        # self.add(g1, g2, g3)

