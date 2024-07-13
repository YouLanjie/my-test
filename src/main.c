/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：main.c
 *   创 建 者：youlanjie
 *   创建日期：2023年08月11日
 *   描    述：主程序
 *
 */


#include "include.h"

static void install_tips() {
	printf("\033[0;1;32m==> \033[0;1m安装提示\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(1) iwd联网并ping检查\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(2) timedatectl set-ntp true\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(3) vim /etc/pacman.conf\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(4) 分区格式化挂载\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(5) pacstrap -i /path/ <package_name...>\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(6) genfstab -U /mnt/ >> /path/etc/fstab\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(7) arch-chroot /path/ /path/to/shell\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(8) 或者使用快速脚本\033[0m\n"
	       "\033[0;1;32m==> \033[0;1m按下任意按键返回\033[0m\n"
	       );
	_getch();
}

static void menu()
{
	int input = 0;
	int flag = 1;
	while (input != 'q') {
		if (flag) {
			printf("\033[0;1;32m==> \033[0;1m主菜单\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m0) 备份\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m1) Archiso配置\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m2) 新系统的安装配置\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1mq) 退出程序\033[0m\n"
			       );
		}
		input = _getch();
		flag = 1;
		switch (input) {
		case '0':
			printf("\033[0;1;32m==> \033[0;1m建议使用备份脚本\033[0m\n");
			return;
			break;
		case '1':
			install_tips();
			return;
			break;
		case '2':
			menu_installed();
			return;
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

static void help(int ret)
{
	printf("Usage: main [options]\n"
	       "Options:\n"
	       "     -h  帮助信息\n"
	       "     -l  包选择列表\n"
	       "     -m  主菜单\n"
	       );
	exit(ret);
	return;
}

int main(int argc, char *argv[])
{
	int ch = 0;
	while ((ch = getopt(argc, argv, "hlm")) != -1) {
		switch (ch) {
		case 'h':
			help(0);
			break;
		case '?':
			help(-1);
			break;
		case 'l':
			package_select();
			break;
		case 'm':
			menu();
			break;
		default:
			break;
		}
	}
	return 0;
}

