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

static Arg version();
static Arg shell();

struct Cmd Command_list[] = {
	{"version", "打印程序版本",             version,      &Command_list[1]},
	{"CPU",     "Using your cpu.",        CPU,          &Command_list[2]},
	{"shell",   "Run shell.",             shell,        &Command_list[3]},
	{"fork",    "Run shell by fork.",     shell_f,      &Command_list[4]},
	{"hex",     "Print the input by hex.",input_to_hex, NULL},
};

int main(void)
{
	char cmd[CMD_MAX_LEN] = "set version=menu program v0.0.1 alpha, powder by C-head";
	cmd_list_set(Command_list);
	/* cmd_run("set version=menu program v0.0.1 alpha, powder by C-head"); */
	cmd_run(cmd);
	cmd_tui();
	return 0;
}

static Arg version()
{
	Arg arg = {.num = 0};
	arg = cmd_run("get version");
	if (arg.num != -1 && arg.ch != NULL) {
		printf("Version:\n"
		       "%s\n",
		       arg.ch);
	} else {
		printf("Version:\n"
		       "Error...No message....\n");
	}
	arg.num = 0;
	return arg;
}

static Arg shell()
{
	char cmd[CMD_MAX_LEN] = "exit";
	printf("shell command > ");
	getchar();
	fgets(cmd, CMD_MAX_LEN, stdin);
	printf("\033[1;33mExec: %s\033[0m\n", cmd);
	system(cmd);
	Arg arg = {.num = 0};
	return arg;
}
