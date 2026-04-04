/*
 *   Copyright (C) 2026 u0_a221
 *
 *   文件名称：sizeof.c
 *   创 建 者：u0_a221
 *   创建日期：2026年04月04日
 *   描    述：打印各种单位的尺寸大小
 *
 */


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PRINT_TABLE(s, d) printf("| %15s | %-11ld |\n", s, d)
#define PRINT_SIZEOF(t) PRINT_TABLE(#t, sizeof(t))
#define PRINT_SEP() printf("|-----------------|-------------|\n")

void gen_vla(int size)
{
	int vla[size];    /* vla会被编译器优化掉 */
	PRINT_TABLE("int vla[size]", sizeof(vla));
	/* sizeof会被展开成4*size */
	return;
}

int main(void)
{
	printf("| %15s | %11s |\n", "sizeof(TYPE)", "SIZE(bytes)");
	PRINT_SEP();
	PRINT_SIZEOF(char);
	PRINT_SIZEOF(short);
	PRINT_SIZEOF(int);
	PRINT_SIZEOF(float);
	PRINT_SIZEOF(long);
	PRINT_SIZEOF(double);
	PRINT_SEP();
	PRINT_SIZEOF(unsigned char);
	PRINT_SIZEOF(unsigned short);
	PRINT_SIZEOF(unsigned);
	PRINT_SIZEOF(unsigned int);
	PRINT_SIZEOF(unsigned long);
	PRINT_SEP();
	PRINT_SIZEOF(int8_t);
	PRINT_SIZEOF(int16_t);
	PRINT_SIZEOF(int32_t);
	PRINT_SIZEOF(int64_t);
	PRINT_SEP();
	PRINT_SIZEOF(uint8_t);
	PRINT_SIZEOF(uint16_t);
	PRINT_SIZEOF(uint32_t);
	PRINT_SIZEOF(uint64_t);
	PRINT_SEP();
	PRINT_SIZEOF(_Bool);
	PRINT_SIZEOF(bool);
	PRINT_SIZEOF(size_t);
	PRINT_SEP();

	int32_t buf[10] = {0};
	int32_t *ptr = buf;
	PRINT_TABLE("int32_t buf[10]", sizeof(buf));
	PRINT_TABLE("buf[0]", sizeof(buf[0]));
	PRINT_TABLE("*buf", sizeof(*buf));
	PRINT_TABLE("int32_t *ptr", sizeof(ptr));
	PRINT_TABLE("*ptr", sizeof(*ptr));
	PRINT_SEP();

	gen_vla(4);
	srand(time(NULL));
	gen_vla(rand()%100+2);
	return 0;
}

