#!/usr/bin/env manim -pql -s
# 关于物理学的动画演示源码

from manim import *

st = Text("高中物理:小部分内容串讲").set_color(RED)
class force(Scene):
    def sub_text(self, text, string, time=0):
        text2 = Text(string)
        text2.set_color(RED)
        text2.shift(3*DOWN)
        self.play(Transform(text, text2))
        self.wait(time)
    def scene_1(self, st):
        arrow = Arrow(DOWN*2, UP*1.5, buff=0)
        self.wait(0.2)
        self.play(Create(arrow))
        self.wait(0.5)
        self.sub_text(st, "这是一个矢量", 0.5)
        self.wait(1.5)
        self.sub_text(st, "它具有大小，方向等要素", 0.5)
        self.play(Transform(arrow, Arrow(DOWN*2, ORIGIN, buff=0)))
        self.play(Transform(arrow, Arrow(DOWN*2, UP*4, buff=0)))
        self.wait(0.3)
        self.play(Rotate(arrow, 0.5 * PI, about_point=DOWN*2))
        self.play(Rotate(arrow, -PI, about_point=DOWN*2))
        self.play(Rotate(arrow, 0.5 * PI, about_point=DOWN*2))
        self.wait(1)

        self.sub_text(st, "我们有很多种方式表示它", 0.5)
        group1 = VGroup()
        group1 += Arrow([-6,-2,0], [-4,2,0], buff=0)
        dot = Dot().next_to(group1, DOWN)
        group1 += Text("A").next_to(dot, 0.25*RIGHT+0.75*UP)
        dot = Dot().next_to(group1, UP)
        group1 += Text("B").next_to(dot, 0.25*LEFT+0.75*DOWN)
        del dot
        group1 += Tex("$\\vec{v}=\\vec{AB}$").next_to(group1, DOWN)
        group1.shift(UP)

        group2 = VGroup()
        group2 += Axes([-5, 5], [-5, 5], 5*2, 5*2)
        group2 += Arrow([0, 0, 0], [2, 4, 0], buff=0)
        group2.scale(0.4)
        group2 += Text("(2,4)").next_to(group2, 0.5*DOWN)
        group2.shift(UP)

        text3 = Text("...").shift(5*RIGHT+UP)
        self.play(ReplacementTransform(arrow, group1), Create(group2), Create(text3), run_time=2.5)
        self.sub_text(st, "与矢量相对应的就是标量", 1)
        self.sub_text(st, "即没有方向只有大小的量", 1)
        self.wait(1)
        self.play(FadeOut(group1, shift=DOWN), FadeOut(group2, shift=UP), FadeOut(text3, shift=RIGHT), run_time=1.3)

    def scene_2(self, st):
        arrow1 = Arrow([-4,0,0], [-2,0,0], buff=0)
        arrow2 = Arrow([-4,0,0], [0,0,0], buff=0)
        arrow3 = Arrow([-4,0,0], [4,0,0], buff=0)
        self.play(Create(arrow1))
        self.sub_text(st, "在物理中，位移，速度和力等物理量都是矢量", 0.5)
        self.sub_text(st, "而矢量的运算法则大致如下", 0.5)
        self.play(Transform(arrow2, Arrow([-4,2,0], [0,2,0], buff=0)))
        self.play(Transform(arrow3, Arrow([-4,-2,0], [0,-2,0], buff=0)))
        # arrow4 = Arrow([-4, 2, 0], [-2, 2, 0], buff=0)
        # arrow5 = Arrow([-4, 2, 0], [2, -2, 0], buff=0)
        # polygon = Polygon([-2, 2, 0], [-4, -2, 0], [2, -2, 0], [4, 2, 0])
        # self.play(Create(arrow4), Create(arrow5))
        # self.play(Create(polygon))

    def construct(self):
        plane = NumberPlane()
        self.add(plane)

        self.play(Write(st))
        self.wait(0.15)
        self.play(ApplyMethod(st.shift, 3*DOWN))

        # self.scene_1(st)
        self.scene_2(st)
        self.wait(2)
        pass
