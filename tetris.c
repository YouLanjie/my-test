/*
 *   Copyright (C) 2024 u0_a221
 *   
 *   文件名称：tetris.c
 *   创 建 者：u0_a221
 *   创建日期：2024年02月16日
 *   描    述：
 *
 */


#include "include/tools.h"

#define SECOND 1000000
#define TPS    (SECOND / 20)

#define Block_no 0
#define Block_yes 1

#define Shape_max 5

#define Bit() ((map.shape[map.type]) << (i + 16) >> 31)

#define Get() for (int i = 0, x = 0, y = 0; x = map.x + i % 4, y = map.y + i / 4 % 4, i < 16; i++)

struct map {/*{{{*/
	int *map;
	int weight;
	int height;
	int size;
	const unsigned int shape[Shape_max*4];
	int type;
	int x;
	int y;
} map = {
	.map    = NULL,
	.weight = 10,
	.height = 25,
	.size   = 0,
	.shape  = {
		0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
		0b1100110000000000, 0b1100110000000000, 0b1100110000000000, 0b1100110000000000,	// #
		0b1111000000000000, 0b1000100010001000, 0b1111000000000000, 0b1000100010001000,	// I
		0b0110110000000000, 0b0100011000100000, 0b0000011011000000, 0b1000110001000000,	// S
		0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
	},
	.type   = 0,
	.x      = 0,
	.y      = 0,
};/*}}}*/


int print_lock = 1;
char print_ch[][3] = {"  ", "[]"};

static int delete()
{/*{{{*/
	Get() Bit() ? map.map[y * map.weight + x] = 0 : 0;
	return 0;
}/*}}}*/

static int create(int x, int y, int type)
{/*{{{*/
	type = abs(type);

	if (type > (Shape_max - 1) * 4) {
		srand(time(NULL));
		type = rand() % (Shape_max - 2) * 4 + 4;
	}
	type %= (Shape_max - 1) * 4;

	map.type = type;
	map.x = x;
	map.y = y;
	Get() {
		if (Bit()) {
			if (map.map[y * map.weight + x] == 1)
				return 1;
			map.map[y * map.weight + x] = 1;
		}
	}
	return 0;
}/*}}}*/

static int clean()
{/*{{{*/
	int x = 0, y = 0;
	for (y = map.height - 1; y >= 0 ; y--) {
		int count = map.weight;
		for (x = map.weight - 1; x >= 0; x--) {
			map.map[y * map.weight + x] && count--;
		}
		if (count)
			continue;
		for (; y >= 0 ; y--) {
			for (x = map.weight - 1; x >= 0; x--) {
				if (y > 0) map.map[y * map.weight + x] = map.map[(y - 1) * map.weight + x];
				else map.map[y * map.weight + x] = 0;
			}
		}
		y = map.height + 1;
	}
	return 0;
}/*}}}*/

static int lmove(int *v, int step)
{/*{{{*/
	int flag = 0;

	delete();
	*v += step;
	Get() (Bit()) && (x < 0 || x >= map.weight || y >= map.height || map.map[y * map.weight + x]) && (flag = 1);
	if (flag) *v -= step;
	create(map.x, map.y, map.type);
	if (flag && v == &map.y) {
		clean();
		flag = create(0, 0, Shape_max * 4) == 0 ? flag : -1;
	}
	return flag;
}/*}}}*/

static void *print_map()
{/*{{{*/
	print_lock = 1;
	printf("\033[?25l");
	while (print_lock) {
		for (int i = 0; i < map.size; i++) {
			printf("%s%s", print_ch[map.map[i]],
			       (i + 1) % map.weight == 0 ? "|\r\n" : "");
		}
		printf("--------------------\r\n");
		printf("\033[%dA", map.height + 1);
		printf("\033[%dC| t[0]:%d t[1]:%d\r", map.weight * 2, map.type / 4, map.type % 4);
		if (print_lock && print_lock % 6 == 0) {
			lmove(&map.y, 1);
			print_lock = 1;
		}
		usleep(TPS);
		print_lock && print_lock++;
	}
	printf("\033[%dB", map.height + 1);
	printf("\033[?25hGame Over\n");
	pthread_exit(NULL);
	return NULL;
}/*}}}*/

int main()
{/*{{{*/
	const struct ctools ctools = ctools_init();
	pthread_t pid;
	int input = 0;

	map.map = calloc(map.size = map.weight * map.height, sizeof(int));
	memset(map.map, 0, map.size);

	pthread_create(&pid, NULL, print_map, NULL);
	create(map.x, map.y, Shape_max * 4);
	while(input != 'Q') {
		input = ctools.getcha();
		input = (input >= 'a' && input <= 'z') ? input - 32 : input;
		switch (input) {
		case 'W':
			lmove(&map.type, map.type % 4 < 3 ? 1 : -3);
			break;
		case 'A':
			input = lmove(&map.x, -1);
			break;
		case 'S':
			lmove(&map.y, 1);
			break;
		case 'D':
			lmove(&map.x, 1);
			break;
		case ' ':
			input = 0;
			while (input == 0)
				input = lmove(&map.y, 1);
			break;
		default:
			break;
		}
		input == -1 && (input = 'Q');
	}

	print_lock = 0;
	usleep(TPS * 2);
	free(map.map);
	return 0;
}/*}}}*/

