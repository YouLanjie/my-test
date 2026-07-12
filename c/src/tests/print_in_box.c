/*
 *   Copyright (C) 2024 u0_a99
 *
 *   文件名称：print_in_box.c
 *   创 建 者：u0_a99
 *   创建日期：2024年03月09日
 *   描    述：
 *
 */


#include "../../include/tools.h"

#define window_num 3

static char *read_entire_file(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (!fp) return NULL;
	fseek(fp, 0L, SEEK_END);
	size_t size = ftell(fp);
	if (size == (size_t)-1) {
		fclose(fp);
		return NULL;
	}
	fseek(fp, 0L, SEEK_SET);
	char *content = malloc(size+1);
	if (!content) {
		fclose(fp);
		return NULL;
	}
	if (fread(content, size, 1, fp) != size) {
		printf("文件长%ld与实际读取大小不一致\n", size);
	}
	fclose(fp);
	return content;
}

#define sprintfcat(dest, fmt, ...) \
	sprintf(dest+strlen(dest), fmt __VA_OPT__(,) __VA_ARGS__);

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	char *str[window_num] = {
		"##########################\n"
		"#                        #\n"
		"#       Upos......       #\n"
		"#                        #\n"
		"##########################\n"
		"Something gose wrong.\n"
		"The input file was not found. :(\n"
		"Try to write somethings in the file\n"
		"to change the text in this window. :)\n"
		" > `"__FILE__"`\n",

		"Tips: [WASD] resize the window [wasd] move the window\n"
		"Tips:  [Tab] switch window        [i] hide info win\n"
		"Tips:   [jk] move focus line     [hl] move the text",

		/*"There is the data window."*/
		malloc(sizeof(char)*1024)
	};

	char refresh_key[UINT8_MAX] = "";
	for (char *p = "wasdWA"; p && *p; p++) {
		refresh_key[(int)*p] = 1;
	}

	char *color[window_num][2] = {
		{"\033[0;30;43m", "\033[0;37;41m"},
		{"\033[0;37;44m", NULL},
		{"\033[0;30;42m", "\033[0;30;47m"},
	};
	str_window_t *win = NULL;
	str_window_t windows[window_num] = {
		/* 文本窗口 */
		(str_window_t){
			.x = 2,
			.y = 5,
			.width = 51,
			.heigh = 15,
			.focus = 1,
		},
		/* 信息提示窗口 */
		(str_window_t){
			.x=1,
			.y=1,
			.width = 53,
			.heigh = 3,
			.focus = 0,
		},
		/* 数值显示窗口 */
		(str_window_t){
			.x=34,
			.y=5,
			.width = 19,
			.heigh = window_num+2,
			.focus = 0,
		},
	};
	int flag_color[window_num] = {1, 1, 1};
	int flag_window = 1;
	int flag_info = 0;
	int inp = 0;

	const char *filename = __FILE__;
	if (argc == 2) filename = argv[1];
	char *content = read_entire_file(filename);
	if (content) str[0] = content;
	else flag_color[0] = 2;

	printf("\033[?25l\e[2J");
	while (inp != 'q') {
		sprintf(str[2], " X  Y WD HI FO HD C\n");
		for (int i = 0; i < window_num; i++)
			sprintfcat(str[2], "%2d %2d %2d %2d %2d %2d %1d\n",
				   windows[i].x,
				   windows[i].y,
				   windows[i].width,
				   windows[i].heigh,
				   windows[i].focus,
				   windows[i].hide,
				   flag_color[i]);
		sprintfcat(str[2], "focus window:%d\n", flag_window);
		printf("\e[0;0H");
		for (int i = 0; i < window_num - flag_info; i++) {
			windows[i].color_code = color[i][flag_color[i] - 1];
			print_in_box(windows[i], str[i]);
		}
		inp = _getch();
		printf("\033[?25l\033[0;0H");
		if (inp != 'q' && refresh_key[inp%UINT8_MAX]) {
			/* 清屏 */
			windows[flag_window - 1].color_code = "\e[0m";
			print_in_box(windows[flag_window - 1], "");
		}
		win = windows + flag_window - 1;
		switch (inp) {
		case 'w': win->y--; break;
		case 's': win->y++; break;
		case 'a': win->x--; break;
		case 'd': win->x++; break;
   
		case 'W': win->heigh--; break;
		case 'S': win->heigh++; break;
		case 'A': win->width--; break;
		case 'D': win->width++; break;

		case 'k': win->focus--; break;
		case 'j': win->focus++; break;
		case 'h': win->hide--; break;
		case 'l': win->hide++; break;
		case 'i': flag_info ^= 1; break;
		case 'c': (flag_color[flag_window - 1] = 3 - flag_color[flag_window - 1]); break;
		case '\t': (flag_window = flag_window < window_num ? flag_window + 1 : 1); break;
		}
	}
	if (content) free(content);
	printf("\033[?25h");

	const int row = get_winsize_row();
	int max_y = 0;
	for (int i = 0; i < window_num; i++) {
		win = windows+i;
		win->y = win->y > row ? 1 : win->y;
		if (win->y < 1) win->y = 1;
		win->heigh = win->heigh + win->y > row ? row - win->y + 1 : win->heigh;
		if (win->heigh < 0) win->heigh = row - win->y + 1;

		if (win->y + win->heigh > max_y) max_y = win->y + win->heigh;
	}
	printf("\033[%d;%dH\n\n", max_y-1, 0);
	return 0;
}

