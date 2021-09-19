#include "include/include.h"
#include <bits/getopt_core.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char * argv[]) {
	FILE * fp;
	char ch;
	char filename[300];
	char url[600];
	char temp[300];

	if (access("DownloadFile", 0) == EOF) {
		if(mkdir("DownloadFile", 0777) == EOF) {
			perror("\033[1;31mmkdir\033[0m");
			return 0;
		}
	}
	opterr = 0;
	while ((ch = getopt(argc, argv, "f:")) != -1) {
		if (ch == '?') {
			printf("download -h \t帮助\n         -f 文件名\t指定url信息文件，前url后文件名\n");
			return 0;
		}
		if (ch == 'f') {
			if (optopt == '?') {
				printf("没有指定文件\n");
				return 1;
			}
			strcpy(optarg, filename);
		}
	}
	if (argc > 2) {
		fp = fopen(filename, "r");
		if (!fp) {
			perror("\033[1;31mfopen\033[0m");
		}
		while (fscanf(fp,"%s",temp) != EOF) {
			fseek(fp, 1L, 1);
			if(fgets(filename, 300, fp) == NULL) {
				break;
			}
			strcpy(url, "wget ");
			strcat(url, temp);
			strcat(url, " -O DownloadFile/");
			strcat(url,filename);
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
				strcat(filename, " -P ./DownloadFile/");
				system(url);
			}
			else {
				strcpy(filename, " -O DownloadFile/");
				strcat(filename, temp);
				strcat(url, filename);
				system(url);
			}
		}
	}
	return 0;
}
