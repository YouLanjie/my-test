#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ===================== 通用 ======================= */

/* ===================== Linux ======================= */
#ifdef __linux
	#include <termios.h>
	#include <fcntl.h>
	#include <sys/ioctl.h>
	#include <asm-generic/ioctls.h>

#ifndef fontColorSet
	#define fontColorSet(a,b) printf("\033[%d;%dm",a, b)
#endif
#ifndef gotoxy
	#define gotoxy(x,y) printf("\033[%d;%dH",x, y)
#endif

	/* 判断有没有输入 */
	extern int kbhit() {
		struct termios oldt, newt;
		int ch;
		int oldf;
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
		ch = getchar();
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
		fcntl(STDIN_FILENO, F_SETFL, oldf);
		if(ch != EOF) {
			ungetc(ch, stdin);
			return 1;
		}
		return 0;
	}

	/* 不循环输入 */
	extern int kbhitGetchar() {
		struct termios oldt, newt;
		int ch;
		int oldf;
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
		ch = getchar();
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
		fcntl(STDIN_FILENO, F_SETFL, oldf);
		if(ch != EOF) {
			ungetc(ch, stdin);
			return ch;
		}
		return 0;
	}

	extern int getch() {
		struct termios tm, tm_old;
		int fd = 0, ch;
	 
		if (tcgetattr(fd, &tm) < 0) {                   //保存现在的终端设置
			return -1;
		}
		tm_old = tm;
		cfmakeraw(&tm);                                 //更改终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理
		if (tcsetattr(fd, TCSANOW, &tm) < 0) {          //设置上更改之后的设置
			return -1;
		}
		ch = getchar();
		if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {      //更改设置为最初的样子
			return -1;
		}
		return ch;
	}
	void Menu2(char title[], short pages, short allPages) {   /* 菜单#1 */
		struct winsize size;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		int winSizeCol = size.ws_col;

		printf("\033[2;32m\033[6;%dH↑\033[10;%dH↓\033[11;%dH\033[2;32m%d/%d\033[1;33m", winSizeCol / 2 - 1, winSizeCol / 2 - 1, winSizeCol / 2 + 25, pages,allPages);
		printf("\033[2;%dH\033[1;32m%s", winSizeCol / 2 - (int)strlen(title) / 2, title);
		printf("\033[5;%dH\033[34m--------------------------------------------------------\033[34m", winSizeCol / 2 - 27);
		for (int i = 0; i < 7; i++) {
			gotoxy(i + 6, winSizeCol / 2 - 27);
			printf("|\033[54C|");
		}
		printf("\033[13;%dH--------------------------------------------------------\033[0m", winSizeCol / 2 - 27);
		printf("\033[11;%dH\033[1;31m请选择:\033[0m", winSizeCol / 2 - 23);
		return;
	}
	void Menu3(char title[]) {  /* 菜单#2 */
			struct winsize size;
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
			int winSizeCol = size.ws_col;

			printf("\033[2;%dH\033[1;32m%s", winSizeCol / 2 - (int)strlen(title) / 2, title);
			printf("\033[5;%dH\033[34m--------------------------------------------------------", winSizeCol / 2 - 27);
			for (int i = 0; i < 7; i++) {
				gotoxy(i + 6, winSizeCol / 2 - 27);
				printf("|\033[54C|");
			}
			printf("\033[13;%dH--------------------------------------------------------\033[0m", winSizeCol / 2 - 27);
			printf("\033[11;%dH\033[1;31m按任意按键返回：\033[0m", winSizeCol / 2 - 23);
		return;
	}
#endif
/* ===================== End ======================= */

