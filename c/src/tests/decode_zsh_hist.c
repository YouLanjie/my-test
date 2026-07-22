/**
 * @file        decode_zsh_hist.c
 * @author      https://www.zsh.org/mla/users/2011/msg00154.html
 * @date        2026-07-22
 * @brief       解码zsh的unicode字符
 */

#define Meta ((char) 0x83)

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

/* from zsh utils.c */
char *unmetafy(char *s, int *len)
{
	char *p, *t;

	for (p = s; *p && *p != Meta; p++) ;
	for (t = p; (*t = *p++);)
		if (*t++ == Meta)
			t[-1] = *p++ ^ 32;
	if (len)
		*len = t - s;
	return s;
}

int main(void)
{
	char *line = NULL;
	size_t size;

	while (getline(&line, &size, stdin) != -1) {
		unmetafy(line, NULL);
		printf("%s", line);
	}

	if (line)
		free(line);
	return EXIT_SUCCESS;
}
