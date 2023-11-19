#!/usr/bin/env python
# I = U / R

if __name__ == "__main__":
    command = "manim -pql ./Physical2.py Physical"
    import os
    print("==> Run Command:")
    print(" -> " + command)
    os.system(command)
    print("==> Exit")
    exit()

from manim import *
from physical_base import *
import time

st = Text("九上物理：欧姆定律").set_color(RED)
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

    def scene_0(self):
        R = Text("R/Ω", color=BLUE).scale(2)
        self.play(Write(R))
        self.sub_text(st, "要认识欧姆定律", 0.4)
        self.sub_text(st, "首先得认识什么是电阻", 1.2)
        self.play(R.animate.to_edge(LEFT+UP))
        dot1 = Dot([-5.5, 1, 0]).set_color(YELLOW)
        dot2 = Dot([-5.5, 0, 0]).set_color(YELLOW)
        dot3 = Dot([-5.5, -1, 0]).set_color(YELLOW)
        group = VGroup(
            dot1,
            Text("定义：电阻是电路对电流阻碍作用的大小").next_to(dot1, RIGHT),
            dot2,
            Text("单位：欧姆，简称欧，符号Ω",
                 t2c={"姆":RED, "欧":RED, "R":RED, "Ω":RED}
                 ).next_to(dot2, RIGHT),
            dot3,
            Text("表示：字母R",
                 t2c={"R":RED}
                 ).next_to(dot3, RIGHT),
        )
        self.play(Write(group))
        self.wait(1)
        self.sub_text(st, "根据实验经验可得", 0.5)
        self.sub_text(st, "电压、电流、电阻直接存在影响关系", 1.5)
        self.sub_text(st, "故我们需要设计实验探究三者之间的关系", 1.6)
        self.play(FadeOut(group,direction=DOWN), FadeOut(R,direction=DOWN))

    def scene_1(self):
        TIT = Text("探究电压和电流之间的关系", color=BLUE).scale(1.25)
        self.play(Write(TIT))
        self.sub_text(st, "现在，按照惯例，让我们绘制一副电路图", 0.4)
        self.play(TIT.animate.to_edge(LEFT+UP))

        # line
        line = Power_line([[0,-1], [4,-1], [4,2], [-4,2], [-4,-1], [0, -1]])
        # Power
        power = Power_source().shift(1*DOWN)
        # power used
        R1 = Power_R().shift(2*LEFT+2*UP)
        Rc = Power_RC().shift(2*RIGHT+2*UP)
        # Switch
        S1 = Power_switch().shift(1.5*RIGHT + 1*DOWN)
        # V
        V1 = Power_V().shift(LEFT*2+UP)
        line2 = Power_line([[-3,2], [-3,1], [-1,1], [-1, 2]])
        # A
        A1 = Power_A().shift(RIGHT*4+UP*0.5)
        self.play(Create(line), Create(line2), Create(power), Create(R1), Create(Rc), Create(V1), Create(A1), run_time=2)

        self.sub_text(st, "在该电路图中", 0.4)
        self.sub_text(st, "串联了一个定值电阻和一个滑动变阻器", 0.2)
        self.play(Indicate(R1))
        self.play(Indicate(Rc.body()))
        self.sub_text(st, "电压表和电流表测量定值电阻的电压和电流", 1.6)
        self.sub_text(st, "通过调节滑动变阻器的阻值")
        self.play(Rc.changer().animate.shift(0.2*LEFT), run_time=0.5)
        self.play(Rc.changer().animate.shift(0.4*RIGHT), run_time=0.7)
        self.play(Rc.changer().animate.shift(0.2*LEFT), run_time=0.4)
        self.sub_text(st, "从而改变定值电阻两段的电压和经过的电流", 1.6)
        self.sub_text(st, "根据实验的结果，我们可以得出这样的结论：", 1.6)
        self.sub_text(st, "电阻不变时，电流和电压成正比", 1.2)
        self.play(FadeOut(line), FadeOut(line2), FadeOut(power), FadeOut(R1), FadeOut(Rc), FadeOut(V1), FadeOut(A1), FadeOut(TIT))

    def scene_2(self):
        TIT = Text("探究电阻和电流之间的关系", color=BLUE).scale(1.25)
        self.play(Write(TIT))
        self.sub_text(st, "现在，让我们再设计另外一个实验", 0.4)
        self.play(TIT.animate.to_edge(LEFT+UP))

        # line
        line = Power_line([[0,-1], [4,-1], [4,2], [-4,2], [-4,-1], [0, -1]])
        # Power
        power = Power_source().shift(1*DOWN)
        # power used
        R1 = Power_R().shift(2*LEFT+2*UP)
        Rc = Power_RC().shift(2*RIGHT+2*UP)
        # Switch
        S1 = Power_switch().shift(1.5*RIGHT + 1*DOWN)
        # V
        V1 = Power_V().shift(LEFT*2+UP)
        line2 = Power_line([[-3,2], [-3,1], [-1,1], [-1, 2]])
        # A
        A1 = Power_A().shift(RIGHT*4+UP*0.5)
        self.play(Create(line), Create(line2), Create(power), Create(R1), Create(Rc), Create(V1), Create(A1), run_time=2)

        self.sub_text(st, "在这次实验里", 0.4)
        self.sub_text(st, "我们更换定值电阻的阻值", 0.2)
        self.play(Indicate(R1))
        self.sub_text(st, "并利用滑动变阻器将定值电阻两端的电压控制不变", 0.6)
        self.play(Rc.changer().animate.shift(0.2*LEFT), run_time=0.5)
        self.play(Rc.changer().animate.shift(0.4*RIGHT), run_time=0.7)
        self.play(Rc.changer().animate.shift(0.2*LEFT), run_time=0.4)
        self.sub_text(st, "根据实验的结果，我们可以得出这样的结论：", 1.6)
        self.sub_text(st, "电压不变时，电流和电阻成反比", 1.2)
        self.play(FadeOut(line), FadeOut(line2), FadeOut(power), FadeOut(R1), FadeOut(Rc), FadeOut(V1), FadeOut(A1), FadeOut(TIT))

    def scene_3(self):
        TIT = Text("欧姆定律", color=BLUE).scale(1.25)
        self.play(Write(TIT))
        self.sub_text(st, "根据刚才的结论，我们就得出了欧姆定律", 0.3)
        OMDL = MathTex("I=\\frac{U}{R}",
                color=BLUE,
            ).scale(2)
        self.play(TIT.animate.to_edge(LEFT+UP), Write(OMDL))
        self.sub_text(st, "式子中，I为电流，U为电压，R为电阻", 1.5)
        self.sub_text(st, "需要注意,这三个量所对应的研究对象相同", 1.6)
        self.sub_text(st, "不可以把电路A部分的电压和B部分的电阻代入计算", 1.2)
        self.play(FadeOut(OMDL))
        self.sub_text(st, "知道了欧姆定律后，我们就可以做许多事情了", 1.6)

        self.sub_text(st, "假设有两个电阻分别为2Ω和3Ω的用电器串联", 1.4)
        T1 = Text("R1=2Ω, R2=3Ω, U=6V, 求U1,U2").to_edge(LEFT).shift(2*UP)
        self.play(Write(T1))
        self.sub_text(st, "接上6V的电源，它们两端的电压分别是多少？", 1.6)
        self.sub_text(st, "根据欧姆定律和串联电路等流分压的特点", 1.5)
        self.sub_text(st, "可得I=I1=I2，即U/R=U1/R1=U2/R2", 1.4)
        self.sub_text(st, "调换位置可得，U1：U2=R1：R2 ......", 0.4)
        T2 = Text("U1:U2=R1:R2 U1:U=R1:R U2:U=R2:R ......").to_edge(LEFT).shift(UP)
        self.play(Write(T2))
        self.sub_text(st, "所以说Un = (Rn / R) * U)", 0.4)
        T3 = MathTex("U_n = \\frac{R_n}{R} \\times U").to_edge(LEFT).shift(0.5*DOWN)
        self.play(Write(T3))
        self.sub_text(st, "代入计算得U1=2.4V，U2=3.6V", 1.5)

        self.sub_text(st, "再例如求并列电路的等效电阻", 1.2)
        self.play(FadeOut(T1), FadeOut(T2), FadeOut(T3))
        T1 = MathTex("I = I_1 + I_2 + ... + I_n").scale(2)
        self.play(Write(T1))
        self.wait(1.2)
        self.play(Transform(T1, MathTex("\\frac{U}{R} = \\frac{U_1}{R_1} + \\frac{U_2}{R_2} + ...  + \\frac{U_n}{R_n}").scale(2)))
        self.sub_text(st, "因为U = U1 = U2 = ... = Un", 1.5)
        self.play(Transform(T1, MathTex("\\frac{1}{R} = \\frac{1}{R_1} + \\frac{1}{R_2} + ... + \\frac{1}{R_n}").scale(2)))
        self.sub_text(st, "这就是求解并列电路等效电阻的推导公式", 1.6)
        self.sub_text(st, "表达的含义大致可理解为", 1.2)
        self.sub_text(st, "电路干路流通性（阻碍作用反义）等于各支路的和", 1.6)

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

        self.scene_0()
        self.scene_1()
        self.scene_2()
        self.scene_3()

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

        self.scene_3()

        self.sub_text(st, "未完待续", 0.5)
        self.wait(2)

class debug2(Physical):
    def construct(self):
        plane = NumberPlane()
        self.add(plane)

        RC = Power_RC()
        arrow = Arrow([-3, 1, 0], [2, 1, 0], color=RED)
        self.add(RC, arrow)
