#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int kbhit() {
	struct termios oldt, newt;
	int ch;
	int oldf;
	do {
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
			return ch;                      //有键入字符返回键入字符
		}
	} while (ch == EOF);                            //如果没有键入字符则重来一次
	return 0;
}

int input() {
	int a;
	a = kbhit();
	getchar();
	return a;
}

void inputp(int *p) {
	*p = kbhit();
	getchar();
}

/*
这个kbhit函数用于input输入
在调用的时候将返回值赋值给一个int变量
可用那个变量判断
输入字符时毫无延迟
由原本的kbhit.h根据需要修改得来
*/

