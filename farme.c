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

#define SECOND 1000000
#define TPS    SECOND / 20

#define Max 22

void *input();
void *logic();
int is_move(int *v, int l);

void b_2_func(void);
void b_3_func(void);


struct block_rules {
	char *p;
	int is_move;
	int is_tp;
	void (*func_u)();    /* use */
	void (*func_t)();    /* touch */
} b_rules[] = {
	{"国", 0, 0, NULL, NULL},
	{"  ", 1, 0, NULL, NULL},
	{"  ", 1, 0, b_2_func,    b_2_func},
	{"国", 1, 0, b_3_func, b_3_func},
};

int inp = 0;
int map[Max][Max] = {/*{{{*/
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
	{1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0}, 
	{0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0}, 
	{0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0}, 
	{0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0}, 
	{0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0}, 
	{0, 1, 0, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0}, 
	{0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0}, 
	{0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0}, 
	{0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0}, 
	{0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0}, 
	{0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0}, 
	{0, 1, 3, 0, 1, 0, 1, 1, 0, 0, 3, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0}, 
	{0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0}, 
	{0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0}, 
	{0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 3, 1, 0}, 
	{0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0}, 
	{0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0}, 
	{0, 1, 1, 0, 1, 0, 1, 0, 3, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0}, 
	{0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 3, 1, 0}, 
	{0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 2}, 
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
};/*}}}*/
int x = 0, y = 1;
int flag_use = 0;
int flag_win = 0;

int main(void)
{/*{{{*/
	pthread_t pid, pid2;
	FILE *fp = fopen("maze.txt", "r");
	if (!fp)
		goto RUN;
	for(int i = 0; i < Max; i++) {
		for(int j = 0; j < Max; j++)
			fscanf(fp, "%d", &map[i][j]);
		if (map[i][Max - 1] == 1) map[i][Max - 1] = 2;
	}
	fclose(fp);

RUN:
	printf("\033[?25l");
	printf("Type `q` return.\n");
	pthread_create(&pid, NULL, input, NULL);
	pthread_create(&pid2, NULL, logic, NULL);
	while (inp != 'q') {
		usleep(TPS);
		printf("Your Type:%c\r\n", inp);
		for (int i = 0; i < Max; i++) {
			for (int j = 0; j < Max; j++) {
				if (i == y && j == x) {
					printf(":;");
					continue;
				}
				printf("%s", b_rules[map[i][j]].p);
			}
			if (i == 0) printf(" |  X:%2d  Y:%2d  | Type `q` to return,`wasd` to move,", x, y);
			else if (i == 1) printf(" |  flag_use:%2d | `u` to use the object you select.", flag_use);
			else if (i == 2) printf(" | flag_win:%3d | Only some block can walk.", flag_win);
			else if (i == 3) printf(" |              | Some block have special event.");
			else if (i == 4) printf(" |  walk:  . /  | Find the way to leave there!");
			else if (i == 5) printf(" |  nwalk: # +  | Good Luck!");
			printf("\r\n");
		}
		printf("\033[%dA", Max + 1);
		if (flag_win) break;
	}
	printf("\033[%dB", Max + 1);
	printf("\033[?25h");
	if (flag_win) printf("You did it!You Win!\n");
	return 0;
}/*}}}*/

void *input()
{/*{{{*/
	while (inp != 'q') {
		inp = _getch();
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
	struct block_rules b = b_rules[map[y][x]];
	if (flag_use && b.func_u) b.func_u();
	if (flag_use || x < 0 || y < 0 || x > (Max - 1) || y > (Max - 1) || b_rules[map[y][x]].is_move == 0) *v-=l;
	else if (b.func_t) b.func_t();
	flag_use = 0;
	return 0;
}/*}}}*/

void b_2_func(void)
{/*{{{*/
	flag_win = 1;
	inp = 'q';
	return;
}/*}}}*/

void b_3_func(void)
{/*{{{*/
	x = 0;
	y = 1;
	return;
}/*}}}*/

