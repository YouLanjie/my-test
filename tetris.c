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
#include <stdio.h>

#define SECOND 1000000
#define TPS (SECOND / 20)

#define Shape_max 9

/* 方块内第i位的状态值 */
#define Stat ((map.shape[map.type]) >> (15 - i) & 1)
#define Stat2(type) ((map.shape[type]) >> (15 - i) & 1)
/* i为顺序号，(x,y)为地图坐标 */
#define Map_for() for (int i = 0, x = 0, y = 0; x = map.x + i % 4, y = map.y + i / 4 % 4, i < 16; i++)
#define mapp map.map[y * map.weight + x]
/* 判断出界条件 */
#define Cond_out (x < 0 || x >= map.weight || y >= map.height || \
		  (map.map[y * map.weight + x] != 0 && map.map[y * map.weight + x] != 8))

struct map {
	int *map;
	int weight;
	int height;
	int size;
	const unsigned int shape[Shape_max*4];
	int type;
	int x;
	int y;
	int line;
	int score;
} map = {
	.map    = NULL,
	.weight = 10,
	.height = 25,
	.size   = 0,
	.shape  = {
		0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
		0b1100110000000000, 0b1100110000000000, 0b1100110000000000, 0b1100110000000000,	// #
		0b0000111100000000, 0b0100010001000100, 0b0000111100000000, 0b0100010001000100,	// I
		0b0110110000000000, 0b0100011000100000, 0b0000011011000000, 0b1000110001000000,	// S
		0b1100011000000000, 0b0010011001000000, 0b0000110001100000, 0b0100110010000000,	// Z
		0b0100010001100000, 0b0000111010000000, 0b1100010001000000, 0b0010111000000000,	// L
		0b0010001001100000, 0b0100011100000000, 0b0011001000100000, 0b0000011100010000,	// J
		0b0100111000000000, 0b0100011001000000, 0b0000111001000000, 0b0100110001000000,	// T
		0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
	},
	.type   = 0,
	.x      = 3,
	.y      = 0,
	.score  = 0,
};


const char *print_color[] = {
	"\033[30m",
	"\033[31m\033[41m",
	"\033[32m\033[42m",
	"\033[33m\033[43m",
	"\033[34m\033[44m",
	"\033[35m\033[45m",
	"\033[36m\033[46m",
	"\033[37m\033[47m",
	"\033[2m",
};
int print_lock = 1;		/* 控制进程、自动下落时间 */
int flag_move_lock = 0;		/* 防止两个进程同时移动 */
int flag_fake = 1;		/* 设置下落最终位置显示 */
int flag_debug = 0;		/* 调试设置 */
int flag_info = 1;
int flag_seed = 0;
time_t game_time1 = 0, game_time2 = 0;
int list[7] = {0, 0, 0, 0, 0, 0, 0};
int list_num = 0;

/* 创建等候列表 */
static int create_list()
{
	static int times = 6;
	int li[] = {1, 2, 3, 4, 5, 6, 7},
	    num = 0;
	int type  = 0,
	    shape = 0;


	list[times] = 0;
	times = 6;
	for (int i = 6; i >= 0 && list[i] != 0; times = i - 1, i--);
	for (int i = 0; i <= times; i++)
		if (list[i]) li[list[i] / 4 - 1] = 0;
	for (int i = 0; i <= times; i++) {
		if (list[i]) continue;
		do {
			num = rand() % (Shape_max - 2);
		} while (li[num] == 0);
		type = li[num];
		li[num] = 0;

		shape = rand() % 4;

		list[i] = type * 4 + shape;
	}

	times++;
	if (times > 6) {
		times = 0;
	}
	return times;
}

/* 移除旧有方块 */
static int delete()
{
	Map_for() Stat ? mapp = 0 : 0;
	return 0;
}

