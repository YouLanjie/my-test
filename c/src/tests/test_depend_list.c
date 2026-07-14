/**
 * @file        test_depend_list.c
 * @author      Chglish
 * @date        2026-07-14
 * @brief       测试“依赖列表”功能
 */

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdcountof.h>
#include "../../include/path.h"

typedef struct Target_t {
	SVA_t name;
	enum {TS_NOCHECK = 0, TS_SUCCESS, TS_FAILD} status;
	double time;
	bool (*build)(struct Target_t*);
	size_t depend_len;
	struct Target_t **dependencies;
	struct Target_t *prev;
	struct Target_t *next;
} Target_t;

Target_t *target_create(SV_t name)
{
	if (name.len == 0 || !name.p) return NULL;
	Target_t *target = malloc(sizeof(*target));
	if (!target) return NULL;
	*target = (Target_t){};
	sva_from_sv(&target->name, name);
	return target;
}

void target_free(Target_t *target)
{
	if (!target) return;
	if (target->prev) target->prev->next = target->next;
	if (target->next) target->next->prev = target->prev;
	if (target->dependencies) free(target->dependencies);
	free(target);
}

void target_append(Target_t *list, Target_t *target)
{
	if (!list || !target) return;
	if (target->prev || target->next) return;
	if (list == target) return;    /* 跳过已有项 */
	while (list->next) {
		if (list == target) return;
		list = list->next;
	}
	list->next = target;
	target->prev = list;
	return;
}

void target_depend_append(Target_t *target, Target_t *dependency)
{
	if (!target || !dependency) return;
	if (!target->dependencies) {
		target->depend_len = 1;
		target->dependencies = malloc(target->depend_len*sizeof(*target->dependencies));
	} else {
		Target_t *p = target;
		while (p) {
			if (p == dependency) return;
			p = p->next;
		}
		target->depend_len++;
		target->dependencies = realloc(target->dependencies, target->depend_len*sizeof(*target->dependencies));
	}
	if (!target->dependencies) {
		target->depend_len = 0;
		return;
	}
	target->dependencies[target->depend_len-1] = dependency;
}

Target_t *target_get_by_name(Target_t *list, SV_t name)
{
	while (list && sv_cmp(sv_from_sva(&list->name), name) != true) {
		list = list->next;
	}
	return list;
}



typedef struct {
	const char *libname;
	const char *libfile;
	const char *sources[10];
} CLIBS_t;

typedef struct {
	const char *filename;
	const char *flg_comp;
	const char *flg_link;
	const char *libs;
	const char *deps;
	const bool no_elf;
} CFLAGS_t;

#define SOURCE_DIR "./src/"
#define LIB_DIR    "./lib/"
#define BUILD_DIR  "./.build/"
#define BIN_DIR    "./bin/"
#define COMPILOR   "gcc"
#define CCOMFLAGS  "-Wall -Wextra -O2 -g"
#define CLINKFLAGS "-L"BUILD_DIR" -lctools"


#define LIB(l, ...) (CLIBS_t){.libname=l, .libfile=BUILD_DIR "lib"l".a", .sources={__VA_ARGS__}}
#define MUSDIR SOURCE_DIR"musicSynth/lib/"
#define R3DDIR SOURCE_DIR"render3d/lib/"
CLIBS_t CLIBS[] = {
	LIB("ctools", LIB_DIR"tools.c", LIB_DIR"print_in_box.c", LIB_DIR"path.c", LIB_DIR"string_view.c"),
	LIB("cmenu", LIB_DIR"menu.c"),
	LIB("cmusicsynth", MUSDIR"core.c", MUSDIR"wave_func.c", MUSDIR"note_parser.c", MUSDIR"music_ctx.c"),
	LIB("render3d", R3DDIR"camera.c", R3DDIR"object.c", R3DDIR"vec.c", R3DDIR"draw_ascii.c", R3DDIR"draw_utf8.c"),
};
#undef R3DDIR
#undef MUSDIR
#undef LIB

#define FLG(f, l, ...) (CFLAGS_t){.filename=SOURCE_DIR f, .libs=l, __VA_ARGS__}
#define MUS(f, l, ...) FLG("musicSynth/" f, l" m cmusicsynth", __VA_ARGS__)
CFLAGS_t CFILEFLAGS[] = {
	MUS("lib/core.c",          "m\0",),
	MUS("alsa_play.c",         "asound",),
	MUS("sdl2_play.c",         "SDL2",),
	MUS("music_synth.c",       ,),

	FLG("render3d/render3d.c", "m render3d"),
	FLG("render3d/r3d_rotate.c", "m render3d"),

	FLG("tests/libav_test.c",  "avformat avcodec avutil swresample m"),
	FLG("tests/social.c",      "m"),
	FLG("tests/try_iconv.c",   "iconv"),
	FLG("tests/input.c",       "m"),
	FLG("tests/sin.c",         "m"),

	FLG("tetris.c",            "ncurses"),
};
#undef MUS
#undef MUSDEP
#undef FLG
// 配置区结束
// ================================


