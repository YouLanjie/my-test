/**
 * @file        build.c
 * @author      u0_a221
 * @date        2026-04-26
 * @brief       构建脚本
 */

#include "lib/path.c"
#include "lib/string_view.c"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>    /* isspace */
#include <unistd.h>   /* usleep */
#include <wait.h>     /* wait */

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

#define ARRAY_LEN(v) (sizeof(v)/sizeof(v[0]))

// ===============================
// 配置区
#define MAX_PTR    8
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
	LIB("ctools", LIB_DIR"tools.c", LIB_DIR"print_in_box.c", LIB_DIR"path.c", LIB_DIR"string_view.c", LIB_DIR"target_list.c"),
	// LIB("cmenu", LIB_DIR"menu.c"),
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


/* fl的文件是否比f的新
 * ret: 0不新 1新的 2原文件不存在 <0文件列表缺失(对应编号)
 * */
int is_newer(const char *f, int fl_len, const char *fl[])
{
	if (!f || !fl) return 0;
	struct stat st;
	if (stat(f, &st) == -1) return 2;
	time_t t = st.st_mtim.tv_sec;
	for (int i = 0; i < fl_len && fl[i]; i++) {
		if (stat(fl[i], &st) == -1) return -1-i;    /* 若任意fl[i]不存在: true */
		if (st.st_mtim.tv_sec > t) return 1;
	}
	return 0;
}

int run_cmd(char *cmd)
{
	if (!cmd) return -1;
	printf("[RUN] %s\n", cmd);
	int ret = 0;
	ret = system(cmd);
	if (ret) fprintf(stderr, "[WARN] 命令执行错误(%d): %s\n", ret, cmd);
	return ret;
}

#define sprintfcat(buf, fmt, ...) snprintf((buf)+strlen(buf), sizeof(buf)-strlen(buf), fmt __VA_OPT__(,) __VA_ARGS__)
typedef Path_t*(*PathHander_t)(Path_t*);

/* RET:
 * <0: faild
 * 0: 无更新
 * &(1<<0): 完成obj编译
 * &(1<<1): 完成elf编译
 * */