/* ===================== Windows ======================= */
#ifdef _WIN32
	#include <windows.h>
	#include <conio.h>

	void gotoxy(int x,int y) { //光标定位函数
		COORD p;//定义结构体变量p
		HANDLE handle=GetStdHandle(STD_OUTPUT_HANDLE);//获取当前函数句柄
		p.X = y;
		p.Y = x;   //将光标的目标移动位置传递给结构体
		SetConsoleCursorPosition(handle,p);//移动光标
	}
	int kbhitGetchar() {
		if (kbhit()) {
			return getch();
		}
		else {
			return 0;
		}
	}
	void Menu2(char title[], short pages, short allPages) {
		int winSizeCol = 56;

		gotoxy(6, winSizeCol / 2 - 1); printf("↑"); gotoxy(10, winSizeCol / 2 - 1); printf("↓"); gotoxy(11, winSizeCol / 2 + 24); printf("%d/%d", pages,allPages);
		gotoxy(2, winSizeCol / 2 - (int)strlen(title) / 2); printf("%s", title);
		gotoxy(5, winSizeCol / 2 - 27); printf("--------------------------------------------------------");
		for (int i = 0; i < 7; i++) {
			gotoxy(i + 6, winSizeCol / 2 - 27);
			printf("|");
			gotoxy(i + 6, winSizeCol / 2 + 27);
			printf("|");
		}
		gotoxy(13, winSizeCol / 2 - 27); printf("--------------------------------------------------------");
		gotoxy(11, winSizeCol / 2 - 23); printf("请选择:");
	}
	void Menu3(char title[]) {
		int winSizeCol = 56;

		gotoxy(2, winSizeCol / 2 - (int)strlen(title) / 2); printf("%s", title);
		gotoxy(5, winSizeCol / 2 - 27); printf("--------------------------------------------------------");
		for (int i = 0; i < 7; i++) {
			gotoxy(0, 0);gotoxy(i + 6, winSizeCol / 2 - 27);
			printf("|");
			gotoxy(i + 6, winSizeCol / 2 + 27);
			printf("|");
		}
		gotoxy(13, winSizeCol / 2 - 27); printf("--------------------------------------------------------");
		gotoxy(11, winSizeCol / 2 - 23); printf("按任意按键返回：");
	}
#endif

#ifdef __linux
	#ifndef Clear
		#define Clear printf("\033[2J\033[1;1H");
	#endif
	#ifndef Clear2
		#define Clear2 system("clear");
	#endif
#endif
#ifdef _WIN32
	#ifndef Clear
		#define Clear gotoxy(0, 0); for (int i = 0;i < 50; i++) { printf("                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    "); } gotoxy(0, 0);
		#define Clear2 gotoxy(0, 0); for (int i = 0;i < 50; i++) { printf("                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    "); } gotoxy(0, 0);
	#endif
#endif

