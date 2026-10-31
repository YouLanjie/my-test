/**
 * @file        test_pathlib.c
 * @author      Chglish
 * @date        2026-10-01
 * @brief       测定path.c及相关组件有没有存在问题
 */

#include "../../include/path.h"

int main(int argc, char *argv[])
{
	Path_t cwd = {};
        Path_t path = {};
        Path_t last = {};
	path_getcwd(&cwd);
	printf("CWD: '%s'\n", cwd.p);
        for (int i = 0; i < argc; i++) {
                sva_from_cstr(&path, argv[i]);
                path_normalize(&path);
                SV_t sv = path_basename(sv_from_sva(&path));
                SV_t stem = path_stemname(sv_from_sva(&path));
                printf("\e[1mARGV[%d]\e[0m \e[33m'%s'\e[0m \e[2m(BASENAME:'%.*s' STEM:'%.*s')\e[0m\n",
                       i, path.p, (int)sv.len, sv.p,
                       (int)stem.len, stem.p);
		if (last.p) {
			path_relative_cwd(&last, sv_from_sva(&path), sv_from_sva(&cwd));
			if (last.len) printf("    \e[36m-> ARGV[%d]\e[0m \e[33m'%s'\e[0m\n", i-1, last.p);
		}
		sva_from_sva(&last, &path);
        }
        sva_free(&path);
        sva_free(&last);
        return EXIT_SUCCESS;
}

