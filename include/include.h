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
#include <sys/stat.h>
/* pass */
#include <sys/types.h>
/* pass */
#include <string.h>
/* strcat() strcmp() strcpy() */
#include <dirent.h>
#include <sys/ioctl.h>

#include "../tool/kbhit.c"
#include "../tool/menu.c"

#ifndef Clear
	#define Clear printf("\033[2J\033[1;1H");
#endif
#ifndef Clear2
	#define Clear2 system("clear");
#endif

/* kbhit */
#ifdef __linux
int getch();
int kbhit();
int kbhitGetchar();
#endif

/* menu */
void Menu(char title[50], short p, short a);
void Menu2(char title[50]);

/* pid */
/* #include <sys/types.h> */
/* pid_t */
#include <signal.h>
/* signal() */
#include <wait.h>

#endif

