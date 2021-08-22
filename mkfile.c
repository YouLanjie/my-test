#include "include/include.h"
#include <stdlib.h>

void stop();

enum unit{Exit = 48, Bit, KB, MB, GB};

int main(int argc,char * argv[]) {
	FILE *fp;
	char filename[30];
	int a = 1,b,c = 1;
	double bit;
	double count;
	double f = 0;

	signal(SIGINT,stop);
	printf("\033[?25l");
	if(argc != 3 && argc != 1) {
		printf("错误，要有两个参数\n格式：mkfile [Size Unit]\033[?25h\n");
		return -1;
	}
	while(1) {
		bit = 0;
		count = 0;
		c = 1;
		do {
			Clear2
			if (argc == 1) {
				printf("请选择单位：\n0:Exit\n1:Bit\n2:KB\n3:MB\n4:GB\n");
				b = input();
				Clear2
			}
			else {
				switch (*argv[2]) {
					case 'b':
					case 'B':
						b = 49;
						break;
					case 'k':
					case 'K':
						b = 50;
						break;
					case 'm':
					case 'M':
						b = 51;
						break;
					case 'g':
					case 'G':
						b = 52;
						break;
				}
				bit = atof(argv[1]);
			}
			switch (b) {
				case Exit:
					Clear2
					printf("\033[?25h");
					return 0;
					break;
				case Bit:
					if (argc == 1) {
						printf("请输入一个数字(Bit):\n");
						scanf("%lf",&bit);
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"Bit");
					c--;
					break;
				case KB:
					if (argc == 1) {
						printf("请输入一个数字(Kb):\n");
						scanf("%lf",&bit);
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"KB");
					bit = bit * 1024;
					c--;
					break;
				case MB:
					if (argc == 1) {
						printf("请输入一个数字(Mb):\n");
						scanf("%lf",&bit);
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"MB");
					bit = bit * 1024 * 1024;
					c--;
					break;
				case GB:
					if (argc == 1) {
						printf("请输入一个数字(Gb):\n");
						scanf("%lf",&bit);
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"GB");
					bit = bit * 1024 * 1024 * 1024;
					c--;
					break;
				default:
					break;
			}
		}while(c);
		Clear2
		fp = fopen(filename,"w");
		if(!fp) {
			printf("错误！\n");
			printf("\033[?25h");
			return 0;
		}
		a = 0;
		f = 0;
		printf("\033[1;1H填充完成度:[\033[1;63H]\n");
		for(count = 0; count <= bit && count < 53687091200 && bit < 53687091200; count++) {
			f = (count / bit);
			while (f * 100 > a) {
				a++;
			}
			for(int i = 0; (int)(f * 100) == a && i <= a; i++) {
				printf("\033[1;%dH=>\033[1;12H[\033[1;64H%d%%\n",(i / 2) + 12,i);
			}
			if((int)(f * 100) == a) {
				a++;
			}
			if (count < bit) {
				fputs("\n",fp);
			}
		}
		printf("\033[2;1H创建完成！\n");
		fclose(fp);
		if (argc == 1) {
			Clear2
		}
		else {
			printf("\033[?25h");
			return 0;
		}
	}
	printf("\033[?25h");
	return 0;
}

void stop() {
	printf("异常退出\033[?25h\n");
	exit(1);
}
