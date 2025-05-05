#!/usr/bin/env -S manim -ql

if __name__ == "__main__":
    import sys
    import os
    command = f"manim -ql {sys.argv[0]}"
    print("==> Run Command:")
    print(" -> " + command)
    os.system(command)
    print("==> Exit")
    exit()

from manim import *

class SubTitle(Text):
    string = "InitText"
    def __init__(self, text):
        Text.__init__(self, text)
        self.string = text
        self.shift(3*DOWN)
    def gt(self):
        print(f"Time:{len(self.string)*0.1}, Text:{self.string}")
        return len(self.string)*0.1
    def dsp(self, time=-1):
        if time == -1:
            time = self.gt()
        return (Write(self),Wait(time+1)) \
            if time > 0 \
            else Write(self)
    def to(self, text, time=-1):
        self.string = text
        if time == -1:
            time = self.gt()
        return (Transform(self, SubTitle(text)), Wait(time+1)) \
            if time > 0 \
            else (Transform(self, SubTitle(text)))

class Main(Scene):
    def construct(self):
        self.add(NumberPlane().set_opacity(0.4))
        vec1 = Arrow(DL, UR)
        group1 = VGroup(Text("A").next_to(DL, LEFT), Text("B").next_to(UR, RIGHT))
        sbt = SubTitle("这是一个向量")
        self.play(Create(group1), GrowArrow(vec1), sbt.dsp(0.3))
        self.play(Rotate(vec1, PI/3,about_point=DL), sbt.to("可旋转"))
        self.play(Rotate(vec1, -PI/2,about_point=DL,run_time=0.7))
        self.play(Rotate(vec1, PI/6,about_point=DL,run_time=0.7))
        self.play(Transform(vec1, vec1.copy().scale(2,about_point=DL)), sbt.to("可伸缩"))
        self.play(Transform(vec1, vec1.copy().scale(1/2,about_point=DL)))
        self.play(sbt.to("人畜无害", 0.6))

        wall = VGroup(Line([-5,3,0],[-5,-2,0]), Line([-5,-2,0],[6,-2,0]),
            *[Line(ORIGIN, [-0.2,-0.2,0]).shift([-5,3-i*0.5,0]) for i in range(10)],
            *[Line(ORIGIN, [-0.2,-0.2,0]).shift([i*0.5-5,-2,0]) for i in range(22)])
        self.play(FadeOut(group1, shift=UP*3),
            FadeOut(vec1, shift=UP*3),
            Create(wall), sbt.to("而这，是一个场景"))

        ball1 = Circle(0.5).shift(2*UP)
        ball2 = Circle(0.5).shift(2*DOWN)
        line1 = Line(1.5*UP, 1.5*DOWN)
        group2 = VGroup(
            ball1,
            group1[0].next_to(ball1, RIGHT),
            ball2,
            group1[1].next_to(ball2, RIGHT),
            line1).set_color(GREEN).set_fill(opacity=1).shift(4.5*LEFT+0.5*UP)
        vec2 = vec1.copy().rotate(-PI/4).next_to(ball2, DOWN).scale(0.5)
        vec1.rotate(-PI*3/4).next_to(ball1, LEFT).scale(0.5)
        self.play(
            Create(vec1), Create(vec2),
            Create(group2), sbt.to("两个质量相等的光滑小球用轻杆连接"))
        self.play(sbt.to("竖直放置在光滑水平面和竖直墙面旁"))
        self.play(sbt.to("稍加轻微扰动，使两小球开始沿箭头方向移动"))
        self.play(sbt.to("问A球即将触地时A和B的瞬时速度"))
        # self.play(sbt.to("由于这只是道高中题，不考虑碰撞等带来的能量损失"))
        self.play(sbt.to("我们先对整个过程进行运动学分析"))
        self.play(sbt.to("由于轻杆的连接"))
        vg = VGroup(MathTex("v_g").next_to(line1, RIGHT),
            Arrow(line1.get_center()+[0.1,0.5,0],line1.get_center()+[0.1,-0.5,0]))
        self.play(sbt.to("A和B在轻杆上的速度是相同的"), Create(vg))
        self.play(sbt.to("若定义轻杆与水平方向上的角为θ"))
        math1 = VGroup(MathTex("v_g = v_a \\times{} sin(\\theta{}) = v_b \\times{} cos(\\theta{})"),
            MathTex("v_a = \\frac{v_b}{\\tan(\\theta{})}").shift(DOWN)).shift(2*UP)
        self.play(Write(math1), sbt.to("则理论上满足以上关系(速度分解)"))
        self.play(sbt.to("由于A落地瞬间θ接近于0"))
        math1.add(MathTex("v_b = v_a tan(\\theta) = 0"))
        self.play(sbt.to("所以会有",0.5), Create(math1[-1]))
        self.play(sbt.to("也就是说球B静止"))
        self.play(sbt.to("这就是这物理道题标准答案所说的"))
        self.play(sbt.to("那这对么？"))
        self.play(sbt.to("肯定是不对啊。。。"), FadeOut(vg, shift=UP), FadeOut(math1, shift=UP))
        axis = Axes([-1,11],[-1,5.5],12,6.5).shift(0.75*UP+0.5*RIGHT).set_color(BLUE)
        self.play(
            Create(axis),
            Transform(group2, group2.copy().set_color(WHITE).set_fill(opacity=0)),
            sbt.to("以B初始位置中心原点建立坐标系", 0))
        self.play(Transform(group1, group1.copy().set_fill(opacity=1)))
        self.play(sbt.to("设A(0,4)，B(0,0)"))
        dot1 = Dot(line1.get_center(), color=RED)
        self.play(Create(dot1), sbt.to("由质量相等易得两小球构成的系统重心(0,2)"))
        group2+=dot1
        self.play(sbt.to("如果沿用上述答案"))
        self.play(
            Transform(group2, group2.copy().rotate(PI/2, about_point=dot1.get_center()).shift(2*DOWN+2*RIGHT)),
            sbt.to("那么，整个系统的运动会像这样"))
        self.play(sbt.to("注意到了么？"))
        self.play(sbt.to("整个系统的中心转移到了(2,0)"))
        self.play(sbt.to("也就是整个系统进行了纵向和横向的位移"))
        # math2 = VGroup(
            # MathTex("2mgh = \\frac{1}{2}m({v_a}^2+{v_b}^2) = F_{x}s_x+F_{y}s_y"),
            # MathTex("F_{y} = mg, s_y = h").shift(DOWN)).shift(2*UP+RIGHT)
        # self.play(sbt.to("也就是说"), Write(math2))
        # self.play(sbt.to("其中h为重心下落高度，m为单个小球的质量"))
        # self.play(sbt.to("F和s均为系统在xy轴的受力和位移"))
        self.play(sbt.to("换言之就是系统受到了向右的力产生了位移"),
            Circumscribe(group2))
        self.play(sbt.to("那么，既然有力，且有时间，那应该就有速度"))
        self.play(sbt.to("如果B的速度为0"))
        vec3 = VGroup(vec2.copy().rotate(PI, UP).shift(2*RIGHT+1.7*UP))
        vec3 += Text("F ?").next_to(vec3, UP)
        self.play(sbt.to("就说明有一个向左的力在牵拉整个系统"), Create(vec3))
        self.play(sbt.to("然而整个环境并没有摩擦力"), Transform(vec3[-1], Text("f ?").shift(vec3[-1].get_center())))
        self.play(sbt.to("也不存在可以向左提供弹力的平面"), Transform(vec3[-1], Text("T ?").shift(vec3[-1].get_center())))
        self.play(sbt.to("整个系统不可能产生向左的加速度"), Transform(vec3[-1], Text("a=0").shift(vec3[-1].get_center())))
        self.play(sbt.to("B也就不可能静止"), Transform(vec3[-1], Text("v!=0").shift(vec3[-1].get_center())))
        self.play(sbt.to("所以说整个系统应当持续向右运动下去"),
            Transform(group2, group2.copy().shift(14*RIGHT)),
            FadeOut(vec3, shift=RIGHT*14))
        self.play(sbt.to("那么，最开始的分析哪里错了呢？"))
        self.play(sbt.to("答案就是这条式子"),
            FadeIn(math1, shift=DOWN*5))
        self.play(sbt.to("在这里，我们实际上已经假设了A球不会水平移动"),
            Circumscribe(math1[0]))
        self.play(sbt.to("但是实际上A球会将自身的重力势能"))
        self.play(sbt.to("转化为A和B的动能"))
        self.play(sbt.to("在B加速到超过A绕B转动的水平分速度时"))
        self.play(sbt.to("A就会脱离墙壁斜向下下落"))
