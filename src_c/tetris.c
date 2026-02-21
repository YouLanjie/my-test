/*
 *   Copyright (C) 2024 u0_a221
 *
 *   文件名称：tetris.c
 *   创 建 者：u0_a221
 *   创建日期：2024年02月16日
 *   描    述：一个俄罗斯方块游戏
 *
 */


#include "tools.h"
#include <stdint.h>

/* 特定type的方块(4*4)内第i位的状态值 */
#define Stat ((shapes[map.type]) >> (15 - i) & 1)
#define Stat2(type) ((shapes[type]) >> (15 - i) & 1)
/* i为顺序号，(x,y)为地图坐标 */
#define Map_for() for (int i = 0, x = 0, y = 0; x = map.x + i % 4, y = map.y + i / 4 % 4, i < 16; i++)
#define mapp map.map[y * map_info.weight + x]
/* 判断出界条件 */
#define Cond_out (x < 0 || x >= map_info.weight || y >= map_info.height || \
		  (map.map[y * map_info.weight + x] != 0 && map.map[y * map_info.weight + x] != 8))
/* 保存map */
#define SaveChunk if (file_save) { fwrite(&map, 1, sizeof(map), file_save);\
				fwrite(map.map, sizeof(int8_t), map_info.size, file_save); }

#define SECOND 1000000
#define TPS (SECOND / 20)

#define Shape_max 9
#define Print_NextSp_ID (map_info.height > 4*5 ? map_info.height / 5 : 4)

struct Map {
	int16_t x;
	int16_t y;
	int16_t type;
	int16_t line;    /* 消行数   */
	int16_t score;   /* 得分 */
	int32_t f;       /* 当前帧数(farme) */
	int8_t  list[7];
	int8_t  list_num;
	int8_t *map;
} map = {
	.x      = 3,
	.y      = 0,
	.type   = 0,
	.line   = 0,
	.score  = 0,
	.f      = 0,
	.map    = NULL,
	.list   = {0, 0, 0, 0, 0, 0, 0},
	.list_num = 0,
};

struct MapInfo {
	int64_t seed;
	int16_t weight;
	int16_t height;
	int16_t size;
	int16_t time;    /* 持续时间 */
	int16_t count_rand;
	int8_t  len_header;
	int8_t  len_farme;
} map_info = {
	.weight = 10,
	.height = 20,
	.size   = 0,
	.time   = 0,
	.count_rand = 0,
	.len_header = sizeof(struct MapInfo),
	.len_farme  = sizeof(struct Map),
};

const uint16_t shapes[Shape_max*4] = {
	0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
	0b1100110000000000, 0b1100110000000000, 0b1100110000000000, 0b1100110000000000,	// #
	0b0000111100000000, 0b0100010001000100, 0b0000111100000000, 0b0100010001000100,	// I
	0b0110110000000000, 0b0100011000100000, 0b0000011011000000, 0b1000110001000000,	// S
	0b1100011000000000, 0b0010011001000000, 0b0000110001100000, 0b0100110010000000,	// Z
	0b0100010001100000, 0b0000111010000000, 0b1100010001000000, 0b0010111000000000,	// L
	0b0010001001100000, 0b0100011100000000, 0b0011001000100000, 0b0000011100010000,	// J
	0b0100111000000000, 0b0100011001000000, 0b0000111001000000, 0b0100110001000000,	// T
	0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
};
const char *print_color[] = {
	"30",
	"31;41",
	"32;42",
	"33;43",
	"34;44",
	"35;45",
	"36;46",
	"37;47",
	"2",
};
#define FLG_FAKE        (1<<0)	/* 设置下落最终位置显示 */
#define FLG_MOVE_LOCK   (1<<1)	/* 防止两个进程同时移动 */
#define FLG_DEBUG       (1<<2)	/* 调试设置 */
#define FLG_FARME       (1<<3)
#define FLG_LOAD        (1<<4)
#define FLG_SKIP        (1<<5)
#define FLG_IGNORE_SIZE (1<<6)
#define FLG_MACHINE     (1<<7)
#define INFO_TIPS       (1<<8)
#define INFO_SEED       (1<<9)
#define INFO_NEXT       (1<<10)
#define INFO_TIME       (1<<11)
uint16_t flags = FLG_FAKE;

