/*
 *   Copyright (C) 2024 u0_a221
 *   
 *   文件名称：guess_number.c
 *   创 建 者：u0_a221
 *   创建日期：2024年02月12日
 *   描    述：
 *
 */

#include "./include/tools.h"

int main()
{
	srand(time(NULL));
	int min = 1,
	    max = 1000,
	    guess_number = rand() % (max - (min - 1)) + 1 + (min - 1),
	    inp = 0,
	    count = 0;

	while (inp != guess_number) {
		printf("Range:[%d-%d]\n"
		       "Please input:\n", min, max);
		scanf("%d", &inp);
		printf("\033[A               \r"
		       "\033[A               \r");
		if (inp > max || inp < min) {
			printf("Wrong number: out of the range!\n");
			count++;
			if (count > 100) {
				printf("Maybe something wrong happend.!\n"
				       "Let's retry again\n");
				return -1;
			}
		} else if (inp > guess_number) {
			max = inp - 1;
			printf("More\n");
		} else if (inp < guess_number) {
			min = inp + 1;
			printf("Less\n");
		}
	}
	printf("Right!\n");
	return 0;
}

