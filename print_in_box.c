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

static int print(char *ch, int x_start, int y_start, int width, int heigh, int hide, int focus)
{
	int count = 0,
	    line_num = 0;
	char buf[5] = "0";

	x_start = x_start > get_winsize_col() ? 0 : x_start;
	x_start < 0 && (x_start = 0);
	y_start = y_start > get_winsize_row() ? 0 : y_start;
	y_start < 0 && (y_start = 0);

	width = width + x_start >= get_winsize_col() ? get_winsize_col() - x_start : width;
	width < 0 && (width = 0);
	heigh = heigh + y_start >= get_winsize_row() ? get_winsize_row() - y_start : heigh;
	heigh < 0 && (heigh = 0);

	printf("\033[0;37;44m");
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
			cond_print && printf("\033[%d;%dH", y_start + line_num - (hide > line_num ? 0 : hide), x_start);
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
		} else if (*ch == '\t')
			count += 8;
		else
			count++;

		cond_print && printf("%s%s%s", line_num == focus ? "\033[7m" : "",
			buf, line_num == focus ? "\033[0;37;44m" : "");
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
	if (!fp)
		return -1;
	char str[1024*10] = "0";
	int  ch = 0;
	for (int i = 0; ch != EOF; i++) {
		ch = fgetc(fp);
		str[i] = ch;
	}
	fclose(fp);
	printf("\033[?25l");

	int inp = 0;
	int x = 10, y = 1, wd = 50, hi = 31;
	int fy = 0, hy = 0;
	while (inp != 'q') {
		printf("\033[2J\033[0;0H");
		print(str, x, y, wd, hi, hy, fy);
		inp = _getch();
		inp ^ 'w' || y--;
		inp ^ 's' || y++;
		inp ^ 'a' || x--;
		inp ^ 'd' || x++;

		inp ^ 'W' || hi--;
		inp ^ 'S' || hi++;
		inp ^ 'A' || wd--;
		inp ^ 'D' || wd++;

		inp ^ 'k' || fy--;
		inp ^ 'j' || fy++;
		inp ^ 'h' || hy--;
		inp ^ 'l' || hy++;
	}
	printf("\033[?25h");
	printf("\033[%d;%dH\n", get_winsize_row(), 0);
	return 0;
}

