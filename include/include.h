/* File name: include.h */
/* This is a head file */

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

#ifndef Clear
	#define Clear printf("\033[2J\033[1;1H");
#endif
#ifndef Clear2
	#define Clear2 system("clear");
#endif

/* kbhit */
#include "../tool/kbhit.c"
int kbhit();
int input();
int kbhit_if();
int kbhit2();

/* menu */
#include "../tool/menu.c"
#ifndef Menu
	#define Menu printf("\033[0m\033[11;11H");
#endif
#ifndef Menu2
	#define Menu2 printf("\033[0m\033[11;19H");
#endif
void menu(char title[50], short p, short a);
void menu2(char a[50]);

/* pid */
/* #include <sys/types.h> */
/* pid_t */
#include <signal.h>
/* signal() */
#include <wait.h>

