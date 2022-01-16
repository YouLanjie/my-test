#include "include/include.h"

int main(int argc,char * argv[]) {
	int a = 0;
	int exit = 0;
	FILE * fp;

	if (argc < 2) {
		printf("没有参数error!!!\n");
		return -1;
	}

	Clear2
	printf("\033[?25l");
	fp = fopen(argv[1],"r");
	if (!fp) {
		printf("错误！！！\n");
		return 0;
	}
	while (!exit) {
		exit = kbhit();
		a = fgetc(fp);
		if(a == EOF) {
			fseek(fp,0L,0);
			system("sleep 0.023");
			Clear
		}
		else {
			printf("%c",a);
		}
	}
	fclose(fp);
	printf("\033[?25h");
	return 0;
}
