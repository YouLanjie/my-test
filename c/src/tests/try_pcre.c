/**
 * @file        try_pcre.c
 * @author      Chglish
 * @date        2026-07-22
 * @brief       PCRE2库使用测试
 */


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "../../include/path.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("usage: %s <FILE>\n", argv[0]);
		return EXIT_FAILURE;
	}

	PCRE2_SPTR pattern = (PCRE2_SPTR)"^(\\*+)\\s+(.*)";
	int errornum;
	PCRE2_SIZE erroroffset;
	pcre2_code *re = pcre2_compile(pattern,
				       PCRE2_ZERO_TERMINATED,
				       PCRE2_MULTILINE,
				       &errornum, &erroroffset,
				       NULL);
	if (!re) {
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message(errornum, buffer, sizeof(buffer));
		printf("PCRE2编译错误[%zu]: %s\n", erroroffset, buffer);
		return EXIT_FAILURE;
	}
	pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

	int exitcode = EXIT_FAILURE;
	SVA_t content = {};
	if (!path_readfile(sv_from_cstr(argv[1]), &content, -1)) {
		printf("读取文件错误\n");
		goto EXIT_AND_CLEAN_UP;
	}

	int count = 0;
	int rc = 0;
	SV_t substring;
	PCRE2_SIZE *ovector;
	// PCRE2_SPTR subject;
	uint32_t options = 0;
	PCRE2_SIZE start_offset = 0;
	while (true) {
		rc = pcre2_match(re, (PCRE2_SPTR)content.p, content.len,
				 start_offset, options, match_data, NULL);
		if (rc >= 0) {
		} else if (rc == PCRE2_ERROR_NOMATCH) {
			if (count) break;
			printf("No match\n");
			goto EXIT_AND_CLEAN_UP;
		} else {
			printf("Matching error %d\n", rc);
			goto EXIT_AND_CLEAN_UP;
		}
		ovector = pcre2_get_ovector_pointer(match_data);
		// printf("Match succeeded at offset %d\n", (int)ovector[0]);
		for (int i = 0; i < rc; i++) {
			if (i == 0) continue;
			substring.p = content.p + ovector[2*i];
			substring.len = ovector[2*i+1] - ovector[2*i];
			printf("T%d:G%d:[%zu]: %.*s\n", count, i, ovector[2*i],
			       (int)substring.len, substring.p);
		}
		if (!pcre2_next_match(match_data, &start_offset, &options))
			break;
		count++;
	}
	exitcode = EXIT_SUCCESS;

EXIT_AND_CLEAN_UP:
	pcre2_match_data_free(match_data);
	pcre2_code_free(re);
	sva_free(&content);
	return exitcode;
}

