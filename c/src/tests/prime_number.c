/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：number.c
 *   创 建 者：youlanjie
 *   创建日期：2023年11月11日
 *   描    述：计算质数
 *
 */


#include "../../include/tools.h"

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
		if (tmp->number && *c % tmp->number == 0) flag = 0;
		tmp = tmp->next;
	}
	if (!flag) {
		(*c)++;
		flag = check(c);
	}
	return flag;
}

/*
 * get list
 */
int get_list(void)
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
	return 0;
}

/*
 * print result
 */
int print_1(int flag)
{
	int count = 0;
	printf("计算出%d以内的质数:\n", Lim);
	Linker *pNew = head, *pEnd = pNew;
	while (pNew != NULL) {
		if (flag) {
			printf("-> %d\t%d\n", pNew->number, pNew->number - pEnd->number);
		} else {
			printf("-> %d\n", pNew->number);
		}
		pEnd = pNew;
		pNew = pNew->next;
		count++;
	}
	printf("Total: %d\n", count);
	return 0;
}


int main(int argc, char *argv[])
{
	int ch = 0;
	int flag_l = 0;
	while ((ch = getopt(argc, argv, "hl")) != -1) {
		switch (ch) {
		case 'h': {
			printf("Usage:\n"
			       "    number [-l]\n"
			       "Options:\n"
			       "    -h      Show this message\n"
			       "    -l      显示质数差\n"
			       );
			return 0;
			break;
		}
		case 'l': {
			flag_l = 1;
			break;
		}
		default:
			break;
		}
	}
	get_list();

	print_1(flag_l);
	return 0;
}

