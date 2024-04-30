/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：chinese.c
 *   创 建 者：youlanjie
 *   创建日期：2023年01月10日
 *   描    述：测试区分中英文的测试程序
 *
 */

#include <stdio.h>
#include <string.h>
 
int main(int argc, char** argv)
{
	if(argc != 2) {
		printf("Usage:\n./xxx str\n");
		return 0;
	}
	
	char *p = argv[1];
	int   len = strlen(p);
	for(int i = 0; i < len; i++) {
		if( *p & 0x80) {
			printf("chinese: %x\n", *p);
		} else {
			printf("english: %x\n", *p);
		}
		p++;
	}
	return 0;
}
