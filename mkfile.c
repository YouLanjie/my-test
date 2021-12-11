#include "include/include.h"

void stop();

enum unit{Exit = 48, Bit, KB, MB, GB};

int main(int argc,char * argv[]) {
	FILE *fp;
	char filename[30], ch, type;
	int a = 1, b, c = 1;
	double bit = 0, count, f = 0;

	signal(SIGINT,stop);
	printf("\033[?25l");
	opterr = 0;
	bit = 0;
	while (ch = getopt(argc, argv, "hs:t:") != -1) {
		if (ch == '?' || ch == 'h') {
			printf("mkfile -h\t帮助\nmkfile -[s 大小] -[t 单位]\033[?25h\n");
			return 0;
		}
		else if (ch == 's') {
			if (optopt == '?') {
				printf("没有指定大小\033[?25h\n");
				return 1;
			}
			bit = atof(optarg);
		}
		else if (ch == 't') {
			if (bit == 0) {
				printf("没有指定大小\033[?25h\n");
				return 1;
			}
			if (optopt == '?') {
				type = 'B';
			}
			else {
				type = *optarg;
			}
		}
	}
	while(1) {
		count = 0;
		c = 1;
		do {
			Clear2
			if (!bit) {
				printf("请选择单位：\n0:Exit\n1:Bit\n2:KB\n3:MB\n4:GB\n");
				b = Input();
				Clear2
			}
			else {
				switch (type) {
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
			}
			switch (b) {
				case Exit:
					Clear2
					printf("\033[?25h");
					return 0;
					break;
				case Bit:
					if (!bit) {
						printf("请输入一个数字(Bit):\n");
						scanf("%lf",&bit);
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"Bit");
					c--;
					break;
				case KB:
					if (!bit) {
						printf("请输入一个数字(Kb):\n");
						scanf("%lf",&bit);
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"KB");
					bit = bit * 1024;
					c--;
					break;
				case MB:
					if (!bit) {
						printf("请输入一个数字(Mb):\n");
						scanf("%lf",&bit);
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"MB");
					bit = bit * 1024 * 1024;
					c--;
					break;
				case GB:
					if (!bit) {
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
		if (argc < 3) {
			bit = 0;
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
