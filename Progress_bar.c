#include "include/tools.h"

int main() {
	double f = 0,i = 0;

	printf("\033[?25l");
	Clear2;
	printf("Please input a number:");
	scanf("%lf",&i);
	getchar();
	Clear2;
	printf("\033[1;1H进度条:[\033[1;59H]\n");
	for(double count = 0; count < i ; count++) {
		f = count / i * 100;
		printf("\033[1;%dH#\033[1;60H%.2lf%%\n",(int)(f / 2) + 9,f);
	}
	printf("\033[2;1HTest is over.\nInput enter return.\n");
	getch();
	printf("\033[?25h");
	Clear2;
	return 0;
}
