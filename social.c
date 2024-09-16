/*
 *   Copyright (C) 2024 u0_a70
 *
 *   文件名称：social.c
 *   创 建 者：u0_a70
 *   创建日期：2024年09月16日
 *   描    述：劳资模拟小程序
 *
 */


#include "include/tools.h"
#include <cstdlib>
#include <stdio.h>

struct social {
	int population;
	int capital;
	int prices;
	int food;
	int time;
} Social = { 10, 1, 200, 0, 0 };

struct worker {
	int flag;
	int age;
	int xp;
	int food;
	int money;
	double save;
	/* Note:
	 * work_ability
	 * cost_ability
	 * life_level
	 * happiness
	 * */
};

struct link {
	void *p;
	struct link *np;
};

struct capital {
	struct link *worker;
	int wages;	/* 工资 */
	int cupidity;	/* 贪心率 */
};


struct link *worker = NULL;
struct link *capital = NULL;


#define T_WORK (0b01)
int worker_flag(struct worker *p,int TYPE, int flag)
{
	if (!p) return -1;
	p->flag ^= (TYPE & (flag ? ~0 : 0));
	return (p->flag & TYPE ? 1 : 0);
}

int capital_add_worker(struct capital *cap)
{
	for (struct link *p = worker; p != NULL; p = p->np) {
		if (worker_flag(p->p, T_WORK, 0)) continue;
		worker_flag(p->p, T_WORK, 1);
		if (!cap->worker) {
			cap->worker = malloc(sizeof(struct link));
			cap->worker->p = p->p;
		}
	}
	return 0;
}

struct link *create_link(int len, void *context)
{
	struct link *head = NULL, *p = NULL, *lp = NULL;

	head = p = malloc(sizeof(struct link));
	for (int i = 0; i < len; i++) {
		p->p = context;
		lp = p;
		lp->np = p = malloc(sizeof(struct link));
	}
	lp->np = NULL;
	free(p);
	return head;
}

struct worker *create_worker()
{
	struct worker *p = malloc(sizeof(struct worker));
	p->flag = 0;
	p->age = 18;
	p->xp = 0;
	p->food = 0;
	p->money = 0;
	p->save = 0.5;
	return p;
}

struct capital *create_capital()
{
	struct capital *p = malloc(sizeof(struct worker));
	p->worker = NULL;
	p->wages = 200;
	p->cupidity = 1;
	return p;
}

int init()
{
	struct link *p = NULL, *lp = NULL;

	worker = create_link(Social.population, create_worker());
	capital = create_link(Social.capital, create_capital());
	return 0;
}

int run()
{
	for (int i = 0; i < 20; i++) {
		Social.time++;
		for (struct link *p = worker; p; p = p->np) {
			((struct worker*)p->p)->age += 20 % Social.time == 0 ? 1 : 0;
			((struct worker*)p->p)->xp += 20 % Social.time == 0 ? 1 : 0;
		}
	}
}

int main(int argc, char *argv[])
{
	int ch = 0;
	while ((ch = getopt(argc, argv, "hp:c:")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			// help();
			printf("Usage: xxx [-h] [-p <population>] [-c <capital>]\n");
			return 0;
			break;
		case 'p':
			Social.population = strtod(optarg, NULL);
			break;
		case 'c':
			Social.capital = strtod(optarg, NULL);
			break;
		default:
			break;
		}
	}
	init();
	printf("Pop:%d,Cap:%d\n", Social.population, Social.capital);
	capital_add_worker(capital->p);
	return 0;
}

