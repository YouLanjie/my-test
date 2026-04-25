/*
 *   Copyright (C) 2024 u0_a70
 *
 *   文件名称：social.c
 *   创 建 者：u0_a70
 *   创建日期：2024年09月16日
 *   描    述：劳资模拟小程序
 *
 */


#include "tools.h"
#include <math.h>

#define Pop 50
#define Cap 1

struct social {
	int prices;
	int food;
	int time;
} Social = { 200, 0, 0 };

struct worker {
	int flag;
	int food;
	int money;
	int xp;
}worker[Pop] = {
	{0, 5, 2000, 0}, {0, 6, 2000, 0}, {0, 7, 2000, 0}, {0, 8, 2000, 0}, {0, 9, 2000, 0},
	{0, 10, 2000, 0}, {0, 11, 2000, 0}, {0, 12, 2000, 0}, {0, 13, 2000, 0}, {0, 14, 2000, 0},
	{0, 16, 2000, 0}, {0, 17, 2000, 0}, {0, 18, 2000, 0}, {0, 19, 2000, 0}, {0, 20, 2000, 0},
	{0, 21, 2000, 0}, {0, 22, 2000, 0}, {0, 23, 2000, 0}, {0, 24, 2000, 0}, {0, 25, 2000, 0},
	{0, 5, 2000, 0}, {0, 6, 2000, 0}, {0, 7, 2000, 0}, {0, 8, 2000, 0}, {0, 9, 2000, 0},
	{0, 10, 2000, 0}, {0, 11, 2000, 0}, {0, 12, 2000, 0}, {0, 13, 2000, 0}, {0, 14, 2000, 0},
	{0, 16, 2000, 0}, {0, 17, 2000, 0}, {0, 18, 2000, 0}, {0, 19, 2000, 0}, {0, 20, 2000, 0},
	{0, 21, 2000, 0}, {0, 22, 2000, 0}, {0, 23, 2000, 0}, {0, 24, 2000, 0}, {0, 25, 2000, 0},
	{0, 5, 2000, 0}, {0, 6, 2000, 0}, {0, 7, 2000, 0}, {0, 8, 2000, 0}, {0, 9, 2000, 0},
	{0, 10, 2000, 0}, {0, 11, 2000, 0}, {0, 12, 2000, 0}, {0, 13, 2000, 0}, {0, 14, 2000, 0},
};

struct capital {
	int worker;
	int wages;	/* 工资 */
	int cupidity;	/* 贪心率 */
	int money;
	int food;
}capital[Cap] = {
	{0, 100, 0, 3000, 1}
};

static int wcount = 0;
int cap_add_worker(struct capital *cap)
{
	if (wcount >= Pop) return -1;
	
	int i = 0;
	for (i = 0; i < Pop && worker[i].flag; i++);
	if (i == Pop) return -1;
	worker[i].flag ^= 1;
	cap->worker |= 1 << i;
	wcount++;
	return 0;
}

int cap_del_worker(struct capital *cap)
{
	if (wcount <= 0) return -1;
	
	int i = 0;
	for (i = 0; i < Pop && !(worker[i].flag & 1); i++);
	if (i == Pop) return -1;
	worker[i].flag ^= 1;
	cap->worker &= ~(1 << i);
	wcount--;
	return 0;
}

#define sigmoid(x) (4.0 / (1.0 + exp(-0.1 * (x))))

int run()
{
	printf("Head:\n");
	printf("wages:%d, cupidity:%d, $:%d, food:%d\n", capital[0].wages, capital[0].cupidity, capital[0].money, capital[0].food);
	for (int i = 0; i < Pop; i++) printf("fl:%d, fd:%d, $:%d, xp:%d%s", worker[i].flag, worker[i].food, worker[i].money, worker[i].xp, i % 2 ? " |\n" : "\t| ");
	printf("\n");

	int lm = capital[0].money, lf = capital[0].food;
	for (int i = 0; i < 3000 && capital[0].money >= 0 && capital[0].food > -5; i++) {
		/*printf("=================================================\n");*/
		Social.time++;
		for (int i = 0; i < Pop; i++) {
			if (worker[i].flag & 2) continue;    /* dead */
			if (!(capital[0].worker & (1 << i))) {    /* no work */
				worker[i].xp -= worker[i].xp > 0 ? 2 : 0;
				continue;
			}
			worker[i].xp += 1;

			worker[i].money += capital[0].wages*sigmoid(worker[i].xp);
			capital[0].money -= capital[0].wages*sigmoid(worker[i].xp);
			capital[0].food += 2+worker[i].xp/20;
			/*printf("wages:%d,XP:%d,sigmoid:%lf\n",capital[0].wages , worker[i].xp, sigmoid(worker[i].xp));*/
			/*printf("cap> wages:%d, cupidity:%d, $:%d, food:%d\n", capital[0].wages, capital[0].cupidity, capital[0].money, capital[0].food);*/
		}

		capital[0].food--;
		/*printf("==========\n");*/
		for (int i = 0; i < Pop; i++) {
			if (worker[i].flag & 2) continue;    /* dead */
			worker[i].food--;
			if (worker[i].food < 5 && worker[i].money >= Social.prices && capital[0].food > 0) {
				worker[i].money -= Social.prices;
				capital[0].money += Social.prices;
				worker[i].food++;
				capital[0].food--;
			}
			if (worker[i].food < 0) {
				worker[i].flag = 2;
				if (capital[0].worker & (1 << i)) wcount--;
				capital[0].worker &= ~(1 << i);    /* no work */
			}
			/*printf("cap> wages:%d, cupidity:%d, $:%d, food:%d\n", capital[0].wages, capital[0].cupidity, capital[0].money, capital[0].food);*/
		}

		int flag = 0;
		if (capital[0].food > 0) {
			/*Social.food += capital[0].food;*/
			/*capital[0].money += Social.prices*capital[0].food;*/
			/*flag = 1;*/
			/*Social.prices--;*/
		}

		if (capital[0].money - lm > -500 && capital[0].food - lf <= 10)
			cap_add_worker(&capital[0]);
		else
			cap_del_worker(&capital[0]);
		lm = capital[0].money;
		lf = capital[0].food;

		if (flag) capital[0].food = 0;

		/*printf("====================\n");*/
		printf("Round:%d, $:%d, food:%d ", i, capital[0].money, capital[0].food);
		int i2 = 0, i3 = 0;
		for (int j = 0; j < 32; j++) {
			if (capital[0].worker & (1 << j)) i2++;
			if (j < Pop && worker[j].flag & 0b10) i3++;
		}
		printf("Workers:%d, Dead:%d\n", i2, i3);
		/*for (int i = 0; i < Pop; i++) printf("fl:%d, fd:%d, $:%d, xp:%d%s", worker[i].flag, worker[i].food, worker[i].money, worker[i].xp, i % 2 ? " |\n" : "\t| ");*/
	}
	printf("Food in social:%d\n", Social.food);
	for (int i = 0; i < Pop; i++) printf("fl:%d, fd:%d, $:%d, xp:%d%s", worker[i].flag, worker[i].food, worker[i].money, worker[i].xp, i % 2 ? " |\n" : "\t| ");
	return 0;
}

int main(int argc, char *argv[])
{
	printf("Pop:%d,Cap:%d\n", Pop, Cap);
	cap_add_worker(&capital[0]);
	run();
	return 0;
}

