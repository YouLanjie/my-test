#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char * argv[]) {
	FILE * fp;
	char filename[300];
	char url[600];
	char temp[300];

	if (access("CloudMusic", 0) == EOF) {
		if(mkdir("CloudMusic", 0777) == EOF) {
			perror("\033[1;31mmkdir\033[0m");
			return 0;
		}
	}
	else {
		if (argc > 1) {
			fp = fopen(argv[1], "r");
			if (!fp) {
				perror("\033[1;31mfope\033[0m");
			}
			while (fgets(temp, 300, fp) != NULL) {
				strcpy(url, "wget ");
				strcat(url, temp);
				system(url);
			}
		}
		else {
			system("clear");
			printf("Please input the url(use \'Q\' to exit):\n");
			scanf("%s",temp);
			if (strcmp(temp, "q") == 0 || strcmp(temp, "Q") == 0) {
				system("clear");
				printf("exit\n");
				return 0;
			}
			else {
				strcpy(url, "wget ");
				strcat(url, temp);
				printf("Please input the file name(use \'Q\' to use default name):\n");
				scanf("%s",temp);
				if (strcmp(temp, "q") == 0 || strcmp(temp, "Q") == 0) {
					strcat(filename, " -O CloudMusic/");
					system(url);
				}
				else {
					strcpy(filename, " -O CloudMusic/");
					strcat(filename, temp);
					strcat(url, filename);
					system(url);
				}
			}
		}
	}
	return 0;
}

