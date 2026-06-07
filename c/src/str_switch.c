/**
 * @file        str_switch.c
 * @author      u0_a221
 * @date        2026-06-07
 * @brief       测试为了像switch一样对比字符串
 */

#include <stddef.h>
#include <stdio.h>
#include <strings.h>

int str_switch(int listlen, const char *strlist[], const char *str, int *possible)
{
	if (!strlist || !str || listlen <= 0) return -1;
	int i = 0, j = 0, max_similar = 0, msid = 0;
	for (; i < listlen && strlist[i]; i++) {
		for (j = 0; strlist[i][j] == str[j]; j++) {
			/* 若字符串未结束(不为'\0')则跳过下面流程
			 * 使j保留最后相同位置 */
			if (str[j]) continue;
			/* 若字符串已结束则让j=-1 */
			j = -1;
			break;
		}
		if (j < 0) return i;
		if (j > max_similar) {
			max_similar = j;
			msid = i;
		}
	}
	if (max_similar == 0) return -1;
	if (possible) *possible = msid;
	return -2;
}

#define LOG(fmt, ...) fprintf(stderr, "%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__ __VA_OPT__(,) __VA_ARGS__)
#define ARRAY_LEN(arr) (sizeof(arr)/sizeof(arr[0]))

int main(int argc, char *argv[])
{
	const char *keywords[] = {
		"sb","sb2","sb3","nm","jav",
		"huwawei","xiaomi","apple","vivo","oppo","oneplus"
	};
	int similar = -1;
	char *inp = "huw";
	if (argc > 1) inp = argv[1];
	switch (str_switch(ARRAY_LEN(keywords), keywords, inp, &similar)) {
	case -1:
		LOG("'%s'未匹配,合法值有:", inp);
		for (size_t i = 0; i < ARRAY_LEN(keywords); i++) {
			LOG("%2ld: %s", i, keywords[i]);
		}
		break;
	case -2:
		if (similar >= 0 && similar < (int)ARRAY_LEN(keywords))
			LOG("'%s'未匹配，是否想找'%s'?", inp, keywords[similar]);
		break;
	}
	return 0;
}