int build_file(Path_t *source, PathHander_t obj_hander, PathHander_t elf_hander, CFLAGS_t cflags)
{
	if (!obj_hander || !source) return -1;
	const CFLAGS_t *flag = &(CFLAGS_t){.filename=NULL};
	Path_t obj = {0};
	uint8_t stat = 0;
	path_normalize(source);
	for (uint64_t i = 0; i < ARRAY_LEN(CFILEFLAGS); i++, sva_free(&obj)) {
		if (!CFILEFLAGS[i].filename) continue;
		path_normalize(sva_from_cstr(&obj, CFILEFLAGS[i].filename));    // 借用obj
		if (!sva_cmp(&obj, source)) continue;
		flag = CFILEFLAGS+i;
		break;
	}

	SVA_t cmd = {0};
	char str_deps[PATH_MAX*2] = {0};
	char str_libs[PATH_MAX] = {0};
	obj_hander(sva_strcpy(&obj, source));
	int ret = is_newer(obj.p, 1, (const char*[]){source->p});
	if (ret < 0) {
		fprintf(stderr, "[WARN] 文件缺失 %s\n", source->p);
		sva_free(&obj);
		return -3;
	}
	if (ret > 0) {
		sva_sprintf(&cmd, "%s -c -o \"%.*s\" \"%.*s\"", COMPILOR,
			    (int)obj.len, obj.p, (int)source->len, source->p);
		if (cflags.flg_comp) sva_sprintfcat(&cmd, " %s", cflags.flg_comp);
		if (flag->flg_comp) sva_sprintfcat(&cmd, " %s", flag->flg_comp);
		if (run_cmd(cmd.p) != 0) return -4;
		stat |= 0b1;
	}
	sva_free(&cmd);
	if (!elf_hander || flag->no_elf) {
		sva_free(&obj);
		return stat;
	}

	SV_t prefix, deps = sv_from_cstr(flag->deps), left;
	int size = 0;
	for (size_t n = 0; n < source->len; n++) if (source->p[n] == '/') size = n;
	prefix = (SV_t){.p=source->p, .len=size};
	size = 0;
	while (deps.len > 0) {    /* 检查明确指定的依赖文件 */
		sv_trim_left_by_type(&deps, isspace);    /* 跳过空格 */
		if (deps.len <= 0) break;
		left = sv_chop_by_type(&deps, isspace);
		if(left.len <= 0) break;

		Path_t dep = {0};
		path_join(sva_from_sv(&dep, prefix), left);
		if (build_file(&dep, obj_hander, NULL, cflags) > 0)    /* 设置elf_hander为NULL防递归 */
			stat |= 0b100;    /* 标记依赖更新 */

		obj_hander(&dep);    /* 需确保hander不先释放dep */
		sva_sprintfcat(&cmd, " \"%.*s\"", (int)dep.len, dep.p);
		sva_free(&dep);
		size++;
	}

	Path_t elf = {0};
	elf_hander(sva_strcpy(&elf, source));
	deps = sv_from_cstr(flag->libs);
	while (deps.len > 0) {
		sv_trim_left_by_type(&deps, isspace);
		if (deps.len <= 0) break;
		left = sv_chop_by_type(&deps, isspace);
		if(left.len <= 0) break;
		sprintfcat(str_libs, " -l%.*s", (int)left.len, left.p);
		uint64_t i = 0;
		for (i = 0; i < ARRAY_LEN(CLIBS); i++) {
			if (strncmp(CLIBS[i].libname, left.p, left.len) != 0) continue;
			if (is_newer(elf.p, 1, (const char*[]){CLIBS[i].libfile})) stat |= 0b1000;
			break;
		}
	}

	ret = is_newer(elf.p, 1, (const char*[]){obj.p});
	if (ret < 0) {
		fprintf(stderr, "[WARN] 文件缺失 %s\n", obj.p);
		sva_free(&obj);
		sva_free(&elf);
		return -5;
	}
	if (ret > 0 || stat & 0b1100) {
		sva_sprintf(&cmd, COMPILOR" -o \"%s\" \"%s\"", elf.p, obj.p);
		if (cflags.flg_link) sva_sprintfcat(&cmd, " %s", cflags.flg_link);
		if (flag->flg_link) sva_sprintfcat(&cmd, " %s", flag->flg_link);
		if (*str_deps) sva_sprintfcat(&cmd, "%s", str_deps);
		if (*str_libs) sva_sprintfcat(&cmd, "%s", str_libs);
		if (run_cmd(cmd.p) != 0) return -6;
		sva_free(&cmd);
		stat |= 0b10;
	}
	sva_free(&obj);
	sva_free(&elf);
	return stat;
}

/* 将路径变为 BUILD_DIR/xxx_xxx_xxx.o */
Path_t *path_hander_obj_replace(Path_t* path)
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
Path_t *path_hander_elf(Path_t* path)
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

void build_libs()
{
	SVA_t cmd = {0};
	Path_t path = {0};
	int ret = 0;
	int pcount = 0;
	for (uint64_t i = 0; i < ARRAY_LEN(CLIBS); i++) {
		ret = is_newer(CLIBS[i].libfile, ARRAY_LEN(CLIBS[i].sources), CLIBS[i].sources);
		if (ret < 0) {
			fprintf(stderr, "[WARN] 库源文件缺失: %s\n", CLIBS[i].sources[-ret-1]);
			continue;
		}
		size_t j = 0;
		for (j = 0; j < ARRAY_LEN(CLIBS[i].sources) && CLIBS[i].sources[j] && ret <= 0; j++) {
			path_hander_obj_replace(sva_from_cstr(&path, CLIBS[i].sources[j]));
			ret = is_newer(CLIBS[i].libfile, 1, (const char*[]){path.p});
			sva_free(&path);
		}
		if (ret == 0) continue;
		sva_sprintf(&cmd, "ar rcs \"%s\" ", CLIBS[i].libfile);
		for (j = 0; j < ARRAY_LEN(CLIBS[i].sources) && CLIBS[i].sources[j]; j++) {
			sva_from_cstr(&path, CLIBS[i].sources[j]);
			while (pcount >= MAX_PTR) if (wait(NULL) > 0) pcount--;
			if (fork() == 0) {
				build_file(&path, path_hander_obj_replace, NULL, (CFLAGS_t){.flg_comp=CCOMFLAGS});
				sva_free(&path);
				exit(0);
			}
			pcount++;

			path_hander_obj_replace(&path);
			sva_sprintfcat(&cmd, " \"%s\"", path.p);
			sva_free(&path);
		}
		if (j == 0) continue;
		while(wait(NULL) > 0) pcount--, usleep(0010000);
		run_cmd(cmd.p);
		sva_free(&cmd);
	}
	while(wait(NULL) > 0) pcount--, usleep(0010000);
}


