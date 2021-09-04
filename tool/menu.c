#include <asm-generic/ioctls.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* 菜单#1 */
void Menu(char title[], short pages, short allPages) {
	struct winsize size;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	printf("\033[2;32m\033[6;%dH↑\033[10;%dH↓\033[11;%dH\033[2;32m%d/%d\033[1;33m", size.ws_col / 2 - 1, size.ws_col / 2 - 1, size.ws_col / 2 + 25, pages,allPages);
	printf("\033[2;%dH\033[1;32m%s", size.ws_col / 2 - (int)strlen(title) / 2, title);
	printf("\033[5;%dH\033[34m--------------------------------------------------------", size.ws_col / 2 - 27);
	printf("\033[6;%dH\033[34m|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[7;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[8;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[9;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[10;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[11;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[12;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[13;%dH--------------------------------------------------------\033[0m\033[11;11H", size.ws_col / 2 - 27);
	printf("\033[11;%dH\033[1;31m请选择:\033[0m\033[11;11H", size.ws_col / 2 - 23);
	return;
}

/* 菜单#2 */
void Menu2(char title[50]) {
	struct winsize size;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	printf("\033[2;%dH\033[1;32m%s", size.ws_col / 2 - (int)strlen(title) / 2, title);
	printf("\033[5;%dH\033[34m--------------------------------------------------------", size.ws_col / 2 - 27);
	printf("\033[6;%dH\033[34m|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[7;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[8;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[9;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[10;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[11;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[12;%dH|\033[54C|", size.ws_col / 2 - 27);
	printf("\033[13;%dH--------------------------------------------------------\033[0m\033[11;11H", size.ws_col / 2 - 27);
	printf("\033[11;%dH\033[1;31m按任意按键返回：\033[0m\033[11;19H", size.ws_col / 2 - 23);
	return;
}
