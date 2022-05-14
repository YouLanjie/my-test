/* File name: include.h */
/* This is a head file */

#ifndef _INCLUDE_H_
#define _INCLUDE_H_

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

#include "tools.c"

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
		#define Clear2 gotoxy(0, 0); for (int i = 0;i < 50; i++) { printf("                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    "); } gotoxy(0, 0);
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

#endif

