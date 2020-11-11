#include <stdio.h>
#include <stdlib.h>

int main() {
	int a,b;
	system("clear");
	printf("\tHello!\nstart test:\n");
	getchar();
	for(a=100;a!=0;a--) {
		system("clear");
		printf("血条：\n");
		for(b=0;b!=a;b++) {
			printf("#");
		}
		printf("\n");
		system("sleep 0.005");
	}
	return 0;
}