int flag_tps = 100;
uint32_t print_lock = 1;	/* 控制进程、自动下落时间(farme) */
FILE *file_save = NULL;
FILE *file_load = NULL;
time_t game_time1 = 0, game_time2 = 0;

static int my_rand()
{
	map_info.count_rand+=1;
	return rand();
}

/* 创建等候列表 */
static void create_list()
{
	int times = map.list_num;
	int li[] = {1, 2, 3, 4, 5, 6, 7},
	    num = 0;
	int type  = 0,
	    shape = 0;


	map.list[times] = 0;
	times = 6;
	for (int i = 6; i >= 0 && map.list[i] != 0; times = i - 1, i--);
	for (int i = 0; i <= times; i++)
		if (map.list[i]) li[map.list[i] / 4 - 1] = 0;
	for (int i = 0; i <= times; i++) {
		if (map.list[i]) continue;
		do {
			num = my_rand() % (Shape_max - 2);
		} while (li[num] == 0);
		type = li[num];
		li[num] = 0;

		shape = my_rand() % 4;

		map.list[i] = type * 4 + shape;
	}

	times++;
	if (times > 6)
		times = 0;
	map.list_num = times;
}

/* 移除旧有方块 */
static int delete()
{
	for (int i = 0, x = 0, y = 0; x = map.x + i % 4, y = map.y + i / 4 % 4, i < 16; i++) {
		if ((shapes[map.type]) >> (15 - i) & 1) {
			map.map[y * map_info.weight + x] = 0;
		}
	}
	/*Map_for() Stat ? mapp = 0 : 0;*/
	return 0;
}

