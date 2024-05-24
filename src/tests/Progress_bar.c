#include "../../include/tools.h"


int main() {
	double f = 0,i = 0;

	printf("Please input a number:");
	scanf("%lf",&i);
	printf("\033[?25l");
	getchar();
	printf("进度条:[\033[50C]\n");
	for(double count = 0; count < i ; count++) {
		f = count / i * 100;
		printf("\033[A\033[%dC#\n",(int)(f / 2) + 8);
		printf("\033[A\033[59C%.2lf%%\n",f);
	}
	printf("\033[?25h");
	return 0;
}
