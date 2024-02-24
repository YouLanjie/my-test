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

static void *version();
static void *shell();

ctools_cmd_list Command_list[] = {
	{"version", "打印程序版本",           version,      &Command_list[1]},
	{"CPU",     "Using your cpu.",        CPU,          &Command_list[2]},
	{"shell",   "Run shell.",             shell,        &Command_list[3]},
	{"fork",    "Run shell by fork.",     shell_f,      &Command_list[4]},
	{"hex",     "Print the input by hex.",input_to_hex, NULL},
};

int main(void)
{
	char cmd[1024] = "set version=menu program v0.0.1 alpha, powder by C-head";
	struct ctools ctools = ctools_init();
	ctools.cmd.cmd_list_set(Command_list);
	/* cmd_run("set version=menu program v0.0.1 alpha, powder by C-head"); */
	ctools.cmd.run(cmd);
	ctools.cmd.ui();
	return 0;
}

static void *version()
{
	char *result = NULL;
	result = ctools_init().cmd.run("get version");
	if (result != NULL && (int)*result != -1) {
		printf("Version:\n" "%s\n", result);
	} else {
		printf("Version:\n"
		       "Error...No message....\n");
	}
	static int ret = 0;
	return &ret;
}

static void *shell()
{
	char cmd[1024] = "exit";
	printf("shell command > ");
	/*getchar();*/
	fgets(cmd, 1024, stdin);
	printf("\033[1;33mExec: %s\033[0m\n", cmd);
	system(cmd);
	return NULL;
}

