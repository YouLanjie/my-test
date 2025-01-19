#!/usr/bin/env python
# Q=I^2Rt

if __name__ == "__main__":
    command = "manim -pqh ./2024_01_24_PHY_QIRt.py Physical"
    import os
    print("==> Run Command:")
    print(" -> " + command)
    os.system(command)
    print("==> Exit")
    exit()

from physical_base import *

st = Text("九上物理：焦耳定律").set_color(RED)
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
        self.wait(time)

    def scene_1(self):
        TIT_1 = Text("基础知识", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "众所周知，焦耳定律的公式是Q=I^2Rt", 0.4)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        text1 = MathTex("Q=I^2Rt").scale(1.5)
        self.play(Create(text1))
        self.sub_text(st, "它表示电流产生的热量跟电流电阻和时间的关系", 1.7)
        self.sub_text(st, "其产热的原因是电流的热效应", 1.2)
        self.sub_text(st, "其定义为", 0.3)
        text_hot = Text("当电流通过电阻时，电流做功而消耗电能，\n" +
                        "产生了热量，这种现象叫做电流的热效应").shift(DOWN)
        self.play(ApplyMethod(text1.shift, UP, run_time=1), Write(text_hot, run_time=3.2))
        self.sub_text(st, "焦耳定律公式中I的单位是A，R的单位是Ω", 1.6)
        self.sub_text(st, "t的单位是s，Q的单位是J", 1.2)
        self.sub_text(st, "而根据我们学过的欧姆定律", 0.5)
        text4 = MathTex("I=\\frac{U}{R}").scale(1.5).shift(UP*2.5)
        self.play(Write(text4))
        self.sub_text(st, "我们可以为其创造出两个变式", 1.2)

        text2 = MathTex("Q=UIt").shift(UP).scale(1.5).shift(4*LEFT)
        text3 = MathTex("Q=\\frac{U^2}{R}t").shift(UP).scale(1.5).shift(4*RIGHT)
        self.play(ApplyMethod(text4.shift, DOWN*8, run_time=0.5), ReplacementTransform(text1.copy(), text2), ReplacementTransform(text1.copy(), text3))
        self.remove(text4)
        del text4
        self.sub_text(st, "但是，问题来了", 1)
        self.sub_text(st, "为什么后面的这两个变式不能是基本公式", 1.6)
        self.sub_text(st, "他们的使用又有那些限制条件呢", 1.3)
        text4 = Text("?", color=RED).scale(6)
        self.play(FadeIn(text4))
        self.wait(2)
        self.play(FadeOut(text1, text2, text3, text4, text_hot, TIT_1, shift=UP))

    def scene_2(self):
        TIT_1 = Text("纯电阻电路", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "实际上", 0.4)
        self.sub_text(st, "电路被分为纯电阻电路和非纯电阻电路两大类", 1.6)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        text1 = Text("电路", color=BLUE).scale(2)
        self.play(Write(text1))
        text2 = Text("纯电阻", color=RED).scale(1.7).shift(LEFT*4)
        text3 = Text("非纯电阻", color=YELLOW).scale(1.7).shift(RIGHT*4)
        self.play(ReplacementTransform(text1, text2), ReplacementTransform(text1.copy(), text3))
        del text1

        self.sub_text(st, "纯电阻电路一般为", 1)
        self.sub_text(st, "只连接电阻而并没有连接其他部件的电路", 1.3)
        R1 = Power_R().shift(LEFT*4 + UP)
        line1 = Power_line([[-6, 1], [-6, -1], [-2, -1], [-2, 1], [-6, 1]])
        P1 = Power_source().shift(LEFT*5 + DOWN)
        S1 = Power_switch().shift(LEFT*3.5 + DOWN)
        self.play(text2.animate.shift(UP*2), Write(R1), Write(line1), Write(P1), Write(S1))
        self.sub_text(st, "电能全部转化为内能", 1.1)
        self.play(S1.animate.state_close(), R1.animate.state_light(70))
        self.sub_text(st, "即只发热的电路", 1.2)

        self.sub_text(st, "而非纯电阻电路则是", 0.5)
        self.sub_text(st, "电能不止转换为内能还转换为了其他能的电路", 1.4)
        M2 = Power_M().shift(RIGHT*4 + UP)
        line2 = Power_line([[6, 1], [6, -1], [2, -1], [2, 1], [6, 1]])
        P2 = Power_source().shift(RIGHT*3 + DOWN)
        S2 = Power_switch().shift(RIGHT*4.5 + DOWN)
        self.play(text3.animate.shift(UP*2), Write(line2), Write(M2), Write(P2), Write(S2))
        self.sub_text(st, "例如转换为光能机械能或者化学能", 1.2)
        self.play(S2.animate.state_close(), M2.animate.state_work())
        self.wait(1)

        self.sub_text(st, "在纯电阻电路中，欧姆定律适用", 1.2)
        text1 = MathTex("I=\\frac{U}{R}").scale(1.5).shift(DOWN*2+LEFT*4)
        self.play(FadeIn(text1, shift=UP))
        self.play(text1.animate.set_color(GREEN))
        self.sub_text(st, "但是出于各种各样的原因", 1.2)
        self.sub_text(st, "非纯电阻电路中，欧姆定律并不适用", 1.2)
        self.play(text1.animate.shift(RIGHT*8))
        self.play(text1.animate.set_color(RED))
        self.sub_text(st, "所以说", 0.4)
        self.sub_text(st, "在纯电阻电路中通过欧姆定律推导出的变式适用", 1.7)
        self.sub_text(st, "故，只有在纯电阻条件下进行计算时", 1.5)
        self.sub_text(st, "才能使用依靠欧姆定律得出的两条变式", 1.5)
        self.sub_text(st, "以电动机这个最经典的非纯电阻电路为例", 1.6)
        self.play(FadeOut(text2, shift=LEFT),FadeOut(R1, shift=LEFT),FadeOut(line1, shift=LEFT),FadeOut(P1, shift=LEFT),FadeOut(S1, shift=LEFT), FadeOut(text1, shift=LEFT))
        self.play(text3.animate.shift(LEFT*8), M2.animate.shift(LEFT*8), line2.animate.shift(LEFT*8), P2.animate.shift(LEFT*8), S2.animate.shift(LEFT*8))
        self.sub_text(st, "根据电功的计算公式W=Pt=UIt", 1.2)
        text4 = MathTex("W=UIt").next_to(ORIGIN, RIGHT).shift(UP*2).scale(1.5)
        self.play(Write(text4))
        self.sub_text(st, "可以得到电流做功的大小", 1.2)
        self.sub_text(st, "而在电动机中电能被转换为了内能和机械能", 1.4)
        text5 = MathTex("W=Q+W_1").next_to(ORIGIN, RIGHT).shift(UP).scale(1.5)
        self.play(Write(text5))
        self.sub_text(st, "故W=Q+W1=I2Rt+W1", 1.2)
        self.play(Transform(text5, MathTex("W=I^2Rt+W_1").next_to(ORIGIN, RIGHT).shift(UP).scale(1.5)))
        self.sub_text(st, "倘若这时你使用变式Q=UIt计算", 1.5)
        self.sub_text(st, "那么你将得到W而非Q", 1.4)
        text6 = MathTex("W\\neq Q").next_to(ORIGIN, RIGHT).scale(1.5)
        self.play(Write(text6))
        self.sub_text(st, "而W并不等于Q", 2)
        self.play(FadeOut(TIT_1, shift=LEFT), FadeOut(text3, shift=LEFT),FadeOut(M2, shift=LEFT),FadeOut(line2, shift=LEFT),FadeOut(P2, shift=LEFT),FadeOut(S2, shift=LEFT), FadeOut(text4, shift=LEFT), FadeOut(text5, shift=LEFT), FadeOut(text6, shift=LEFT))

    def scene_3(self):
        TIT_1 = Text("判断注意事项", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "我们仍需要注意的是要留意区分", 1.2)
        self.sub_text(st, "电路元件为纯电阻电路中的元件还是非纯电阻元件", 1.7)
        self.sub_text(st, "电熨斗，热水壶这些很简单", 1.2)
        self.sub_text(st, "需要注意的是灯泡这一元件", 1.2)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        L1 = Power_used().scale(2.5)
        line1 = Line([-6, 0, 0], [-2, 0, 0]).shift(RIGHT*4)
        self.play(Write(line1), Write(L1))
        self.sub_text(st, "一般而言倘若用小灯泡出需要计算的题", 1.5)
        self.sub_text(st, "则须看作纯电阻电路以计算", 1.2)
        R1 = Power_R().scale(2.5).shift(RIGHT*4)
        arrow = Arrow(LEFT*1.5, RIGHT*1.5, color=YELLOW)
        line2 = Line([6, 0, 0], [2, 0, 0])
        self.play(L1.animate.shift(LEFT*4), line1.animate.shift(LEFT*4), Write(line2), Write(R1), Write(arrow))
        self.sub_text(st, "分类题则需要看作为非纯电阻电路", 1.5)
        self.play(FadeOut(arrow))
        self.sub_text(st, "主要原因是灯泡将电能转换为内能有不同的方式", 1.7)
        self.sub_text(st, "一种为将电能转换为内能再通过热辐射转换为光能", 1.7)
        self.sub_text(st, "即热光源", 1.2)
        text1 = Text("热光源", color=YELLOW).scale(1.7).shift(UP)
        self.play(FadeOut(line2), FadeOut(R1), Write(text1))
        self.sub_text(st, "白炽灯或者称为小灯泡为其典型代表", 1.5)
        text11 = Text("白炽灯", color=GREEN).next_to(text1, RIGHT)
        self.play(Write(text11))
        self.sub_text(st, "不通过转换为内能再转换为光能这种方式发光的", 1.7)
        self.sub_text(st, "即冷光源", 1.2)
        text2 = Text("冷光源", color=BLUE).scale(1.7).shift(DOWN)
        self.play(Write(text2))
        self.sub_text(st, "LED（发光二极管）为典型代表", 1.2)
        text22 = Text("LED", color=GREEN).next_to(text2, RIGHT)
        self.play(Write(text22))
        self.wait(2)
        self.play(FadeOut(TIT_1 ,shift=DOWN), FadeOut(line1 ,shift=DOWN), FadeOut(L1,shift=DOWN), FadeOut(text1,shift=DOWN), FadeOut(text11,shift=DOWN), FadeOut(text2,shift=DOWN), FadeOut(text22,shift=DOWN))

    def scene_4(self):
        TIT_1 = Text("计算题注意事项", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "除开简单的电路元件类别判断", 1.2)
        self.sub_text(st, "还有些需要注意的事情", 1.2)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        self.sub_text(st, "在做计算题时，倘若要用到非基本公式时", 1.6)
        self.sub_text(st, "必须要在答题时交代一句", 1.2)
        text1 = Text("电路属于纯电阻电路，\n"+
                     "t秒内电流产生热量Q等于总功W=UIt\n"+
                     "（或是直接交代Q=W=UIt)", color=YELLOW)
        self.play(Write(text1))
        self.sub_text(st, "然后再进行相应的计算", 1.2)
        self.sub_text(st, "有些时候还需要对电路进行分析", 1.3)
        self.sub_text(st, "一般是对电路进行一个简单的判断", 1.5)
        self.sub_text(st, "答出某某部分之间在某种状态下是串或并联状态", 1.7)
        self.sub_text(st, "再加上电路特点等压分流或是等流分压。", 1.5)
        self.sub_text(st, "然后再进行相应的计算", 1.1)
        self.play(FadeOut(TIT_1, shift=RIGHT), FadeOut(text1, shift=RIGHT))

    def scene_5(self):
        TIT_1 = Text("课本的实验题", color=BLUE).scale(1.5)
        self.play(Write(TIT_1))
        self.sub_text(st, "最后让我们简单地提一提课本的实验题", 1.5)
        self.play(TIT_1.animate.to_edge(LEFT+UP))

        self.sub_text(st, "课本中的探究实验有两个:", 1.2)
        self.sub_text(st, "一是探究导体产生的热量跟电阻的关系", 1.6)
        self.sub_text(st, "二是探究导体产生的热量跟电流的关系", 1.5)

        R11 = Power_R().shift(LEFT*5 + UP)
        R12 = Power_R().shift(LEFT*3 + UP)
        R11.text_change("5Ω")
        R12.text_change("10Ω")
        line1 = Power_line([[-6, 1], [-6, -1], [-2, -1], [-2, 1], [-6, 1]])
        P1 = Power_source().shift(LEFT*5 + DOWN)
        P1.text_change("3V", DOWN)
        S1 = Power_switch().shift(LEFT*3.5 + DOWN)
        self.sub_text(st, "前者需控制通过电阻电流的大小相等电阻大小不等", 1.7)
        self.play(Create(line1), Create(R11), Create(R12), Create(P1), Create(S1))
        self.sub_text(st, "我们通过将两个电阻串联实现", 1.2)
        self.play(Circumscribe(R11), Circumscribe(R12))
        A1 = Power_A().shift(LEFT*4+UP).scale(0.7)
        self.play(Create(A1))
        self.play(A1.text_change("0.2A"), S1.animate.state_close())
        self.play(A1.animate.shift(LEFT*1.2))
        self.play(A1.animate.shift(RIGHT*2.4))
        self.play(A1.animate.shift(LEFT*1.2))
        self.sub_text(st, "并通过转换法反映比较放出热量的差异", 1.5)

        R21 = Power_R().shift(RIGHT*3 + UP)
        R22 = Power_R().shift(RIGHT*5 + UP)
        R23 = Power_R().shift(RIGHT*5 + UP*2)
        R21.text_change("10Ω")
        R22.text_change("10Ω", 0.5*DOWN)
        R23.text_change("10Ω")
        line2 = Power_line([[6, 1], [6, -1], [2, -1], [2, 1], [6, 1]])
        line3 = Power_line([[6, 1], [6, 2], [4, 2], [4, 1], [6, 1]])
        P2 = Power_source().shift(RIGHT*3 + DOWN)
        P2.text_change("3V", DOWN)
        S2 = Power_switch().shift(RIGHT*4.5 + DOWN)
        self.sub_text(st, "后者需要控制通过电阻的大小相等而电流不等", 1.6)
        self.play(Create(line2), Create(line3), Create(R21), Create(R22), Create(R23), Create(P2), Create(S2))
        self.sub_text(st, "我们通过在一个电阻之外并联一个电阻实现", 1.2)
        self.play(R21.text_change("0.2A"), R23.text_change("0.1A"), R22.text_change("0.1A", 0.5*DOWN), S2.animate.state_close())
        self.sub_text(st, "其中并联的两个电阻电流之和", 1.2)
        self.sub_text(st, "为另外一个电阻通过的电流大小", 1.2)
        self.sub_text(st, "只需控制待测电阻阻值相等即可", 1.3)
        self.play(FadeOut(TIT_1), FadeOut(R11), FadeOut(R12), FadeOut(line1), FadeOut(line2), FadeOut(line3), FadeOut(R21), FadeOut(R22), FadeOut(R23), FadeOut(P1), FadeOut(P2), FadeOut(S1), FadeOut(S2), FadeOut(A1))
        #     字符串上限尺 =================0.5====1.2===1.5==1.6=|1.7=

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

        self.sub_text(st, "感谢观看", 0.5)
        self.wait(2)

class debug(Physical):
    def construct(self):
        plane = NumberPlane()
        self.add(plane)
        t_sub = self.get_date()

        self.play(Write(st))
        self.remove(t_sub)
        del t_sub
        self.wait(0.15)
        self.play(ApplyMethod(st.shift, 3*DOWN))
        self.wait(1)

        self.scene_5()

        self.sub_text(st, "未完待续", 0.5)
        self.wait(2)

class debug2(Physical):
    def construct(self):
        plane = NumberPlane()
        text1 = Text("电路属于纯电阻电路，\n"+
                     "t秒内电流产生热量Q等于总功W=UIt\n"+
                     "（或是直接交代Q=W=UIt)", color=YELLOW)
        #    串上限尺 =================0.5====1.2===1.5==1.6=|1.7=
        self.add(plane, text1)
