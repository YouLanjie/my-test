#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define Clear system("clear");

int main(int argc,char * argv[]) {
	Clear
	if(argc == 1) {
		perror("\033[1;31mbuild: error :没有参数！\033[0m");
		return 0;
	}
	if(strcmp(argv[1],"-b") == 0) {
		system("gcc src/main.c -o build/main");
	}
	else if(strcmp(argv[1],"-d") == 0) {
		if(access("./build/main",0) != EOF) {
			system("./build/main");
		}
		else {
			perror("\033[1;31m可执行文件不存在！\033[0m");
			return 0;
		}
	}
	else {
		perror("\033[1;31m参数不正确！\033[0m");
		return 0;
	}
	if(argc == 3) {
		if(strcmp(argv[2],"-b") == 0) {
			system("gcc src/main.c -o build/main");
		}
		else if(strcmp(argv[2],"-d") == 0) {
			if(access("./build/main",0) != EOF) {
				system("./build/main");
			}
			else {
				perror("\033[1;31m可执行文件不存在！\033[0m");
				return 0;
			}
		}
	}
	return 0;
}

