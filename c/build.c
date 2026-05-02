/**
 * @file        build.c
 * @author      u0_a221
 * @date        2026-04-26
 * @brief       构建脚本
 */

#include "lib/path.c"
#include "lib/string_view.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <wait.h>

typedef struct {
	const char *libname;
	const char *sources[5];
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
#define SOURCE_DIR "./src/"
#define LIB_DIR    "./lib/"
#define BUILD_DIR  "./.build/"
#define BIN_DIR    "./bin/"
#define COMPILOR   "gcc"
#define CCOMFLAGS  "-Wall -Wextra -O2 -g"
#define CLINKFLAGS "-L"BUILD_DIR" -lctools"


#define LIB(l, ...) (CLIBS_t){.libname=BUILD_DIR l, .sources={__VA_ARGS__}}
CLIBS_t CLIBS[] = {
	LIB("libctools.a", LIB_DIR"tools.c", LIB_DIR"print_in_box.c", LIB_DIR"path.c", LIB_DIR"string_view.c"),
	LIB("libcmenu.a", LIB_DIR"menu.c"),
};
#undef LIB

#define FLG(f, l, ...) (CFLAGS_t){.filename=SOURCE_DIR f, .libs=l, __VA_ARGS__}
#define MUS(f, l, ...) FLG("musicSynth/" f, "m "l, .flg_comp="-fopenmp", .flg_link="-fopenmp", __VA_ARGS__)
#define MUSDEP .deps="lib/core.c lib/wave_func.c lib/note_parser.c lib/melody.c"
CFLAGS_t CFILEFLAGS[] = {
	MUS("music_synth.c",      , MUSDEP),
	MUS("alsa_play.c",        "asound", MUSDEP),

	FLG("tests/libav_test.c", "avformat avcodec avutil swresample m"),
	FLG("tests/social.c",     "m"),
	FLG("tests/try_iconv.c",  "iconv"),
	FLG("lrc.c",              "SDL2_mixer SDL2"),
	FLG("tetris.c",           "ncurses"),
	FLG("render3d.c",         "m"),
	FLG("tests/input.c",      "m"),
	FLG("tests/sin.c",        "m"),
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

/* RET:
 * <0: faild
 * 0: 无更新
 * &(1<<0): 完成obj编译
 * &(1<<1): 完成elf编译
 * */
int build_file(Path_t source, Path_t (*obj_hander)(Path_t), Path_t (*elf_hander)(Path_t), CFLAGS_t cflags)
{
	if (!obj_hander) return -1;
	const CFLAGS_t *flag = &(CFLAGS_t){.filename=NULL};
	Path_t obj;
	uint8_t stat = 0;
	source = path_normalize(source.path);
	for (uint64_t i = 0; i < ARRAY_LEN(CFILEFLAGS); i++) {
		if (!CFILEFLAGS[i].filename) continue;
		obj = path_normalize(CFILEFLAGS[i].filename);    // 借用obj
		if (strcmp(obj.path, source.path) != 0) continue;
		flag = CFILEFLAGS+i;
		break;
	}

	char cmd[PATH_MAX*5] = {0};
	obj = obj_hander(source);
	int ret = is_newer(obj.path, 1, (const char*[]){source.path});
	if (ret < 0) {
		fprintf(stderr, "[WARN] 文件缺失 %s\n", source.path);
		return -3;
	}
	if (ret > 0) {
		sprintf(cmd, COMPILOR" -c -o \"%s\" \"%s\"", obj.path, source.path);
		if (cflags.flg_comp) {
			strlcat(cmd, " ", sizeof(cmd));
			strlcat(cmd, cflags.flg_comp, sizeof(cmd));
		}
		if (flag->flg_comp) {
			strlcat(cmd, " ", sizeof(cmd));
			strlcat(cmd, flag->flg_comp, sizeof(cmd));
		}
		if (run_cmd(cmd) != 0) return -4;
		stat |= 0b1;
	}
	if (!elf_hander || flag->no_elf) return stat;

	SV_t prefix, deps = sv_from_cstr(flag->deps), left;
	int size = 0;
	for (char *p = source.path; p && *p; p++) if (*p == '/') size = p-source.path;
	prefix = (SV_t){.p=source.path, .len=size};
	size = 0;
	while (deps.len > 0) {    /* 检查明确指定的依赖文件 */
		sv_trim_left_by_type(&deps, isspace);    /* 跳过空格 */
		if (deps.len <= 0) break;
		left = sv_chop_by_type(&deps, isspace);
		if(left.len <= 0) break;
		if (build_file(path_join(path_from_sv(prefix), left), obj_hander, NULL, cflags) > 0)    /* 设置elf_hander为NULL防递归 */
			stat |= 0b100;    /* 标记依赖更新 */
		size++;
	}

	Path_t elf = elf_hander(source);
	ret = is_newer(elf.path, 1, (const char*[]){obj.path});
	if (ret < 0) {
		fprintf(stderr, "[WARN] 文件缺失 %s\n", obj.path);
		return -5;
	}
	if (ret > 0 || stat & 0b100) {
		sprintf(cmd, COMPILOR" -o \"%s\" \"%s\"", elf.path, obj.path);
		if (cflags.flg_link) {
			strlcat(cmd, " ", sizeof(cmd));
			strlcat(cmd, cflags.flg_link, sizeof(cmd));
		}
		if (flag->flg_link) {
			strlcat(cmd, " ", sizeof(cmd));
			strlcat(cmd, flag->flg_link, sizeof(cmd));
		}
		deps = sv_from_cstr(flag->deps);
		while (deps.len > 0) {
			sv_trim_left_by_type(&deps, isspace);
			if (deps.len <= 0) break;
			left = sv_chop_by_type(&deps, isspace);
			if(left.len <= 0) break;
			strlcat(cmd, " \"", sizeof(cmd));
			strlcat(cmd, obj_hander(path_join(path_from_sv(prefix), left)).path, sizeof(cmd));
			strlcat(cmd, "\"", sizeof(cmd));
		}
		deps = sv_from_cstr(flag->libs);
		while (deps.len > 0) {
			sv_trim_left_by_type(&deps, isspace);
			if (deps.len <= 0) break;
			left = sv_chop_by_type(&deps, isspace);
			if(left.len <= 0) break;
			strlcat(cmd, " -l", sizeof(cmd));
			strncat(cmd, left.p, left.len);
		}
		if (run_cmd(cmd) != 0) return -6;
		stat |= 0b10;
	}
	return stat;
}

/* 将路径变为 BUILD_DIR/xxx_xxx_xxx.o */
Path_t path_hander_obj_replace(Path_t path)
{
	for (char *p = path.path; p && *p; p++) if (*p == '/') *p = '_';
	SV_t sv = path_basename(path);
	while (sv.len > 0 && (sv.p[0] == '.' || sv.p[0] == '_')) sv_chop_left(&sv, 1);
	path = path_join((Path_t){BUILD_DIR}, sv);
	strlcat(path.path, ".o", sizeof(path.path));
	return path;
}
/* 将路径变为 BIN_DIR/xxx */
Path_t path_hander_elf(Path_t path)
{
	SV_t sv = path_basename(path);
	path = path_join((Path_t){BIN_DIR}, sv);
	return path;
}

void build_libs()
{
	char cmd[5*PATH_MAX] = {0};
	Path_t path = {0};
	int ret = 0;
	for (uint64_t i = 0; i < ARRAY_LEN(CLIBS); i++) {
		ret = is_newer(CLIBS[i].libname, ARRAY_LEN(CLIBS[i].sources), CLIBS[i].sources);
		if (ret < 0) {
			fprintf(stderr, "[WARN] 库源文件缺失: %s\n", CLIBS[i].sources[-i-1]);
			continue;
		}
		if (ret == 0) continue;
		sprintf(cmd, "ar rcs \"%s\" ", CLIBS[i].libname);
		uint64_t j = 0;
		for (j = 0; j < ARRAY_LEN(CLIBS[i].sources) && CLIBS[i].sources[j]; j++) {
			strlcpy(path.path, CLIBS[i].sources[j], sizeof(path.path));
			build_file(path, path_hander_obj_replace, NULL, (CFLAGS_t){.flg_comp=CCOMFLAGS});

			path = path_hander_obj_replace(path);
			int size = strlen(cmd);
			snprintf(cmd+size, sizeof(cmd)-size, " \"%s\"", path.path);
		}
		if (j == 0) continue;
		run_cmd(cmd);
	}
}


void fordir(char *cwd, char *dirname)
{
	if (!dirname) return;
	if (!cwd) cwd = "./";
	if (str_start_with(dirname, "lib")) return;

	static int pcount = 0;
	Path_t path = {0};
	sprintf(path.path, "%s/%s", cwd, dirname);
	path = path_normalize(path.path);

	DIR *dp = opendir(path.path);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", dirname);
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
			fordir(path.path, dp_item->d_name);
		}
		if (type != DT_REG || !str_end_with(dp_item->d_name,".c"))
			continue;
		while (pcount >= 8) if (wait(NULL) > 0) pcount--;
		pid_t pid = fork();
		if (pid == 0) {
			build_file(path_join(path, sv_from_cstr(dp_item->d_name)),
				   path_hander_obj_replace, path_hander_elf,
				   (CFLAGS_t){.flg_comp=CCOMFLAGS, .flg_link=CLINKFLAGS});
			exit(0);
		}
		pcount++;
	}
	closedir(dp);
	return;
}

static Path_t path_hander_self(Path_t c)
{
	(void)c;    /* 消警告用的 */
	return (Path_t){"./build"};
}

int main(int argc, char *argv[])
{
	(void)argc;
	printf("[INFO] 构建程序 ("__FILE__") 构建时间 "__TIMESTAMP__"\n");
	mkdir(BUILD_DIR, 0755);
	mkdir(BIN_DIR, 0755);
	if (build_file((Path_t){__FILE_NAME__}, path_hander_obj_replace,
		       path_hander_self, (CFLAGS_t){.flg_comp=CCOMFLAGS}) == 0b11) {
		execvp(argv[0], argv);
		return 1;
	}
	build_libs();
	fordir(NULL, SOURCE_DIR);
	while(wait(NULL) > 0) usleep(0010000);
	return 0;
}

