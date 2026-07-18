/*
 *   Copyright (C) 2024 u0_a99
 *
 *   文件名称：print_in_box.c
 *   创 建 者：u0_a99
 *   创建日期：2024年03月09日
 *   描    述：
 *
 */


#define _GNU_SOURCE
#include "../include/tools.h"
#include <wchar.h>

#define TAB_WIDTH 8

static void check_border(str_window_t *win)
{
	if (!win) return;
	const int col = get_winsize_col();
	const int row = get_winsize_row();

	win->x = win->x > col ? 1 : win->x;
	win->y = win->y > row ? 1 : win->y;
	if (win->x < 1) win->x = 1;
	if (win->y < 1) win->y = 1;

	win->width = win->width + win->x > col ? col - win->x + 1 : win->width;
	win->heigh = win->heigh + win->y > row ? row - win->y + 1 : win->heigh;
	if (win->width < 0) win->width = col - win->x + 1;
	if (win->heigh < 0) win->heigh = row - win->y + 1;
}

/* @brief 在指定范围内打印文本
 * （基于宽字符和wcwidth）
 * 需要 setlocale(LC_ALL, ""); 让宽字符解释正常工作
 * @return 剩余字节数 */
int print_in_box(str_window_t win, const char *str)
{
	check_border(&win);
	if (!win.color_code) win.color_code = "\033[0m";
	if (!str) str = "(null)";

	// 清屏
	if (!win.follow_end) {
		printf("%s", win.color_code);
		for (int i1 = win.y; i1 < win.y + win.heigh; i1++) {
			for (int i2 = win.x; i2 < win.x + win.width; i2++) {
				printf("\033[%d;%dH ", i1, i2);
			}
		}
		printf("\033[%d;%dH", win.y - (win.hide > 0 ? 0 : win.hide), win.x);
	}

	size_t size = strlen(str);
	size_t position = 0;
	const char *fallback = NULL;
	wchar_t wc = L'\0';
	mbstate_t state = (mbstate_t){};
	size_t len = 0;
	int width = 0;
	int column = 0, line = 0;
	while (position < size && (line - win.hide < win.heigh || win.follow_end)) {
		len = mbrtowc(&wc, str+position, size-position, &state);
		if (len == (size_t)-1 || len == (size_t)-2 || len == 0) {
			// -1 无效的多字节序列：按一个字节处理，视觉宽度视为1
			// -2 剩余字节不完整（不应发生在完整字符串中），跳出
			//  0 遇到空字符（L'\0'），通常不会出现在字符串中间，将其视为宽度1
			len = 1;
			width = 1;
			wc = L'?';
			// 重置状态，尝试从下一个字节重新同步
			state = (mbstate_t){};
		} else if (wc == L'\b') {
			width = -1;
		} else if (wc == L'\e') {
			fallback = "<ESC>";
			width = strlen(fallback);
		} else if (wc == L'\t') {
			width = TAB_WIDTH - (win.x-1+column) % TAB_WIDTH;
		} else if (wc == L'\r' || wc == L'\n') {
			width = -column;
		} else {
			width = wcwidth(wc);
		}
		if (width > win.width) {
			wc = L'?';
			fallback = NULL;
			width = 1;
		}
		column += width;

#define cond_print (!win.follow_end && line >= win.hide && line - win.hide < win.heigh)
		if (column > win.width || wc == L'\r' || wc == L'\n') {
			line++;
			if (cond_print)
				printf("\033[%d;%dH",
				       win.y + line - (win.hide > line ? 0 : win.hide),
				       win.x);
			if (column <= win.width) {
				column = 0;
				position += len;
				continue;
			}
			column = width >= 0 ? width : 0;
		}
		if (cond_print) {
			if (line == win.focus) printf("\033[7m");
			if (fallback) {
				printf("%s", fallback);
				fallback = NULL;
			} else printf("%lc", wc);
			if (line == win.focus) printf("%s", win.color_code);
		}
		position += len;
	}
	if (!win.follow_end) {
		printf("\033[0m");
		fflush(stdout);
	} else {
		/* 通过两次计算测得最后长度 */
		if (line+1 > win.heigh && win.hide < line+1-win.heigh) win.hide = line+1-win.heigh;
		win.follow_end = false;
		print_in_box(win, str);
	}
	return (int)position-(int)size;
}

