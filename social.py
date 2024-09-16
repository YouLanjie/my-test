#!/usr/bin/python

class Illness:
    effort = 0.5
    time = 0 #发病结束倒计时
    cost = 0 #治疗花费
    death = 0.5
    posible = 0.1

class Person:
    "人"
    stat = {"kind":0, "work":0}
    age = 18.00
    XP = 0.01
    level = 0.5
    food = 0
    money = 0
    save = 0
    illness = Illness()
    @property
    def work_ability(self):
        pass
    @property
    def cost_ability(self):
        pass
    @property
    def happiness(self):
        pass

    def update(self):
        "日更新值"
        self.age += 0.01


class Worker(Person):
    def update(self):
        super().update()
        if self.stat["work"] == 1:
            self.XP += 0.001
        else if self.XP >= 0.001
            self.XP -= 0.002

    def have_work(self):
        self.stat["work"] = 1

    def lose_work(self):
        self.stat["work"] = 0

class Capital(Person):
    "资"
    worker = []
    wages = 100    #工资
    cupidity = 0.5 #贪心率
    @property
    def happiness(self):
        pass

    def __init__(self, worker = [Person()]):
        self.worker.append(worker)
    def add_worker(self):
        if len(self.worker)
        self.worker()

class Social:
    "社会环境"
    pop = []
    need_work = []
    cap = []
    price = 200
    food = 0
    require = 0

    def __init__(self, pop = 10, cap = 1, worker = [1], price = 200):
        if cap > len(worker) or sum(worker) > pop:
            print("Error:impossible number")
            return
        self.pop = [Worker() for i in range(pop)]
        self.need_work = self.pop
        self.cap = [Capital(self.pop[sum(worker[:i]):worker[i]]) for i in range(cap)]

if __name__ == "__main__":
    print("Here are nothing")
