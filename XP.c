#include <stdio.h>
#include <stdlib.h>

int main() {
	int a,b;
	system("clear");
	printf("\tHello!\nstart test:\n");
	getchar();
	for(a=100;a!=0;a--) {
		printf("\033[2J\033[1;1H");
		printf("血条：\n");
		for(b=0;b!=a;b++) {
			printf("#");
		}
		printf("\n");
		system("sleep 0.013");
	}
	return 0;
}
