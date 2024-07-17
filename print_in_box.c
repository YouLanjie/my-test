/*
 *   Copyright (C) 2024 u0_a99
 *
 *   文件名称：print_in_box.c
 *   创 建 者：u0_a99
 *   创建日期：2024年03月09日
 *   描    述：
 *
 */


#include "include/tools.h"

static int print_in_box(char *ch, int x_start, int y_start, int width, int heigh, int hide, int focus, char *color_code, int flag_hl)
{
	int count = 0,
	    line_num = 0;
	char buf[20] = "0";

	x_start = x_start > get_winsize_col() ? 1 : x_start;
	if (x_start < 1) x_start = 1;
	y_start = y_start > get_winsize_row() ? 1 : y_start;
	if (y_start < 1) y_start = 1;

	width = width + x_start > get_winsize_col() ? get_winsize_col() - x_start + 1 : width;
	if (width < 0) width = 0;
	heigh = heigh + y_start > get_winsize_row() ? get_winsize_row() - y_start + 1 : heigh;
	if (heigh < 0) heigh = 0;

	focus = focus - 1;
	if (!color_code) color_code = "\033[0m";

	printf("%s", color_code);
	for (int i1 = y_start; i1 < y_start + heigh; i1++) {
		for (int i2 = x_start; i2 < x_start + width; i2++) {
			printf("\033[%d;%dH ", i1, i2);
		}
	}
	printf("\033[%d;%dH", y_start + line_num - (hide > line_num ? 0 : hide), x_start);
	while (ch && *ch != '\0') {
		int cond_out = (count + (*ch & 0x80 ? 2 : 1) > width && ch && *ch != '\0');
		int cond_print = (line_num - hide >= 0 && line_num - hide < heigh);

		if (cond_out || *ch == '\n' || *ch == '\r') {
			/* 行数增加 */
			line_num++;
			/* 字符清零 */
			count = 0;
			/* 移动光标 */
			if (cond_print) printf("\033[%d;%dH", y_start + line_num - (hide > line_num ? 0 : hide), x_start);
			/*kbhitGetchar();*/
			/* 字符指针下移 */
			if (*ch == '\n' || *ch == '\r') {
				ch++;
				continue;
			}
		}

		buf[0] = *ch;
		buf[1] = '\0';
		if (*ch & 0x80) {
			buf[1] = ch[1];
			buf[2] = ch[2];
			buf[3] = '\0';
			ch+=2;
			count += 3;
		} else if (*ch == '\t') {
			count += 8;
			strcpy(buf, "        ");
		} else
			count++;

		int cond_hl = (line_num == focus && flag_hl);
		cond_print = (line_num - hide >= 0 && line_num - hide < heigh);
		if (cond_print) printf("%s%s%s", cond_hl ? "\033[7m" : "", buf, cond_hl ? color_code : "");
		/*kbhitGetchar();*/
		/*usleep(100000/10);*/
		/* 字符指针下移 */
		ch++;
	}
	printf("\033[0m");
	kbhitGetchar();
	return 0;
}

int main()
{
	FILE *fp = fopen("./print_in_box.c", "r");
	char str[2][1024*10] = {
		"Something gose wrong.\n"
		"The file `./print_in_box.c` was not found. :(\n"
		"Try to write somethings in the file `./print_in_box.c` to change the text in this window. :)",

		"Tips: [WASD] resize the window [wasd] move the window\n"
		"Tips:  [Tab] switch window\n"
		"Tips:  [jk]  move focus line    [hl]  move the text"};
	if (fp) {
		int  ch = 0;
		for (int i = 0; ch != EOF; i++) {
			ch = fgetc(fp);
			str[0][i] = ch;
		}
		fclose(fp);
	}
	printf("\033[?25l");

	char *color[2][2] = {{"\033[0;37;43m", "\033[0;37;41m"}, {"\033[0;37;44m", NULL}};
	int flag_color[2] = {1, 1};
	int flag_window = 1;
	int inp = 0;
	int x[2] = {2, 1}, y[2] = {5, 1}, wd[2] = {51, 53}, hi[2] = {15, 3};
	int fy[2] = {2, 1}, hy[2] = {0, 0};
	while (inp != 'q') {
		printf("\033[2J\033[0;0H");
		print_in_box(str[0], x[0], y[0], wd[0], hi[0], hy[0], fy[0], color[0][flag_color[0] - 1], 1);
		print_in_box(str[1], x[1], y[1], wd[1], hi[1], hy[1], fy[1], color[1][flag_color[1] - 1], 1);
		inp = _getch();
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
		if (inp == '\t') (flag_window = 3 - flag_window);
	}
	printf("\033[?25h");
	printf("\033[%d;%dH\n", get_winsize_row(), 0);
	return 0;
}

