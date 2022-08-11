#include "include/tools.h"

int conversion(char inputF[150], char outputF[150], char dirname[100], int i);    //转换函数

int main(int argc,char * argv[]) {
	int opt;
	int a = 0x31,i = 0, i2 = 1;
	unsigned long i4;
	char inputF[150], outputF[150], dirname[100] = "./", type[10]= "NULL";
	DIR * dp = NULL;
	struct dirent * name;  //文件夹指针
	pid_t pid;

	while ((opt = getopt(argc, argv, "t:d:h")) != -1) {
		switch (opt) {
			case 't':
				if (strcmp(optarg,"?") == 0) {
					printf("请指定类型(mp3,m4a,mp4,gif,jpg,png)\n");
					scanf("%s", type);
					getchar();
				}
				else {
					strcpy(type,optarg);
				}
				break;
			case 'd':
				if(strcmp(optarg,"?") == 0) {
					printf("请指定文件夹\n");
					scanf("%s", dirname);
					getchar();
				}
				else {
					strcpy(dirname,optarg);
				}
				break;
			case '?':
			case 'h':
			default:
				printf("本程序基于ffmpeg，转换格式时需要安装ffmpeg\n参数：Type_conversion -[t 目标格式] -[d 文件夹] -[h]帮助\n");
				return 0;
				break;
		}
	}
	if (strcmp(dirname,"./") == 0) {
		printf("请指定文件夹\n");
		scanf("%s", dirname);
		getchar();
	}
	if (strcmp(type,"NULL") == 0) {
		printf("请指定类型(mp3,m4a,mp4,gif,jpg,png)\n");
		scanf("%s", type);
		getchar();
	}
	while(a != 0x30) {
		Clear2
		printf("\033[1;32m欢迎使用批量格式转换小程序\n\033[31m原文件夹:\033[36m%s\t\033[31m目标格式:\033[36m%s\n\033[33m按下 1 键开始,按 0 退出\n",dirname,type);
		a = getch();
		if(a == 0x31) {
			Clear2
			dp = opendir(dirname);
			if (!dp) {
				perror(dirname);
				return -1;
			}
			name = readdir(dp);
			for (i = 0;name != NULL; i++) {
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
				strcpy(inputF, dirname);
				strcat(inputF, "/");
				strcpy(outputF, inputF);
				strcat(inputF, name -> d_name);
				strcat(outputF, "../out/");
				mkdir(outputF,0777);
				strcat(outputF, name -> d_name);
				i4 = strlen(outputF);
				i2 = 1;
				while(outputF[i4 - i2] != '.') {
					i2++;
				}
				i2--;
				for (unsigned long i3 = 0; i3 <= strlen(type); i3++) {
					outputF[i4 - i2] = type[i3];
					i2--;
				}
				pid = fork();
				if(pid == 0) {
					conversion(inputF,outputF,dirname,i);
					exit(-1);
				}
				name = readdir(dp);
			}
			while(wait(NULL) != -1);
			printf("\033[1;33m所有文件转换完成\033[0m\n");
			getch();
		}
	}
	if(dp) {
		closedir(dp);
	}
	return 0;
}

int conversion(char inputF[150], char outputF[150], char dirname[100], int i) {
	struct stat statbuf;
	char command[250] = "ffmpeg -i \"";
	char count[5] = "0";

	if (access(outputF, 0) == 0) {
		printf("\033[1;31m[文件已存在，跳过]filename:\033[1;32m%s\033[0m\n",outputF);
		exit(1);
	}
	strcat(command, inputF);
	strcat(command,"\" \"");
	strcat(command,outputF);
	strcat(command,"\" > ");
	strcat(command,dirname);
	strcat(command,"/../out/Log");
	sprintf(count,"%03d",i);
	strcat(command,count);
	strcat(command," 2>&1");
	if (system(command) != 0) {
		stat(outputF, &statbuf);
		if (statbuf.st_size == 0) {
			printf("\033[1;31mfilename: %s\n",inputF);
			printf("转换错误!请从%s/../out/Log%s中查看记录\033[0m\n",dirname,count);
			remove(outputF);
			exit(-1);
		}
		printf("\033[1;31m[system]: \033[1;31m%s\033[1;32m\n",command);
		perror(inputF);
		printf("\033[0m");
		exit(-1);
	}
	strcat(dirname, "/../out/Log");
	strcat(dirname, count);
	remove(dirname);
	printf("\033[1;32m文件\033[1;33m\'%s\'\033[1;32m转换完成\033[0m\n",inputF);
	exit(0);
}