/* 创建方块 */
static int create(int x, int y, int type)
{
	type = abs(type);

	if (type > (Shape_max - 1) * 4) {
		list_num = create_list();
		type = list[list_num];
		/*srand(time(NULL));*/
		/*type = rand() % (Shape_max - 2) * 4 + 4;*/
	}
	type %= (Shape_max - 1) * 4;

	map.type = type;
	map.x = x;
	map.y = y;
	Map_for() {
		if (!Stat) continue;
		if (mapp != 0 && mapp != 8)
			return 1;
		mapp = type / 4;
	}
	return 0;
}

static int delete_fake()
{
	for (int i = 0; i < map.size; i++) {
		if (map.map[i] == 8)
			map.map[i] = 0;
	}
	return 0;
}

static int create_fake()
{
	int x1 = map.x, y1 = map.y;
	int flag = 0;

	delete();
	delete_fake();
	while (! flag) {
		map.y += 1;
		Map_for() (Stat) && Cond_out && (flag = 1);
	}
	map.y -= 1;
	Map_for() Stat ? mapp = 8 : 0;
	map.x = x1;
	map.y = y1;
	create(map.x, map.y, map.type);
	return 0;
}

/* 消除拼成一行的方块 */
static int clean()
{
	int x = 0, y = 0;
	int line = map.line;
	for (y = map.height - 1; y >= 0 ; y--) {
		for (x = map.weight - 1; x >= 0 && mapp; x--);
		if (x >= 0) continue;
		for (; y >= 0 ; y--) {
			for (x = map.weight - 1; x >= 0; x--) {
				if (y > 0) mapp = *(&mapp - map.weight);
				else mapp = 0;
			}
		}
		map.line += 1;
		y = map.height;
	}
	map.score += (map.line - line) * (map.line - line);
	return 0;
}

static int lmove(int *v, int step)
{
	int flag = 0;

	if (flag_move_lock)
		return 0;
	flag_move_lock = 1;
	delete();
	*v += step;
	Map_for() (Stat) && Cond_out && (flag = 1);
	if (flag) *v -= step;
	create(map.x, map.y, map.type);
	if (flag && v == &map.y) {
		clean();
		flag = create(map.weight / 2 - 2, 0, Shape_max * 4) == 0 ? flag : -1;
	}
	flag_move_lock = 0;
	return flag;
}

static void print_block(int shape, int margin_left)
{
	for (int i = 0; i < 16; i++) {
		if (i % 4 == 0) printf("\033[%dC| ", margin_left);
		printf("%s[]\033[0m", print_color[Stat2(shape) ? shape / 4 : 8]);
		if ((i + 1) % 4 == 0) printf("\r\n");
	}
	printf("\r\n");
	return;
}

static void print_next()
{
	for (int i = 0; i <= 6; i++) {
		int shape = list[flag_debug ? (i > 6 ? i - 6 : i) : (list_num + i > 6 ? list_num + i - 7 : list_num + i)];
		if (i <= 4) print_block(shape, map.weight * 2);
		else {
			if (i == 5) printf("\033[%dA", 5*5);
			print_block(shape, map.weight * 2 + 2 + 4*2 + 1);
		}
	}
	printf("\033[%dA", 5*2);
	return;
}

static void print_map()
{
	for (int i = 0; i < map.size; i++) {
		printf("%s", print_color[map.map[i]]);
		if (flag_debug) printf("\033[37m%02d\033[0m", map.map[i]);
		else printf("[]\033[0m");
		if ((i + 1) % map.weight == 0) printf("|\r\n");
	}
	for (int i = 0; i < map.weight; i++) printf("--");
	printf("-\r\n");
	return;
}

