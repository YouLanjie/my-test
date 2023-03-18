#include "include/tools.h"

#define Files 2
// #define CC "clang"
#define CC "gcc"

char unbuild[Files][50] = {  //不编译的文件列表
	"build.c",
	"gtk.c",
};

int main() {
#ifdef __linux
	char filename[250] = "./";
	DIR * dp = NULL;
	struct dirent * name;  //文件夹指针

	if (access("bin/", 0) == EOF) {
		mkdir("bin", 0755);
	}
	dp = opendir("./");
	if (!dp) {
		perror("./ :");
		return -1;
	}
	name = readdir(dp);
	while (name != NULL) {
		while (1) { //选取特定的文件
			if (name == NULL || name -> d_type == 8) { /* 是普通文件时 */
				for (int i = 0; i < Files; i++) {
					if (name != NULL && strcmp(name->d_name, unbuild[i]) == 0) {
						name = readdir(dp);
						break;
					}
				}
				if (name == NULL || name -> d_type == 8) {
					break;
				}
			}
			name = readdir(dp);
		}
		if (name == NULL) {
			break;
		}
		if (name -> d_name[strlen(name -> d_name) - 2] == '.' && name -> d_name[strlen(name -> d_name) - 1] == 'c') {
			strcpy(filename, CC);
			strcat(filename, " ");
			strcat(filename, name -> d_name);
			strcat(filename, " -g -Wall -L lib -ltools -lncurses");
			strcat(filename, " -o bin/");
			strcat(filename, name -> d_name);
			filename[strlen(filename) - 2] = '\0';
			if (strcmp(name -> d_name, "RSA.c") == 0) {
				strcat(filename, " -lm");
			}
			printf("\033[1;33mcommand: \033[1;32m\'%s\'\033[0m\n", filename);
			system(filename);
		}
		name = readdir(dp);
	}
	if(dp) {
		closedir(dp);
	}
#else 
	printf("This program Only run in LINUX SYSTEM!!!\n");
#endif
	return 0;
}