Target_t *get_target_by_libname(Target_t *list, SV_t libname)
{
	if (!list) return NULL;
	Path_t libfile = {0};
	sva_sprintf(&libfile, "%s/lib%.*s.a", BUILD_DIR, (int)libname.len, libname.p);
	path_normalize(&libfile);
	Target_t *ret = target_get_by_name(list, sv_from_sva(&libfile)),
		 *sub_target;
	sva_free(&libfile);
	if (ret) return ret;

	uint64_t i = 0, j = 0;
	for (i = 0; i < countof(CLIBS); i++) {
		if (strncmp(CLIBS[i].libname, libname.p, libname.len) != 0) continue;
		// 只添加在表内的库
		ret = target_create(sv_from_sva(&libfile));
		target_append(list, ret);
		for (j = 0; j < countof(CLIBS[i].sources); j++) {
			sub_target = target_get_by_name(list, sv_from_cstr(CLIBS[i].sources[j]));
			if (!sub_target) {
				sub_target = target_create(sv_from_cstr(CLIBS[i].sources[j]));
				target_append(list, sub_target);
			}
			target_depend_append(ret, sub_target);
			
		}
		break;
	}
	return list;
}


/* 将路径变为 BUILD_DIR/xxx_xxx_xxx.o */
static Path_t *path_hander_obj_replace(Path_t* path)
{
	if (!path || !path->p) return NULL;
	for (size_t n = 0; n < path->len; n++) if (path->p[n] == '/') path->p[n] = '_';
	SV_t sv = path_basename(sv_from_sva(path));
	while (sv.len > 0 && (sv.p[0] == '.' || sv.p[0] == '_')) sv_chop_left(&sv, 1);
	SVA_t new = {0};
	sva_from_sva(&new, path);
	sva_strcpy(path, sva_sprintfcat(path_join(sva_from_cstr(&new, BUILD_DIR), sv), ".o"));
	sva_free(&new);
	return path;
}
/* 将路径变为 BIN_DIR/xxx */
static Path_t *path_hander_elf(Path_t* path)
{
	if (!path || !path->p) return NULL;
	SVA_t basename = {0};
	sva_from_sv(&basename, path_basename(sv_from_sva(path)));
	if (basename.p == NULL) return NULL;
	sva_sprintf(path, "%s/%.*s", BIN_DIR, (int)basename.len, basename.p);
	sva_free(&basename);
	path_normalize(path);
	return path;
}

static Target_t *fordir(Target_t *list, char *cwd, char *dirname)
{
	if (!dirname) return list;
	if (!cwd) cwd = "./";
	if (sv_begin_with(sv_from_cstr(dirname), "lib")) return list;

	Path_t path = {0};
	Path_t obj = {0};
	Path_t tmp = {0};
	Target_t *target, *sub_target;
	path_join(sva_from_cstr(&path, cwd), sv_from_cstr(dirname));

	DIR *dp = opendir(path.p);
	if (!dp) return list;
	/*if (level <= 1) printf("%s\n", dirname);*/
	struct dirent *dp_item = NULL;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		uint8_t type = dp_item->d_type;
		if (strcmp(dp_item->d_name, "..")==0 || strcmp(dp_item->d_name, ".")==0) {
			continue;
		}
		/*printf("%s\n", dp_item->d_name);*/
		if (type == DT_DIR) {
			list = fordir(list, path.p, dp_item->d_name);
		}
		if (type != DT_REG || !sv_end_with(sv_from_cstr(dp_item->d_name),".c"))
			continue;
		path_join(sva_from_sv(&obj, sv_from_sva(&path)),
			  sv_from_cstr(dp_item->d_name));

		target = target_create(sv_from_sva(&obj));
		if (!list) list = target;
		target_append(list, target);

		sva_strcpy(&tmp, &obj);
		path_hander_obj_replace(&tmp);
		sub_target = target_create(sv_from_sva(&tmp));
		target_append(list, sub_target);
		target_depend_append(sub_target, target);  // obj 依赖 .c
		target = sub_target;

		sva_strcpy(&tmp, &obj);
		path_hander_elf(&tmp);
		sub_target = target_create(sv_from_sva(&tmp));
		target_append(list, sub_target);
		target_depend_append(sub_target, target);  // elf 依赖 .o

		const CFLAGS_t *flag = NULL;
		for (uint64_t i = 0; i < countof(CFILEFLAGS); i++) {
			if (!CFILEFLAGS[i].filename) continue;
			path_normalize(sva_from_cstr(&tmp, CFILEFLAGS[i].filename));    // 借用obj
			if (!sva_cmp(&tmp, &obj)) continue;
			flag = CFILEFLAGS+i;
			break;
		}
		if (!flag) continue;
		SV_t deps = sv_from_cstr(flag->deps), left;
		deps = sv_from_cstr(flag->libs);
		while (deps.len > 0) {
			sv_trim_left_by_type(&deps, isspace);
			if (deps.len <= 0) break;
			left = sv_chop_by_type(&deps, isspace);
			if(left.len <= 0) break;

			target = get_target_by_libname(list, left);
			target_depend_append(sub_target, target);  // elf 依赖 libs
		}
	}
	closedir(dp);
	sva_free(&tmp);
	sva_free(&obj);
	sva_free(&path);
	return list;
}

int main(void)
{
	Target_t *list = fordir(NULL, NULL, "./");

	/* 打印 */
	for (Target_t *p = list; p; p = p->next) {
		printf("[%p] '%.*s' <- {", p, (int)p->name.len, p->name.p);
		for (size_t i = 0; i < p->depend_len; i++) {
			if (!p->dependencies[i]) continue;
			printf("%s,", p->dependencies[i]->name.p);
		}
		printf("}\n");
	}

	Target_t *next;
	while (list) {
		next = list->next;
		target_free(list);
		list = next;
	}
	return 0;
}

