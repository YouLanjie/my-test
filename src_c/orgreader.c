/*
 *   Copyright (C) 2025 Chglish
 *
 *   文件名称：orgreader.c
 *   创 建 者：Chglish
 *   创建日期：2025年06月07日
 *   描    述：
 *
 */


#include "./include/tools.h"

typedef struct Node{
	char *name;
	char *content;
	struct Node * child_node;
	struct Node * next_node;
} Node;

char **split_string(char *str)
{
	if(!str) return NULL;
	int len = strlen(str);
	long count = 2;
	for (int i = 0; i < len && (str[i] == '\n' ? count++ : 1); i++);
	char **p = malloc(sizeof(char*)*count);
	int posi = 0, line = 0;
	for (int i = 0; i < len; i++) {
		if (str[i] != '\n') continue;
		str[i] = '\0';
		p[line+1] = malloc(sizeof(char)*(i-posi+1));
		strcpy(p[line+1], str+posi);
		line++;
		posi = i+1;
	}
	p[0] = (char*)count-1;
	return p;
}

int main(int argc, char *argv[])
{
	FILE *fp = fopen("../python/res/test.org", "r");
	if (!fp) {
		printf("FILE `../python/res/test.org` is not exist\n");
		return 1;
	}
	char **ret = split_string(_fread(fp));
	long lines = (long)ret[0];
	char **content = ret+1;
	fclose(fp);
	/*printf(">>> CONTENT:\n%s\n", content);*/

	printf(">>> LINES: %ld\n", (long)ret[0]);
	int stat = 0;
	for (int i = 0; i < lines; i++) {
		if (!content[i]) continue;
		int len = strlen(content[i]);
		int is_begin = 1;
		for (int j = 0; j < len; j++) {
			if (is_begin) {
				if (content[i][j] == ' ' || content[i][j] == '\t') {
					/*printf(">>> SKIP WHITE SPACE at (%d,%d):%s\n", i,j, content[i]);*/
					continue;
				}
				if (content[i][j] == '#') stat |= 1;
			}
			if (stat & 1) {
				printf(">>> OUTPUT:%s\n", content[i]);
				stat &= (~1);
			}
			is_begin = 0;
		}
	}
	free(ret);
	printf("功能未完成，仅供测试\n");
	return 0;
}


