#!/bin/python
# 关于物理电学的动画演示源码

if __name__ == "__main__":
    command = "manim -pql ./Physical.py Physical"
    import os
    print("==> Run Command:")
    print(" -> " + command)
    os.system(command)
    print("==> Exit")
    exit()

from manim import *

class Power_source(VGroup):
    def __init__(self):
        VGroup.__init__(self)
        # center
        self.add(Line([-0.15, 0, 0], [0.15, 0, 0]).set_color(BLACK))
        self.add(VGroup())
        # short
        self[1].add(Line([-0.15, 0.25, 0], [-0.15, -0.25, 0]))
        # long
        self[1].add(Line([0.15, 0.5, 0], [0.15, -0.5, 0]))
    def state_normal(self):
        self[0].set_stroke(BLACK)
        self[1].set_stroke(WHITE)
    def state_work(self):
        self[1].set_stroke(GREEN)
    def state_error(self):
        self[1].set_stroke(RED)
    def body(self):
        return self[1]

class Power_used(VGroup):
    def __init__(self):
        VGroup.__init__(self)
        cir = Circle(color=WHITE).scale(0.5).set_fill(BLACK, opacity=1)
        cir2 = Circle(color=WHITE).scale(0.5).set_fill(YELLOW, opacity=0)
        line1 = Line([-0.5, 0, 0], [0.5, 0, 0]).rotate(45*DEGREES)
        line2 = Line([0, -0.5, 0], [0, 0.5, 0]).rotate(45*DEGREES)
        self.add(cir, cir2, line1, line2)
    def state_normal(self):
        # self[0].set_fill(BLACK, opacity=1)
        self.set_stroke(WHITE)
    def state_work(self):
        self.set_stroke(GREEN)
    def state_error(self):
        self.set_stroke(RED)
    def state_light(self, light=0):
        self[1].set_fill(YELLOW, opacity=light)
    def text_change(self, text, direction = UP):
        if len(self) == 4:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            self.add(text2)
            return Write(self[4])
        else:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            return Transform(self[4], text2)
    def body(self):
        return VGroup().add(self[0], self[1], self[2], self[3])

class Power_switch(VGroup):
    def __init__(self):
        VGroup.__init__(self)
        # blackground
        self.add(Line([0, 0, 0], [0.8, 0, 0]).set_color(BLACK))

        switch = VGroup()
        switch.add(Line([0, 0, 0], [0.8, 0.56, 0]))
        switch.add(Dot([0, 0, 0]).scale(1.25))

        self.add(switch)
    def state_close(self):
        dot = self[1][1]
        return self[1][0].rotate(-30*DEGREES, about_point=[dot.get_x(), dot.get_y(),0])
    def state_open(self):
        dot = self[1][1]
        return self[1][0].rotate(30*DEGREES, about_point=[dot.get_x(), dot.get_y(),0])
    def text_change(self, text, direction = UP):
        if len(self) == 2:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            self.add(text2)
            return Write(self[2])
        else:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            return Transform(self[2], text2)
    def body(self):
        return self[1]

class Power_line(VGroup):
    def __init__(self, point = [[-1, 0], [1, 0]], z = 0):
        VGroup.__init__(self)
        l = len(point)
        count = 1
        pos_last = point[0]
        pos_last.append(z)
        while count < l:
            pos_now = point[count]
            pos_now.append(z)
            self.add(Line(pos_last, pos_now))
            pos_last = pos_now
            count += 1

class Power_A(VGroup):
    def __init__(self, point = [[-1, 0], [1, 0]]):
        VGroup.__init__(self)
        cir = Circle(color=WHITE).scale(0.5).set_fill(BLACK, opacity=1)
        text = Text("A")
        self.add(cir, text)
    def text_change(self, text, direction = UP):
        if len(self) == 2:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            self.add(text2)
            return Write(self[2])
        else:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            return Transform(self[2], text2)
    def body(self):
        return VGroup().add(self[0], self[1])

class Power_V(VGroup):
    def __init__(self, point = [[-1, 0], [1, 0]]):
        VGroup.__init__(self)
        cir = Circle(color=WHITE).scale(0.5).set_fill(BLACK, opacity=1)
        text = Text("V")
        self.add(cir, text)
    def text_change(self, text, direction = UP):
        if len(self) == 2:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            self.add(text2)
            return Write(self[2])
        else:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            return Transform(self[2], text2)
    def body(self):
        return VGroup().add(self[0], self[1])

