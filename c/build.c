/**
 * @file        build.c
 * @author      u0_a221 和 Chglish
 * @date        2026-04-26 和 2026-07-14
 * @brief       构建脚本，已经迁移依赖列表过来
 */

#include "lib/path.c"
#include "lib/string_view.c"
#include "lib/target_list.c"
#include <ctype.h>    /* isspace */
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdcountof.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>   /* usleep */
#include <wait.h>     /* wait */


typedef struct {
	const char *libname;
	const char **header;
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

// ===============================
// 配置区
#define SOURCE_DIR "./src/"
#define LIB_DIR    "./lib/"
#define BUILD_DIR  "./.build/"
#define BIN_DIR    "./bin/"
#define COMPILOR   "gcc"
#define CCOMFLAGS  "-Wall -Wextra -O2 -g"
#define CLINKFLAGS "-L"BUILD_DIR

#define LIB(l, h, ...) (CLIBS_t){.libname=l, .header=h, .sources={__VA_ARGS__}}
CLIBS_t CLIBS[] = {
	LIB("ctools", ((const char*[]){"include/*", NULL}), LIB_DIR"tools.c", LIB_DIR"*"),
	LIB("cmenu", ((const char *[]){"lib/cmneu.h", NULL}), SOURCE_DIR"cmenu/lib/menu.c"),
	LIB("cmusicsynth", ((const char *[]){"lib/music_synth.h", NULL}), SOURCE_DIR"musicSynth/lib/*"),
	LIB("render3d", ((const char *[]){"lib/render3d.h", NULL}),    SOURCE_DIR"render3d/lib/*"),
};
#undef LIB

#define FLG(f, l, ...) (CFLAGS_t){.filename=SOURCE_DIR f, .libs=l, __VA_ARGS__}
CFLAGS_t CFILEFLAGS[] = {
	FLG("musicSynth/alsa_play.c",   "m asound",),
	FLG("musicSynth/sdl2_play.c",   "m SDL2",),
	FLG("musicSynth/music_synth.c", "m",),

	FLG("render3d/render3d.c",   "m"),
	FLG("render3d/r3d_rotate.c", "m"),

	FLG("tests/libav_test.c", "avformat avcodec avutil swresample m"),
	FLG("tests/try_iconv.c",  "iconv"),
	FLG("tests/try_pcre.c",  "pcre2-8"),
	FLG("tests/social.c",     "m"),
	FLG("tests/input.c",      "m"),
	FLG("tests/sin.c",        "m"),

	// FLG("tetris.c",            "ncurses"),
};
#undef MUS
#undef MUSDEP
#undef FLG
// 配置区结束
// ================================



// ===============================
// 声明区

// 文件名转换规则
static Path_t *path_hander_obj_replace(Path_t* path);
static Path_t *path_hander_elf(Path_t* path);
// 递归文件跳过规则
static bool rule_fordir(SV_t d_name, uint8_t d_type);
// 递归文件处理和目标生成规则
static Target_t *action_c_lib(Target_t *list, SV_t full_path);
static Target_t *action_c_elf(Target_t *list, SV_t full_path);
static Target_t *action_c_lib(Target_t *list, SV_t full_path);
// 文件构建规则
static bool build_c2obj(Target_t *target);
static bool build_obj2elf(Target_t *target);
static bool build_obj2alib(Target_t *target);

// 声明区结束
// ===============================

#define EXEC_AND_PRINT() \
	printf("[RUN] '\e[45m%.*s\e[0m'\n", (int)cmd.len, cmd.p); \
	bool ret = system(cmd.p); \
	if (ret != 0) printf("[\e[31mWARN\e[0m] 构建`%s`时发生错误 (return: %d)\n", target->name.p, ret);

static bool build_c2obj(Target_t *target)
{
	if (!target) return false;
	SVA_t cmd = {};
	sva_sprintf(&cmd, "%s -o '%.*s' -c", COMPILOR, (int)target->name.len, target->name.p);
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

	sva_from_sv(&obj, path_father(sv_from_sva(&target->name)));
	if (access(obj.p, F_OK) != 0) {
		path_mkdir(sv_from_sva(&obj), 0755);
	}

	EXEC_AND_PRINT();
	sva_free(&obj);
	sva_free(&cmd);
	return ret == 0;
}

static bool build_obj2elf(Target_t *target)
{
	if (!target) return false;
	SVA_t cmd = {}, obj = {};
	sva_sprintf(&cmd, "%s -o '%.*s'", "gcc", (int)target->name.len, target->name.p);
	for (size_t i = 0; i < target->depend_len; i++) {
		if (!target->dependencies[i]) break;
		if (target->dependencies[i]->type == TY_PHONY) continue;
		if (target->dependencies[i]->type == TY_DEP &&
		    sv_end_with(sv_from_sva(&target->dependencies[i]->name), sv_from_lstr(".a"))) {
			const CLIBS_t *lib = target->dependencies[i]->cfgdata;
			if (!lib) continue;
			sva_sprintfcat(&obj, " -l%s", lib->libname);
			continue;
		}
		sva_sprintfcat(&cmd, " '%.*s'",
			       (int)target->dependencies[i]->name.len,
			       target->dependencies[i]->name.p);
	}

	/* 加载flag */
	const CFLAGS_t *flag = &(CFLAGS_t){.filename=NULL};
	if (target->cfgdata) flag = target->cfgdata;
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
	if (obj.p && obj.len) {
		sva_sprintfcat(&cmd, "%.*s", (int)obj.len, obj.p);
	}

	sva_from_sv(&obj, path_father(sv_from_sva(&target->name)));
	if (obj.p && access(obj.p, F_OK) != 0) {
		path_mkdir(sv_from_sva(&obj), 0755);
	}

	EXEC_AND_PRINT();
	sva_free(&obj);
	sva_free(&cmd);
	return ret == 0;
}

static bool build_obj2alib(Target_t *target)
{
	if (!target) return false;
	if (target->depend_len == 0) return false;
	SVA_t cmd = {};

	sva_from_sv(&cmd, path_father(sv_from_sva(&target->name)));
	if (cmd.p && access(cmd.p, F_OK) != 0) {
		path_mkdir(sv_from_sva(&cmd), 0755);
	}

	sva_sprintf(&cmd, "%s '%.*s'", "ar rcs ", (int)target->name.len, target->name.p);
	for (size_t i = 0; i < target->depend_len; i++) {
		if (!target->dependencies[i]) break;
		sva_sprintfcat(&cmd, " '%.*s'",
			       (int)target->dependencies[i]->name.len,
			       target->dependencies[i]->name.p);
	}
	EXEC_AND_PRINT();
	sva_free(&cmd);
	return ret == 0;
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
		} else {
			fprintf(stderr, "ERROR 无法打开文件夹:%s\n", path.p);
			fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		}
		sva_free(&path);
		return list;
	}
	Path_t tmp = {0};
	struct dirent *dp_item = NULL;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		if (rule && rule(sv_from_cstr(dp_item->d_name), dp_item->d_type) == false) continue;
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
		ret->cfgdata = (void*)(CLIBS+i);    /* 存储配置指针 */
		ret->build = build_obj2alib;
		ret->type = TY_DEP;
		target_append(list, ret);
		// 追加表源文件
		for (j = 0; j < countof(CLIBS[i].sources); j++) {
			sourcefile = sv_from_cstr(CLIBS[i].sources[j]);
			if (sourcefile.len == 0) break;
			if (sv_end_with(sourcefile, sv_from_lstr("/*"))) {
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

		sva_sprintf(&libfile, "lib%.*s.so", (int)libname.len, libname.p);
		void *handle = NULL;
		handle = dlopen(libfile.p, RTLD_NOW);
		if (handle) {
			stat = true;
			dlclose(handle);
		}
		if (!stat) {
			const char *prefix = getenv("PREFIX");
			if (!prefix) prefix = "/usr";
			sva_sprintf(&libfile, "%s/lib/lib%.*s.a", prefix, (int)libname.len, libname.p);
			stat = (access(libfile.p, F_OK) == 0);
		}
		if (stat) {
			ret = target_get_or_create(list, sv_from_sva(&libfile));
			if (ret) {
				ret->status = TS_SUCCESS;
				ret->type = TY_PHONY;
			}
		}
	}
	if (!ret) {
		sva_sprintf(&libfile, "<lib_not_found>%.*s", (int)libname.len, libname.p);
		ret = target_get_or_create(list, sv_from_sva(&libfile));
		if (ret) {
			ret->status = TS_FAILD;
			ret->type = TY_PHONY;
		}
	}
	sva_free(&libfile);
	return ret;
}

