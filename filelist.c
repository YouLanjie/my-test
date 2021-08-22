#include "include/include.h"

int main(int argc, char * argv[]) {
	char dirname[501];
	DIR * dp;
	struct dirent * name;

	if (argc == 1) {
		printf("input dir name\n");
		fgets(dirname,501,stdin);
		dirname[strlen(dirname) - 1] = '\0';
		dp = opendir(dirname);
	}
	else {
		printf("\033[1;33m类  型  ||  名  字\033[1;36m\n==================\033[0m\n");
		for (int count = 1; count < argc; count++) {
			dp = opendir(argv[count]);
			printf("\033[1;33m%s :\033[0m\n",argv[count]);
			if (!dp) {
				perror(argv[count]);
				printf("\033[1;36m==================\033[0m\n");
				continue;
			}
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
			printf("\033[1;36m==================\033[0m\n");
		}
		return 0;
	}
	if (!dp) {
		printf("\033[1;31m[Error]\033[0m");
		perror(dirname);
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

