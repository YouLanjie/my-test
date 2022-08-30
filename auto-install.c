#include "include/tools.h"
#include <stdio.h>

// #define USER               youlanjie
// #define SHELL              /usr/bin/zsh
// #define CONFIG             "install.conf"

int backup();                   /* 备份配置文件 */
int delete();                   /* 移除安装进度配置 */
int install();                  /* 安装软件进行配置 */
int readconfig(char *);         /* 读取文件信息 */
int clean();                    /* 增加完成注释 */
void help();                    /* 打印帮助信息 */

char *CONFIG;

int main(int argc, char * argv[]) {
	int opt, stat = 0;

	if (argc == 1) {
		help();
		return -1;
	}
	while ((opt = getopt(argc, argv, "hbirdf:")) != -1) {
		switch (opt) {
		case 'b':
			stat = backup();
			stat = stat | delete();
			break;
		case 'i':
			stat = install();
			break;
		case 'd':
		case 'r':
			stat = delete();
			break;
		case 'f':
			if (strcmp(optarg,"?") == 0) {
				help();
				return -1;
			}
			else {
				CONFIG = optarg;
			}
			break;
		case '?':
		case 'h':
			help();
			return 0;
			break;
		default:
			help();
			return -1;
			break;
		}
	}
	if (argc == 1) {
		help();
		return -1;
	}
	return stat;
}

int backup() {
	CONFIG = "./backup.conf";
	return install();
}

int delete() {
	char ch[100];
	long size;
	FILE *fp;

	fp = fopen(CONFIG, "r+");
	while (feof(fp) == 0) {
		fgets(ch, 100, fp);
		while ((ch[0] != '#' || ch[1] != '{') && feof(fp) == 0) {
			size = ftell(fp);
			fgets(ch, 100, fp);
		}
		if (feof(fp) != 0) {
			return 0;
		}
		fseek(fp, size, 0);
		fputs("{%", fp);
	}
	fclose(fp);
	return 0;
}

int install() {
	char command[1048576];
	int stat = 0;

	while (1) {
		switch (readconfig(command)) {
			case -1:
				printf("\033[0;1;33mError: 文件异常\033[0m");
				printf("\033[0;1;33mError: [perror]: \033[0;1;32m");
				fflush(stdout);
				perror(command);
				fflush(stderr);
				printf("\033[0m\n");
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
		}
		else {
			if (clean() == -1) {
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
	}
	else {
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
	while (ch[0] != '{' && feof(fp) == 0) fgets(ch, 1024, fp);
	if (feof(fp) != 0) {
		return 1;
	}
	fgets(ch, 1024, fp);
	strcpy(command, ch);
	fgets(ch, 1024, fp);
	while (ch[0] != '{' && ch[0] != '}' && feof(fp) == 0) {
		strcat(command, ch);
		fgets(ch, 1024, fp);
	}
	fclose(fp);
	return 0;
}

int clean() {
	char ch[100];
	long size;
	FILE *fp;

	fp = fopen(CONFIG, "r+");
	fgets(ch, 100, fp);
	while (ch[0] != '{' && feof(fp) == 0) {
		size = ftell(fp);
		fgets(ch, 100, fp);
	}
	if (feof(fp) != 0) {
		perror("Clean");
		return -1;
	}
	fseek(fp, size, 0);
	fputs("#{", fp);
	fclose(fp);
	return 0;
}

void help() {
	printf("hello!\ninstall -b「备份」 -i「安装」 -[rd]「重新配置」 -f「指定配置文件」 -h「打印帮助信息并退出」\n");
	return;
}

