/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：input.c
 *   创 建 者：youlanjie
 *   创建日期：2023年12月09日
 *   描    述：Input Test
 *
 */


#include "./include/tools.h"
#include <math.h>

/*
 * Input Test Function
 */
int input_int_number(void)
{
	printf("Number input test:\n");
	printf("[__________]\r[");
	int inp = 0;
	int count = 0;
	while (inp != '\r') {
		inp = _getch();
		printf("\r");
		int count2 = count * 10 + inp - 48;
		if (inp >= '0' && inp <= '9' && count2 > count && count2 < 1000000000) {
			count = count2;
		} else if (inp == 0x7F) {    /* Key 'Backspace */
			count /= 10;
		}

		printf("[__________]\r[");
		if (count != 0) {
			printf("\033[4m%d\033[0m", count);
		}
	}
	printf("\n");
	printf("Final result:%d\n", count);
	return 0;
}

int input_float_number(void)
{
	printf("Number input test:\n");
	printf("[_________________]\r[");
	int inp = 0;
	double count = 0;
	int flag_small = 0;
	int level = 0;
	while (inp != '\r') {
		inp = _getch();
		printf("\r");
		double tmp = inp - 48;
		double count2 = flag_small ? (count + tmp / pow(10, level)) : (count * 10 + inp - 48);
		if (inp >= '0' && inp <= '9' && count2 > count && count2 < 1000000000 && level <= 6) {
			count = count2;
			if (flag_small) level++;
		} else if (inp == 0x7F) {    /* Key 'Backspace */
			if (flag_small) {
				int left = count;
				double tmp = 0;
				tmp += left;
				double right = count - tmp;
				int bit = right * pow(10, level - 1);
				bit %= 10;
				right = right - bit / pow(10, level - 1);
				count = left + right;
				level--;
				if (level == 0) flag_small = 0;
			} else {
				int left = count;
				count = (double)left / 10;
			}
		} else if (inp == '.') {
			flag_small = 1;
			level = 1;
		}


		printf("[_________________]\r[");
		if (count != 0) {
			printf("\033[4m%f\033[0m\033[7D", count);
			if (flag_small) printf("\033[%dC", level);
		}
	}
	printf("\n");
	printf("Final result:%f\n", count);
	return 0;
}

typedef struct t_inp
{
	int ch;
	int id;
	struct t_inp *n;
	struct t_inp *l;
} t_inp;

t_inp *new_n()
{
	t_inp *new = malloc(sizeof(t_inp));
	new->ch = 0;
	new->id = 0;
	new->n  = NULL;
	new->l  = NULL;
	return new;
}

void t_print(t_inp *tmp, int max)
{
	printf("\r[");
	for (; tmp != NULL; tmp = tmp->n) {
		if (tmp->id <= max-15) continue;
		printf("\033[3m%c\033[0m", tmp->ch);
	}
	return;
}

int input_string(void)
{
	printf("String input test(Only ASCII):\n");
	printf("[_______________]\r[");
	int count = 0;
	int inp = 0;
	t_inp *pH = new_n();
	t_inp *pN = pH, *pL = NULL;
	while (inp != '\r') {
		t_print(pH, count);
		inp = _getch();
		if ((inp > '!' && inp < '~') || inp == ' ') {
			count++;
			pN->id = count;
			pN->ch = inp;
			pN->n  = new_n();
			pL = pN;
			pN = pN->n;
			pN->l  = pL;
		} else if (inp == 0x7F && count > 0) {
			count--;
			pL = pL->l;
			pN = pN->l;
			free(pN->n);
			pN->n = NULL;
			pN->ch = 0;
			printf("\b \b");
		}
	}
	if (count == 0) {
		printf("\nFinal result:[\033[3m%s\033[0m]\n", "");
		return 0;
	}
	pN = pH;
	char *least = malloc(sizeof(char)*(count + 1));
	while (pN != NULL) {
		least[pN->id - 1] = pN->ch;
		pN = pN->n;
	}
	least[count] = '\0';
	printf("\nFinal result:[\033[3m%s\033[0m]\n", least);
	return 0;
}

int input_char(void)
{
	printf("Char input test:\n");
	printf("[____]\r[");
	int inp = 0;
	inp = _getch();
	printf("\033[4m%c\033[0m\n", inp);
	if (inp == 0x1B) printf("Final result:[] <0x1B>\n");
	else if (inp == '\r' || inp == '\n') printf("Final result:[] <0x%X>\n", inp);
	else printf("Final result:[%c] <0x%X>\n", inp, inp);
	return 0;
}

int main(int argc, char *argv[])
{
	printf("What Type Data You Want To Input?\n"
	       "1) Number(int)\n"
	       "2) Number(float)\n"
	       "3) String[Default]\n"
	       "4) Char\n");
	int inp = _getch();
	if (inp == '1') {
		input_int_number();
	} else if (inp == '2') {
		input_float_number();
	} else if (inp == '4') {
		input_char();
	} else {
		input_string();
	}
	return 0;
}


