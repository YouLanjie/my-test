/*
 *   Copyright (C) 2024 u0_a221
 *   
 *   文件名称：farme.c
 *   创 建 者：u0_a221
 *   创建日期：2024年01月31日
 *   描    述：测试C语言双线程刷新
 *
 */


#include "include/tools.h"
#include <pthread.h>

#define SECOND 100000
#define TPS    SECOND / 20

void *input();
void *logic();
int is_move(int *v, int l);

void b_null_func(void);
void b_2_func(void);
void b_3_func(void);
void b_4_func(void);
void b_5_func(void);


struct block_rules {
	char p;
	int is_move;
	int is_tp;
	void (*func)();
} b_rules[] = {
	{'#', 0, 0, b_null_func},
	{'.', 1, 0, b_null_func},
	{'+', 0, 0, b_2_func},
	{'/', 1, 1, b_3_func},
	{'+', 0, 0, b_4_func},
	{'/', 1, 1, b_5_func},
};

int inp = 0;
int map[10][10] = {/*{{{*/
	{0, 0, 2, 0, 4, 0, 0, 2, 0, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{2, 1, 1, 1, 1, 1, 1, 1, 1, 4},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 0, 0, 0, 2, 0, 2, 0, 3, 0}
};/*}}}*/
int x = 4, y = 4;
int flag_use = 0;
int flag_win = 0;
int flag_bl5 = 0;

int main(void)
{/*{{{*/
	pthread_t pid, pid2;

	printf("\033[?25l");
	printf("Type `q` return.\n");
	pthread_create(&pid, NULL, input, NULL);
	pthread_create(&pid2, NULL, logic, NULL);
	while (inp != 'q') {
		usleep(TPS);
		printf("Your Type:%c\r\n", inp);
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 10; j++) {
				if (i == y && j == x) {
					printf("@");
					continue;
				}
				printf("%c", b_rules[map[i][j]].p);
			}
			if (i == 0) printf(" |  X:%2d  Y:%2d  | Type `q` to return,`wasd` to move,", x, y);
			else if (i == 1) printf(" |  flag_use:%2d | `u` to use the object in your select way.", flag_use);
			else if (i == 2) printf(" | flag_win:%3d | Some block can walk and some are not.", flag_win);
			else if (i == 3) printf(" |              | Some block have special event.");
			else if (i == 4) printf(" |  walk:  . /  | Find the way to leave there!");
			else if (i == 5) printf(" |  nwalk: # +  | Good Luck!");
			printf("\r\n");
		}
		printf("\033[11A");
	}
	printf("\033[11B");
	printf("\033[?25h");
	return 0;
}/*}}}*/

void *input()
{/*{{{*/
	while (inp != 'q') {
		inp = ctools_getch();
	}
	pthread_exit(NULL);
	return NULL;
}/*}}}*/

void *logic()
{/*{{{*/
	while (inp != 'q') {
		usleep(TPS);
		switch (inp) {
		case 'w':
			is_move(&y, -1);
			break;
		case 'a':
			is_move(&x, -1);
			break;
		case 's':
			is_move(&y, 1);
			break;
		case 'd':
			is_move(&x, 1);
			break;
		case 'q':
			pthread_exit(NULL);
			return NULL;
			break;
		case 'u':
			flag_use = 1;
			break;
		default:
			break;
		}
		inp = ' ';
	}
	pthread_exit(NULL);
	return NULL;
}/*}}}*/

int is_move(int *v, int l)
{/*{{{*/
	*v+=l;
	if (flag_use) b_rules[map[y][x]].func();
	if (flag_use || x < 0 || y < 0 || x > 9 || y > 9 || b_rules[map[y][x]].is_move == 0) *v-=l;
	else b_rules[map[y][x]].func();
	flag_use = 0;
	return 0;
}/*}}}*/

void b_null_func(void)
{/*{{{*/
	return;
}/*}}}*/

void b_2_func(void)
{/*{{{*/
	map[y][x] = 3;
	return;
}/*}}}*/

void b_3_func(void)
{/*{{{*/
	srand(time(NULL));
	if (flag_bl5 == 0) {
		flag_win -= (rand() % 10 > 5 ? 0 : rand() % 10 );
		if (rand() % 100 > 95) map[y][x] = 5;
	}
	int i = 0, j = 0;
	int all = 0;
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 10; j++) {
			if (b_rules[map[i][j]].is_tp && (i != 0 || j != 0) && i != y && j != x) {
				all++;
			}
		}
	}
	srand(time(NULL));
	int count = (all == 1) ? 1 : rand() % (all - 1) + 1;
	int count2 = 0;
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 10; j++) {
			if (b_rules[map[i][j]].is_tp && (i != 0 || j != 0) && i != y && j != x) {
				count2++;
				if (count2 == count) {
					x = j;
					y = i;
					break;
				}
			}
		}
	}
	return;
}/*}}}*/

void b_4_func(void)
{/*{{{*/
	map[y][x] = 5;
	return;
}/*}}}*/

void b_5_func(void)
{/*{{{*/
	srand(time(NULL));
	if (rand() % 100 < 6) map[y][x] = 3;
	flag_bl5 = 1;
	flag_win++;
	b_3_func();
	flag_bl5 = 0;
	return;
}/*}}}*/

