/*
 * File: 256color.c
 * User: Chglish
 * Describe: 256色、真彩色输出测试文件
 */
#include <stdio.h>

int main()
{
        int color_code = 0;
        /* 在这里将各段控制字符分开是为了方便理解，不是必须的 */
	for (color_code = 0; color_code < 256; color_code++) {
		if (color_code >= 16 && (color_code - 16) % 6 == 0) {
			printf("\n");
		}
		printf("\033["
		       "48;"
		       "5;"
		       "%dm"
		       "%03d"
		       "\033[0m ",
		       color_code, color_code);
	}
	printf("\n");
        return 0;
}