static bool check_header(SV_t content, SV_t header)
{
	if (!content.p || !header.p) return false;
	char buf[1024] = {};
	SV_t match = {},
	     father = sv_end_with(header, sv_from_lstr("/*")) ? path_father(header) : (SV_t){},
	     line;
	int count = 0;
	while (content.len && count < 30) {
		line = sv_chop_by_delim(&content, '\n');
		count++;
		if (sscanf(line.p, " # include <%[^>]>", buf) > 0) {
			match = sv_from_cstr(buf);
		} else if (sscanf(line.p, " # include \"%[^\"]\"", buf) > 0) {
			match = sv_from_cstr(buf);
		} else match = (SV_t){};

		if (match.p) {
			if (father.p && sv_end_with(path_father(match), father)) return true;
			else if (!father.p && sv_end_with(match, header)) return true;
		}
	}
	return false;
}

/* 自动为elf目标导入需要链接的库 */
static void autoimport_by_header(Target_t *list, Target_t *target_elf, Target_t *target_c)
{
	if (!list || !target_elf) return;
	SVA_t content = {};
	path_readfile(sv_from_sva(&target_c->name), &content, 10*PATH_MAX);
	if (!content.p) return;
	Target_t *lib = NULL;
	size_t i, j;
	for (i = 0; i < countof(CLIBS); i++) {
		for (j = 0; CLIBS[i].header[j]; j++) {
			if (!check_header(sv_from_sva(&content), sv_from_cstr(CLIBS[i].header[j])))
				continue;
			lib = get_target_by_libname(list, sv_from_cstr(CLIBS[i].libname));
			if (!lib) continue;
			target_depend_append(target_elf, lib);
		}
	}
	sva_free(&content);
	return;
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
		if (sv_begin_with(d_name, sv_from_lstr("lib"))) return false;
		return true;
	}
	/* 跳过非文件 */
	if (d_type != DT_REG)
		return false;
	if (sv_end_with(d_name,sv_from_lstr(".c")))
		return true;
	return false;
}

