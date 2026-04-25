/*
 *   Copyright (C) 2025 Chglish
 *
 *   文件名称：sin.c
 *   创 建 者：Chglish
 *   创建日期：2025年04月05日
 *   描    述：尝试终端绘制sin图像
 *
 */


#include <stdio.h>
#include <math.h>
#include <string.h>

int main(int argc, char *argv[])
{
	double A = 10,
	       omega = 1,
	       phi = 0,
	       x = 0;
	double x0 = 0;
	if (argc > 1) {
		if (strcmp(argv[1],"-h") == 0) {
			printf("Usage: %s <amplitude=10> <omega=1> <phi=0> <x0=0>\n"
			       "       %s [-h]\n", argv[0], argv[0]);
			return 0;
		}
		sscanf(argv[1], "%lf %lf %lf %lf", &A, &omega, &phi, &x0);
		A = A<0 ? -A : A;
	}
	for (int i = A; i > -A; i--) {
		for (x = x0; x <= 10; x+=0.1) {
			if (fabs(A*sin(omega*x+phi) - i) < 0.1*omega+0.9) printf("#");
			else printf(" ");
		}
		printf("\n");
	}
}


