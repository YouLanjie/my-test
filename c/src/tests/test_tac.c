/**
 * @file        test_tac.c
 * @author      Chglish
 * @date        2026-08-08
 * @brief       尝试模仿实现tac
 */

#include "../../include/path.h"
#include <stdio.h>
#include <time.h>

int int_log10(int x)
{
	if (x <= 0) return 0;
	int i = 1;
	while (x > 0) {
		x /= 10;
		i++;
	}
	return i;
}

void tac(SV_t content)
{
	int num = 0;
	SV_t line = {}, left = content;
	int linenum = sv_countlines(content);
	int len = int_log10(linenum);
	srand(time(NULL));

	if (left.len && left.p[left.len-1] == '\n') sv_chop_right(&left, 1);
	while (sv_forline_reverse(&line, &left)) {
		printf("%*d >> '%.*s'\n",
		       len, linenum-num,
		       (int)line.len, line.p);

	// while (sv_forline(&line, &left)) {
	// 	printf("%*d >> '%.*s'\n", len, num+1, (int)line.len, line.p);

		if ((rand()%1000)/1000. > 0.9) {
			SV_t seek = sv_seekline(content, line, -3);
			printf("\e[31m%*zu (-3 LINE) >> '%.*s'\e[0m\n",
			       len, sv_getlinenum(content, seek),
			       (int)seek.len, seek.p);
		}
		num++;
	}
	printf("Total lines: %d\n", linenum);
	return;
}

void process(char *path)
{
	if (!path) return;
	SVA_t content = {};
	path_readfile(sv_from_cstr(path), &content, -1);
	if (content.len > 0) tac(sv_from_sva(&content));
	sva_free(&content);
	return;
}

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		process(argv[i]);
	}
	return EXIT_SUCCESS;
}

