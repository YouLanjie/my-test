/*
 *   Copyright (C) 2025 u0_a216
 *
 *   文件名称：get_elf_str.c
 *   创 建 者：u0_a216
 *   创建日期：2025年10月19日
 *   描    述：
 *
 */


#include "include/tools.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("usage: ./xxx stra\n");
		return 1;
	}
	FILE *fp = fopen(argv[1], "rb");
	if (!fp) {
		printf("Can't open file '%s'\n", argv[1]);
		return 2;
	}
	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);
	/*printf("SIZE: %ld\n", size);*/
	fseek(fp, 0L, SEEK_SET);
	/*printf("POS: %ld\n", ftell(fp));*/
	char buff[4] = "";
	int s = 0;
	int count = 0;
	int white = 0;
	char table[256] = {['\n']=1, ['\r']=1, ['\t']=1, [' ']=1};
	for(long i = 0; i < size && s != EOF; i++) {
		buff[count] = s = fgetc(fp);
		if(buff[count] & 0x80) {
			if (count == 0) {
				if (!(buff[count] >= 0xE0 && buff[count] <= 0xEF))
					continue;
			} else if (buff[count] & 0b01000000) {
				count = 0;
				continue;
			}

			if (count == 2) {
				printf("%s", buff);
				buff[0] = 0;
				buff[1] = 0;
				buff[2] = 0;
				buff[3] = 0;
				count = 0;
				white++;
				continue;
			}
			count++;
		} else if ((buff[count] > '!' && buff[count] < '~') || 
			   table[(int)buff[count]]) {
			printf("%c", buff[count]);
			count = 0;
			white++;
		} else {
			if (white != 0)
				printf("\n");
			white = 0;
		}
	}
	/*printf("\n-- END OF FILE --\n");*/
	fclose(fp);
	return 0;
}

