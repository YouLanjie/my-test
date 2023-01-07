#include "include/tools.h"

// #define USER               youlanjie
// #define SHELL              /usr/bin/zsh
// #define CONFIG             "install.conf"

//  #define COMMAND_FLAG  "# SCRIPT HELPER ENABLE"
#define COMMAND_START "# COMMAND START\n"
#define COMMAND_DID   "# COMMAND DID  \n"
#define COMMAND_END   "# COMMAND END  \n"

int reset();               /* 重置脚本 */
int run();                 /* 执行脚本 */
int readconfig(char *);    /* 读取文件信息 */
int finish();              /* 增加完成注释 */
void help();               /* 打印帮助信息 */

char *CONFIG = NULL;

int main(int argc, char * argv[]) {
	int opt   = 0,
	    stat  = 0;

	if (argc == 1) {
		help();
		return -1;
	}
	while ((opt = getopt(argc, argv, "hf:dr")) != -1) {
		switch (opt) {
		case 'h':
			help();
			return 0;
			break;
		case 'f':
			if (strcmp(optarg, "?") != 0) {
				CONFIG = optarg;
			}
			break;
		case 'd':
			if (stat == 0) {
				stat += 1;
			} else {
				stat += 2;
			}
			break;
		case 'r':
			stat += 2;
			break;
		default:
			help();
			return -1;
			break;
		}
	}
	if (argc == 1 || stat == 0 || CONFIG == NULL) {
		if (CONFIG == NULL) {
			printf("没有指定文件\n");
		} else if (stat == 0) {
			printf("没有指定有效动作-d或-r\n");
		}
		help();
		return -1;
	} else {
		switch (stat) {
		case 1:
			stat = run();
			break;
		case 2:
			stat = reset();
			break;
		case 3:
			stat = run();
			if (stat) {
				return stat;
			} else {
				stat = reset();
			}
			break;
		case 4:
			stat = reset();
			if (stat) {
				return stat;
			} else {
				stat = run();
			}
			break;
		default:
			return -1;
			break;
		}
	}
	return stat;
}

int reset() {
	char ch[100];
	long size = 0L;
	FILE *fp;

	if (access(CONFIG, 0) == 0) {
		fp = fopen(CONFIG, "r+");
	} else {
		perror("CONFIG");
		printf("\033[0;1;33mError: 文件异常\033[0m\n");
		printf("\033[0;1;33mError: \033[0;32m%s\033[0m\n",CONFIG);
		return -1;
	}
	if (!fp) {
		perror("CONFIG");
		printf("\033[0;1;33mError: 文件异常\033[0m\n");
		printf("\033[0;1;33mError: \033[0;32m%s\033[0m\n",CONFIG);
		return -1;
	}
	if (feof(fp) != 0) {
		return 1;
	}
	fp = fopen(CONFIG, "r+");
	while (feof(fp) == 0) {
		fgets(ch, 100, fp);
		while (strcmp(ch, COMMAND_DID) != 0 && feof(fp) == 0) {
			size = ftell(fp);
			fgets(ch, 100, fp);
		}
		if (feof(fp) != 0) {
			return 0;
		}
		fseek(fp, size, 0);
		fputs(COMMAND_START, fp);
	}
	fclose(fp);
	return 0;
}

int run() {
	char command[1048576];
	int stat = 0;

	while (1) {
		switch (readconfig(command)) {
			case -1:
				printf("\033[0;1;33mError: 文件异常\033[0m\n");
				printf("\033[0;1;33mError: \033[0;32m%s\033[0m\n",CONFIG);
				return -1;
				break;
			case 1:
				return 0;
				break;
		}
		printf("\033[0;1;33m-------------------------Command Begin-------------------------\n\033[0;1;33mCommand:\n\033[0;1;32m%s\033[0m", command);
		fflush(stdout);
		// sleep(1);
		stat = system(command);
		if (stat != 0) {
			printf("\033[0;1;33mError: [command]:\n\033[0;1;32m%s\033[0m\n", command);
			printf("\033[0;1;33mError: [return ]: \033[0;1;32m%d\033[0m\n", stat);
			printf("\033[0;1;33mError: [perror ]:\n\033[0;1;32m");
			fflush(stdout);
			perror(command);
			fflush(stderr);
			printf("\033[0m\n");
			return -1;
		} else {
			if (finish() == -1) {
				return -1;
			}
		}
	}
	return 0;
}

int readconfig(char *command) {
	char ch[1024];
	FILE *fp;

	if (access(CONFIG, 0) == 0) {
		fp = fopen(CONFIG, "r+");
	} else {
		perror("CONFIG");
		return -1;
	}
	if (!fp) {
		perror("CONFIG");
		return -1;
	}
	if (feof(fp) != 0) {
		return 1;
	}
	fgets(ch, 1024, fp);
	while (strcmp(ch, COMMAND_START) != 0 && feof(fp) == 0) {
		fgets(ch, 1024, fp);
	}
	if (feof(fp) != 0) {
		return 1;
	}
	fgets(ch, 1024, fp);
	strcpy(command, ch);
	fgets(ch, 1024, fp);
	while (strcmp(ch, COMMAND_START) != 0 && strcmp(ch, COMMAND_END) != 0 && feof(fp) == 0) {
		strcat(command, ch);
		fgets(ch, 1024, fp);
	}
	fclose(fp);
	return 0;
}

int finish() {
	char ch[100];
	long size = 0L;
	FILE *fp;

	fp = fopen(CONFIG, "r+");
	fgets(ch, 100, fp);
	while (strcmp(ch, COMMAND_START) != 0 && feof(fp) == 0) {
		size = ftell(fp);
		fgets(ch, 100, fp);
	}
	if (feof(fp) != 0) {
		perror("Add commit");
		return -1;
	}
	fseek(fp, size, 0);
	fputs(COMMAND_DID, fp);
	fclose(fp);
	return 0;
}

void help() {
	printf("Usage: script-helper <options>\n");
	printf("	-f  指定脚本文件\n");
	printf("	-d  执行脚本\n");
	printf("	-r  重置脚本\n");
	printf("	-h  打印帮助信息并退出\n");
	return;
}