static void *print_ui()
{
	int dtime = 0;
	int info = flag_info;
	print_lock = 1;
	printf("\033[?25l");

	while (print_lock) {
		time(&game_time2);
		dtime = difftime(game_time2, game_time1);
		print_map();
		if (info ^  flag_info) {
			printf("                                                         \r\n\r\n");
			printf("                                                          \r\n"
				"                                                                \r\n");
			fflush(stdout);
			printf("\033[%dA", 4);
			fflush(stdout);
			info = flag_info;
		}
		if (flag_info) {
			printf("Line:%3d /Score:%3d /Time:(%02d:%02d:%02d) /Seed:%d\r\n\r\n", map.line, map.score,
			       dtime / 3600, dtime / 60, dtime % 60, flag_seed);
			printf("<Game> [a/s/d] Move   [w/j/k] rotate  [SPACE] fast descend\r\n"
			       "<Setting>  [b] Debug      [i] this infos  [f] predicted position\r\n");
			printf("\033[%dA", 2);
		} else {
			printf("Ln:%3d /Sc:%3d /T:(%02d:%02d:%02d) /Sd:%d\r\n\r\n", map.line, map.score,
			       dtime / 3600, dtime / 60, dtime % 60, flag_seed);
		}
		printf("\033[%dA", map.height + 3);
		print_next();
		if (print_lock && print_lock % 6 == 0) {    /* 默认每0.3s就移动一次 */
			int inp = 0;
			inp = lmove(&map.y, 1);
			print_lock = 1;
			if (inp == -1) print_lock = 0;
		}
		usleep(TPS);    /* 每0.05s就刷新一次 */
		if (print_lock) print_lock++;
	}

	printf("\033[%dB\r\n", map.height + 4);
	printf("\033[?25hGame Over\r\nIf the game did not close,press 'enter'\r\n");
	if (flag_debug) {
		printf("Exit dump:\r\n");
		print_map();
	}
	pthread_exit(NULL);
	return NULL;
}

static int key(int input)
{
	int table[256] = {['A'] = 'W', ['B'] = 'S', ['C'] = 'D', ['D'] = 'A'};

	input = (input >= 'a' && input <= 'z') ? input - 32 : input;
	if (input != 0x1B || !kbhit()) return input;

	getchar();
	input = getchar();
	input = table[input];
	return input;
}

static void run()
{
	int input = 0;

	create(map.weight / 2 - 2, map.y, Shape_max * 4);
	while(input != 'Q' && print_lock) {
		if (flag_fake) create_fake();
		input = key(_getch());
		if (flag_fake) delete_fake();
		switch (input) {
		case 'J':
			lmove(&map.type, map.type % 4 > 0 ? -1 : 3);
			break;
		case 'K':
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
		case 'F':
			flag_fake = flag_fake ? 0 : 1;
			break;
		case 'B':
			flag_debug = flag_debug ? 0 : 1;
			break;
		case 'I':
			flag_info ^= 1;
			break;
		case ':':
			usleep(TPS*6);
			if (kbhitGetchar() == 'c')
				printf("\033[2J\033[0;0H");
			break;
		default:
			break;
		}
		input == -1 && (input = 'Q');
	}
}

static void help()
{
	printf("Usage:\n"
	       "    tetris [OPTIONS]\n"
	       "    socket -h\n"
	       "Option:\n"
	       "    -i        Hide some of the info below the main ui\n"
	       "    -s <SEED> Set a random number seed\n"
	       "    -W <WIDE> Set the weight\n"
	       "    -H <HIGH> Set the height\n"
	       "    -h        Show this page\n"
	    );
	return;
}

int main(int argc, char *argv[])
{
	pthread_t pid;
	flag_seed = time(NULL);

	int ch = 0;
	while ((ch = getopt(argc, argv, "his:H:W:")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			help();
			return ch == '?' ? -1 : 0;
			break;
		case 'i':
			flag_info = 0;
			break;
		case 's':
			flag_seed = strtod(optarg, NULL);
			break;
		case 'H':
			map.height = strtod(optarg, NULL);
			map.height = map.height >= 10 ? map.height : 25;
			break;
		case 'W':
			map.weight = strtod(optarg, NULL);
			map.weight = map.weight >= 4 ? map.weight : 10;
			break;
		default:
			break;
		}
	}

	srand(flag_seed);
	map.map = calloc(map.size = map.weight * map.height, sizeof(int));
	memset(map.map, 0, map.size);

	if (get_winsize_col() < 66) flag_info = 0;

	time(&game_time1);
	pthread_create(&pid, NULL, print_ui, NULL);
	run();

	print_lock = 0;
	usleep(TPS * 2);
	free(map.map);
	return 0;
}

