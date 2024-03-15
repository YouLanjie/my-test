#include "include/tools.h"


int main() {
	int a = 0, b = 0, c = 0, d = 0, e = 0;

	printf("print ALL?\n");
	a = _getch();
	printf("print background colors?\n");
	b = _getch();
	printf("print all colors?\n");
	e = _getch();
	if (a == 'y' || a == 'Y') {
		printf("print ALL and background colors?\n");
		c = _getch();
		printf("print all colors in ALL?\n");
		d = _getch();
	}

	printf("\ntext:\n");
	for (int i = 0; i <= 8; i++) {
		printf("\033[%dm\\033[%dm This is text \033[0m\n", i, i);
	}

	printf("mouse:\n");
	//  getch();
	printf("\033[?25l\\033[?25l This is text(No mouse) \033[0m\n");
	//  getch();
	printf("\033[?25h\\033[?25h This is text(Has mouse) \033[0m\n");
	//  getch();

	printf("\ncolor(in front of):\n");
	for (int i = 30; i <= 37; i++) {
		printf("\033[%dm\\033[%dm This is text \033[0m\n", i, i);
	}

	if (b == 'y' || b == 'Y') {
		printf("\ncolor(background):\n");
		for (int i = 40; i <= 47; i++) {
			printf("\033[%dm\\033[%dm This is text \033[0m\n", i, i);
		}
	}

	if (e == 'y' || e == 'Y') {
		printf("color(ALL):\n");
		for (int i = 30; i <= 37; i++) {
			for (int i2 = 40; i2 <= 47; i2++) {
				printf("\033[%d;%dm\\033[%d;%dm\033[0m\t", i, i2, i, i2);
			}
			printf("\n");
		}
		printf("\n");
	}

	if (a == 'y' || a == 'Y') {
		for (int i = 0; i <= 8; i++) {
			printf("\n\033[%dm\\033[%dm\033[0m\n", i, i);
			printf("color(in front of):\n");
			for (int i2 = 30; i2 <= 37; i2++) {
				printf("\033[%d;%dm\\033[%d;%dm\033[0m\t", i, i2, i, i2);
			}
			printf("\n");
			if (c == 'y' || c == 'Y') {
				printf("color(background):\n");
				for (int i2 = 40; i2 <= 47; i2++) {
					printf("\033[%d;%dm\\033[%d;%dm\033[0m\t", i, i2, i, i2);
				}
				printf("\n");
			}
			if (d == 'y' || d == 'Y') {
				printf("color(ALL):\n");
				for (int i2 = 30; i2 <= 37; i2++) {
					for (int i3 = 40; i3 <= 47; i3++) {
						printf("\033[%d;%d;%dm\\033[%d;%d;%dm\033[0m\t", i, i2, i3, i, i2, i3);
					}
					printf("\n");
				}
				printf("\n");
			}
		}
	}
	printf("\033[0m");
	return 0;
}
