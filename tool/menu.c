#include <stdio.h>

void menu(char title[50], short p, short a) {
	printf("\033[2;32m\033[6;26H↑\033[10;26H↓\033[11;52H\033[2;32m%d/%d\033[1;33m",p,a);
	printf("\033[2;25H\033[1;32m%s",title);
	printf("\033[5;1H\033[34m--------------------------------------------------------");
	printf("\033[6;1H\033[34m|\033[6;56H|");
	printf("\033[7;1H|\033[7;56H|");
	printf("\033[8;1H|\033[8;56H|");
	printf("\033[9;1H|\033[9;56H|");
	printf("\033[10;1H|\033[10;56H|");
	printf("\033[11;1H|\033[11;56H|");
	printf("\033[12;1H|\033[12;56H|");
	printf("\033[13;1H--------------------------------------------------------\033[0m\033[11;11H");
	printf("\033[11;4H\033[1;31m请选择:\033[0m\033[11;11H");
	return;
}

void menu2(char title[50]) {
	printf("\033[2;25H\033[1;32m%s",title);
	printf("\033[5;1H\033[34m--------------------------------------------------------");
	printf("\033[6;1H\033[34m|\033[6;56H|");
	printf("\033[7;1H|\033[7;56H|");
	printf("\033[8;1H|\033[8;56H|");
	printf("\033[9;1H|\033[9;56H|");
	printf("\033[10;1H|\033[10;56H|");
	printf("\033[11;1H|\033[11;56H|");
	printf("\033[12;1H|\033[12;56H|");
	printf("\033[13;1H--------------------------------------------------------\033[0m\033[11;11H");
	printf("\033[11;4H\033[1;31m按任意按键返回：\033[0m\033[11;19H");
	return;
}
