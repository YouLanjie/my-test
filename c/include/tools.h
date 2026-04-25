/*
 * File name: tools.h
 * Describe : C-head公开使用头文件
 */

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
#include <sys/time.h>
#include <pthread.h>
#include <locale.h>

#ifdef __linux__
#include <sys/ioctl.h>
#include <wait.h>
#include <ncurses.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif

#define LOG(fmt, ...) fprintf(stderr, "%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__ __VA_OPT__(,) __VA_ARGS__)
#define LOGVAR(typ, var) LOG("var '%s' = " typ " (as %s)", #var, (var), #typ)
#define CALL(func, ...) (LOG("call '%s' at (%p)", #func, (func)), (func)(__VA_ARGS__))

/*
 * kbhit getch
 */
#ifdef __linux__
/* 判断有没有输入 */
extern int kbhit();
#endif
/* 利用终端特性做的getch */
extern int _getch(void);
extern int _getch_cond(int *cond);
/* 不阻塞输入 */
extern int kbhitGetchar();
/* Get the size(x) of the window(range:0~) */
extern int get_winsize_col();
/* Get the size(y) of the window(range:0~) */
extern int get_winsize_row();
/* 读取文件 */
extern char *_fread(FILE *fp);
/* 在指定范围内打印 */
extern int print_in_box(char *ch, int x_start, int y_start, int width, int heigh, int hide, int focus, char *color_code, int flag_hl);

#endif

