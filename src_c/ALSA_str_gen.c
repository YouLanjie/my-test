/*
 *   Copyright (C) 2025 Chglish
 *
 *   文件名称：ALSA_str_gen.c
 *   创 建 者：Chglish
 *   创建日期：2025年08月28日
 *   描    述：简陋的测试性的用于为ALSA程序生成音符字符串用的程序
 *
 */


#include "include/tools.h"

int main()
{
	int inp = 0;
	int mode = 1;
	int index = 0;
	char str[1024*1024] = {0};
	char info[125] = "";
	char key_table[3][256] = {
		{0, ['0'] = '0', 'c', 'd', 'e', 'f', 'g', 'a', 'b'},
		{0, ['0'] = '0', 'C', 'D', 'E', 'F', 'G', 'A', 'B'},
		{0, ['0'] = '0', '1', '2', '3', '4', '5', '6', '7'},
	};
	char othre_allowed[256] = {
		0, ['.'] = 1, ['/'] = 1, ['*'] = 1, ['+'] = 1, ['-'] = 1,
		['~'] = 1, ['S'] = 's',
	};
	printf("type your music below\n----------------------------------\n");
	/*printf("\e[7");*/
	printf("\n\n\n\n\n\n");
	while (inp != 'Q') {
		sprintf(info, "MODE:%d (press 'l' 'm' 'h' to switch)", mode);
		print_in_box(info, 0, get_winsize_row()-6, -1, 1, 0, 0, NULL, 1);
		print_in_box(str, 0, get_winsize_row()-5, -1, 5, 0, 0, NULL, 1);
		inp = _getch();
		inp = (inp >= 'a' && inp <= 'z') ? inp - 32 : inp;
		if (inp == 'L') mode = 0;
		else if (inp == 'M') mode = 1;
		else if (inp == 'H') mode = 2;
		else if (othre_allowed[(int)inp]) {
			str[index] = othre_allowed[(int)inp] == 1 ? inp : othre_allowed[(int)inp];
			index++;
		} else if (inp >= '0' && inp <= '7') {
			str[index] = ' ';
			str[index+1] = key_table[mode][(int)inp];
			index +=2 ;
		} else if (inp == 127 && index >= 0) {
			index -- ;
			str[index] = 0;
		}
	}
	printf("\n");
	return 0;
}


