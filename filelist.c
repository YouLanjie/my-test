#include <stdio.h>
#include <dirent.h>
#include <string.h>

int main() {
	char dirname[100];
	DIR * dp;
	struct dirent * name;

	printf("input dir name\n");
	scanf("%s",dirname);
	dp = opendir(dirname);
	if (!dp) {
		perror("\033[1;31mError\033[0m");
		return 0;
	}
	printf("\033[1;36m类  型  ||  名  字\n==================\033[0m\n");
	name = readdir(dp);
	while (name) {
		if (strcmp(".", name -> d_name) == 0 || strcmp("..", name -> d_name) == 0) {
			name = readdir(dp);
			continue;
		}
		if (name -> d_type == 2) {
			printf("块设备  ||  \033[1;33m");
		}
		else if (name -> d_type == 4) {
			printf("文件夹  ||  \033[1;34m");
		}
		else if (name -> d_type == 6) {
			printf("存  储  ||  \033[1;33m");
		}
		else if (name -> d_type == 8) {
			printf("文  件  ||  \033[1;37m");
		}
		else if (name -> d_type == 10) {
			printf("链  接  ||  \033[1;36m");
		}
		else if (name -> d_type == 12) {
			printf("套接字  ||  \033[1;35m");
		}
		printf("%s\033[0m\n",name -> d_name);
		name = readdir(dp);
	}
	closedir(dp);
	return 0;
}