static Target_t *action_c_elf(Target_t *list, SV_t full_path)
{
	if (!full_path.p) return list;
	Target_t *target_c, *target_obj, *target_elf, *target_lib;
	Path_t tmp = {0};

	/* 创建.c目标 */
	target_c = target_get_or_create(list, full_path);
	if (!list) list = target_c;
	if (target_c) target_c->type = TY_DEP;

	sva_from_sv(&tmp, full_path);
	path_hander_obj_replace(&tmp);
	/* 创建.o目标 */
	target_obj = target_get_or_create(list, sv_from_sva(&tmp));
	if (target_obj) {
		target_obj->type = TY_DEP;
		target_obj->build = build_c2obj;
	}
	target_depend_append(target_obj, target_c);  // obj 依赖 .c

	sva_from_sv(&tmp, full_path);
	path_hander_elf(&tmp);
	/* 创建elf目标 */
	target_elf = target_get_or_create(list, sv_from_sva(&tmp));
	if (target_elf) {
		target_elf->build = build_obj2elf;
	}
	target_depend_append(target_elf, target_obj);  // elf 依赖 .o

	/* 自动导入 */
	autoimport_by_header(list, target_elf, target_c);
	/* 查找显式要求的库 */
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
	if (target_obj) target_obj->cfgdata = (void*)flag;    /* 为obj添加cfg */
	if (target_elf) target_elf->cfgdata = (void*)flag;    /* 为elf添加cfg */
	SV_t deps = sv_from_cstr(flag->deps), left;
	deps = sv_from_cstr(flag->libs);
	while (target_elf && deps.len > 0) {
		sv_trim_left_by_type(&deps, isspace);
		if (deps.len <= 0) break;
		left = sv_chop_by_type(&deps, isspace);
		if(left.len <= 0) break;

		target_lib = get_target_by_libname(list, left);
		if (!target_lib) target_elf->status = TS_FAILD;
		else target_lib->build = build_obj2alib;
		target_depend_append(target_elf, target_lib);  // elf 依赖 libs
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
		if (sv_end_with(testpath, sv_from_lstr("/*"))) sv_chop_right(&testpath, 1);
		else testpath = path_father(testpath);
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

static Path_t *path_hander_self(Path_t* ret)
{
	sva_free(ret);
	return sva_from_cstr(ret, "./build");
}

static Target_t *self_check(char *argv[] ,char *oldpwd)
{
	if (!argv || !*argv) return NULL;

	// 处理目录切换问题
	char *path_list[] = {argv[0], "/proc/self/exe"};
	size_t i = 0;
	while (i < countof(path_list)) {
		if (path_list[i] && access(path_list[i], F_OK) == 0) break;
		i++;
	}
	if (i >= countof(path_list)) return NULL;
	SVA_t exe_path = {0};
	if (i == 1) {
		char *path = realpath(path_list[i], NULL);
		if (!path) return NULL;
		sva_from_cstr(&exe_path, path);
		free(path);
		printf("[INFO] 全局模式工作\n");
	} else sva_from_cstr(&exe_path, path_list[i]);
	path_normalize(&exe_path);
	if (!exe_path.p) return NULL;
	SVA_t parent = {0};
	sva_from_sv(&parent, path_father(sv_from_sva(&exe_path)));
	sva_free(&exe_path);
	if (!parent.len) sva_sprintf(&parent, "./");
	if (!parent.p) return NULL;
	if (parent.len == parent.capacity) sva_double(&parent);
	parent.p[parent.len] = 0;
	if (chdir(parent.p) < 0) {
		printf("[INFO] 切换工作区失败: '%.*s'\n", (int)parent.len, parent.p);
		sva_free(&parent);
		return NULL;
	}
	// 自动重构建
	Target_t *list = target_fordir(NULL, NULL, sv_from_cstr(__FILE_NAME__), rule_fordir, action_c_elf);

	/* 篡改自身ELF目标位置 */
	path_hander_elf(sva_from_cstr(&parent, __FILE_NAME__));
	Target_t *target_self = target_get_by_name(list, sv_from_sva(&parent));
	sva_free(&parent);
	if (!list) return NULL;
	if (!target_self) return list;
	path_hander_self(sva_from_cstr(&target_self->name, __FILE_NAME__));

	/* 独自重构可能拖慢运行速度 */
	target_buildlist(list);
	if (target_self->status == TS_SUCCESS && target_self->isupdated) {
		/* 切回原本的目录 */
		chdir(oldpwd);
		if (execvp(argv[0], argv) == -1)
			target_self->status = TS_FAILD;
		return list;
	}
	return list;
}

#ifndef BUILD_C_AS_LIB
int main(int argc, char *argv[])
{
	printf("[INFO] 构建程序 ("__FILE__") 构建时间 "__TIMESTAMP__"\n");

	int mode = 0, ret = 0;
	SVA_t pwd = {}, tmp = {}, newpwd = {};
	sva_adjust_minimun(&pwd, 1024);
	getcwd(pwd.p, pwd.capacity-1);
	pwd.len = strlen(pwd.p);

	Target_t *list = self_check(argv, pwd.p);
	if (!list) {
		ret = 1;
		goto EXIT_AND_CLEANUP;
	}

	if (argc <= 1) list = target_fordir(list, NULL, sv_from_cstr(SOURCE_DIR), rule_fordir, action_c_elf);
	else {
		if (strcmp(argv[1], "help") == 0) {
			printf("Usage: %s [help|clean|list|run] <...>\n", argv[0]);
			goto EXIT_AND_CLEANUP;
		} else if (strcmp(argv[1], "clean") == 0) {
			// 好像暂时还会乱删东西。。。
			// ret += path_remove(sv_from_lstr(BUILD_DIR));
			// ret += path_remove(sv_from_lstr(BIN_DIR));
			goto EXIT_AND_CLEANUP;
		} else if (strcmp(argv[1], "list") == 0) {
			mode = 1;
			target_freelist(list);
			list = NULL;
		} else if (strcmp(argv[1], "run") == 0) {
			mode = 2;
		}
	}

	for (int i = 1+(mode?1:0); i < argc;) {
		sva_adjust_minimun(&newpwd, 1024);
		getcwd(newpwd.p, newpwd.capacity-1);
		newpwd.len = strlen(newpwd.p);
		path_normalize(sva_sprintfcat(&newpwd, "/"));
		SV_t path;
		for (; i < argc; i++) {
			path_normalize(sva_sprintf(&tmp, "%s/%s", pwd.p, argv[i]));
			path = sv_from_sva(&tmp);
			if (!sv_begin_with(path, sv_from_sva(&newpwd))) continue;
			sv_chop_left(&path, newpwd.len);
			path_normalize(sva_from_sv(&tmp, path));
			list = target_fordir(list, NULL, sv_from_sva(&tmp), rule_fordir, action_c_elf);
			if (mode == 2) break;
		}
		sva_free(&tmp);
		sva_free(&newpwd);
		break;
	}

	if (mode == 1) {
		if (argc == 2) target_fordir(list, NULL, sv_from_cstr(SOURCE_DIR), rule_fordir, action_c_elf);
		target_printlist(list, 0b110);
		goto EXIT_AND_CLEANUP;
	}
	/* 打印 */
	// target_printlist(list, 0);
	// printf("[INFO] 运行构建\n");
	target_buildlist_for_pthread(list, 8);
	target_printlist(list, 0);

	while (mode == 2 && argc >= 3) {
		sva_adjust_minimun(&newpwd, 1024);
		getcwd(newpwd.p, newpwd.capacity-1);
		newpwd.len = strlen(newpwd.p);
		path_normalize(sva_sprintfcat(&newpwd, "/"));
		SV_t path;
		path_normalize(sva_sprintf(&tmp, "%s/%s", pwd.p, argv[2]));
		path = sv_from_sva(&tmp);
		if (!sv_begin_with(path, sv_from_sva(&newpwd))) {
			ret = 1;
			break;
		}
		sv_chop_left(&path, newpwd.len);
		path_normalize(sva_from_sv(&tmp, path));

		path_hander_elf(&tmp);
		Target_t *target = target_get_by_name(list, sv_from_sva(&tmp));
		if (target && target->status == TS_SUCCESS && access(tmp.p, F_OK) == 0) {
			char *exe = realpath(tmp.p, NULL);
			printf("[INFO] 准备execvp到'%s'\n", exe);
			printf("----------------------------------------\n");
			argv[2] = tmp.p;
			chdir(pwd.p);
			if (execvp(exe, argv+2) == -1)
				printf("[ERROR] execvp失败: (%s) %s\n", tmp.p, strerror(errno));
			if (exe) free(exe);
		}
		break;
	}

EXIT_AND_CLEANUP:
	sva_free(&tmp);
	sva_free(&newpwd);
	sva_free(&pwd);
	target_freelist(list);
	return ret;
}
#endif
