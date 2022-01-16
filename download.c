#include "include/include.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char * argv[]) {
	pid_t pid;
	FILE * fp;
	int ch;
	char filename[300];
	char url[600];
	char temp[300];
	int i = 0;

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
			strcpy(filename, optarg);
		}
	}
	if (argc > 2) {
		fp = fopen(filename, "r");
		if (!fp) {
			perror("\033[1;31mfopen\033[0m");
			return -1;
		}
		i = 0;
		while (fscanf(fp,"%s",temp) != EOF) {
			fseek(fp, 1L, 1);
			if(fgets(filename, 300, fp) == NULL) {
				break;
			}
			filename[strlen(filename) -1] = '\0';
			strcpy(url, "wget -nc \'");
			strcat(url, temp);
			strcat(url, "\' -O \'DownloadFile/");
			strcat(url, filename);
			strcat(url, "\' -o \'DownloadFile/Log");
			sprintf(filename, "%03d", i);
			strcat(url, filename);
			strcat(url, "\'");
			pid = fork();
			if (pid == 0) {
				if (system(url) != 0) {
					printf("\033[1;31m下载链接: \033[1;33m\"%s\"\033[1;31m 出现错误，请在 DownloadFile/Log%s 中查看日志\033[0m\n", temp, filename);
					exit(-1);
				}
				strcpy(temp, "DownloadFile/Log");
				strcat(temp, filename);
				remove(temp);
				printf("\033[1;32m下载链接: \033[1;33m\"%s\"\033[1;32m下载完成\033[0m\n", temp);
				exit(0);
			}
			i++;
		}
		printf("\033[1;33m等待下载任务完成...\033[0m\n");
		while(wait(NULL) != -1);
		printf("\033[1;33m所有下载任务完成\033[0m\n");
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
			strcpy(url, "wget -nc \'");
			strcat(url, temp);
			printf("Please input the file name(use \'Q\' to use default name):\n");
			scanf("%s",filename);
			if (strcmp(filename, "q") == 0 || strcmp(filename, "Q") == 0) {
				strcat(url, "\' -P \'./DownloadFile/\'");
				system(url);
			}
			else {
				strcpy(url, "\' -O \'DownloadFile/");
				strcat(url, temp);
				strcat(url, "\'");
				system(url);
			}
		}
	}
	return 0;
}
