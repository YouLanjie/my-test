/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：sys_installed.c
 *   创 建 者：youlanjie
 *   创建日期：2023年08月12日
 *   描    述：安装后配置
 *
 */

#include "include.h"

static int cmd_prepare()
{
	int result = 0;
	printf("\033[0;1;32m==> \033[0;1m安装archlinuxcn密钥\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m注意：需要启用archlinuxcn源\033[0m\n");
	result = system("sudo pacman -S archlinuxcn-keyring");
	if (result) {
		printf("\033[0;1;33m==> \033[0;1m命令执行错误\033[0m\n"
		       "\033[0;1;34m  -> \033[0;1m退出码：%d\033[0m\n", result);
	}
	return result;
}

static void cmd_mark_install()
{
	struct Package *tmp = package_list;
	int result = 0;
	while (tmp != NULL && tmp->name != NULL) {
		result = cmd_query(tmp);
		if (result != -1) tmp->installed = result;
		tmp++;
	}
	return;
}

void menu_installed()
{
	int input = 0;
	int flag = 1;
	printf("\033[0;1;32m==> \033[0;1m提示\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m请在开始前将`root_before.tar.gz`文件解压至根目录\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m示例：`tar -xzf root_before.tar.gz -C /`\033[0m\n"
	       );
	while (input != 'q') {
		if (flag) {
			printf("\033[0;1;32m==> \033[0;1m系统安装后菜单\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m0) 默认流程\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m1) 包选择界面\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m2) 更新软件包缓存\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m3) 安装archlinuxcn源的公钥\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m4) 更新软件包安装信息\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m5) 安装库软件\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m6) 安装AUR软件(需要AUR)\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m7) 选择命令\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m8) 执行命令\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1mq) 退出程序\033[0m\n"
			       );
		}
		input = _getch();
		flag = 1;
		switch (input) {
		case '0':
			if (package_select()) break;
			if (cmd_update()) break;
			if (cmd_prepare()) break;
			cmd_mark_install();
			if (cmd_update()) break;
			if (package_select()) break;
			if (cmd_pacman()) break;
			if (cmd_yay()) break;
			command_select();
			command_run();
			break;
		case '1':
			package_select();
			break;
		case '2':
			cmd_update();
			break;
		case '3':
			cmd_prepare();
			break;
		case '4':
			cmd_mark_install();
			break;
		case '5':
			cmd_pacman();
			break;
		case '6':
			cmd_yay();
			break;
		case '7':
			command_select();
			break;
		case '8':
			command_run();
			break;
		case 'q':
		case 'Q':
			return;
			break;
		default:
			flag = 0;
			break;
		}
	}
	return;
}

