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

	if (access("DownloadFile", 0) == EOF) {
		if(mkdir("DownloadFile", 0777) == EOF) {
			perror("\033[1;31mmkdir\033[0m");
			return 0;
		}
	}
	if (argc > 1) {
		fp = fopen(argv[1], "r");
		if (!fp) {
			perror("\033[1;31mfope\033[0m");
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
				strcat(filename, " -O DownloadFile/");
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

