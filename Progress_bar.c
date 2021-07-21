#include <stdio.h>
#include "headfile/kbhit_input.h"

#define Clear printf("\033[2J\033[1;1H");

int main() {
	int a = 0;
	double f = 0,i = 0;

	printf("\033[?25l");
	Clear
	printf("Please input a number:");
	scanf("%lf",&i);
	getchar();
	Clear
	printf("\033[1;1H进度条:[\033[1;59H]\n");
	for(double count = 0; count < i ; count++) {
		f = count / i * 100;
		printf("\033[1;%dH#\033[1;60H%.2lf%%\n",(int)(f / 2) + 9,f);
	}
	printf("\033[2;1HTest is over.\nInput enter return.\n");
	input();
	printf("\033[?25h");
	Clear
	return 0;
}