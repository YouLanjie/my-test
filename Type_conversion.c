#include "include/include.h"
#include <signal.h>
#include <wait.h>      //wait();

int conversion(char filename[150], char filename_o[150], char dirname[100]);

int main(int argc,char * argv[]) {
	int a = 0x31,i = 0, i2 = 1;
	char filename[150] = "ffmpeg -i ", filename_o[150], dirname[100] = "./", type[10];
	DIR * dp = NULL;
	struct dirent * name;
	pid_t pid;

	if (argc < 2) {
		printf("Error!!!请指定类型(mp3,m4a,mp4,gif,jpg,png)\n");
		scanf("%s", type);
		getchar();
		printf("请指定文件夹\n");
		scanf("%s", dirname);
		getchar();
	}
	else if (argc < 3){
		strcpy(type, argv[1]);
		printf("Error!!!请指定文件夹\n");
		scanf("%s", dirname);
		getchar();
	}
	else {
		strcpy(type, argv[1]);
		strcpy(dirname, argv[2]);
	}
	while(a != 0x30) {
		Clear2
		printf("\033[1;32mWelocome\n\033[33minput '1' to start,'0' to exit\n");
		a = input();
		if(a == 0x31) {
			Clear2
			dp = opendir(dirname);
			if (!dp) {
				perror(dirname);
				return -1;
			}
			name = readdir(dp);
			for (i = 0;name != NULL && i < 10; i++) {
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
				strcpy(filename, dirname);
				strcat(filename, "/");
				strcpy(filename_o, filename);
				strcat(filename, name -> d_name);
				strcat(filename_o, "../out/");
				mkdir(filename_o,0777);
				strcat(filename_o, name -> d_name);
				i2 = 1;
				do {
					filename_o[strlen(filename_o) - i2] = type[strlen(type) - i2];
					i2++;
				}while(filename_o[strlen(filename_o) - i2] != '.');
				pid = fork();
				if(pid == 0) {
					conversion(filename,filename_o,dirname);
					exit(-1);
				}
				name = readdir(dp);
			}
			while(wait(NULL) != -1);
			strcat(dirname, "/../out/Log");
			remove(dirname);
			printf("\033[1;32m所有文件转换完成\033[0m\n");
			input();
		}
	}
	if(dp) {
		closedir(dp);
	}
	return 0;
}

int conversion(char filename[150], char filename_o[150], char dirname[100]) {
	struct stat statbuf;
	char command[250] = "ffmpeg -i \"";

	if (access(filename_o, 0) == 0) {
		printf("\033[1;31m[文件已存在，跳过]filename:\033[1;32m%s\033[0m\n",filename_o);
		exit(1);
	}
	strcat(command, filename);
	strcat(command,"\" \"");
	strcat(command,filename_o);
	strcat(command,"\" >> ");
	strcat(command,dirname);
	strcat(command,"/../out/Log 2>&1");
	if (system(command) != 0) {
		stat(filename_o, &statbuf);
		if (statbuf.st_size == 0) {
			printf("\033[1;31mfilename: %s\n",filename);
			printf("转换错误!请从%s/../out/Log中查看记录\033[0m\n",dirname);
			remove(filename_o);
			exit(-1);
		}
		printf("\033[1;31m[system]: \033[1;31m%s\033[1;32m\n",command);
		perror(filename);
		printf("\033[0m");
		exit(-1);
	}
	printf("\033[1;32m文件\'%s\'转换完成\033[0m\n",filename);
	exit(0);
}
