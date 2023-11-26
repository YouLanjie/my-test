#!/usr/bin/env python
# W = Pt , P = UI , Q = I^2 * t

if __name__ == "__main__":
    command = "manim -pql ./Physical3.py Physical"
    import os
    print("==> Run Command:")
    print(" -> " + command)
    os.system(command)
    print("==> Exit")
    exit()

from manim import *
from physical_base import *
import time

st = Text("九上物理：电能与电功率").set_color(RED)
class Physical(Scene):
    def get_date(self):
        t = time.localtime()
        t_str = str(t[0])+"."+str(t[1])+"."+str(t[2])+" "+str(t[3])+":"+str(t[4])+":"+str(t[5])
        t_sub = Text(t_str, color=YELLOW, opacity=0.3).to_edge(LEFT+UP)
        self.add(t_sub)
        return t_sub

    def sub_text(self, text, string, time=0):
        text2 = Text(string)
        text2.set_color(RED)
        text2.shift(3*DOWN)
        self.play(Transform(text, text2))
        self.wait(time)

    def scene_1(self):
        TIT_1 = Text("电能 电功", color=BLUE).scale(2)
        self.play(Write(TIT_1))
        self.sub_text(st, "首先,认识什么是电能和电功", 1.2)
        self.play(TIT_1.animate.to_edge(LEFT+UP))
        dot1 = Dot([-5.5, 1.5, 0]).set_color(YELLOW)
        dot2 = Dot([-5.5, 0.5, 0]).set_color(YELLOW)
        dot3 = Dot([-5.5, -0.5, 0]).set_color(YELLOW)
        dot4 = Dot([-5.5, -1.5, 0]).set_color(YELLOW)
        group = VGroup(
            dot1,
            Text("电能：是一种能量的形式").next_to(dot1, RIGHT),
            dot2,
            Text("电功：电流做功(转换为其他能量)的多少").next_to(dot2, RIGHT),
            dot3,
            Text("单位：焦耳，简称焦，符号J",
                 t2c={"耳":RED, "焦":RED, "J":RED}
                 ).next_to(dot3, RIGHT),
            dot4,
            Text("表示：字母W",
                 t2c={"W":RED}
                 ).next_to(dot4, RIGHT),
        )
        self.play(Write(group))
        self.wait(1)
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==
        self.sub_text(st, "电能是一种能量的形式，", 0.5)
        self.sub_text(st, "当电能转化为其他形式的能时", 1.3)
        self.sub_text(st, "我们说电流做了电功", 1.2)
        self.sub_text(st, "电能转化为其他形式能的多少", 1.3)
        self.sub_text(st, "可以用电功的大小来量度", 1.3)
        self.sub_text(st, "所以在选择题里抠字眼时", 1)
        self.sub_text(st, "电能是可以'拥有'的，电功不能", 1.4)
        self.play(FadeOut(group,shift=DOWN), FadeOut(TIT_1,shift=DOWN))

    def scene_2(self):
        TIT_1 = Text("电功率", color=BLUE).scale(2)
        self.play(Write(TIT_1))
        self.sub_text(st, "接下来,认识什么是电功率", 1.2)
        self.play(TIT_1.animate.to_edge(LEFT+UP))
        dot1 = Dot([-5.5, 1.5, 0]).set_color(YELLOW)
        dot2 = Dot([-5.5, 0.5, 0]).set_color(YELLOW)
        dot3 = Dot([-5.5, -0.5, 0]).set_color(YELLOW)
        dot4 = Dot([-5.5, -1.5, 0]).set_color(YELLOW)
        group = VGroup(
            dot1,
            Text("定义：某段时间电流所做电功与时间之比").next_to(dot1, RIGHT),
            dot2,
            Text("意义：电流做功的速度").next_to(dot2, RIGHT),
            dot3,
            Text("单位：瓦特，简称瓦，符号W",
                 t2c={"特":RED, "瓦":RED, "W":RED}
                 ).next_to(dot3, RIGHT),
            dot4,
            Text("表示：字母P",
                 t2c={"P":RED}
                 ).next_to(dot4, RIGHT),
        )
        self.play(Write(group))
        self.wait(1.5)
        self.sub_text(st, "电功率的计算公式同机械功的计算公式，即，", 0.3)
        text = MathTex("P = \\frac{W}{t}").scale(1.5)
        self.play(FadeOut(group,shift=DOWN), Write(text))
        self.sub_text(st, "P = W / t", 0.5)
        self.sub_text(st, "其中，t的单位是秒(s)", 1.2)
        self.sub_text(st, "将公式转换下便可得到 W = Pt", 0.2)
        self.play(Transform(text, MathTex("W = Pt").scale(1.5)))
        self.sub_text(st, "当P的单位为千瓦(kW)，t的单位为小时(h)时", 1.5)
        self.sub_text(st, "我们便可得到W的第二个单位:kW·h", 0.5)
        self.play(text.animate.shift(UP))
        text2 = MathTex("1kW = 1000W,1h = 60 \\times 60s = 3600s").scale(1.5)
        self.sub_text(st, "因为1kW = 1000W,1h = 60*60s", 0.4)
        self.play(Write(text2))
        text3 = MathTex("1 kW·h = 1000 W \\times 3600 s = 3.6 \\times 10^6 J").scale(1.5).shift(DOWN)
        self.sub_text(st, "所以1kW·h=1000W*3600s=3.6*(10^6)J", 0.4)
        self.play(Write(text3))
        self.play(Transform(text3, MathTex("1 kW·h = 3.6 \\times 10^6 J").scale(1.5).shift(DOWN)))
        self.sub_text(st, "千瓦时又被称作度，在生活中广泛使用", 1.5)
        self.sub_text(st, "例如测量电功的电表:电功表（电度表）", 0.3)
        power_J = VGroup(
            Text("220V 10(20)A 50Hz").shift(0.5*UP),
            Text("1250r/kW·h").shift(0.5*DOWN),
            Polygon([-4, 1.5, 0], [4, 1.5, 0], [4, -1.5, 0], [-4, -1.5, 0]).set_color(WHITE)
        )
        self.play(FadeOut(text,shift=UP), FadeOut(text2,shift=DOWN), FadeOut(text3,shift=DOWN), Create(power_J))
        self.sub_text(st, "如现在画面所示，其中的数据的意思分别为:", 1.6)
        self.sub_text(st, "220V:额定工作电压220V（应于该电压工作）", 1.6)
        self.sub_text(st, "10A:基本电流  20A:额定最大电流", 1.3)
        self.sub_text(st, "50Hz:应于50Hz的交流电路中工作", 1.3)
        self.sub_text(st, "1250r:每消耗1kW·h的电能，转盘转动1250圈", 1.6)
        self.sub_text(st, "若为XX imp则为指示灯的闪烁次数", 1.5)
        self.sub_text(st, "此外，信息栏上方的数字栏最后一位为小数位", 1.6)
        text.shift(DOWN)
        self.play(FadeOut(power_J,shift=DOWN), FadeIn(text, shift=RIGHT))
        self.sub_text(st, "除了P=W/t 外，电功率还可通过P=UI计算")
        self.play(Transform(text, MathTex("P = UI").scale(2)))
        self.wait(0.6)
        self.sub_text(st, "并可以得出推到公式P=(I^2)R 和P=(U^2)/R", 0.6)
        text4 = MathTex("P = I^2R").shift(3*LEFT+DOWN).scale(2)
        text5 = MathTex("P = \\frac{U^2}{R}").shift(3*RIGHT+DOWN).scale(2)
        self.play(Write(text4), Write(text5), text.animate.shift(UP))
        self.sub_text(st, "观察公式，可以得出结论:", 0.7)
        self.sub_text(st, "一段电路中电压不变，电阻下降，其功率上升", 1.6)
        self.sub_text(st, "即在电压相等的电路里，电阻小的功率大", 1.6)
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==
        self.play(FadeOut(TIT_1,shift=DOWN), FadeOut(text,shift=DOWN), FadeOut(text4,shift=DOWN), FadeOut(text5,shift=DOWN), )

    def scene_3(self):
        TIT_1 = Text("电功 电功率的计算与应用", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "那么，请看例子", 0.3)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        P_l1 = Power_line([[-2, -1], [2, -1], [2, 1], [-2, 1], [-2, -1]])
        P_l2 = Power_line([[2, 0], [0, 0], [0, 1]])
        P_s = Power_source().shift(LEFT+DOWN)
        P_S1 = Power_switch().shift(0.5*RIGHT+DOWN)
        P_S2 = Power_switch().shift(0.5*RIGHT)
        P_R1 = Power_R().shift(LEFT+UP)
        P_R2 = Power_R().shift(RIGHT+UP)

        P_R1.text_change("R1")
        P_R2.text_change("R2")

        self.play(Create(P_l1), Create(P_l2), Create(P_s), Create(P_S1), Create(P_S2), Create(P_R1), Create(P_R2))
        self.sub_text(st, "在这个例子中，R1=5Ω，R2=95Ω", 0.2)
        self.play(P_R1.text_change("5Ω"), P_R2.text_change("95Ω"))
        self.sub_text(st, "电源电压为50V", 0.4)
        self.sub_text(st, "当两个开关均闭合时，右侧电阻短路", 1.5)
        self.play(P_S1.animate.state_close(), P_S2.animate.state_close())
        self.play(P_R1.animate.state_work(), P_R2.animate.state_error())
        self.sub_text(st, "故整个电路中只有5Ω电阻", 0.6)
        self.sub_text(st, "P=(U^2)/R=(50V*50V)/5Ω=500W", 1.6)
        self.sub_text(st, "当只有下面的那一个开关闭合时")
        self.play(P_S2.animate.state_open())
        self.sub_text(st, "两个电阻均接入电路", 0.5)
        self.play(P_R2.animate.state_work())
        self.sub_text(st, "整个电路中有电阻5Ω+95Ω=100Ω", 1.5)
        self.sub_text(st, "P=(U^2)/R=(50V*50V)/100Ω=25W", 1.6)
        self.sub_text(st, "500W很明显大于25W", 0.5)
        self.sub_text(st, "所以电压相等时，电阻越小，功率越大", 1.6)
        self.sub_text(st, "电流做功越快", 1.5)
        self.play(FadeOut(P_l1, shift=DOWN), FadeOut(P_l2, shift=DOWN), FadeOut(P_s, shift=DOWN), FadeOut(P_S1, shift=DOWN), FadeOut(P_S2, shift=DOWN), FadeOut(P_R1, shift=DOWN), FadeOut(P_R2, shift=DOWN), FadeOut(TIT_1, shift=DOWN))
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==

    def scene_4(self):
        TIT_1 = Text("怎样使用电器正常工作", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "这里要涉及六个概念:三额三实", 0.3)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        self.sub_text(st, "在用电器正常工作时", 0.5)
        self.sub_text(st, "其电压值，电流值及其功率被称为", 1.5)
        text1 = Text("额定电压 额定电流 额定功率").set_color(YELLOW).scale(1.5).shift(0.5*UP)
        self.play(Write(text1))
        self.sub_text(st, "额定电压，额定电流和额定功率", 0.7)
        self.sub_text(st, "在用电器实际工作时的电压电流和功率", 0.5)
        text2 = Text("实际电压 实际电流 实际功率").set_color(GREEN).scale(1.5).shift(0.5*DOWN)
        self.play(Write(text2))
        self.sub_text(st, "实际电压，实际电流和实际功率", 1.6)
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==
        self.play(FadeOut(TIT_1, shift=DOWN), FadeOut(text1, shift=DOWN), FadeOut(text2, shift=DOWN))

    def scene_5(self):
        TIT_1 = Text("焦耳定律", color=BLUE).scale(2)
        self.play(Write(TIT_1))
        self.sub_text(st, "焦耳定律表达了电流流过导体放出的热量", 1.6)
        self.sub_text(st, "同流过导体的电流大小及导体电阻的关系", 0.5)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        self.sub_text(st, "其用公式表达如上", 0.5)
        text = MathTex("Q=I^2Rt").scale(2)
        self.play(Write(text))
        self.sub_text(st, "由于还没学到这，更多内容待以后补充", 0.6)
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==
        self.play(FadeOut(TIT_1, shift=DOWN), FadeOut(text, shift=DOWN))

    def scene_6(self):
        TIT_1 = Text("电路故障", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "现在，让我们了解下", 0.7)
        self.sub_text(st, "假若电路的不同部分出现不同的故障", 1.5)
        self.sub_text(st, "将会发生什么", 0.5)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        P_l1 = Power_line([[-3, -1], [3, -1], [3, 1], [-3, 1], [-3, -1]])
        P_l2 = Power_line([[0, 1], [0, 2], [-2, 2], [-2, 1]])
        P_l3 = Power_line([[-2, 1], [-2, 0], [0, 0], [0, 1]])
        P_s = Power_source().shift(DOWN)
        P_S1 = Power_switch().shift(RIGHT+DOWN)
        P_R1 = Power_R().shift(LEFT+UP)
        P_R2 = Power_R().shift(RIGHT+UP)
        P_V1 = Power_V().shift(LEFT+2*UP)
        P_A1 = Power_A().shift(2*LEFT+DOWN)

        P_R1.text_change("2Ω", 0.5*DOWN)
        P_R2.text_change("3Ω", 0.5*DOWN)

        self.play(Create(P_l1), Create(P_l2), Create(P_s), Create(P_S1), Create(P_R1), Create(P_V1), Create(P_A1))

        self.sub_text(st, "上面是一幅电路图", 0.5)
        self.sub_text(st, "闭合开关，构成回路，各部分正常工作", 0.5)
        self.play(P_S1.animate.state_close(), P_R1.animate.state_work(), P_V1.text_change("3V"), P_A1.text_change("1.5A", 2.25*LEFT))
        self.wait(1.5)
        self.sub_text(st, "倘若电压表或是用电器短路了会发生什么呢", 0.5)
        self.play(Create(P_l3))
        self.play(P_s.animate.state_error(), P_R1.animate.state_error(), P_A1.animate.state_error(), P_V1.text_change("0V"), P_A1.text_change("?A", 2.25*LEFT))
        self.sub_text(st, "观察可得，用电器不工作，电压表无示数", 1.5)
        self.sub_text(st, "电流表损坏，电源损坏", 1.2)
        self.sub_text(st, "假若我们增加一个电阻")
        self.play(Create(P_R2))
        self.sub_text(st, "就可对电源和电流表起保护作用", 0.4)
        self.play(P_s.animate.state_normal(), P_A1.animate.state_normal(), P_A1.text_change("1A", 2.25*LEFT))
        self.wait(2)
        self.play(P_R1.animate.state_normal(), P_V1.text_change("3V"), P_A1.text_change("1.5A", 2.25*LEFT), FadeOut(P_l3), FadeOut(P_R2))
        self.sub_text(st, "那么倘若是电压表断路呢?", 1.2)
        self.sub_text(st, "实际上，出了电压表没有示数外", 1.2)
        self.sub_text(st, "不会对电路有任何影响", 0.5)
        self.sub_text(st, "因为电压表本身就具有非常高的电阻", 1.5)
        self.sub_text(st, "本身就像是一个断路", 0.5)
        self.sub_text(st, "电流表短路也是同理", 0.5)
        self.sub_text(st, "也正因如此，只要电压在量程内", 1.3)
        self.sub_text(st, "电压表就不可能被烧毁", 0.6)
        self.sub_text(st, "假若用电器断路", 0.4)
        self.sub_text(st, "那么该支路上所有用电器不工作", 0.5)
        self.play(P_R1.animate.state_error(), P_A1.text_change("0A", 2.25*LEFT))
        self.sub_text(st, "且和电压表串联的用电器是无法工作的", 1.5)
        self.sub_text(st, "电流表没有示数，电压表有示数", 1.3)
        self.sub_text(st, "电流表断路,所有要经过它构成的回路会断开", 1.6)
        self.sub_text(st, "相当于是一个断开的开关", 1.1)
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==
        self.play(FadeOut(TIT_1, shift=DOWN), FadeOut(P_l1, shift=DOWN), FadeOut(P_l2, shift=DOWN), FadeOut(P_s, shift=DOWN), FadeOut(P_S1, shift=DOWN), FadeOut(P_R1, shift=DOWN), FadeOut(P_V1, shift=DOWN), FadeOut(P_A1, shift=DOWN))

    def construct(self):
        # plane = NumberPlane()
        # self.add(plane)
        # t_sub = self.get_date()

        self.play(Write(st))
        self.wait(0.15)
        self.play(ApplyMethod(st.shift, 3*DOWN))
        self.wait(1)
        # self.remove(t_sub)
        # del t_sub

        self.scene_1()
        self.scene_2()
        self.scene_3()
        self.scene_4()
        self.scene_5()
        self.scene_6()

        self.sub_text(st, "未完待续", 0.5)
        self.wait(2)

class debug(Physical):
    def construct(self):
        plane = NumberPlane()
        self.add(plane)
        t_sub = self.get_date()

        self.play(Write(st))
        self.wait(0.15)
        self.play(ApplyMethod(st.shift, 3*DOWN))
        self.wait(1)
        self.remove(t_sub)
        del t_sub

        self.scene_6()

        self.sub_text(st, "未完待续", 0.5)
        self.wait(2)