class Physical(Scene):
    def sub_text(self, text, string, time=0):
        text2 = Text(string)
        text2.set_color(RED)
        text2.shift(3*DOWN)
        self.play(Transform(text, text2))
        self.wait(time)

    def scene_init(self):
        # line
        line = Power_line([[0,-1], [4,-1], [4,2], [-4,2], [-4,-1], [0, -1]])
        # Power
        power = Power_source().shift(1*DOWN)
        # power used
        L1 = Power_used().shift(2*LEFT+2*UP)
        L2 = Power_used().shift(2*RIGHT+2*UP)
        # Switch
        S1 = Power_switch().shift(1.5*RIGHT + 1*DOWN)
        return power, S1, L1, L2, line

    def scene_1(self, st, power, S1, L1, L2, line):
        self.play(Create(line), Create(L1), Create(L2), Create(S1), Create(power), run_time=1.2)
        self.wait(0.25)
        self.sub_text(st, "这是电源", 1)
        self.play(Indicate(power.body()))
        self.sub_text(st, "是提供电压的装置", 1.2)
        self.wait(0.25)
        self.sub_text(st, "这是开关", 1)
        self.play(Indicate(S1.body()))
        self.sub_text(st, "是控制电路开合的装置", 1.2)
        self.sub_text(st, "这是俩用电器", 0.4)
        self.play(Indicate(L1), Indicate(L2))
        self.sub_text(st, "现在，它们构成了一条开路/断路", 1)
        self.sub_text(st, "闭合开关")
        self.play(S1.animate.state_close())
        self.sub_text(st, "电路各部分接通，构成通路")
        self.play(Circumscribe(line))
        self.sub_text(st, "此时，电流通过用电器，用电器正常工作", 0.6)
        self.play(L1.animate.state_work(), L2.animate.state_work())
        self.sub_text(st, "在电路图中", 0.5)
        self.sub_text(st, "电路元件连接在相同的两根导线中的效果等效", 1.5)
        self.sub_text(st, "即电路元件像动画所示移动电路的效果相同")
        self.play(L1.animate.shift(LEFT), run_time=0.8)
        self.play(L1.animate.shift(3*RIGHT), run_time=2)
        self.play(L1.animate.shift(2*LEFT), run_time=1.4)
        self.sub_text(st, "但是", 0.3)
        self.sub_text(st, "将电路元件画在电路图的边角是不允许的")
        self.play(L1.animate.shift(2*LEFT))

        tmp_text = Text("X").next_to(L1, LEFT).set_color(RED)
        self.play(Write(tmp_text), run_time=0.7)
        self.wait(1)
        self.play(FadeOut(tmp_text))
        del tmp_text

        self.sub_text(st, "不过也要注意，将用电器拽过边角是可以的")
        self.play(L1.animate.shift(2*DOWN), run_time=1.5)
        self.sub_text(st, "这种虽然绘制不一，效果却一样的电路图", 1.5)
        self.sub_text(st, "我个人（划掉）称为等效电路图", 1.3)
        self.sub_text(st, "在做题的时候可以发挥想象力", 1)
        self.sub_text(st, "将电路图'东扯西扯'出另外一个更直观的电路图", 1.6)

        self.play(L1.animate.shift(2*UP+2*RIGHT), run_time=0.8)
        self.play(L1.animate.state_normal(), L2.animate.state_normal(), run_time=0.8)

        return st, power, S1, L1, L2, line

    def scene_2(self, st, power, S1, L1, L2, line):
        self.sub_text(st, "回到正题", 0.4)
        self.sub_text(st, "电路的状态除了断路（开路）、通路外", 1.5)
        self.sub_text(st, "还有一种，叫做短路", 1)

        self.play(L1.text_change("L1"), L2.text_change("L2"), run_time=0.5)

        self.sub_text(st, "如动画所示", 0.5)
        line2 = Power_line([[-4,1],[0,1],[0,2]])
        self.play(Create(line2))
        self.sub_text(st, "我们在灯泡L1的两旁用导线直接连接起来", 1.5)
        self.sub_text(st, "L1就会被短路而熄灭", 1)
        self.play(L1.animate.state_error())
        self.sub_text(st, "原因在于电流总是倾向于从电阻小的的通路流过", 1.6)
        self.sub_text(st, "好比你开车总喜欢开通畅的道路而非塞车的那条", 1.6)
        self.sub_text(st, "在这个例子中")
        self.play(Circumscribe(line2))
        self.sub_text(st, "电线的电阻（初中一般假设为0）一定小于灯泡的", 1.6)
        self.sub_text(st, "所以说绝大多数的电路都从电线流过了", 1.5)
        self.sub_text(st, "流经灯泡的电流（初中一般假设为0）", 1.5)
        self.sub_text(st, "不足以让灯泡工作", 0.8)
        self.sub_text(st, "故灯泡L1熄灭", 1)
        self.sub_text(st, "这种只有部分电路被短路的情况，又称为短接", 1.6)
        self.play(L1.animate.state_normal(), FadeOut(line2))
        del line2
        self.sub_text(st, "倘若将电池直接用导线首尾连接", 1.3)
        line2 = Power_line([[1,-1], [1,0], [-1, 0], [-1, -1]])
        self.play(Create(line2))
        self.sub_text(st, "因为导线电阻非常小，会形成非常强大电流")
        self.play(Circumscribe(line2))
        self.sub_text(st, "很有可能会烧坏电池或导线，所以这是不允许的", 1.5)
        self.play(FadeOut(line2))
        del line2
        return st, power, S1, L1, L2, line

    def scene_3(self, st, power, S1, L1, L2, line):
        self.sub_text(st, "下一节，电路的组织形式", 1.5)
        self.sub_text(st, "如现在所示", 0.7)
        self.sub_text(st, "电路元件依次首位连接的，是串联电路", 1.5)
        self.sub_text(st, "其特点为，任意一处断开后，就无法形成回路", 1.6)
        self.sub_text(st, "又如动画现在所示", 0.5)
        line2 = Power_line([[-4,0.5], [4,0.5], [4,-1], [-4,-1], [-4,0.5]])
        self.play(Create(line2), L2.animate.shift(1.5*DOWN+4*LEFT))
        self.add(L2, S1, power)
        self.sub_text(st, "不同的两部分电路元件并列连成的电路", 1.4)
        self.play(Indicate(line))
        self.play(Indicate(line2))
        self.sub_text(st, "称为并联", 1.2)
        self.sub_text(st, "构成回路的共同部分为干路，不同部分为支路", 1.6)
        self.sub_text(st, "这两种组织方式可互相嵌套，其特性后面详讲", 1.6)
        self.play(FadeOut(line2), FadeOut(L2))
        L2.shift(1.5*UP+4*RIGHT)
        return st, power, S1, L1, L2, line

    def scene_4(self, st, power, S1, L1, L2, line):
        self.sub_text(st, "下一节，电流", 0.5)
        I1 = Power_A().shift(2*RIGHT+2*UP)
        self.play(Create(I1))
        self.sub_text(st, "这是一个电流表")
        self.play(Indicate(I1))
        self.sub_text(st, "它能够显示经过它流过的电流大小", 1)
        self.sub_text(st, "其内阻很小，分析时一般将其看作一根导线", 1.4)
        self.sub_text(st, "故其可以与被测用电器串联", 1.2)
        self.play(I1.text_change("0.5A", UP))
        self.sub_text(st, "在串联电路中，电流处处相等")
        self.play(Indicate(line))
        self.play(L1.animate.shift(2*RIGHT))
        self.play(I1.animate.shift(5*LEFT))
        self.play(I1.animate.shift(5*RIGHT))
        self.play(L1.animate.shift(LEFT))
        self.sub_text(st, "而在并联电路中，电流大小等于各支路的电流总和")
        line2 = Line([-4,1,0], [4,1,0], color=WHITE)
        L2.shift(4*LEFT+DOWN)
        I2 = I1.copy().shift(RIGHT)
        I3 = I1.copy().shift(DOWN)
        self.play(Create(line2), Create(L2), I1.animate.shift(2*RIGHT+2*DOWN))
        self.play(L2.text_change("L2", DOWN), I1.text_change("0.5A",1.5*RIGHT), Create(I2), Create(I3))
        self.play(I3.text_change("0.8A", DOWN))
        self.play(I1.text_change("0.5+0.8A", 2*DOWN))
        self.wait(0.8)
        self.play(I1.text_change("1.3A", 1.5*RIGHT))
        self.sub_text(st, "倘若我们将一条支路安上开关并控制", 0.4)
        S2 = Power_switch().shift(UP)
        self.play(Create(S2), I3.text_change("0A", DOWN))
        self.play(I1.text_change("0.5A", 1.5*RIGHT))
        self.sub_text(st, "则闭合支路电流不变，总电流变化", 1.4)
        self.sub_text(st, "即并联电路中各支路电流互不影响", 0.4)
        self.wait(1)
        self.play(S2.animate.state_close(), I3.text_change("0.8A", DOWN))
        self.play(I1.text_change("1.3A", 1.5*RIGHT))
        self.sub_text(st, "倘若我们将一条支路多安装一个用电器", 0.4)
        L3 = L1.copy().shift(2*LEFT)
        self.play(Create(L3), L3.text_change("L3"))
        self.play(I2.text_change("0.3A"))
        self.play(I1.text_change("1.1A", 1.5*RIGHT))
        self.sub_text(st, "则该支路电流受到影响，其他支路不受干扰", 1.6)
        self.play(FadeOut(line2), FadeOut(I1), FadeOut(I2), FadeOut(I3), FadeOut(S2), FadeOut(L3), L1.animate.shift(LEFT), L2.animate.shift(UP+4*RIGHT))
        self.play(L2.text_change("L2"))
        return st, power, S1, L1, L2, line

    def scene_5(self, st, power, S1, L1, L2, line):
        self.sub_text(st, "下一节，电压", 1)
        self.play(ReplacementTransform(line, Power_line([[0,-2], [4,-2], [4,2], [-4,2], [-4,-2], [0, -2]])), power.animate.shift(DOWN), S1.animate.shift(DOWN))
        U1 = Power_V().shift(0.5*DOWN)
        U2 = Power_V().shift(2*LEFT+0.5*UP)
        U3 = Power_V().shift(2*RIGHT+0.5*UP)
        line2 = Power_line([[1,-2], [1,-0.5], [-1,-0.5], [-1,-2]])
        line3 = Power_line([[3,2], [3,0.5], [1,0.5], [1,2]])
        line4 = Power_line([[-1,2], [-1,0.5], [-3,0.5], [-3,2]])
        self.play(Create(line2), Create(line3), Create(line4), Create(U1), Create(U2), Create(U3))
        self.sub_text(st, "这是仨电压表",)
        self.play(Indicate(U1), Indicate(U2), Indicate(U3))
        self.sub_text(st, "其内阻很大，能通过的电流很少", 1.2)
        self.sub_text(st, "分析时看做断开的开关", 1)
        self.sub_text(st, "所以用电器要和它并联", 1)
        self.sub_text(st, "但是要注意通电时电压表本身是可以工作的", 1.6)
        self.sub_text(st, "在串联电路中，各部分电压之和等于总电压")
        self.play(U1.text_change("3V"), U2.text_change("1.8V", DOWN), U3.text_change("1.2V", DOWN))
        self.sub_text(st, "即 U = U1 + U2", 1.2)

        self.sub_text(st, "在并联电路中，各支路以及干路的电压相等")
        self.play(FadeOut(line2), FadeOut(line3), FadeOut(line4), FadeOut(U2), FadeOut(U3))
        del line2, line3, line4, U2, U3
        line2 = Line([-4,-0.5,0],[4,-0.5,0])
        line3 = Line([-4,1,0],[4,1,0])
        self.play(Create(line2), Create(line3), L2.animate.shift(DOWN))
        self.add(U1, L2)
        self.play(U1.text_change("3V"))
        self.sub_text(st, "因为电压在导线中不损耗", 1)
        self.sub_text(st, "故在同一条导线上的电压相等", 1)
        self.sub_text(st, "因为电压表连的导线直连电源两极、用电器两端", 1.6)
        self.sub_text(st, "所以无论怎么测电压都是相等的")
        self.play(U1.animate.shift(3*LEFT), run_time=1.5)
        self.play(U1.animate.shift(6*RIGHT), run_time=3)
        self.play(U1.animate.shift(3*LEFT), run_time=1.2)

        self.sub_text(st, "总结：", 0.7)
        self.sub_text(st, "同一根导线电势相等，导线分开只分电流", 1.5)
        self.sub_text(st, "电流经过用电器电流不变，但会形成电压", 1.5)
        self.play(FadeOut(U1), FadeOut(line2), FadeOut(line3), L2.animate.shift(UP))

        return st, power, S1, L1, L2, line

    def scene_6(self, st, power, S1, L1, L2, line):
        return st, power, S1, L1, L2, line

    def construct(self):
        # plane = NumberPlane()
        # self.add(plane)

        st = Text("这里有一副电路图").set_color(RED)
        self.play(Write(st))
        self.wait(0.15)
        self.play(ApplyMethod(st.shift, 3*DOWN))
        self.wait(1)

        power, S1, L1, L2, line = self.scene_init()
        st, power, S1, L1, L2, line = self.scene_1(st, power, S1, L1, L2, line)
        st, power, S1, L1, L2, line = self.scene_2(st, power, S1, L1, L2, line)
        st, power, S1, L1, L2, line = self.scene_3(st, power, S1, L1, L2, line)
        st, power, S1, L1, L2, line = self.scene_4(st, power, S1, L1, L2, line)
        st, power, S1, L1, L2, line = self.scene_5(st, power, S1, L1, L2, line)

        self.sub_text(st, "未完待续", 0.5)
        self.wait(2)

class debug(Physical):
    def construct(self):
        plane = NumberPlane()
        self.add(plane)

        st = Text("这里有一副电路图").set_color(RED)
        self.play(Write(st))
        self.wait(0.15)
        self.play(ApplyMethod(st.shift, 3*DOWN))
        self.wait(1)

        power, S1, L1, L2, line = self.scene_init()
        self.play(Create(line), Create(L1), Create(S1), Create(power), run_time=1.2)
        self.scene_5(st, power, S1, L1, L2, line)

        self.wait(2)

