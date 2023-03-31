/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：command.c
 *   创 建 者：youlanjie
 *   创建日期：2023年03月25日
 *   描    述：尝试制作一个简单的命令行程序
 *
 */

#include "include/head.h"

struct Cmd {
	char *name;
	char *describe;
	int (*v)();
	struct Cmd *next;
};

static int help();
static int quit();
static int shell();

struct Cmd Command_list[] = {
	{"help",    "Print all command and describe.",        help,         &Command_list[1]},
	{"man",     "Print the help manual for echo command", NULL,         &Command_list[2]},
	{"version", "menu program v0.0.1 alpha",              NULL,         &Command_list[3]},
	{"quit",    "Exit the program.",                      quit,         &Command_list[4]},
	{"exit",    "Exit the program.",                      quit,         &Command_list[5]},
	{"CPU",     "Using your cpu.",                        CPU,          &Command_list[6]},
	{"shell",   "Run shell.",                             shell,        &Command_list[7]},
	{"fork",    "Run shell by fork.",                     shell_f,      &Command_list[8]},
	{"hex",     "Print the input by hex.",                input_to_hex, NULL},
};

int main(void)
{
	char cmd[CMD_MAX_LEN] = "help";
	struct Cmd *tmp;
	while (1) {
		printf("\033[1;32mPlease input a command >\033[0m ");
		scanf("%s", cmd);
		tmp = Command_list;
		while (tmp != NULL) {
			if (strcmp(cmd, tmp->name) == 0) {
				printf("%s -- %s\n", tmp->name, tmp->describe);
				if (tmp->v != NULL) {
					tmp->v();
				}
				break;
			}
			tmp = tmp->next;
		}
		if (tmp == NULL) {
			printf("Someting is wrong.  --  \033[1;31m:(\033[0m\n"
			       "Try input `help`\n");
		}
	}
	return 0;
}

static int help()
{
	struct Cmd *p = Command_list;
	printf("\033[1;33mCommand list:\033[0m\n");
	while (p != NULL) {
		printf("%s -- %s\n", p->name, p->describe);
		p = p->next;
	}
	return 0;
}

static int quit()
{
	exit(0);
}

static int shell()
{
	char cmd[CMD_MAX_LEN] = "exit";
	printf("shell command > ");
	getchar();
	fgets(cmd, CMD_MAX_LEN, stdin);
	printf("\033[1;33mExec: %s\033[0m\n", cmd);
	system(cmd);
	return 0;
}

