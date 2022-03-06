#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
	void Menu(char title[], short pages, short allPages) {   /* 菜单#1 */
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

	void Menu2(char title[]) {  /* 菜单#2 */
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
	void Menu(char title[], short pages, short allPages) {
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
	void Menu2(char title[]) {
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

