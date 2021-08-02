#include "include/include.h"

enum unit{Exit = 48, Bit, KB, MB, GB};

int main() {
	FILE *fp;
	char filename[30];
	int a = 1,b,c = 1;
	double bit;
	double count;
	double f = 0;

	printf("\033[?25l");
	while(1) {
		bit = 0;
		count = 0;
		c = 1;
		do {
			system("clear");
			printf("请选择单位：\n0:Exit\n1:Bit\n2:KB\n3:MB\n4:GB\n");
			b = input();
			system("clear");
			switch (b) {
				case Exit:
					system("clear");
					printf("\033[?25h");
					return 0;
					break;
				case Bit:
					printf("请输入一个数字(Bit):\n");
					scanf("%lf",&bit);
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"Bit");
					c--;
					break;
				case KB:
					printf("请输入一个数字(Kb):\n");
					scanf("%lf",&bit);
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"KB");
					bit = bit * 1024;
					c--;
					break;
				case MB:
					printf("请输入一个数字(Mb):\n");
					scanf("%lf",&bit);
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"MB");
					bit = bit * 1024 * 1024;
					c--;
					break;
				case GB:
					printf("请输入一个数字(Gb):\n");
					scanf("%lf",&bit);
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"GB");
					bit = bit * 1024 * 1024 * 1024;
					c--;
					break;
				default:
					break;
			}
		}while(c);
		system("clear");
		fp = fopen(filename,"w");
		if(!fp) {
			printf("错误！\n");
			printf("\033[?25h");
			return 0;
		}
		a = 0;
		f = 0;
		printf("\033[1;1H填充完成度:[\033[1;63H]\n");
		for(count = 0; count < bit && count < 53687091200 && bit < 53687091200; count++) {
			fputs("a",fp);
			f = (count / bit);
			if((int)(f * 100) == a) {
				printf("\033[1;%dH#\033[1;64H%d%%\n",(a / 2) + 13,a);
				a++;
			}
		}
		printf("\033[2;1H创建完成！\n");
		fclose(fp);
		system("clear");
	}
	printf("\033[?25h");
	return 0;
}

