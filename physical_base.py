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

class Power_arrow(VGroup):
    def __init__(self, point = [[-1, 0], [1, 0]], z = 0):
        VGroup.__init__(self)
        l = len(point)
        count = 1
        pos_last = point[0]
        pos_last.append(z)
        while count < l - 1:
            pos_now = point[count]
            pos_now.append(z)
            self.add(Line(pos_last, pos_now))
            pos_last = pos_now
            count += 1
        pos_now = point[count]
        pos_now.append(z)
        self.add(Arrow(pos_last, pos_now))

class Power_M(VGroup):
    def __init__(self, point = [[-1, 0], [1, 0]]):
        VGroup.__init__(self)
        cir = Circle(color=WHITE).scale(0.5).set_fill(BLACK, opacity=1)
        text = Text("M")
        self.add(cir, text)
    def state_normal(self):
        # self[0].set_fill(BLACK, opacity=1)
        self.set_stroke(WHITE)
    def state_work(self):
        self.set_stroke(GREEN)
    def state_error(self):
        self.set_stroke(RED)
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

class Power_A(VGroup):
    def __init__(self, point = [[-1, 0], [1, 0]]):
        VGroup.__init__(self)
        cir = Circle(color=WHITE).scale(0.5).set_fill(BLACK, opacity=1)
        text = Text("A")
        self.add(cir, text)
    def state_normal(self):
        # self[0].set_fill(BLACK, opacity=1)
        self.set_stroke(WHITE)
    def state_work(self):
        self.set_stroke(GREEN)
    def state_error(self):
        self.set_stroke(RED)
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

class Power_R(Power_used):
    def __init__(self):
        VGroup.__init__(self)
        pol = Polygon([-0.5, 0.1, 0], [0.5, 0.1, 0], [0.5, -0.1, 0], [-0.5, -0.1, 0], color=WHITE).set_fill(BLACK, opacity=1)
        pol2 = Polygon([-0.5, 0.1, 0], [0.5, 0.1, 0], [0.5, -0.1, 0], [-0.5, -0.1, 0], color=WHITE).set_fill(YELLOW, opacity=0)
        self.add(pol, pol2)
    def text_change(self, text, direction = 0.5*UP):
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

class Power_RC(Power_R):
    def __init__(self):
        VGroup.__init__(self)
        pol = Polygon([-0.5, 0.1, 0], [0.5, 0.1, 0], [0.5, -0.1, 0], [-0.5, -0.1, 0], color=WHITE).set_fill(BLACK, opacity=1)
        pol2 = Polygon([-0.5, 0.1, 0], [0.5, 0.1, 0], [0.5, -0.1, 0], [-0.5, -0.1, 0], color=WHITE).set_fill(YELLOW, opacity=0)
        line2 = Power_line([[1, 0], [0.75, 0], [0.75, 0.5], [0.25, 0.5]])
        arrow = Power_arrow([[0.5, 0.5], [0, 0.5], [0, 0.1]])
        line = Line([0.5, 0, 0], [0.75, 0, 0], color=BLACK)
        self.add(line, pol, pol2, line2, arrow)
    def text_change(self, text, direction = UP):
        if len(self) == 5:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            self.add(text2)
            return Write(self[5])
        else:
            text2 = Text(text)
            text2.next_to(self[0], ORIGIN)
            text2.shift(direction)
            return Transform(self[5], text2)
    def body(self):
        return VGroup().add(self[1], self[2], self[3], self[4])
    def changer(self):
        return self[4]
