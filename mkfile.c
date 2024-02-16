#include "include/tools.h"

void stop();
int  get_opt(int argc, char * argv[], char * type);
int  input(char * type, char * filename);
int  get_type(char * type);
int  get_size(char * type, char * filename);
int  mkfile(int argc, char * filename);

enum unit{Exit = 48, Bit, KiB, MiB, GiB, KB, MB, GB};
double bit = 0;    /* 比特值 */

int main(int argc, char * argv[])
{
	char     filename[30],    /* 文件名 */
	         type;            /* 单位 */
	int      exited = 0;      /* 退出代码 */

	signal(SIGINT,stop);
	exited = get_opt(argc, argv, &type);
	if (exited) {
		return 0;
	}
	while(!exited) {    /* 用户选择 */
		exited = input(&type, filename);
		if (exited) {
			return 0;
		}
		exited = mkfile(argc, filename);
	}
	return exited;
}

/* 
 * 获取参数
 */
int get_opt(int argc,char * argv[], char * type)
{
	int ch = 0;

	// opterr = 0;
	while ((ch = getopt(argc, argv, "hs:t:")) != -1) {    /* 获取参数 */
		if (ch == '?' || ch == 'h') {
			printf("mkfile -h\t帮助\nmkfile -[s 大小] -[t 单位]\033[?25h\n");
			return 1;
		} else if (ch == 's') {    /* size大小 */
			if (optopt == '?') {
				printf("没有指定大小\033[?25h\n");
				return -1;
			}
			bit = atof(optarg);    /* 字符转数字 */
		} else if (ch == 't') {    /* type单位类型 */
			if (bit == 0) {
				printf("没有指定大小\033[?25h\n");
				return -1;
			}
			if (optopt == '?') {
				*type = 'B';
			} else {
				*type = *optarg;
			}
		}
	}
	return 0;
}

/*
 * 主菜单输入
 */
int input(char * type, char * filename)
{
	get_type(type);
	switch (*type) {
	case Exit:    /* 退出 */
	case 'q':
	case '8':
		return 1;
		break;
	case Bit:    /* 比特 */
		get_size(type, filename);
		strcat(filename,"Bit");
		break;
	case KiB:
		get_size(type, filename);
		strcat(filename,"KiB");
		bit = bit * 1024;
		break;
	case MiB:
		get_size(type, filename);
		strcat(filename,"MiB");
		bit = bit * 1024 * 1024;
		break;
	case GiB:
		get_size(type, filename);
		strcat(filename,"GiB");
		bit = bit * 1024 * 1024 * 1024;
		break;
	default:
		break;
	}
	return 0;
}

/* 
 * 通过两种方式获得单位类型：
 * 1 显示菜单选择
 * 2 通过命令行传入参数
 */
int get_type(char * type) {
	struct ctools ctools = ctools_init();
	struct ctools_menu_t *data = NULL;    /* 使用工具库中的菜单 */

	if (!bit) {    /* 如果比特数为0:非命令行模式 */
		ctools.menu.ncurses_init();
		ctools.menu.data_init(&data);

		ctools.menu.set_title(data, "创建文件");
		ctools.menu.set_text(data, "1.Bit", "2.KiB", "3.MiB", "4.GiB", "5.KB", "6.MB", "7.GB", "0.Exit", NULL);

		*type = (char)ctools.menu.show(data);    /* 获取类型 */
		endwin();

	} else {    /* 命令行模式 */
		switch (*type) {    /* 将传入参数转换为输入 */
		case 'b':
		case 'B':
			*type = '1';
			break;
		case 'k':
		case 'K':
			*type = '2';
			break;
		case 'm':
		case 'M':
			*type = '3';
			break;
		case 'g':
		case 'G':
			*type = '4';
			break;
		}
	}
	return 0;
}

/* 
 * 获得大小
 */
int get_size(char * type, char * filename) {
	if (!bit) {
		printf("请输入一个数字(");
		switch (*type) {
		case Bit:
			printf("Bit");
			break;
		case KiB:
			printf("KiB");
			break;
		case MiB:
			printf("MiB");
			break;
		case GiB:
			printf("GiB");
			break;
		default:
			printf("???");
			break;
		}
		printf(")，回车确定:\n");
		scanf("%lf",&bit);
		sprintf(filename,"%.1lf",bit);
	}
	return 0;
}

/* 
 * 创建文件
 */
int mkfile(int argc, char *filename)
{
	FILE   * fp;                 /* 文件指针 */
	double   count = 0,          /* 填充的字符数 */
	         percent = 0;        /* 已填充的字符占字符总数的百分比 */
	int      percent_int = 0;    /* 同上百分比，但是是整型 */
	struct ctools ctools = ctools_init();

	fp = fopen(filename,"w");
	if(!fp) {
		perror("错误:");
		return -1;
	}

	printf("填充完成度:[\033[52C]\n");
	for(count = 0; count <= bit && count < 53687091200 && bit < 53687091200; count++) {
		percent = (count / bit);
		while (percent * 100 > percent_int) {
			percent_int++;
			printf("\033[A\033[%dC=>\n\033[A\033[65C%3d%%\n",(percent_int / 2) + 12, percent_int);
			ctools.kbhitGetchar();
		}
		if (count < bit) {    /* 插入文件内容 */
			fputs("\n",fp);
		}
	}
	fclose(fp);
	printf("创建完成！按下回车返回\n");
	ctools.getcha();
	ctools.getcha();
	if (argc < 3) {    /* 是否进入了界面 */
		bit = 0;
		return 0;
	} else {
		return 1;
	}
}


void stop() {
	endwin();
	printf("异常退出\n");
	exit(1);
}

