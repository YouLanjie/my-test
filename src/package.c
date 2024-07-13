/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：package.c
 *   创 建 者：youlanjie
 *   创建日期：2023年08月12日
 *   描    述：软件包通用操作
 *
 */

#include "include.h"

int cmd_query(struct Package *p)
{
	char command[1024] = "pacman -Q ";
	if (p != NULL && p->name != NULL) {
		strcat(command, p->name);
		return (system(command) == 0 ? 1 : 0);
	}
	return -1;
}

int cmd_pacman()
{
	char command[8192] = "sudo pacman -S ";
	struct Package *tmp = package_list;
	int count = 0;
	while (tmp != NULL && tmp->name != NULL) {
		if (tmp->type != 10 && (tmp->select == -1 || tmp->select == 1) && tmp->installed == 0) {
			strcat(command, tmp->name);
			strcat(command, " ");
			count++;
		}
		tmp++;
	}
	if (count) {
		printf("\033[0;1;32m==> \033[0;1m输出命令\033[0m\n"
		       "\033[0;1;34m  -> \033[0;1m%s\033[0m\n",
		       command
		       );
		int result = system(command);
		if (result) {
			printf("\033[0;1;33m==> \033[0;1m命令执行错误\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m退出码：%d\033[0m\n", result);
		}
		return result;
	}
	printf("\033[0;1;32m==> \033[0;1m没有需要安装的包\033[0m\n");
	return 0;
}

int cmd_yay()
{
	int exit_code = 0;
	char command[8192] = "yay -S ";
	struct Package *tmp = package_list;
	int count = 0;
	while (tmp != NULL && tmp->name != NULL && exit_code == 0) {
		if (tmp->type == 10 && (tmp->select == -1 || tmp->select == 1) && tmp->installed == 0) {
			strcpy(command, "yay -S ");
			strcat(command, tmp->name);
			printf("\033[0;1;32m==> \033[0;1m输出命令\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m%s\033[0m\n",
			       command
			       );
			exit_code = system(command);
			count++;
			if (exit_code == 0) tmp->installed = 1;
		}
		tmp++;
	}
	if (count == 0) printf("\033[0;1;32m==> \033[0;1m没有需要安装的包\033[0m\n");
	if (exit_code) {
		printf("\033[0;1;33m==> \033[0;1m命令执行错误\033[0m\n"
		       "\033[0;1;34m  -> \033[0;1m退出码：%d\033[0m\n", exit_code);
	}
	return exit_code;
}

int cmd_update()
{
	printf("\033[0;1;32m==> \033[0;1m更新Pacman缓存\033[0m\n");
	return system("sudo pacman -Sy");
}

