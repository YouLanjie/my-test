#include "include/tools.h"

int flag_times = 0;

int print()
{
	printf("\ntext:\n");
	for (int i = 0; i <= 9; i++) {
		printf("\033[%dm%03d\033[0m ", i, i);
	}
	printf("\n");

	printf("mouse:\n");
	//  getch();
	printf("\033[?25l\\033[?25l This is text(No mouse) \033[0m\n");
	//  getch();
	printf("\033[?25h\\033[?25h This is text(Has mouse) \033[0m\n");
	//  getch();


	printf("\ncolor:\n");
	for (int i = 0; i <= flag_times; i++) {
		for (int i2 = 30; i2 <= 37; i2++) {
			printf("\033[%d;%dm%d;%02d\033[0m ", i, i2, i, i2);
		}
		for (int i2 = 40; i2 <= 47; i2++) {
			printf("\033[%d;%dm%d;%02d\033[0m ", i, i2, i, i2);
		}
		printf("\n");

		for (int i2 = 30; i2 <= 37; i2++) {
			for (int i3 = 40; i3 <= 47; i3++) {
				printf("\033[%d;%d;%dm%d;%02d;%02d\033[0m ", i, i2, i3, i, i2, i3);
			}
			printf("\n");
		}
		printf("\n");
	}
	printf("\033[0m");
	return 0;
}

int color256()
{
        int color_code = 0;
	printf("color(256)(\\033[48;5;%%d):\n");
	for (color_code = 0; color_code < 256; color_code++) {
		if (color_code >= 16 && (color_code - 16) % 6 == 0)
			printf("\n");
		printf("\033[48;5;%dm%03d\033[0m ", color_code, color_code);
	}
	printf("\n");
        return 0;
}

int main(int argc, char *argv[])
{
	int ch = 0;
	int flag_256 = 0;
	while ((ch = getopt(argc, argv, "tch")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: Text_effects [-t text+color] [-c 256color]\n");
			return 0;
			break;
		case 't':
			flag_times = 9;
			break;
		case 'c':
			flag_256 = 9;
			break;
		default:
			break;
		}
	}
	print();
	flag_256 && color256();
	return 0;
}

