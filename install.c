#include "include/include.h"
#include <string.h>

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
	for (int i = 0;name != NULL; i++) {
		while (name != NULL && name -> d_type != 8) {
			name = readdir(dp);
			i++;
			if(i > 10) {
				break;
			}
		}
		if (name == NULL) {
			break;
		}
		if (strcmp(name -> d_name, "install.c") != 0 && name -> d_name[strlen(name -> d_name) - 2] == '.' && name -> d_name[strlen(name -> d_name) - 1] == 'c') {
			strcpy(filename, "gcc ");
			strcat(filename, name -> d_name);
			strcat(filename, " -o bin/");
			strcat(filename, name -> d_name);
			filename[strlen(filename) - 2] = '\0';
			printf("command: \'%s\'\n", filename);
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
