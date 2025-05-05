#!/usr/bin/env python
# PZ220V - 100W

if __name__ == "__main__":
    command = "manim -pql ./2023_12_03_PZ220-100.py Physical"
    import os
    print("==> Run Command:")
    print(" -> " + command)
    os.system(command)
    print("==> Exit")
    exit()

from physical_base import *

st = Text("九上物理：电功率相关的计算").set_color(RED)
class Physical(Scene):
    def get_date(self):
        import time
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
        if time > 0:
            self.wait(time)

    def scene_1(self):
        TIT_1 = Tex("PZ","200V","-","100W").set_color(BLUE).scale(2)
        self.play(Write(TIT_1))
        self.sub_text(st, "这是一个非常经典的灯泡铭牌", 0.3)
        self.play(Circumscribe(TIT_1))
        self.sub_text(st, "它由三部分组成", 0.5)
        self.sub_text(st, "即“PZ”，“220V”和“100W”")
        self.play(Indicate(TIT_1[0]))
        self.play(Indicate(TIT_1[1]))
        self.play(Indicate(TIT_1[3]))
        self.sub_text(st, "其含义分别为：", 0.2)
        box1 = SurroundingRectangle(TIT_1[0]).set_color(YELLOW)
        self.play(Create(box1))
        self.sub_text(st, "中文“普通照明”的缩写", 0.3)
        self.play(Transform(box1, SurroundingRectangle(TIT_1[1]).set_color(YELLOW)))
        self.sub_text(st, "灯泡的额定电压", 0.3)
        self.play(Transform(box1, SurroundingRectangle(TIT_1[3]).set_color(YELLOW)))
        self.sub_text(st, "和灯泡的额定功率", 0.5)
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==
        text1 = MathTex("U_e=220V,P_e=100V").to_edge(LEFT).shift(2*UP+RIGHT)
        self.play(FadeOut(box1), TIT_1.animate.to_edge(LEFT+UP), Write(text1), run_time=2)

        self.sub_text(st, "那么，现在，我们从获得了两个数值", 1.4)
        self.sub_text(st, "额定电压和额定电流", 0.2)
        self.play(Indicate(text1))
        self.sub_text(st, "那么我们可以用它算出什么呢？", 1.3)
        self.sub_text(st, "这时候，我们就可以掏出那套经典的公式了", 0.6)
        text2 = MathTex("P=UI").to_edge(LEFT).shift(UP+RIGHT)
        text3 = MathTex("P=\\frac{U^2}{R}").next_to(text2, 2*RIGHT)
        text4 = MathTex("P=I^2R").next_to(text3, 2*RIGHT)
        self.play(Create(text2))
        self.sub_text(st, "再掏出它的变形公式", 0.5)
        self.play(
            ReplacementTransform(text2.copy(), text3),
            ReplacementTransform(text2.copy(), text4)
        )
        self.sub_text(st, "再将这些公式变形一下并把已知量代入进去", 0.6)
        self.play(
            Transform(text2, MathTex("I_e=\\frac{P_e}{U_e}").next_to(text2, ORIGIN)),
            Transform(text3, MathTex("R=\\frac{U_e^2}{P_e}").next_to(text2, 1.8*DOWN)),
            FadeOut(text4)
        )
        # self.play(text3.animate.to_edge(LEFT).shift(1.7*DOWN+RIGHT))
        self.sub_text(st, "便得到了灯泡的额定电流和电阻的求解公式", 1.6)
        self.sub_text(st, "一般情况下", 0.3)
        self.sub_text(st, "填空题还会问将灯泡接入nV电压时的实际功率", 1.6)
        self.sub_text(st, "那么根据公式P=(U^2)/R和R=(Ue^2)/Pe", 0.5)
        self.play(Transform(text4, MathTex("P=\\frac{U^2}{R}").next_to(text3, 1.5*DOWN)))
        self.play(Transform(text4, MathTex("P=\\frac{U^2}{\\frac{U_e^2}{P_e}}").next_to(text4, ORIGIN)))
        self.sub_text(st, "最终可以推演出", 0.4)
        self.play(Transform(text4, MathTex("P=\\frac{U^2}{U_e^2}\\times P_e").next_to(text4, ORIGIN)))
        self.sub_text(st, "至此，我们便得到了实际功率的计算方法", 1.6)
        self.sub_text(st, "但是这些只是电路中只有一个用电器的情况", 1.3)
        self.play(
            FadeOut(text1, shift=DOWN),
            FadeOut(text2, shift=DOWN),
            FadeOut(text3, shift=DOWN),
            FadeOut(text4, shift=DOWN)
        )
        del text1, text2, text3, text4
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==

        self.sub_text(st, "让我们先绘制一副电路图", 1.3)

        P_l1 = Power_line([[-2, -1], [2, -1], [2, 1], [-2, 1], [-2, -1]])
        P_l2 = Power_line([[-2, 0], [2, 0]])
        P_s = Power_source().shift(LEFT+DOWN)
        P_S1 = Power_switch().shift(0.5*RIGHT+DOWN) # Main Switch
        P_R1 = Power_R().shift(LEFT+UP)
        P_R2 = Power_R().shift(RIGHT+UP)

        P_R1.text_change("R1")
        P_R2.text_change("R2")

        self.play(
            Create(P_l1),
            Create(P_s),
            Create(P_S1),
            Create(P_R1),
            Create(P_R2)
        )
        self.sub_text(st, "在这个电路图中，R1和R2串联", 1.2)
        self.sub_text(st, "根据串联电路的电流特点", 1.1)
        self.sub_text(st, "我们可以得到I=I1=I2,U=U1+U2等式子", 1.5)
        self.sub_text(st, "进一步推演可得到P1:P2=R1:R2和P=P1+P2", 1.1)
        self.sub_text(st, "所以说在串联电路中额定电压相等时", 1.5)
        self.sub_text(st, "功率越低，电阻越大，实际功率越高", 1.5)
        self.sub_text(st, "而在并联电路中则相反，额定与实际正相关", 1.6)
        self.play(FadeOut(P_l1, shift=DOWN), FadeOut(P_s, shift=DOWN), FadeOut(P_S1, shift=DOWN), FadeOut(P_R1, shift=DOWN), FadeOut(P_R2, shift=DOWN), FadeOut(TIT_1, shift=DOWN))
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==

    def scene_2(self):
        TIT_1 = Text("一些需要注意的事情", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "做题的时候，题目往往会有一些隐藏条件", 1.6)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        self.sub_text(st, "例如灯泡的电阻到底是变还是不变？", 0.5)
        L1 = Power_used()
        L1.text_change("?Ω")
        self.play(Create(L1))
        self.sub_text(st, "一般情况下，如果它没有给出如下的曲线图", 1.2)
        ax = Axes([0, 0.4], [0, 3]).scale(0.35).shift(2*RIGHT)
        curve = ax.plot(lambda x: 40*x*x, x_range=[0, 0.25], color=RED)
        self.play(Create(ax), Create(curve), L1.animate.shift(2*LEFT))
        self.sub_text(st, "或不是实验探究题", 0.6)
        self.sub_text(st, "那么我们会默认小灯泡的电阻不变", 1.5)
        self.play(FadeOut(ax, shift=DOWN), FadeOut(curve, shift=DOWN), L1.animate.shift(2*RIGHT))
        self.play(L1.text_change(" "))
        self.sub_text(st, "倘若说某个用电器正常工作", 0.3)
        line1 = Power_line([[-2, 0], [2, 0]])
        line2 = Power_line([[-1, 0.05], [-1, 1.25]])
        line3 = Power_line([[1, 0.05], [1, 1.25]])
        line4 = Power_arrow([[1, 0.65], [-1.2, 0.65]])
        line5 = Power_arrow([[-1, 0.65], [1.2, 0.65]])
        text1 = MathTex("U").shift(1.2*UP)
        self.remove(L1)
        self.play(
            Create(line1),
            Create(line2),
            Create(line3),
            Create(line4),
            Create(line5),
            Create(text1),
            Create(L1)
        )
        self.sub_text(st, "则说明其两端电压为额定电压", 0.4)
        self.play(Transform(text1, MathTex("U=U_e").shift(1.2*UP)))
        self.wait(1.5)
        self.sub_text(st, "倘若说用电器接入“照明电路”或“家庭电路”", 1.6)
        self.sub_text(st, "实际上都是一个意思", 0.3)
        self.play(Transform(text1, MathTex("U=220V").shift(1.2*UP)))
        self.wait(1.5)
        self.sub_text(st, "即接入正常的家庭电路（220V）", 1.6)
        self.play(FadeOut(TIT_1, shift=DOWN), FadeOut(line1, shift=DOWN), FadeOut(line2, shift=DOWN), FadeOut(line3, shift=DOWN), FadeOut(line4, shift=DOWN), FadeOut(line5, shift=DOWN), FadeOut(text1, shift=DOWN), FadeOut(L1, shift=DOWN))
        #     字符串上限尺 =================0.5====1.2===1.5==1.6==

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

        self.scene_1()

        self.sub_text(st, "未完待续", 0.5)
        self.wait(2)

class debug2(Physical):
    def construct(self):
        plane = NumberPlane()
        self.add(plane)
        ax = Axes([0, 0.4], [0, 3]).scale(0.35).shift(2*RIGHT)
        curve = ax.plot(lambda x: 40*x*x, x_range=[0, 0.25], color=RED)
        self.add(ax, curve)