void fordir(char *cwd, char *dirname)
{
	if (!dirname) return;
	if (!cwd) cwd = "./";
	if (sv_begin_with(sv_from_cstr(dirname), sv_from_lstr("lib"))) return;

	static int pcount = 0;
	Path_t path = {0};
	path_join(sva_from_cstr(&path, cwd), sv_from_cstr(dirname));

	DIR *dp = opendir(path.p);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", path.p);
		fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		return;
	}
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
			fordir(path.p, dp_item->d_name);
		}
		if (type != DT_REG || !sv_end_with(sv_from_cstr(dp_item->d_name),sv_from_lstr(".c")))
			continue;
		while (pcount >= MAX_PTR) if (wait(NULL) > 0) pcount--;
		pid_t pid = fork();
		if (pid == 0) {
			path_join(&path, sv_from_cstr(dp_item->d_name));
			build_file(&path, path_hander_obj_replace, path_hander_elf, (CFLAGS_t){.flg_comp=CCOMFLAGS, .flg_link=CLINKFLAGS});
			sva_free(&path);
			exit(0);
		}
		pcount++;
	}
	closedir(dp);
	return;
}

static Path_t *path_hander_self(Path_t* ret)
{
	sva_free(ret);
	return sva_from_cstr(ret, "./build");
}

int self_check(char *argv[])
{
	if (!argv || !*argv) return 1;

	// 处理目录切换问题
	char *path_list[] = {argv[0], "/proc/self/exe"};
	size_t i = 0;
	while (i < ARRAY_LEN(path_list)) {
		if (path_list[i] && access(path_list[i], F_OK) == 0) break;
		i++;
	}
	if (i >= ARRAY_LEN(path_list)) return 2;
	SVA_t exe_path = {0};
	if (i == ARRAY_LEN(path_list)-1) {
		char *path = realpath(path_list[i], NULL);
		if (!path) return 3;
		sva_from_cstr(&exe_path, path);
		free(path);
		printf("[INFO] 全局模式工作\n");
	} else sva_from_cstr(&exe_path, path_list[i]);
	path_normalize(&exe_path);
	if (!exe_path.p) return 4;
	SVA_t parent = {0};
	sva_from_sv(&parent, path_father(sv_from_sva(&exe_path)));
	sva_free(&exe_path);
	if (!parent.len) sva_sprintf(&parent, "./");
	if (!parent.p) return 5;
	if (parent.len == parent.capacity) sva_double(&parent);
	parent.p[parent.len] = 0;
	if (chdir(parent.p) < 0) {
		printf("[INFO] 切换工作区失败: '%.*s'\n", (int)parent.len, parent.p);
		sva_free(&parent);
		return 6;
	}
	sva_free(&parent);

	// 新建目录
	if (access(BUILD_DIR, F_OK) != 0) mkdir(BUILD_DIR, 0755);
	if (access(BIN_DIR, F_OK) != 0) mkdir(BIN_DIR, 0755);

	// 自动重构建
	SVA_t self = {0};
	sva_from_cstr(&self, __FILE_NAME__);
	const int result = build_file(&self, path_hander_obj_replace, path_hander_self, (CFLAGS_t){.flg_comp=CCOMFLAGS});
	if (result == 0b11) {
		execvp(argv[0], argv);
		return 7;
	}
	sva_free(&self);
	return 0;
}

#ifndef BUILD_C_AS_LIB
int main(int argc, char *argv[])
{
	(void)argc;
	printf("[INFO] 构建程序 ("__FILE__") 构建时间 "__TIMESTAMP__"\n");
	for (int ret = self_check(argv); ret; ) {
		return ret;
	}

	build_libs();
	fordir(NULL, SOURCE_DIR);
	while(wait(NULL) > 0) usleep(0010000);
	return 0;
}
#endif
