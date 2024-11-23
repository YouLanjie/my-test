#!/usr/bin/env manim -pqh -s

from manim import *

class Color(Scene):
    def construct(self):
        # plane = NumberPlane()
        # self.add(plane)
        colors = dir(color.manim_colors)[:-10]
        colors.remove("List")
        colors.remove("ManimColor")
        boxs = VGroup()
        for i in colors:
            group = VGroup()
            col = color.manim_colors.__dict__[i]
            box = Rectangle(width=0.5, height=0.5, color=col, fill_opacity=1)
            text = Text(i).scale(0.2).next_to(box, UP*0.2)
            group.add(box, text)
            boxs.add(group)
        boxs.arrange_in_grid(buff=0.2)
        self.add(boxs)
