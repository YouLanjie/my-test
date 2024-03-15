#include "include/tools.h"

int conversion(char inputF[150], char outputF[150], char dirname[100], int i);	//转换函数

int flag_command = 0;

int main(int argc, char *argv[])
{
	char             inputF[150],            /* 输入文件名 */
	                 outputF[150],           /* 输出文件名 */
	                 dirname[100] = "./",    /* 目录文件名 */
	                 type[10] = "NULL";      /* 文件后缀名 */
	int              opt,
	                 a = 0x31,     /* 输入 */
	                 i = 0,
	                 i2 = 1;
	unsigned long    i4;
	pid_t            pid;          /* 进程的pid */
	DIR            * dp = NULL;    /* 目录指针 */
	struct dirent  * name;         /* 文件夹指针 */
	struct timeval   time,
		         time2,
		         time3;

	while ((opt = getopt(argc, argv, "t:d:h")) != -1) {
		switch (opt) {
		case 't':
			if (strcmp(optarg, "?") == 0) {
				printf("请指定类型(mp3,m4a,mp4,gif,jpg,png)\n");
				scanf("%s", type);
				getchar();
			} else {
				strcpy(type, optarg);
			}
			break;
		case 'd':
			if (strcmp(optarg, "?") == 0) {
				printf("请指定文件夹\n");
				scanf("%s", dirname);
				getchar();
			} else {
				strcpy(dirname, optarg);
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
	if (strcmp(dirname, "./") == 0) {
		printf("请指定文件夹\n");
		scanf("%s", dirname);
		getchar();
	}
	if (strcmp(type, "NULL") == 0) {
		printf("请指定类型(mp3,m4a,mp4,gif,jpg,png)\n");
		scanf("%s", type);
		getchar();
	}
	while (a != '0' && a != 'q' && a != 'Q' && a != 0x1B) {
		system("clear");
		printf("\033[1;32m欢迎使用批量格式转换小程序\n\033[31m原文件夹:\033[1;32m%s\t\033[0;31m目标格式:\033[1;32m%s\n\033[33m按下 1 键开始,按下 2 键显示调试信息,按 0 退出\033[0m\n", dirname, type);
		a = _getch();
		if (a != '0' && a != 'q' && a != 'Q' && a != 0x1B) {
			printf("\033[1;32m开始转换...\033[0m\n\n");
			if (a == '2') {
				flag_command = 1;
				a = '1';
			} else flag_command = 0;
		}
		if (a == '\r' || a == '\n' || a == '1') {
			dp = opendir(dirname);
			if (!dp) {
				perror(dirname);
				return -1;
			}
			name = readdir(dp);
			gettimeofday(&time, NULL);
			while (name != NULL && name->d_type != 8 && name->d_type != 10) {	/* 获取文件名 */
				name = readdir(dp);
				i++;
			}
			if (name == NULL) {
				break;
			}
			for (i = 0; name != NULL && (name->d_type == 8 || name->d_type == 10); i++) {
				while (name != NULL && name->d_type != 8 && name->d_type != 10) {	/* 获取文件名 */
					name = readdir(dp);
					i++;
				}
				if (name == NULL) {
					break;
				}
				strcpy(inputF, dirname);
				strcat(inputF, "/");
				strcpy(outputF, inputF);
				strcat(inputF, name->d_name);
				strcat(outputF, "../out/");
				mkdir(outputF, 0777);
				strcat(outputF, name->d_name);
				i4 = strlen(outputF);
				i2 = 1;
				while (outputF[i4 - i2] != '.') {
					i2++;
				}
				i2--;
				for (unsigned long i3 = 0; i3 <= strlen(type); i3++) {
					outputF[i4 - i2] = type[i3];
					i2--;
				}
				pid = fork();
				if (pid == 0) {
					printf("\033[1;33mLoading: [file]: \033[1;32m\'%s\'\033[0m\n", inputF);
					fflush(stdout);
					conversion(inputF, outputF, dirname, i);
					exit(-1);
				}
				name = readdir(dp);
			}
			/* 计算时间差 */
			gettimeofday(&time2, NULL);

			fflush(stdout);
			usleep(900);
			printf("\n\033[1;33mInfo: \033[1;32m耗时%2.6f秒加载完毕，开始等待\033[0m\n\n",
			     time2.tv_sec - time.tv_sec + (double)(time2.tv_usec - time.tv_usec) / 1000000);
			fflush(stdout);

			gettimeofday(&time2, NULL);
			while (wait(NULL) != -1) ;

			gettimeofday(&time3, NULL);

			printf("\n\033[1;33mInfo: \033[1;32m共%d个目标文件转换完成，耗时%2.6f秒，共耗时%2.6f秒\033[0m\n",
			     i,
			     time3.tv_sec - time2.tv_sec + (double)(time3.tv_usec - time2.tv_usec) / 1000000,
			     time3.tv_sec - time.tv_sec + (double)(time3.tv_usec - time.tv_usec) / 1000000);
			_getch();
		}
	}
	if (dp) {
		closedir(dp);
	}
	return 0;
}

int conversion(char inputF[150], char outputF[150], char dirname[100], int i)
{
	struct stat statbuf;
	struct timeval time, time2;
	char command[250] = "ffmpeg -i \"";
	char outputLog[30];
	char count[5] = "0";

	usleep(10000);
	if (access(outputF, 0) == 0) {
		printf("\033[1;33mWarning: [跳过已存在文件]: \033[1;32m%s\033[0m\n", outputF);
		exit(1);
	}
	strcat(command, inputF);
	strcat(command, "\" \"");
	strcat(command, outputF);
	strcat(command, "\" > ");

	strcpy(outputLog, dirname);
	strcat(outputLog, "/../out/Log");
	sprintf(count, "%03d", i);
	strcat(outputLog, count);

	strcat(command, outputLog);
	strcat(command, " 2>&1");

	gettimeofday(&time, NULL);
	if (flag_command) printf("\033[1;33mINFO: [cmd]: \033[1;32m\'%s\'\033[0m\n", command);
	fflush(stdout);
	fflush(stderr);
	if (system(command) != 0) {
		stat(outputF, &statbuf);
		if (statbuf.st_size == 0) {
			remove(outputF);
		}
		printf("\033[1;33m---------------Error Message Start---------------\033[0m\n\033[1;33mError: [filename]: \033[1;32m%s\n\033[1;33mError: [command ]: \033[1;32m%s\033[1;32m\n\033[1;33mError: [log file]: \033[1;32m%s/../out/Log%s\033[0m\n\033[1;33mError: [ perror ]: \033[1;32m",
		     inputF, command, dirname, count);
		fflush(stdout);
		perror(inputF);
		fflush(stderr);
		printf("\033[1;33mError: 转换错误!\033[0m\n\033[1;33m---------------Error Message End---------------\033[0m\n");

		freopen(outputLog, "a", stdout);
		freopen(outputLog, "a", stderr);
		printf("\n---------------Error Message Start---------------\nError: [filename]: %s\nError: [command ]: 1;32m%s\nError: [log file]: %s/../out/Log%s\nError: [ perror ]: ",
		     inputF, command, dirname, count);
		perror(inputF);
		fflush(stdout);
		perror(inputF);
		fflush(stderr);
		printf("Error: 转换错误!\n---------------Error Message End---------------\n");
		fclose(stdout);
		fclose(stderr);
		exit(-1);
	}
	gettimeofday(&time2, NULL);

	strcat(dirname, "/../out/Log");
	strcat(dirname, count);
	remove(dirname);
	printf("\033[1;33m耗时约\033[1;32m%2.6f秒\033[1;33m转换完成：\033[1;32m\'%s\'\033[0m\n",
	     time2.tv_sec - time.tv_sec + (double)(time2.tv_usec - time.tv_usec) / 1000000,
	     inputF);
	fflush(stdout);
	exit(0);
}
