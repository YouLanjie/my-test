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

int main(int argc, char *argv[])
{
	char *filename = "./print_in_box.c";
	if (argc == 2) {
		filename = argv[1];
	}
	FILE *fp = fopen(filename, "r");
	char *str[window_num] = {
		"##########################\n"
		"#                        #\n"
		"#       Upos......       #\n"
		"#                        #\n"
		"##########################\n"
		"Something gose wrong.\n"
		"The input file was not found. :(\n"
		"Try to write somethings in the file `./print_in_box.c` to change the text in this window. :)",

		"Tips: [WASD] resize the window [wasd] move the window\n"
		"Tips:  [Tab] switch window        [i] hide info win\n"
		"Tips:   [jk] move focus line     [hl] move the text",

		/*"There is the data window."*/
		malloc(sizeof(char)*1024)
	};

	char *color[window_num][2] = {
		{"\033[0;30;43m", "\033[0;37;41m"},
		{"\033[0;37;44m", NULL},
		{"\033[0;30;42m", "\033[0;30;47m"},
	};
	int flag_color[window_num] = {1, 1, 1};
	int flag_window = 1;
	int flag_info = 0;
	int inp = 0;
	int x[window_num] = {2, 1, 34},    /* number about window */
	    y[window_num] = {5, 1, 5},
	    wd[window_num] = {51, 53, 19},
	    hi[window_num] = {15, 3, window_num + 2};
	int fy[window_num] = {2, 1, 1},    /* number about control */
	    hy[window_num] = {0, 0, 0};

	printf("\033[?25l");

	if (fp) {
		str[0] = _fread(fp);
		fclose(fp);
	} else flag_color[0] = 2;

	printf("\033[2J");
	while (inp != 'q') {
		sprintf(str[2], " X  Y WD HI FY HY C\n");
		for (int i = 0; i < window_num; i++)
			sprintf(str[2], "%s%2d %2d %2d %2d %2d %2d %1d\n", str[2], x[i], y[i], wd[i], hi[i], hy[i], fy[i], flag_color[i]);
		sprintf(str[2], "%sfocus window:%d\n", str[2], flag_window);
		printf("\033[0;0H");
		for (int i = 0; i < window_num - flag_info; i++)
			print_in_box(str[i], x[i], y[i], wd[i], hi[i], hy[i], fy[i], color[i][flag_color[i] - 1], 1);
		inp = _getch();
		printf("\033[0;0H");
		for (int i = 0; i < window_num - flag_info && inp != 'q'; i++) {
			print_in_box("", x[i], y[i], wd[i], 1, 0, 0, "\e[0m", 0);
			print_in_box("", x[i], y[i], 1, hi[i], 0, 0, "\e[0m", 0);
			print_in_box("", x[i]+wd[i]-1, y[i], 1, hi[i], 0, 0, "\e[0m", 0);
			print_in_box("", x[i], y[i]+hi[i]-1, wd[i], 1, 0, 0, "\e[0m", 0);
		}
		if (inp == 'w') y[flag_window - 1]--;
		if (inp == 's') y[flag_window - 1]++;
		if (inp == 'a') x[flag_window - 1]--;
		if (inp == 'd') x[flag_window - 1]++;
   
		if (inp == 'W') hi[flag_window - 1]--;
		if (inp == 'S') hi[flag_window - 1]++;
		if (inp == 'A') wd[flag_window - 1]--;
		if (inp == 'D') wd[flag_window - 1]++;

		if (inp == 'k') fy[flag_window - 1]--;
		if (inp == 'j') fy[flag_window - 1]++;
		if (inp == 'h') hy[flag_window - 1]--;
		if (inp == 'l') hy[flag_window - 1]++;
		if (inp == 'c') (flag_color[flag_window - 1] = 3 - flag_color[flag_window - 1]);
		if (inp == '\t') (flag_window = flag_window < window_num ? flag_window + 1 : 1);
		if (inp == 'i') flag_info ^= 1;
	}
	printf("\033[?25h");
	printf("\033[%d;%dH\n", get_winsize_row(), 0);
	return 0;
}

