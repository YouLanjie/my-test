/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：package_select.c
 *   创 建 者：youlanjie
 *   创建日期：2023年08月12日
 *   描    述：软件包选择界面
 *
 */

#include "include.h"

static void help_in() {
	printf("\033[0;0H\033[2J\033[0;0H");
	printf("\033[0;1;32m==> \033[0;1m内部界面帮助\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mh k 上一项\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mj l 下一项\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mg G 第一与最后一项\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mH   本帮助信息\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mq   退出软件包选择界面\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m` ` <空格>切换软件包选择状态\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m    灰色的不能更改，红色可更改\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mA D 选择全部与取消全部(可更改的)\033[0m\n"
	       "\033[0;1;32m==> \033[0;1m按下任意按键返回\033[0m\n"
	       );
	_getch();
	printf("\033[0;0H\033[2J\033[0;0H");
}

int package_select()
{
#define list_max 11
	struct winsize size;
	const char *ch_list[list_max] = {
		"系统",
		"开发",
		"CLI工具",
		"网络工具",
		"图形界面",
		"字体",
		"GUI工具",
		"多媒体",
		"Java",
		"游戏",
		"AUR"
	};
	struct Package *tmp = package_list,
		       *mark = tmp;
	int flag = 1;
	int count = 0, lines = 0;
	int select_num = 1;
	int select_line = 1;
	int count_select = 0;
	int hide = 0;
	int input = 0;
	printf("\033[2J");
	while (input != 'q' && input != 'Q') {
		count = 0;
		lines = 0;
		count_select = 0;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		printf("\033[0;0H\033[2J\033[0;0H");
		for (int i = 0; i < list_max ; i++) {
			tmp = package_list;
			flag = 1;
			while (tmp != NULL && tmp->name != NULL) {
				if (tmp->type != i) {
					tmp++;
					continue;
				}
				if (lines - hide <= size.ws_row - 4 && count >= hide) {    /* 打印 */
					if (flag) {
						flag = 0;
						printf("\033[0;1;32m==> \033[0;1m%s\033[0m\n", ch_list[i]);
						lines++;
					}
					char *marker = NULL;
					if (tmp->select == 1) marker = "\033[0;1;31mx";
					else if (tmp->select == -1) marker = "\033[0;1;30mx";
					else marker = " ";
					if (tmp->installed == 1) marker = "\033[0;1;32mx";
					printf("\033[0;1;34m  -> \033[0;1m[%s\033[0;1m] %s - ", marker, tmp->name);
					if (tmp->desrcibe == NULL || strcmp(tmp->desrcibe, "") == 0) printf("<NULL>\033[0m\n");
					else printf("%s\033[0m\n", tmp->desrcibe);
				}
				if (tmp->select != 0) count_select++;
				if (count + 1 == select_num)
					select_line = lines + 1;
				tmp++;
				count++;
				lines++;
			}
		}
		// printf("C:%d; L:%d; SN:%d; SL:%d; HD:%d; IN:%c\n", count, lines, select_num, select_line, hide, input);
		printf("\033[0;1;32m==> \033[0;1m[%03d/%03d] | 已选：%3d | 按`H`获取帮助\033[0m\n", select_num, count, count_select);
		printf("\033[%d;7H", select_line - hide);
		kbhitGetchar();
		input = _getch();
		switch (input) {
		case 'j':
		case 'l':
			if (select_num < count) {
				while (select_line - hide > size.ws_row - 4) hide++;
				mark++;
				select_num++;
			}
			break;
		case 'k':
		case 'h':
			if (select_num > 1) {
				while (select_line - hide <= 2) hide--;
				mark--;
				select_num--;
			}
			break;
		case ' ':
			if (mark != NULL && mark->name != NULL) {
				if (mark->select == 1) mark->select = 0;
				else if (mark->select == 0) mark->select = 1;
			}
			break;
		case 'A':
			tmp = package_list;
			while (tmp != NULL && tmp->name != NULL) {
				if (tmp->select == 0) tmp->select = 1;
				tmp++;
			}
			break;
		case 'D':
			tmp = package_list;
			while (tmp != NULL && tmp->name != NULL) {
				if (tmp->select == 1) tmp->select = 0;
				tmp++;
			}
			break;
		case 'g':
			select_num = 1;
			select_line = 1;
			hide = 0;
			mark = package_list;
			break;
		case 'G':
			for (int i = select_num; i < count; i++) mark++;
			select_num = count;
			select_line = lines;
			while (select_line - hide > size.ws_row - 4) hide++;
			break;
		case 'H':
			help_in();
			break;
		default:
			break;
		}
	}
	printf("\033[0;0H\033[2J\033[0;0H");
	// printf("\033[2J\033[0;0H%d packages and %d lines total.\n", count, lines);
	return 0;
#undef list_max
}

