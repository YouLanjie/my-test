/*
 *   Copyright (C) 2024 YouLanjie
 *
 *   文件名称：score.c
 *   创 建 者：youlanjie
 *   创建日期：2024年04月05日
 *   描    述：囚徒困境、博弈论测试文件
 *
 */


#include "../../include/tools.h"

FILE *fp = NULL;
unsigned short int score[2] = {0};
unsigned short int record[2][250] = {0};
int count = 0;

int zero_all(int f) { return 0; }
int one_all(int f) { return 1; }
int f_random(int f)
{/*{{{*/
	struct timeval gettime;
	gettimeofday(&gettime, NULL);
	srand(gettime.tv_usec + time(NULL));
	usleep(rand() % 25);
	return rand() % 2;
}/*}}}*/
int tit_for_tat(int f)
{/*{{{*/
	if (count <= 0) return 1;
	if (record[!f][count - 1 > 0 ? count - 1 : 0]) return 1;
	return 0;
}/*}}}*/
int zero_after_one(int f)
{/*{{{*/
	if (count <= 0) return 1;
	if (!record[!f][count - 1 > 0 ? count - 1 : 0]) return 0;
	if (!record[f][count - 1 > 0 ? count - 1 : 0]) return 0;
	return 1;
}/*}}}*/


int clean()
{/*{{{*/
	for(int i = 0; i < 250; i++) {
		record[0][i] = 0;
		record[1][i] = 0;
	}
	score[0] = 0;
	score[1] = 0;
	return 0;
}/*}}}*/

int run(int (*f_left)(int f), int (*f_right)(int f))
{/*{{{*/
	clean();
	int left = 0,
	    right = 0;
	int max = 200;
	for(count = 0; count < max; count++) {
		left = f_left(0);
		right = f_right(1);
		record[0][count] = left;
		record[1][count] = right;
		if (left && right) score[0] += 3, score[1] += 3;
		else if (!left && !right) score[0] += 1, score[1] += 1;
		else score[left] += 5;
	}
	fprintf(fp, "==================================================\n"
		    "Chose A:\n");
	for(count = 0; count < max; count++) fprintf(fp, "%d", record[0][count]);
	fprintf(fp, "\nChose B\n");
	for(count = 0; count < max; count++) fprintf(fp, "%d", record[1][count]);
	fprintf(fp, "\n");
	fprintf(fp, "Socre: A:%d, B:%d\n", score[0], score[1]);
	fprintf(fp, "==================================================\n");

	printf("Socre: A:%d, B:%d\n", score[0], score[1]);
	printf("==================================================\n");
	return 0;
}/*}}}*/

#define LEN 5
int lists[LEN] = {0};
int (*list[LEN])(int f) = {
	f_random,
	zero_all,
	one_all,
	zero_after_one,
	tit_for_tat,
};
char *listc[LEN] = {
	"RANDOM",
	"Betray ALL",
	"Cooperate ALL",
	"Betray After Cooperate",
	"Tit For Tat"
};

int main(void)
{/*{{{*/
	fp = fopen("./output.txt", "a");
	for (int i = 0; i < LEN; i++) {
		for (int j = i; j < LEN; j++) {
			fprintf(fp, "\nFUNCTION: A:%d:%s, B:%d:%s\n", i, listc[i], j, listc[j]);
			printf("FUNCTION: A:%d:%s, B:%d:%s\n", i, listc[i], j, listc[j]);
			run(list[i], list[j]);
			lists[i] += score[0];
			lists[j] += score[1];
		}
	}
	fprintf(fp, "==================================================\n");
	fprintf(fp, "TOTAL:\n");
	for (int i = 0; i < LEN; i++) fprintf(fp, "%s:%d\n", listc[i], lists[i]);
	fprintf(fp, "==================================================\n");

	printf("TOTAL:\n");
	for (int i = 0; i < LEN; i++) printf("%s:%d\n", listc[i], lists[i]);
	fclose(fp);
	return 0;
}/*}}}*/

