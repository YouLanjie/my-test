/**
 * @file        test_depend_list.c
 * @author      Chglish
 * @date        2026-07-14
 * @brief       测试“依赖列表”功能
 */

#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdcountof.h>
#include <unistd.h>
#include "../../include/path.h"

typedef struct Target_t {
	SVA_t name;
	enum {TY_NORM = 0, TY_PHONY, TY_DEP} type;
	enum {TS_NOCHECK = 0, TS_WORKING, TS_SUCCESS, TS_FAILD} status;
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

void target_freelist(Target_t *list)
{
	Target_t *next;
	while (list) {
		next = list->next;
		target_free(list);
		list = next;
	}
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
		for (size_t i = 0; i < target->depend_len; i++)
			if (dependency == target->dependencies[i]) return;
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

Target_t *target_get_or_create(Target_t *list, SV_t name)
{
	Target_t *get = target_get_by_name(list, name);
	if (get) return get;
	get = target_create(name);
	target_append(list, get);
	return get;
}

void target_build(Target_t *target)
{
	if (!target) return;
	/* 跳过已操作项目 */
	if (target->status != TS_NOCHECK) return;
	target->status = TS_WORKING;
	bool isexist    = false;
	bool need_wait  = false;
	bool need_build = false;
	Path_st_t st = path_get_st(target->name);
	target->time = st.st.st_mtim.tv_sec + st.st.st_mtim.tv_nsec*1e-9;
	isexist = st.isexist;
	if (!st.isexist) need_build = true;
	for (size_t i = 0; i < target->depend_len; i++) {
		target_build(target->dependencies[i]);
		switch (target->dependencies[i]->status) {
		case TS_WORKING:
			need_wait = true;
			break;
		case TS_SUCCESS:
			if (target->dependencies[i]->time > target->time)
				need_build = true;
			break;
		case TS_NOCHECK:
		case TS_FAILD:
			target->status = TS_FAILD;
			return;
			break;
		}
		if (need_wait && i+1 >= target->depend_len) {
			i = -1;
			usleep(0.1e6);
		}
	}
	if (!need_build) {
		target->status = TS_SUCCESS;
		return;
	}
	if (target->type != TY_PHONY && !isexist && !target->build) {
		printf("[ERROR] 没有规则用于构建 %.*s\n", (int)target->name.len, target->name.p);
		target->status = TS_FAILD;
		return;
	}
	if (target->build) {
		printf("[INFO] 构建 %s\n", target->name.p);
		target->status = target->build(target) ? TS_SUCCESS : TS_FAILD;
		if (target->status == TS_SUCCESS) {
			st = path_get_st(target->name);
			target->time = st.st.st_mtim.tv_sec + st.st.st_mtim.tv_nsec*1e-9;
		}
	}
	return;
}

void target_buildlist(Target_t *list)
{
	if (!list) return;
	for (Target_t *p = list; p; p = p->next) {
		if (p->type != TY_NORM) continue;
		target_build(p);
	}
}

/**
 * @brief 递归遍历文件夹
 *
 * @param list 要查找的目标列表(留空自动创建)
 * @param cwd 工作目录，可以为空
 * @param dirname 要查找的工作目录下的子目录
 * @param rule 规则判断函数
 * @param action 对文件的行为函数
 * @return 构建好的列表
 */
static Target_t *target_fordir(Target_t *list, char *cwd, SV_t dirname,
			bool (*rule)(SV_t d_name, uint8_t d_type),
			Target_t *(*action)(Target_t *list, SV_t full_path))
{
	if (!cwd) cwd = "./";

	Path_t path = {0};
	path_join(sva_from_cstr(&path, cwd), dirname);

	DIR *dp = opendir(path.p);
	if (!dp) {
		if (path_get_st(path).isfile && rule(path_basename(sv_from_sva(&path)), DT_REG)) {
			list = action(list, sv_from_sva(&path));
		}
		sva_free(&path);
		return list;
	}
	Path_t tmp = {0};
	struct dirent *dp_item = NULL;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		if (rule(sv_from_cstr(dp_item->d_name), dp_item->d_type) == false) continue;
		if (dp_item->d_type == DT_DIR) {
			list = target_fordir(list, path.p, sv_from_cstr(dp_item->d_name), rule, action);
			continue;
		}
		path_join(sva_from_sv(&tmp, sv_from_sva(&path)),
			  sv_from_cstr(dp_item->d_name));
		list = action(list, sv_from_sva(&tmp));

	}
	closedir(dp);
	sva_free(&tmp);
	sva_free(&path);
	return list;
}

void target_printlist(Target_t *list)
{
	if (!list) return;
	static const char *statusstr[] = {
		[TS_NOCHECK] = "",
		[TS_SUCCESS] = "\e[32m<DONE>\e[0m",
		[TS_WORKING] = "\e[33m<WORKING>\e[0m",
		[TS_FAILD] = "\e[31m<FAILD>\e[0m",
	};
	static const char *typestr[] = {
		[TY_NORM] = "",
		[TY_PHONY] = "\e[2m(PHONY)\e[0m",
		[TY_DEP] = "\e[2m(DEP)\e[0m",
	};
	for (Target_t *p = list; p; p = p->next) {
		printf("[\e[2m%p\e[0m] %s%s'\e[32m%.*s\e[0m'",
		       p, statusstr[p->status%countof(statusstr)],
		       typestr[p->type%countof(typestr)],
		       (int)p->name.len, p->name.p);
		if (p->depend_len > 0) printf(" <- {");
		for (size_t i = 0; i < p->depend_len; i++) {
			if (!p->dependencies[i]) continue;
			// printf("[%p]%s,", p->dependencies[i], p->dependencies[i]->name.p);
			printf("\e[33m%s\e[0m%s", p->dependencies[i]->name.p,
			       i+1 >= p->depend_len ? "" : ", ");
		}
		if (p->depend_len > 0) printf("}");
		if (p->build) printf(" <- func<%p>", p->build);
		printf("\n");
	}
}



typedef struct {
	const char *libname;
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


#define LIB(l, ...) (CLIBS_t){.libname=l, .sources={__VA_ARGS__}}
CLIBS_t CLIBS[] = {
	LIB("ctools", LIB_DIR"tools.c", LIB_DIR"print_in_box.c", LIB_DIR"path.c", LIB_DIR"string_view.c"),
	LIB("cmenu", LIB_DIR"menu.c"),
	LIB("cmusicsynth", SOURCE_DIR"musicSynth/lib/*"),
	LIB("render3d",    SOURCE_DIR"render3d/lib/*"),
};
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

	// FLG("tetris.c",            "ncurses"),
};
#undef MUS
#undef MUSDEP
#undef FLG
// 配置区结束
// ================================







// 声明区

static bool rule_fordir(SV_t d_name, uint8_t d_type);
static Target_t *action_c_lib(Target_t *list, SV_t full_path);

// 声明区结束

#define EXEC_AND_PRINT() \
	printf("[RUN] '\e[45m%.*s\e[0m'\n", (int)cmd.len, cmd.p); \
	bool ret = system(cmd.p) == 0; \
	printf("[RESULT] %s\n", ret ? "\e[32mtrue\e[0m" : "\e[31mfalse\e[0m")

static bool build_c2obj(Target_t *target)
{
	if (!target) return false;
	// const size_t arr_len = ;
	SVA_t cmd = {};
	sva_sprintf(&cmd, "%s -o '%.*s' -c", "gcc", (int)target->name.len, target->name.p);
	// TODO: get flags
	for (size_t i = 0; i < target->depend_len; i++) {
		if (!target->dependencies[i]) break;
		sva_sprintfcat(&cmd, " '%.*s'",
			       (int)target->dependencies[i]->name.len,
			       target->dependencies[i]->name.p);
	}
	sva_sprintfcat(&cmd, " %s", CCOMFLAGS);

	SVA_t obj = {};
	const CFLAGS_t *flag = &(CFLAGS_t){.filename=NULL};
	for (uint64_t i = 0; i < countof(CFILEFLAGS); i++) {
		if (!CFILEFLAGS[i].filename) continue;
		path_normalize(sva_from_cstr(&obj, CFILEFLAGS[i].filename));
		if (!sva_cmp(&obj, &target->name)) continue;
		flag = CFILEFLAGS+i;
		break;
	}
	if (flag->flg_comp) sva_sprintfcat(&cmd, " %s", flag->flg_comp);

	EXEC_AND_PRINT();
	sva_free(&obj);
	sva_free(&cmd);
	return ret;
}

static bool build_obj2elf(Target_t *target)
{
	if (!target) return false;
	// const size_t arr_len = ;
	SVA_t cmd = {};
	sva_sprintf(&cmd, "%s -o '%.*s'", "gcc", (int)target->name.len, target->name.p);
	// TODO: get flags
	Target_t *sourcefile = NULL;
	for (size_t i = 0; i < target->depend_len; i++) {
		if (!target->dependencies[i] || target->dependencies[i]->type == TY_PHONY) break;
		/* 根据依赖反查配置的c文件 */
		if (!sourcefile && sv_end_with(sv_from_sva(&target->dependencies[i]->name), ".o")) {
			for (size_t j = 0; j < target->dependencies[i]->depend_len; j++) {
				if (sv_end_with(sv_from_sva(&target->dependencies[i]->dependencies[j]->name), ".c"))
					sourcefile = target->dependencies[i]->dependencies[j];
			}
		}
		sva_sprintfcat(&cmd, " '%.*s'",
			       (int)target->dependencies[i]->name.len,
			       target->dependencies[i]->name.p);
	}

	if (!sourcefile) {
		sva_free(&cmd);
		return false;
	}

	/* 加载flag */
	SVA_t obj = {};
	const CFLAGS_t *flag = &(CFLAGS_t){.filename=NULL};
	for (uint64_t i = 0; i < countof(CFILEFLAGS); i++) {
		if (!CFILEFLAGS[i].filename) continue;
		path_normalize(sva_from_cstr(&obj, CFILEFLAGS[i].filename));
		if (!sva_cmp(&obj, &sourcefile->name)) continue;
		flag = CFILEFLAGS+i;
		break;
	}
	if (flag->flg_link) sva_sprintfcat(&cmd, " %s", flag->flg_link);

	/* 加载链接库 */
	SV_t deps = sv_from_cstr(flag->libs), left;
	sva_sprintfcat(&cmd, " %s", CLINKFLAGS);
	while (deps.len > 0) {
		sv_trim_left_by_type(&deps, isspace);
		if (deps.len <= 0) break;
		left = sv_chop_by_type(&deps, isspace);
		if(left.len <= 0) break;
		sva_sprintfcat(&cmd, " -l%.*s", (int)left.len, left.p);
	}

	EXEC_AND_PRINT();
	sva_free(&obj);
	sva_free(&cmd);
	return ret;
}

static bool build_obj2alib(Target_t *target)
{
	if (!target) return false;
	SVA_t cmd = {};
	sva_sprintf(&cmd, "%s '%.*s'", "ar rcs ", (int)target->name.len, target->name.p);
	for (size_t i = 0; i < target->depend_len; i++) {
		if (!target->dependencies[i]) break;
		sva_sprintfcat(&cmd, " '%.*s'",
			       (int)target->dependencies[i]->name.len,
			       target->dependencies[i]->name.p);
	}
	EXEC_AND_PRINT();
	sva_free(&cmd);
	return ret;
}


/* 通过库简称(-l后名)获得库文件libxxx.a目标 */
static Target_t *get_target_by_libname(Target_t *list, SV_t libname)
{
	if (!list || !libname.p || libname.len==0) return NULL;
	Path_t libfile = {0};
	sva_sprintf(&libfile, "%s/lib%.*s.a", BUILD_DIR, (int)libname.len, libname.p);
	path_normalize(&libfile);
	Target_t *ret = target_get_by_name(list, sv_from_sva(&libfile));
	if (ret) {
		sva_free(&libfile);
		return ret;
	}

	size_t i = 0, j = 0;
	SV_t sourcefile;
	for (i = 0; i < countof(CLIBS); i++) {
		if (strncmp(CLIBS[i].libname, libname.p, libname.len) != 0) continue;
		// 只添加在表内的库
		ret = target_create(sv_from_sva(&libfile));
		if (!ret) continue;
		ret->build = build_obj2alib;
		ret->type = TY_DEP;
		target_append(list, ret);
		// 追加表源文件
		for (j = 0; j < countof(CLIBS[i].sources); j++) {
			sourcefile = sv_from_cstr(CLIBS[i].sources[j]);
			if (sv_end_with(sourcefile, "/*")) {
				sv_chop_right(&sourcefile, 1);
				list = target_fordir(list, NULL, sourcefile, rule_fordir, action_c_lib);
				continue;
			}
			list = action_c_lib(list, sourcefile);
			
		}
		break;
	}
	/* 若没有找到则尝试查找系统库 */
	if (!ret) {
		bool stat = false;
		const char *prefix = getenv("PREFIX");
		if (!prefix) prefix = "/usr";
		sva_sprintf(&libfile, "%s/lib/lib%.*s.so", prefix, (int)libname.len, libname.p);
		if (!(stat = (access(libfile.p, F_OK) == 0))) {
			sva_sprintf(&libfile, "%s/lib/lib%.*s.a", prefix, (int)libname.len, libname.p);
			stat = (access(libfile.p, F_OK) == 0);
		}
		if (stat) {
			ret = target_get_or_create(list, sv_from_sva(&libfile));
			if (ret) ret->type = TY_PHONY;
		}
	}
	sva_free(&libfile);
	return ret;
}


/* 将路径变为 BUILD_DIR/xxx_xxx_xxx.o */
static Path_t *path_hander_obj_replace(Path_t* path)
{
	if (!path || !path->p) return NULL;
	for (size_t n = 0; n < path->len; n++) if (path->p[n] == '/') path->p[n] = '_';
	SV_t sv = path_stemname(sv_from_sva(path));
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
	sva_from_sv(&basename, path_stemname(sv_from_sva(path)));
	if (basename.p == NULL) return NULL;
	sva_sprintf(path, "%s/%.*s", BIN_DIR, (int)basename.len, basename.p);
	sva_free(&basename);
	path_normalize(path);
	return path;
}

static bool rule_fordir(SV_t d_name, uint8_t d_type)
{
	if (!d_name.p || !d_name.len) return false;
	/* 跳过特殊路径 */
	if (sv_cmp(d_name, sv_from_cstr("..")) ||
	    sv_cmp(d_name, sv_from_cstr(".")))
		return false;
	if (d_type == DT_DIR) {
		/* 跳过lib.*文件夹 */
		if (sv_begin_with(d_name, "lib")) return false;
		return true;
	}
	/* 跳过非文件 */
	if (d_type != DT_REG)
		return false;
	if (sv_end_with(d_name,".c"))
		return true;
	// printf("[INFO] FAILD ON %.*s\n", (int)d_name.len, d_name.p);
	// else if (sv_end_with(sv_from_cstr(d_name),".h"))
	// 	return true;
	return false;
}

static Target_t *action_c_elf(Target_t *list, SV_t full_path)
{
	if (!full_path.p) return list;
	Target_t *target, *sub_target;
	Path_t tmp = {0};

	/* 创建.c目标 */
	target = target_get_or_create(list, full_path);
	if (!list) list = target;
	if (target) target->type = TY_DEP;

	sva_from_sv(&tmp, full_path);
	path_hander_obj_replace(&tmp);
	/* 创建.o目标 */
	sub_target = target_get_or_create(list, sv_from_sva(&tmp));
	if (sub_target) {
		sub_target->type = TY_DEP;
		sub_target->build = build_c2obj;
	}
	target_depend_append(sub_target, target);  // obj 依赖 .c
	target = sub_target;

	sva_from_sv(&tmp, full_path);
	path_hander_elf(&tmp);
	/* 创建elf目标 */
	sub_target = target_get_or_create(list, sv_from_sva(&tmp));
	if (sub_target) {
		sub_target->build = build_obj2elf;
	}
	target_depend_append(sub_target, target);  // elf 依赖 .o
	target = sub_target;

	/* 查找对应的需求库 */
	const CFLAGS_t *flag = NULL;
	for (uint64_t i = 0; i < countof(CFILEFLAGS); i++) {
		if (!CFILEFLAGS[i].filename) continue;
		path_normalize(sva_from_cstr(&tmp, CFILEFLAGS[i].filename));
		if (!sv_cmp(sv_from_sva(&tmp), full_path)) continue;
		flag = CFILEFLAGS+i;
		break;
	}
	if (!flag) {
		sva_free(&tmp);
		return list;
	}
	/* 添加需求库 */
	SV_t deps = sv_from_cstr(flag->deps), left;
	deps = sv_from_cstr(flag->libs);
	while (target && deps.len > 0) {
		sv_trim_left_by_type(&deps, isspace);
		if (deps.len <= 0) break;
		left = sv_chop_by_type(&deps, isspace);
		if(left.len <= 0) break;

		sub_target = get_target_by_libname(list, left);
		if (!sub_target) target->status = TS_FAILD;
		else sub_target->build = build_obj2alib;
		target_depend_append(target, sub_target);  // elf 依赖 libs
	}
	sva_free(&tmp);
	return list;
}

static Target_t *action_c_lib(Target_t *list, SV_t full_path)
{
	if (!full_path.p) return list;
	Target_t *lib;
	Target_t *target_c = target_get_by_name(list, full_path);
	Target_t *target_obj;
	if (target_c) return list;
	SV_t libdir = path_father(full_path);
	SV_t testpath;
	SVA_t objpath = {};
	for (size_t i = 0; i < countof(CLIBS); i++) {
		testpath = sv_from_cstr(CLIBS[i].sources[0]);
		if (!sv_end_with(testpath, "/*")) continue;
		sv_chop_right(&testpath, 1);
		if (!sv_cmp(testpath, libdir)) continue;

		lib = get_target_by_libname(list, sv_from_cstr(CLIBS[i].libname));
		if (!lib) break;

		target_c = target_get_or_create(list, full_path);
		if (!target_c) return list;
		if (!list) list = target_c;
		target_c->type = TY_DEP;

		path_hander_obj_replace(sva_from_sv(&objpath, full_path));
		target_obj = target_get_or_create(list, sv_from_sva(&objpath));
		if (target_obj) {
			target_obj->type = TY_DEP;
			target_obj->build = build_c2obj;
		}
		target_depend_append(lib, target_obj);
		target_depend_append(target_obj, target_c);
		sva_free(&objpath);
		break;
	}
	return list;
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	Target_t *list = NULL;
	if (argc <= 1) list = target_fordir(list, NULL, sv_from_cstr("./"), rule_fordir, action_c_elf);
	for (int i = 1; i < argc; i++) {
		list = target_fordir(list, NULL, sv_from_cstr(argv[i]), rule_fordir, action_c_elf);
	}

	/* 打印 */
	printf("[INFO] 运行构建\n");
	target_buildlist(list);
	target_printlist(list);

	target_freelist(list);
	return 0;
}

