#include <stdio.h>
#include <stdlib.h>       //包含system函数

int main() {
	system("clear");                              //清屏
	printf("Number | Hex | Char\n");
	for (unsigned short a = 0; a < 260; a++) {
		printf("%03d | 0x%03x | %c\n",a,a,a);  //循环打印输出
	}
	return 0;
}

