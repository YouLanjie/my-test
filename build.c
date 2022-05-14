#include "include/include.h"

#define Files 2

char unbuild[Files][50] = {
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
					if (strcmp(name -> d_name, unbuild[i]) == 0) {
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
			strcpy(filename, "gcc -g ");
			strcat(filename, name -> d_name);
			strcat(filename, " -o bin/");
			strcat(filename, name -> d_name);
			filename[strlen(filename) - 2] = '\0';
			if (strcmp(name -> d_name, "RSA.c") == 0) {
				strcat(filename, " -lm");
			}
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
