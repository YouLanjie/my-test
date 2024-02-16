#include "include/tools.h"

int main(int argc,char * argv[]) {
	struct ctools ctools = ctools_init();
	int a = 0;
	int exit = 0;
	FILE * fp;

	if (argc < 2) {
		printf("没有参数error!!!\n");
		return -1;
	}

	system("clear");
	printf("\033[?25l");
	fp = fopen(argv[1],"r");
	if (!fp) {
		printf("错误！！！\n");
		return 0;
	}
	while (!exit) {
		exit = ctools.kbhit();
		a = fgetc(fp);
		if(a == EOF) {
			fseek(fp,0L,0);
			usleep(50000);
			printf("\033[2J\033[0;0H");
		}
		else {
			printf("%c",a);
		}
	}
	fclose(fp);
	printf("\033[?25h");
	return 0;
}
