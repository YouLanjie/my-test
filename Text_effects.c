#include <stdio.h>

int main() {
	printf("text:\n\033[0m0m This is text \033[0m\n");
	printf("\033[1m1m This is text \033[0m\n");
	printf("\033[2m2m This is text \033[0m\n");
	printf("\033[3m3m This is text \033[0m\n");
	printf("\033[4m4m This is text \033[0m\n");
	printf("\033[5m5m This is text \033[0m\n");
	printf("\033[6m6m This is text \033[0m\n");
	printf("\033[7m7m This is text \033[0m\n");
	printf("\033[8m8m This is text \033[0m\nmouse:\n");
	getchar();
	printf("\033[?25l?25l This is text \033[0m\n");
	getchar();
	printf("\033[?25h?25h This is text \033[0m\n");
	getchar();
	printf("color:\n\033[31m This is text \033[0m\n");
	printf("\033[32m This is text \033[0m\n");
	printf("\033[33m This is text \033[0m\n");
	printf("\033[34m This is text \033[0m\n");
	printf("\033[35m This is text \033[0m\n");
	printf("\033[36m This is text \033[0m\n");
	printf("\033[37m This is text \033[0m\n");
	printf("\033[38m This is text \033[0m\n");
	printf("\033[39m This is text \033[0m\n");
	return 0;
}
