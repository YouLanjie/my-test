#include "include/tools.h"

void stop();

enum unit{Exit = 48, Bit, KiB, MiB, GiB, KB, MB, GB};

int main(int argc,char * argv[]) {
	FILE   * fp;              /* 文件指针 */
	char     filename[30],    /* 文件名 */
	         type;            /* 单位 */
	int      a   = 1,
	         b,               /* 输入 */
	         c   = 1,
	         ch  = 0;
	double   bit = 0,         /* 比特值 */
	         count,           /* 填充的字符数 */
	         f   = 0;         /* 已填充的字符占字符总数的百分比 */
	menuData data;            /* 使用工具库中的菜单 */

	menuDataInit(&data);
	data.title = "创建文件";
	data.addText(&data, "1.Bit", "2.KiB", "3.MiB", "4.GiB", "5.KB", "6.MB", "7.GB", "0.Exit", NULL);

	signal(SIGINT,stop);
	printf("\033[?25l");
	opterr = 0;
	while ((ch = getopt(argc, argv, "hs:t:")) != -1) {    /* 获取参数 */
		if (ch == '?' || ch == 'h') {
			printf("mkfile -h\t帮助\nmkfile -[s 大小] -[t 单位]\033[?25h\n");
			return 0;
		}
		else if (ch == 's') {    /* size大小 */
			if (optopt == '?') {
				printf("没有指定大小\033[?25h\n");
				return -1;
			}
			bit = atof(optarg);    /* 字符转数字 */
		}
		else if (ch == 't') {    /* type单位类型 */
			if (bit == 0) {
				printf("没有指定大小\033[?25h\n");
				return -1;
			}
			if (optopt == '?') {
				type = 'B';
			}
			else {
				type = *optarg;
			}
		}
	}

	while(1) {    /* 用户选择 */
		count = 0;
		c = 1;
		do {    /* 限制界面在主界面 */
			if (!bit) {    /* 如果比特数为0 */
				Clear2
				b = data.menuShow(&data);    /* 获取类型 */
				Clear2
			}
			else {
				switch (type) {    /* 转换为输入 */
					case 'b':
					case 'B':
						b = '1';
						break;
					case 'k':
					case 'K':
						b = '2';
						break;
					case 'm':
					case 'M':
						b = '3';
						break;
					case 'g':
					case 'G':
						b = '4';
						break;
				}
			}
			switch (b) {
				case Exit:    /* 退出 */
				case 'q':
					Clear2
					printf("\033[?25h");
					return 0;
					break;
				case Bit:    /* 比特 */
					if (!bit) {
						printf("请输入一个数字(Bit)，回车确定:\n");
						scanf("%lf",&bit);
						getch();
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"Bit");
					c--;
					break;
				case KiB:
					if (!bit) {
						printf("请输入一个数字(KiB)，回车确定:\n");
						scanf("%lf",&bit);
						getch();
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"KiB");
					bit = bit * 1024;
					c--;
					break;
				case MiB:
					if (!bit) {
						printf("请输入一个数字(MiB)，回车确定:\n");
						scanf("%lf",&bit);
						getch();
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"MiB");
					bit = bit * 1024 * 1024;
					c--;
					break;
				case GiB:
					if (!bit) {
						printf("请输入一个数字(GiB)，回车确定:\n");
						scanf("%lf",&bit);
						getch();
					}
					sprintf(filename,"%.1lf",bit);
					strcat(filename,"GiB");
					bit = bit * 1024 * 1024 * 1024;
					c--;
					break;
				default:
					break;
			}
		}while(c);

		fp = fopen(filename,"w");
		if(!fp) {
			printf("错误！\n");
			printf("\033[?25h");
			return 0;
		}

		a = 0;
		f = 0;
		printf("填充完成度:[\033[51C]\n");
		for(count = 0; count <= bit && count < 53687091200 && bit < 53687091200; count++) {
			f = (count / bit);
			while (f * 100 > a) {
				a++;
				printf("\033[A\033[%dC=>\n\033[A\033[64C%3d%%\n",(a / 2) + 11,a);
			}
			if((int)(f * 100) == a) {
				a++;
			}
			if (count < bit) {
				fputs("\n",fp);
			}
		}
		printf("创建完成！\n");
		fclose(fp);
		if (argc < 3) {
			bit = 0;
			getch();
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
