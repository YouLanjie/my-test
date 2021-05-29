#include "src/headfile/head.h"

int main() {
	int a = 0;
	int exit = 0;
	FILE * fp;

	Clear2
	printf("\033[?25l");
	fp = fopen("input.txt","r");
	if (!fp) {
		printf("错误！！！\n");
		return 0;
	}
	while (!exit) {
		exit = kbhit_if();
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
