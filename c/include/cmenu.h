/**
 * @file        cmenu.h
 * @author      u0_a221
 * @date        2026-04-25
 * @brief       放c菜单相关
 */

#pragma once

#ifndef _CMENU_H
#define _CMENU_H

/* 初始化Ncurses库(开启ncurses模式) */
extern void ctools_ncurses_init();

#ifdef __linux__
/*
 * 新菜单
 */
typedef void* cmenu;
/* 初始化数据 */
extern cmenu cmenu_create();
/* 设置标题 */
extern void cmenu_set_title(cmenu menu, char *title);
/* 设置菜单类型 
 * Key: "normal", "main_only", "help", "setting", "help_only" */
extern void cmenu_set_type(cmenu menu, char *type_str);
/* 添加选项 */
extern void cmenu_add_text(cmenu menu, int id, char *text, char *describe, void (*func)(), int *var, char *type, int foot, int max, int min);
/* 设置选项
 * Key: "text", "describe", "func", "var", "type", "foot", "max", "min" */
extern void cmenu_set_text(cmenu menu, int id, char *tag, void *value);
/* 移除选项 */
extern void cmenu_del_text(cmenu menu, int id);
/* 显示菜单 */
extern int cmenu_show(cmenu menu);
#endif

#endif //CMENU_H

