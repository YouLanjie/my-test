#include "include/include.h"

struct Command {
	char command[45];        //命令
	short stat;
	struct Command *pNext;   //下一个节点
};

struct Command * mkpoint(short count, struct Command *pNext, char *command);
struct Command * filelist(struct Command *pNext);
int doing(struct Command *pHead, struct Command *pFilelist[]);

int main(int argc, char * argv[]) {
	int opt, count = 0;
	struct Command *pHead, *pNext, *pFilelist[10];

	/* 获取命令参数 */
	while ((opt = getopt(argc, argv, "hc:d:")) != -1) {
		switch (opt) {
			case '?':
			case 'h':
				printf("这是一个批量执行命令的程序\n");
				return 0;
			case 'c':
				if (strcmp(optarg,"?") == 0) {
					printf("这是一个批量执行命令的程序\n");
					return -1;
				}
				else {
					pNext = mkpoint(count, pNext, optarg);
					pNext -> stat = 0;
					count++;
				}
				break;
			case 'd':
				if (strcmp(optarg,"?") == 0) {
					printf("这是一个批量执行命令的程序\n");
					return -1;
				}
				else {
					pNext = mkpoint(count, pNext, optarg);
					pNext -> stat = 1;
					count++;
				}
				break;
		}
		if (count == 1) {
			pHead = pNext;
		}
	}

	/* 获取命令中要替换的文件列表 */
	pNext = pHead;
	count = 0;
	while (pNext -> pNext != NULL) {
		if (pNext -> stat == 1) {
			pFilelist[count] = filelist(pNext);
			if (pFilelist[count] == NULL) {
				return -1;
			}
			count++;
		}
		pNext = pNext -> pNext;
	}

	/* 执行命令 */
	doing(pHead, pFilelist);
	while (wait(NULL) != -1);
	
	return 0;
}

struct Command * mkpoint(short count, struct Command *pNext, char *command) {  //创建一个节点
	if (count == 0) {
		pNext = malloc(sizeof(struct Command));
	}
	else {
		pNext -> pNext = malloc(sizeof(struct Command));
		pNext = pNext -> pNext;
	}
	strcpy(pNext -> command, command);
	return pNext;
}

struct Command * filelist(struct Command *pHead) {
	DIR * dp = NULL;
	struct dirent * name;  //文件夹指针
	int count = 0;
	struct Command *pNext = pHead;

	dp = opendir(pNext -> command);
	if (!dp) {
		perror(pNext -> command);
		return NULL;
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
		pNext = mkpoint(count, pNext, name -> d_name);
		if (count == 0) {
			pHead = pNext;
		}
		count++;
		name = readdir(dp);
	}
	return pHead;
}

int doing(struct Command *pHead, struct Command *pFilelist[]) {
	char command[400];
	int count2 = 0;
	struct Command *pNext = pHead;
	pid_t pid;
	
	while (pFilelist[count2] -> pNext != NULL) {
		strcpy(command, pNext -> command);
		count2 = 0;
		while (pNext -> pNext != NULL) {  //拼合命令
			pNext = pNext -> pNext;
			strcat(command, pNext -> command);
			if (pNext -> stat == 1) {
				strcat(command, pFilelist[count2] -> command);
				pFilelist[count2] = pFilelist[count2] -> pNext;
				count2++;
			}
		}
		pNext = pHead;
		count2 = 0;
		pid = fork();
		if (pid == 0) {
			printf("command: %s\n",command);
			if (system(command) != 0) {
				printf("\033[1;31m[system]: \033[1;31m%s\033[1;32m\n",command);
				perror(command);
				printf("\033[0m");
				exit(-1);
			}
			exit(0);
		}
	}
	exit(0);
}
