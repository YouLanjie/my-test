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
	#ifndef Clear
		#define Clear printf("\033[2J\033[1;1H");
	#endif
	#ifndef Clear2
		#define Clear2 system("clear");
	#endif
	#ifndef fontColorSet
		#define fontColorSet(a,b) printf("\033[%d;%dm",a, b)
	#endif
	#ifndef gotoxy
		#define gotoxy(x,y) printf("\033[%d;%dH",x, y)
	#endif
	/* kbhit */
	int getch();
	int kbhit();
#endif
/* 预定义windows要用到的东西 */
#ifdef _WIN32
	#include <windows.h>
	#include <conio.h>
	#ifndef Clear
		#define Clear gotoxy(0, 0); for (int i = 0;i < 50; i++) { printf("                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    "); } gotoxy(0, 0);
	#endif
	#ifndef Clear2
		#define Clear2 system("cls");
	#endif
	#ifndef fontColorSet
		#define fontColorSet(a,b) (a + b)
	#endif
	void gotoxy(int x,int y);
#endif

/* kbhit */
int kbhitGetchar();

/* menu */
int Menu(char *title, char *text[], int tl, int list);
void Menu2(char title[], short p, short a);
void Menu3(char title[]);


// The new menu function.
// ======================================================================================================================================================
struct Text {
	char        * text;         /* 条例内容 */
	char        * describe;     /* 描述/帮助信息 */
	void       (* function);    /* 调用的函数 */
	int         * var;          /* 调整的变量值 */
	int           number;       /* 编号 */
	int           cfg;          /* 类型 */
	int           foot;         /* 调整宽度 */
	struct Text * nextText;     /* 下一条例（链表） */
};                                  /* 条例结构体 */

typedef struct _menuData{
	char        * title;                                                                      /* 标题 */
	struct Text * text;                                                                       /* 条例链表头 */
	struct Text * focus;                                                                      /* 选中的条例 */
	int           cfg;                                                                        /* 菜单状态 */
	void       (* addText)    (struct _menuData * data, ...);                                 /* 添加条例 */
	void       (* addTextData)(struct _menuData * data, int type, char * format, ...);        /* 添加条例信息 */
	void       (* getFocus)   (struct _menuData * data, int number);                          /* 更改焦点指针 */
	int        (* menuShow)   (struct _menuData * data);                                      /* 更改焦点指针 */
}menuData;                                                                                        /* 菜单类/结构体 */

extern void  menuDataInit(menuData * data);

#endif

