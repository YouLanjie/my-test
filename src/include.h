/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：include.h
 *   创 建 者：youlanjie
 *   创建日期：2023年08月12日
 *   描    述：头文件
 *
 */


#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#include "../include/tools.h"

struct Package {
	const int type;
	const char *name;
	const char *desrcibe;
	int select;
	int installed;
};

extern struct Package package_list[];

/* 选择包 */
int package_select();
/* 通用功能 */
int cmd_query(struct Package *p);
int cmd_pacman();
int cmd_yay();
int cmd_update();
/* 安装后 */
void menu_installed();
/* 选择命令 */
void command_select();
/* 运行命令 */
int command_run();

#endif

