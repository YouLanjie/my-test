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

#ifdef _WIN32
#define LOG(fmt, ...) fprintf(stderr, "%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__ ,## __VA_ARGS__)
#else
#define LOG(fmt, ...) fprintf(stderr, "%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__ __VA_OPT__(,) __VA_ARGS__)
#endif

#define LOGVAR(typ, var) LOG("var '%s' = " typ " (as %s)", #var, (var), #typ)
#define CALL(func, ...) (LOG("call '%s' at (%p)", #func, (func)), (func)(__VA_ARGS__))
#ifndef ARRAY_LEN
#define ARRAY_LEN(v) (sizeof(v)/sizeof(v[0]))
#endif

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


typedef struct {
	int x;        /* 起始列 */
	int y;        /* 起始行 */
	int width;    /* 窗口宽度 */
	int heigh;    /* 窗口高度 */
	int hide;     /* 隐藏的行数 */
	int focus;    /* 焦点行（反色行）行号 */
	const char *color_code;    /* 背景颜色 */
} str_window_t;
/* 在指定范围内打印 */
int print_in_box(str_window_t win, const char *str);

#ifdef __linux__
/* timespec操作，高精度单步sleep */
struct timespec timespec_add(struct timespec t, struct timespec t2);
struct timespec timespec_from_sec(double t);
double sleep_fixed_step(double sec);
#endif

#endif

