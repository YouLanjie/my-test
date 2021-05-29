#include <stdio.h>
#include <stdlib.h>

int main() {
	unsigned short a = 1;

	system("clear");
	printf("Number | Hex | Char\n");
	while(a < 260) {
		printf("%3d | 0x%x | %c\n",a,a,a);
		a++;
	}
	return 0;
}
