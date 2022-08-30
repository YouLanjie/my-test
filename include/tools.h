/* File name: tools.h */
/* This is a head file */

#ifndef _TOOLS_H_
#define _TOOLS_H_

/* include head file */
#include <stdio.h>
#include <stdlib.h>
/* system() srand() rand() malloc() free() exit() */
#include <unistd.h>
/* pause() */
#include <string.h>
/* strcat() strcmp() strcpy() */
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
/* signal() */
#include <stdarg.h>

/* 预定义Linux要用到的东西 */
#ifdef __linux
#include <sys/ioctl.h>
#include <wait.h>
#include <pthread.h>
#include <ncurses.h>
#include <locale.h>

#ifndef Clear
	#define Clear clear();
#endif
#ifndef Clear2
	#define Clear2 clear();
#endif
#ifndef fontColorSet
	#define fontColorSet(a,b) printf("\033[%d;%dm",a, b)
#endif
#ifndef gotoxy
	#define gotoxy(x,y) printf("\033[%d;%dH",x, y)
#endif

/* kbhit */
extern int kbhit();
extern int getch_old();
#endif

/* 预定义windows要用到的东西 */
#ifdef _WIN32
#include <windows.h>
#include <conio.h>

#ifndef Clear
	#define Clear printf("\033[2J\033[1;1H");
#endif
#ifndef Clear2
	#define Clear2 system("cls");
#endif
#ifndef fontColorSet
	#define fontColorSet(a,b) printf("\033[%d;%dm",a, b)
#endif
#ifndef gotoxy
	#define gotoxy(x,y) printf("\033[%d;%dH",x, y)
#endif
#endif

/* kbhit */
extern int kbhitGetchar();

/* menu */
extern int Menu(char *title, char *text[], int tl, int list);
extern void Menu2(char title[], short p, short a);
extern void Menu3(char title[]);


// The new menu function.
// ======================================================================================================================================================
struct Text {
	char        * text;         /* 条例内容 */
	char        * describe;     /* 描述/帮助信息 */
	void       (* function);    /* 调用的函数 */
	int         * var;          /* 调整的变量值 */
	int           number;       /* 编号 */
	int           cfg;          /* 类型：1数值，2开关 */
	int           foot;         /* 设置的步长 */
	int           max;          /* 设置的最大值 */
	int           min;          /* 设置的最小值 */
	struct Text * nextText;     /* 下一条例（链表） */
};                                  /* 条例结构体 */

typedef struct _menuData{
	char         *       title;                                                                      /* 标题 */
	struct Text  *       text;                                                                       /* 条例链表头 */
	struct Text  *       focus;                                                                      /* 选中的条例 */
	int                  cfg;                                                                        /* 菜单类型: 0.默认 1.仅显示主界面 2.显示帮助 3.显示设置 4.仅显示帮助，无输入处理 */
	void        (* const addText)    (struct _menuData * data, ...);                                 /* 添加条例 */
	void        (* const addTextData)(struct _menuData * data, int type, char * format, ...);        /* 添加条例信息 */
	void        (* const getFocus)   (struct _menuData * data, int number);                          /* 更改焦点指针 */
	int         (* const menuShow)   (struct _menuData * data);                                      /* 更改焦点指针 */
}menuData;                                                                                        /* 菜单类/结构体 */

extern menuData menuDataInit();

#define menuType_OnlyMain    1
#define menuType_Help        2
#define menuType_Setting     3
#define menuType_OnlyHelp    4

#define menuTextTypeNumber   1
#define menuTextTypeButton   1

#define menuTextDataDescribe 0
#define menuTextDataSetType  1
#define menuTextDataSetVar   2
#define menuTextDataSetFoot  3
#define menuTextDataSetMax   4
#define menuTextDataSetMin   5

#endif