/* 创建方块 */
static int create(int x, int y, int type)
{
	type = abs(type);

	if (type > (Shape_max - 1) * 4) {
		create_list();
		type = map.list[map.list_num];
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

extern int delete_fake()
{
	for (int i = 0; i < map_info.size; i++) {
		if (map.map[i] == 8)
			map.map[i] = 0;
	}
	return 0;
}

extern int create_fake()
{
	int x1 = map.x, y1 = map.y;
	int flag = 0;

	if (~flags&FLG_FAKE) return 0;
	delete();
	delete_fake();
	while (! flag) {
		map.y += 1;
		for (int i = 0, x = 0, y = 0; x = map.x + i % 4, y = map.y + i / 4 % 4, i < 16; i++) {
			if (!map.f) printf("nmlgbd\n");
			if ((shapes[map.type]) >> (15 - i) & 1) {
				if ((x < 0 || x >= map_info.weight || y >= map_info.height || \
				     (map.map[y * map_info.weight + x] != 0 && map.map[y * map_info.weight + x] != 8))) {
					flag = 1;
				}
			}
		}
		/*Map_for() (Stat) && Cond_out && (flag = 1);*/
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
	for (y = map_info.height - 1; y >= 0 ; y--) {
		for (x = map_info.weight - 1; x >= 0 && mapp; x--);
		if (x >= 0) continue;
		for (; y >= 0 ; y--) {
			for (x = map_info.weight - 1; x >= 0; x--) {
				if (y > 0) mapp = *(&mapp - map_info.weight);
				else mapp = 0;
			}
		}
		map.line += 1;
		y = map_info.height;
	}
	map.score += (map.line - line) * (map.line - line);
	return 0;
}

/* RET CODE: 0:nothing, 1:failed, -1:end_of_game */
static int _move(int16_t *v, int step)
{
	int flag = 0;

	if (flags&FLG_MOVE_LOCK)
		return 0;
	flags |= FLG_MOVE_LOCK;
	delete();
	*v += step;
	Map_for() (Stat) && Cond_out && (flag = 1);
	if (flag) *v -= step;
	create(map.x, map.y, map.type);
	if (flag && v == &map.y) {
		clean();
		flag = create(map_info.weight / 2 - 2, 0, Shape_max * 4) == 0 ? flag : -1;
	}
	flags &= ~FLG_MOVE_LOCK;
	if (!flag) SaveChunk;
	return flag;
}

static void print_block(int shape, int margin_left, int is_current)
{
	char *ch[2] = {"", ";2"};
	for (int i = 0; i < 16; i++) {
		if (i % 4 == 0) printf("\033[%dC| ", margin_left);
		printf("\033[%s%sm[]\033[0m", print_color[Stat2(shape) ? shape / 4 : 8], ch[is_current]);
		if ((i + 1) % 4 == 0) printf("\r\n");
	}
	printf("\r\n");
	return;
}

static void print_next()
{
	int seperate_id = Print_NextSp_ID;
	int i = 0;
	for (i = 0; i <= 6; i++) {
		int shape = map.list[((flags&FLG_DEBUG ? 0 : map.list_num) + i) % 7];
		int is_current = flags&FLG_DEBUG ? i == map.list_num : 0;
		if (i < seperate_id) print_block(shape, map_info.weight * 2, is_current);
		else {
			if (i == seperate_id) printf("\033[%dA", seperate_id*5);
			print_block(shape, map_info.weight * 2 + 2 + 4*2 + 1, is_current);
		}
	}
	printf("\033[%dA", (i > seperate_id ? 7-seperate_id : i)*5);
	return;
}

static void print_map()
{
	for (int i = 0; i < map_info.size; i++) {
		printf("\033[%sm", print_color[map.map[i]]);
		if (flags&FLG_DEBUG) printf("\033[37m%02d\033[0m", map.map[i]);
		else printf("[]\033[0m");
		if ((i + 1) % map_info.weight == 0) printf("|\r\n");
	}
	for (int i = 0; i < map_info.weight; i++) printf("--");
	printf("-\r\n");
	return;
}

int refit_screen() {
	flags |= INFO_NEXT|INFO_SEED|INFO_TIME|INFO_TIPS;
	if (flags&FLG_IGNORE_SIZE) return 0;
	int cond_next_w = map_info.weight*2+2+4*2+(4*2+3)*(Print_NextSp_ID < 7);
	/* 宽度 */
	if (get_winsize_col() < 66) flags &= ~INFO_TIPS;
	if (get_winsize_col() < cond_next_w) flags &= ~INFO_NEXT;
	if (get_winsize_col() < 43) flags &= ~INFO_SEED;
	if (get_winsize_col() < 29) flags &= ~INFO_TIME;
	/* 高度 */
	if (map_info.height < 19) flags &= ~INFO_NEXT;
	if (get_winsize_row() < map_info.height+5) flags &= ~INFO_TIPS;
	if (get_winsize_col() < map_info.weight*2+1 || get_winsize_row() < map_info.height+2) {
		printf("WARN:\n"
		       "要求最小'宽x高':%dx%d\n"
		       "目前终端大小为：%dx%d\n",
		       map_info.weight*2+2, map_info.height+2,
		       get_winsize_col(), get_winsize_row());
		return -1;
	}
	return 0;
}

extern int control(int input)
{
	int table[256] = {['A'] = 'W', ['B'] = 'S', ['C'] = 'D', ['D'] = 'A'};
	input = (input >= 'a' && input <= 'z') ? input - 32 : input;
	if (input == 0x1B && kbhit()) {
		input = getchar();
		input = getchar();
		input = table[input];
	}
	switch (input) {
	case 'J':
		input = _move(&map.type, map.type % 4 > 0 ? -1 : 3);
		break;
	case 'K':
	case 'W':
		input = _move(&map.type, map.type % 4 < 3 ? 1 : -3);
		break;
	case 'H':
	case 'A':
		input = _move(&map.x, -1);
		break;
	case 'S':
		input = _move(&map.y, 1);
		break;
	case 'L':
	case 'D':
		input = _move(&map.x, 1);
		break;
	case ' ':
		input = 0;
		while (input == 0)
			input = _move(&map.y, 1);
		break;
	case 'F':
		flags ^= FLG_FAKE;
		break;
	case 'B':
		flags ^= FLG_DEBUG;
		break;
	case 'I':
		flags ^= INFO_TIPS;
		break;
	default:
		break;
	}
	return input;
}

extern char *get_map_info()
{
	char header[100] = "";
	time(&game_time2);
	int dtime = difftime(game_time2, game_time1);
	sprintf(header, "%d,%d,%d,%d, ",map.line, map.score, map.f, dtime);
	for (int i = 0; i <= 6; i++) {
		char temp[5] = "";
		int ind = map.list_num + i > 6 ? map.list_num + i - 7 : map.list_num + i;
		sprintf(temp, "%d,", map.list[ind]);
		strcat(header, temp);
	}
	strcat(header, " ");

	char map_str[map_info.size * 3];
	memset(map_str, 0, map_info.size*3);
	for (int i = 0; i < map_info.size; i++) {
		char temp[5] = "";
		sprintf(temp, "%d,", map.map[i]);
		strcat(map_str, temp);
	}

	static char *result = NULL;
	if (result) free(result);
	int size = strlen(header) + strlen(map_str) + 3;
	result = malloc(size*sizeof(char));
	strcpy(result, header);
	strcat(result, map_str);
	return result;
}

extern void print_ui_machine_code()
{
	// info
	time(&game_time2);
	int dtime = difftime(game_time2, game_time1);
	printf("%d,%d,%d,%d, ",map.line, map.score, map.f, dtime);
	// next:
	for (int i = 0; i <= 6; i++)
		printf("%d,", map.list[map.list_num + i > 6 ? map.list_num + i - 7 : map.list_num + i]);
	printf(" ");
	// map
	for (int i = 0; i < map_info.size; i++)
		printf("%d,", map.map[i]);
	printf("\n");
	fflush(stdout);
	return;
}

extern void print_ui_human(int *info)
{
	time(&game_time2);
	print_map();
	if (info && (*info ^ flags)&INFO_TIPS) {
		printf("                                                               \r\n\r\n");
		printf("                                                          \r\n"
			"                                                                \r");
		fflush(stdout);
		printf("\033[%dA", 3);
		fflush(stdout);
		*info = flags;
	}
	int dtime = difftime(game_time2, game_time1);
	printf(flags&INFO_TIPS ? "消行:%3d /分数:%3d": "Ln:%3d /Sc:%3d", map.line, map.score);
	if (flags&INFO_TIME)
		printf(flags&INFO_TIPS ? " /时间:(%02d:%02d:%02d)" : "  /T:(%02d:%02d:%02d)",
		       dtime / 3600, dtime % 3600 / 60, dtime % 60);
	if (flags&INFO_SEED) printf(flags&INFO_TIPS ? " /种子:%ld" : " /Sd:%ld", map_info.seed);
	if (flags&FLG_FARME) printf(flags&INFO_TIPS ? " /帧值:%d" : " /F:%d", map.f);
	static int flag_farme = 0;
	if (flags ^ flag_farme) {
		printf("             ");
		flag_farme = flags & FLG_FARME;
	}
	printf("\r");
	if (flags&INFO_TIPS) {
		printf("\n\n\r");
		printf("<游戏> [a/s/d] 移动   [w/j/k] 旋转    [SPACE] 速降\r\n"
		       "<设置>     [b] Debug      [i] 按键提示    [f] 预期落点显示\r"
		       "\033[2A");
	}
	printf("\033[%dA", map_info.height + (flags&INFO_TIPS ? 2 : 1));
	if (flags&INFO_NEXT) print_next();
	return;
}

static void *print_ui(void *p)
{
	int info = flags;
	int last_farme = map.f;
	map.f = 1;

	if (!(flags&FLG_MACHINE)) printf("\033[?25l");
	while (map.f) {
		last_farme = map.f;
		if (~flags&(FLG_LOAD|FLG_SKIP)) {
			if (flags&FLG_MACHINE) print_ui_machine_code();
			else print_ui_human(&info);
		}

		if (file_load && feof(file_load)) {
			if (file_load) fclose(file_load);
			file_load = NULL;
			flags &= ~FLG_LOAD;
			game_time1 -= map_info.time;
		}
		if (flags&FLG_LOAD && !feof(file_load)) {
			int8_t *pmap = map.map;
			fread(&map, 1, sizeof(map), file_load);
			map.map = pmap;
			fread(map.map, sizeof(int8_t), map_info.size, file_load);
			SaveChunk;
		}
		for (;~flags&(FLG_LOAD|FLG_SKIP) && last_farme <= map.f; last_farme++)
			usleep(TPS/(flags&FLG_LOAD ? flag_tps : 1));    /* 每0.05s就刷新一次 */
		if (flags&FLG_LOAD) continue;

		if (map.f && map.f % 6 == 0) {    /* 默认每0.3s就移动一次 */
			int inp = 0;
			inp = _move(&map.y, 1);
			if (inp == -1) {
				map.f = 0;
				ungetc('q', stdin);
			}
		}
		if (map.f) map.f++;
	}

	if (!(flags&FLG_MACHINE)) {
		printf("\033[%dB\r\n", map_info.height + (flags&INFO_TIPS ? 4 : 1));
		printf("\033[?25h游戏结束\r\n若程序未退出，请按'回车'\r\n");
		if (flags & FLG_DEBUG) {
			printf("最终棋盘数据:\r\n");
			print_map();
		}
	}
	pthread_exit(NULL);
	return NULL;
}

/* 处理用户输入，主线程 */
static void get_input_and_process()
{
	int input = 0;
	while(input != 'Q' && map.f) {
		if (~flags & FLG_LOAD) {
			create_fake();
			input = _getch_cond((int*)&map.f);
		} else {
			while (flags&FLG_LOAD && !kbhit()) usleep(TPS);
			flags &= ~FLG_LOAD;
			if (file_load) fclose(file_load);
			file_load = NULL;
			continue;
		}
		if (flags & FLG_FAKE) delete_fake();
		if (input == ':') {
			usleep(TPS*8);
			int inp = kbhitGetchar();
			if (inp == 'c')
				printf("\033[2J\033[0;0H");
			else if (inp == 'f') {
				flags ^= FLG_FARME;
			} else if (inp == 'n') {
				flags ^= INFO_NEXT;
			} else if (inp == 'r') {
				refit_screen();
			}
		}
		input = control(input);
		input == -1 && (input = 'Q');
		/*if (flag_machine) print_ui_machine_code();*/
	}
}

static void help(int type)
{
	char *help[] = {
		"Usage:\n"
		"    tetris [OPTIONS]\n"
		"Option:\n"
		"    -i        简化主ui的信息显示\n"
		"    -I        忽略终端大小不自动调整显示等级\n"
		"    -n        隐藏下一步的信息显示\n"
		"    -s <SEED> 设置随机数种子\n"
		"    -W <WIDE> 设置棋盘宽度\n"
		"    -H <HIGH> 设置棋盘高度\n"
		"    -o <FILE> 保存历史记录的文件\n"
		"    -l <FILE> 加载指定的历史记录文件\n"
		"    -d <NUM>  设置回放速度(值越高速度越快)\n"
		"    -k        跳过回放显示直接加载存档\n"
		"    -M        打印机器友好信息\n"
		"    -K        内部按键说明\n"
		"    -h        显示帮助\n",
		"内部按键说明：\n"
		",------------+----+----------------------------------.\n"
		"|            | a  | 左移                             |\n"
		"|            | s  | 下移                             |\n"
		"| 移动类     | d  | 右移                             |\n"
		"|            |    | 方向键                           |\n"
		"|            |    | 速降（空格）                     |\n"
		"|------------+----+----------------------------------|\n"
		"|            | w  | 右旋                             |\n"
		"| 旋转类     | j  | 左旋                             |\n"
		"|            | k  | 右旋                             |\n"
		"|------------+----+----------------------------------|\n"
		"|            | i  | 内置按键提示显示切换             |\n"
		"|            | f  | 预期速降落点显示切换             |\n"
		"| 显示信息类 | b  | 以调试方式打印地图               |\n"
		"|            | :n | 隐藏下一步和种子显示（连按）     |\n"
		"|            | :f | 显示当前运行帧数(调试用)（连按） |\n"
		"|            | :c | 清屏（连按）                     |\n"
		"`------------+----+----------------------------------'\n"
	};
	printf("%s", help[type]);
	return;
}

extern int init_game(int flag_has_seed)
{
	time(&game_time1);
	time(&game_time2);
	if (!flag_has_seed) map_info.seed = time(NULL);
	if (file_load) fread(&map_info, sizeof(map_info), 1, file_load);
	if (map_info.len_header != sizeof(struct MapInfo) ||
	    map_info.len_farme  != sizeof(struct Map)) {
		fprintf(stderr, "Unsupport save format, give up\n");
		map_info = (struct MapInfo){
			.weight = 10,
			.height = 20,
			.size   = 0,
			.time   = 0,
			.len_header = sizeof(struct MapInfo),
			.len_farme  = sizeof(struct Map),
		};
		flags &= ~(FLG_LOAD);
		if (file_load) fclose(file_load);
		file_load = NULL;
	}
	map_info.weight = map_info.weight >= 4 ? map_info.weight : 10;
	map_info.height = map_info.height >= 10 ? map_info.height : 25;
	srand(map_info.seed);

	for (int ret = refit_screen(); ret < 0;) return ret;
	map.map = calloc(map_info.size = map_info.weight * map_info.height, sizeof(int8_t));
	memset(map.map, 0, map_info.size);

	map.f = 1;
	if (file_load) {
		for (int i = 0; i < map_info.count_rand; ++i) {
			rand();
		}
	} else create(map_info.weight / 2 - 2, 0, Shape_max * 4);
	if (file_save) fwrite(&map_info, sizeof(map_info), 1, file_save);
	return 0;
}

extern int end_game()
{
	map.f = 0;
	usleep(TPS * 2);
	free(map.map);
	map.map = NULL;
	if (file_save) {
		fseek(file_save, 0L, SEEK_SET);
		map_info.time = difftime(game_time2, game_time1);
		fwrite(&map_info, sizeof(map_info), 1, file_save);
		fclose(file_save);
	}
	if (file_load) fclose(file_load);
	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t pid;
	int flag_has_seed = 0;
	int ch = 0;
	while ((ch = getopt(argc, argv, "hiIns:W:H:o:l:d:kKMx")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			help(0);
			return ch == '?' ? -1 : 0;
			break;
		case 'i':
			flags ^= INFO_TIPS;
			break;
		case 'I':
			flags ^= FLG_IGNORE_SIZE;
			break;
		case 'n':
			flags ^= INFO_NEXT;
			break;
		case 's':
			flag_has_seed = 1;
			map_info.seed = strtod(optarg, NULL);
			break;
		case 'W':
			map_info.weight = strtod(optarg, NULL);
			break;
		case 'H':
			map_info.height = strtod(optarg, NULL);
			break;
		case 'o':
			file_save = fopen(optarg, "w");
			break;
		case 'l':
			file_load = fopen(optarg, "r");
			flags |= file_load ? FLG_LOAD : 0;
			if (!file_load) printf("NOTE: 存档加载失败\n");
			break;
		case 'd':
			flag_tps = strtod(optarg, NULL);
			if (flag_tps <= 0) flag_tps = 1;
			break;
		case 'k':
			flags ^= FLG_SKIP;
			break;
		case 'K':
			help(1);
			return 0;
			break;
		case 'M':
			flags ^= FLG_MACHINE;
			break;
		case 'x':
			flags ^= FLG_DEBUG;
			break;
		default:
			break;
		}
	}

	if (init_game(flag_has_seed) < 0) return -1;
	pthread_create(&pid, NULL, print_ui, NULL);
	get_input_and_process();
	end_game();
	return 0;
}

