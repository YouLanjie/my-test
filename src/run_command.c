/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：run_command.c
 *   创 建 者：youlanjie
 *   创建日期：2023年08月12日
 *   描    述：
 *
 */

#include "include.h"

struct Command {
	/* 标记，是否执行过 */
	int mark;
	/* 类型: 0:错误重试 1:出错不重试 */
	int type;
	const char *cmd;
} command[] = {
	/* 服务类 */
	{0, 1, "sudo systemctl enable atd"},
	{0, 0, "sudo systemctl enable dhcpcd"},
	{0, 1, "sudo systemctl enable firewalld"},
	{0, 0, "sudo systemctl enable iwd"},
	{0, 1, "sudo systemctl enable syslog-ng@default.service"},
	{0, 1, "sudo systemctl enable systemd-resolvconf"},
	/* Java链接 */
	{0, 1, "sudo ln -s /usr/lib/jvm/java-8-openjdk/bin/java /usr/bin/java8"},
	{0, 1, "sudo ln -s /usr/lib/jvm/java-11-openjdk/bin/java /usr/bin/java11"},
	{0, 1, "sudo ln -s /usr/lib/jvm/java-17-openjdk/bin/java /usr/bin/java17"},
	{0, 1, "sudo rm /usr/lib/jvm/default /usr/lib/jvm/default-runtime"},
	{0, 1, "sudo ln -s /usr/lib/jvm/java-11-openjdk/ /usr/lib/jvm/default-runtime"},
	/* npm包 */
	{0, 0, "sudo npm install -g http-server"},
	{0, 0, "sudo npm install -g neovim"},
	/* pip包 */
	{0, 0, "sudo pip install neovim"},
	/* 用户 */
	{0, 0, "sudo useradd -m Chglish"},
	{0, 0, "sudo groupadd sudo"},
	{0, 0, "sudo usermod Chglish -aG video"},
	{0, 0, "sudo usermod Chglish -aG sudo"},
	{0, 0, "sudo usermod Chglish -aG games"},
	{0, 0, "sudo usermod root -aG video"},
	/* 美化资源 */
	{0, 0, "# 请确认资源存在！"},
	{0, 0, "sudo tar -xzf res/Data.tar.gz -C /"},
	{0, 0, "sudo tar -xzf res/root_after.tar.gz -C /"},
	{0, 0, "# Tips: Documentation Pictures Videos"},
	{0, 0, NULL},
};

void command_select()
{
	struct winsize size;
	struct Command *tmp = command,
		       *mark = command;
	int count = 0;
	int select_num = 1;
	int hide = 0;
	int input = 0;
	printf("\033[2J");
	while (input != 'q' && input != 'Q') {
		count = 0;
		tmp = command;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		printf("\033[0;0H\033[2J\033[0;0H");
		printf("\033[0;1;32m==> \033[0;1m请标记无需执行的命令\033[0m\n");
		while (tmp != NULL && tmp->cmd != NULL) {
			if (count - hide <= size.ws_row - 4 && count >= hide) {    /* 打印 */
				char *marker = NULL;
				if (tmp->mark == 1) marker = "\033[0;1;31mx";
				else marker = " ";
				printf("\033[0;1;34m  -> \033[0;1m[%s\033[0;1m] `%s`\n", marker, tmp->cmd);
			}
			tmp++;
			count++;
		}
		// printf("C:%d; L:%d; SN:%d; SL:%d; HD:%d; IN:%c\n", count, lines, select_num, select_line, hide, input);
		printf("\033[0;1;32m==> \033[0;1m[%03d/%03d]\033[0m\n", select_num, count);
		printf("\033[%d;7H", select_num - hide + 1);
		kbhitGetchar();
		input = _getch();
		switch (input) {
		case ' ':
			if (mark != NULL && mark->cmd != NULL) {
				if (mark->mark == 1) mark->mark = 0;
				else if (mark->mark == 0) mark->mark = 1;
			}
		case 'j':
		case 'l':
			if (select_num < count) {
				while (count - hide > size.ws_row - 4) hide++;
				mark++;
				select_num++;
			}
			break;
		case 'k':
		case 'h':
			if (select_num > 1) {
				while (count - hide <= 2) hide--;
				mark--;
				select_num--;
			}
			break;
		case 'g':
			select_num = 1;
			hide = 0;
			mark = command;
			break;
		case 'G':
			for (int i = select_num; i < count; i++) mark++;
			select_num = count;
			while (count - hide > size.ws_row - 4) hide++;
			break;
		default:
			break;
		}
	}
	printf("\033[0;0H\033[2J\033[0;0H");
}

int command_run()
{
	struct Command *tmp = command;
	int flag = 0;
	int count = 0;
	printf("\033[0;1;32m==> \033[0;1m执行命令\033[0m\n");
	while (tmp != NULL && tmp->cmd != NULL) {
		if (tmp->mark != 1) {
			printf("\033[0;1;34m  -> \033[0;1m%s\n", tmp->cmd);
			flag = system(tmp->cmd);
			if (!flag) tmp->mark = 1;
			else {
				printf("\033[0;1;33m==> \033[0;1m错误，退出码：%d\n", flag);
				return flag;
			}
			count++;
		}
		tmp++;
	}
	if (!count) printf("\033[0;1;32m==> \033[0;1m无可执行命令\033[0m\n");
	return 0;
}

