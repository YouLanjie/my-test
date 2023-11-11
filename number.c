/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：number.c
 *   创 建 者：youlanjie
 *   创建日期：2023年11月11日
 *   描    述：计算质数
 *
 */


#include "include/tools.h"

#define Lim 10000

typedef struct Linker{
	int number;
	struct Linker *next;
}Linker;

/*
 * init data
 */
int init(Linker *p)
{
	p->number = 0;
	p->next = NULL;
	return 0;
}

Linker *head;

/*
 * check condiction
 */
int check(int *c)
{
	int flag = 1;
	Linker *tmp = head;
	while (tmp != NULL) {
		if (*c % tmp->number == 0) flag = 0;
		tmp = tmp->next;
	}
	if (!flag) {
		(*c)++;
		flag = check(c);
	}
	return flag;
}


int main(void)
{
	int count = 2;

	head = malloc(sizeof(Linker));
	init(head);

	Linker *pNew = head, *pEnd = NULL;
	while (count < Lim) {
		if (check(&count) && pNew != NULL) pNew->number = count;
		pNew->next = malloc(sizeof(Linker));
		count++;
		pEnd = pNew;
		pNew = pNew->next;
		init(pNew);
	}
	free(pNew);
	pEnd->next = NULL;

	printf("计算出%d以内的质数:\n", Lim);
	pNew = head;
	while (pNew != NULL) {
		printf("-> %d\n", pNew->number);
		pNew = pNew->next;
	}
	return 0;
}