int Menu(char *title, char *text[], int tl, int list) {
#ifdef __linux
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	int winSizeCol = size.ws_col;
#else
	int winSizeCol = 56;
#endif
	int input = 1, currentPage = 1, count = 1, allPages = (tl - 1) / 6 + 1;

	if (list <= 0 || list > 2) {
		list = 2;
	}

	while (input != 0x30 && input != 0x1B) {
		Clear2
#ifdef __linux
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		winSizeCol = size.ws_col;
#endif

#ifdef __linux
		printf("\033[5;%dH\033[34m--------------------------------------------------------\033[34m", winSizeCol / 2 - 27);
		for (int i = 0; i < 7; i++) {
			gotoxy(i + 6, winSizeCol / 2 - 27);
			printf("|\033[54C|");
		}
		printf("\033[13;%dH--------------------------------------------------------\033[0m", winSizeCol / 2 - 27);
		printf("\033[2;32m\033[6;%dH↑\033[11;%dH↓\033[11;%dH\033[2;32m%d/%d\033[1;33m", winSizeCol / 2 - 1, winSizeCol / 2 - 1, winSizeCol / 2 + 25, currentPage,allPages);
		printf("\033[2;%dH\033[1;32m%s\033[0m", winSizeCol / 2 - (int)strlen(title) / 2, title);
#else
		gotoxy(6, winSizeCol / 2 - 1); printf("↑"); gotoxy(11, winSizeCol / 2 - 1); printf("↓"); gotoxy(11, winSizeCol / 2 + 24); printf("%d/%d", currentPage,allPages);
		gotoxy(2, winSizeCol / 2 - (int)strlen(title) / 2); printf("%s", title);
		gotoxy(5, winSizeCol / 2 - 27); printf("--------------------------------------------------------");
		for (int i = 0; i < 7; i++) {
			gotoxy(i + 6, winSizeCol / 2 - 27);
			printf("|");
			gotoxy(i + 6, winSizeCol / 2 + 27);
			printf("|");
		}
		gotoxy(13, winSizeCol / 2 - 27); printf("--------------------------------------------------------");
#endif
		for (int i = 1; i <= tl - list * 4 * (currentPage - 1) && i <= list * 4; i++) {
			if (i != count) {
#ifdef __linux
				printf("\033[33m\033[%d;%dH%s\033[0m", (i + 1) / list + 6, winSizeCol / list - 20 + ((i + 1) % list) * 32, text[i - 1 +  list * 4 * (currentPage - 1)]);
#else
				gotoxy((i + 1) / list + 6, winSizeCol / list - 20 + ((i + 1) % list) * 32);
				printf("%s", text[i - 1 +  list * 4 * (currentPage - 1)]);
#endif
			}
			else {
#ifdef __linux
				printf("\033[1;7;33m\033[%d;%dH%s\033[0m", (i + 1) / list + 6, winSizeCol / list - 20 + ((i + 1) % list) * 32, text[i - 1 +  list * 4 * (currentPage - 1)]);
#else
				gotoxy((i + 1) / list + 6, winSizeCol / list - 20 + ((i + 1) % list) * 32 - list);
				printf("> %s", text[i - 1 +  list * 4 * (currentPage - 1)]);
#endif
			}
			kbhitGetchar();
		}
		input = getch();
		if (input == 0x1B) {
			if (kbhit()) {
				getchar();
				input = getchar();
				if (input == 'A') {
					if (count > list) {
						count -= list;
					}
					else if (currentPage > 1){
						count += list * 3;
						currentPage--;
					}
				}
				else if (input == 'B') {
					if (count < list * 4 - 1 && count + list * 4 * (currentPage - 1) <= tl - list) {
						count += list;
					}
					else if (currentPage < allPages){
						count -= list * 3;
						currentPage++;
						if (count + 8 * (currentPage - 1) > tl) {
							count -= 1;
						}
					}
				}
				else if (input == 'C') {
					if (count < list * 4 && count + list * 4 * (currentPage - 1) < tl) {
						count++;
					}
					else if (currentPage < allPages){
						count = 1;
						currentPage++;
					}
				}
				else if (input == 'D') {
					if (count > 1) {
						count--;
					}
					else if (currentPage > 1){
						count = list * 4;
						currentPage--;
					}
				}
			}
			else {
				Clear2
				return 0;
			}
		}
		else if (input == 'w' || input == 'W') {
			if (count > list) {
				count -= list;
			}
			else if (currentPage > 1){
				count += list * 3;
				currentPage--;
			}
		}
		else if (input == 's' || input == 'S') {
			if (count < 7 && count + 8 * (currentPage - 1) <= tl - list) {
				count += list;
			}
			else if (currentPage < allPages){
				count -= list * 3;
				currentPage++;
				if (count + 8 * (currentPage - 1) > tl) {
					count -= 1;
				}
			}
		}
		else if (input == 'd' || input == 'D') {
			if (count < 8 && count + 8 * (currentPage - 1) < tl) {
				count++;
			}
			else if (currentPage < allPages){
				count = 1;
				currentPage++;
			}
		}
		else if (input == 'a' || input == 'A') {
			if (count > 1) {
				count--;
			}
			else if (currentPage > 1){
				count = 8;
				currentPage--;
			}
		}
		else if (input == 'q' || input == 'Q') {
			Clear
			return '0';
		}
		else if (input == ' ' || input == '\r' || input == '\n') {
			Clear2
			char output[10];
			sprintf(output, "%d", count + 8 * (currentPage - 1));
			return output[0];
		}
		else {
			Clear2
			return input;
		}
	}
	return 0;
}

