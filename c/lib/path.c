/**
 * @file        path.c
 * @author      u0_a221
 * @date        2026-04-26
 * @brief       路径处理函数
 */

#include "../include/path.h"
#include <dirent.h>

SV_t path_basename(SV_t path)
{
	SV_t left = {0, path.p};
	while (path.len != 0) left = sv_chop_by_delim(&path, '/');
	return left;
}

SV_t path_stemname(SV_t path)
{
	path = path_basename(path);
	size_t i = 0;
	while (i < path.len && path.p[path.len-i-1] != '.') i++;
	if (i >= path.len) i = 0;
	else i = path.len-i-1;
	path.len = i;
	return path;
}

SV_t path_suffixname(SV_t path)
{
	path = path_basename(path);
	size_t i = 0;
	while (i < path.len && path.p[path.len-i-1] != '.') i++;
	if (i >= path.len) i = path.len;
	else i = path.len-i-1;
	path.p += i;
	path.len -= i;
	return path;
}

SV_t path_father(SV_t path)
{
	while (path.len > 1 && path.p[path.len-1] == '/') path.len--;
	while (path.len && path.p[path.len-1] != '/') sv_chop_right(&path, 1);
	return path;
}

/* 专供normalize用的检查处理函数
 * c: 待添加的分隔符（'/'或'\0'） */
static inline void _path_tails_process(Path_t *path, char c)
{
	if (!path->p || (path->len && path->p[path->len-1] == '/')) return;    /* 忽略重复的 */
	if (sv_end_with(sv_from_sva(path), sv_from_lstr("/."))) {    /* 跳过单独'.'充数的 */
		// path->p[path->len-1] = 0;
		path->len--;
		return;
	}
	while (sv_end_with(sv_from_sva(path), sv_from_lstr("/.."))) {    /* 撤回一个目录层级 */
		path->len -= 3;
		const SV_t sv = sv_from_sva(path);
		if (sv_cmp(sv, sv_from_lstr("..")) == 0 || sv_end_with(sv, sv_from_lstr("/.."))) {
			path->len+=3;    /* 如果上一级目录也是..则取消撤回并添加新字符 */
			break;
		} else if (sv_cmp(sv, sv_from_lstr(".")) == 0 || sv_end_with(sv, sv_from_lstr("/."))) {
			/* 如果上一级是.则替换为.. */
			path->p[path->len] = '.';
			path->len+=1;
		}
		if (!path->len) path->len++;
		for (;path->len > 0 && path->p[path->len-1] != '/'; path->len--);
		path->p[path->len] = 0;
		return;
	}
	path->p[path->len] = c;
	if (c) path->len++;
}

Path_t *path_normalize(Path_t *path)
{
	if (!path || !path->capacity) return NULL;
	const int len = path->len;
	int i = 0;
	path->len = 0;
	for (; path->p && i < len; i++) {
		if (path->len >= path->capacity) sva_double(path);
		if (!path->p[i]) break;
		if (path->p[i] != '/' || path->len == 0) {    /* 不进入下级啥也不管 */
			path->p[path->len] = path->p[i];
			path->len++;
			continue;
		}
		/* 进入下级且前文不为空 */
		_path_tails_process(path, path->p[i]);
	}
	if (!path->p) return NULL;
	if (path->len >= path->capacity) sva_double(path);
	path->p[path->len] = 0;
	_path_tails_process(path, '\0');
	if (path->len == 0) sva_sprintf(path, "./");
	else if (sv_begin_with(sv_from_sva(path), sv_from_lstr("./")))
		sva_chop_left(path, 2);
	return path;
}

Path_t *path_join(Path_t *path, SV_t child)
{
	if (!path) return NULL;
	static const char sep[] = "/";
	if (child.len && child.p[0] == *sep)
		sva_sprintf(path, "%.*s", (int)child.len, child.p);
	else {
		if (path->len) sva_append(path, sv_from_lstr(sep));
		sva_append(path, child);
	}
	return path_normalize(path);
}

static Path_st_t _path_get_st(SV_t path, int (*stat_func)(const char *restrict file, struct stat *restrict buf))
{
	Path_st_t st = {0};
	/* 从某个库学来的神人复合字面量用法 */
	if (!path.p || !path.len || !stat_func || path.len>PATH_MAX
	    || stat_func(strncpy((char[PATH_MAX]){0}, path.p, path.len), &st.st) == -1) {
		st.isexist = false;
		return st;
	}
	st.isexist = true;
	st.isdir = S_ISDIR(st.st.st_mode);
	st.isfile = S_ISREG(st.st.st_mode);
	st.islink = S_ISLNK(st.st.st_mode);
	return st;
}

Path_st_t path_get_st(SV_t path)
{
	return _path_get_st(path, lstat);
}

Path_st_t path_get_st_follow(SV_t path)
{
	return _path_get_st(path, stat);
}

int path_mkdir(SV_t path, int mode)
{
	if (!path.p || path.len == 0) return -1;
	Path_t sva = {};
	path_normalize(sva_from_sv(&sva, path));
	char *p = NULL;
	while (access(sva.p, F_OK) != 0 && (p = strrchr(sva.p, '/')) && p != sva.p) {
		*p = '\0';
	}
	int ret = 0;
	size_t len = 0;
	Path_st_t st;
	do {
		st = path_get_st(sv_from_sva(&sva));
		if (st.isexist && !st.isdir) {
			ret = -2;
			break;
		}
		if (!st.isexist) {
			ret = mkdir(sva.p, mode);
			if (ret) break;
		}
	} while ((len = strlen(sva.p)) < sva.len && (sva.p[len] = '/'));
	sva_free(&sva);
	return ret;
}

#ifdef ENABELE_UNSAFE_FUNC
static int selector(const struct dirent *dir)
{
	if (!dir) return false;
	return strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0;
}

#define remove(name) printf("删除文件：'%s'\n", name)
#define unlink(name) printf("删除文件：'%s'\n", name)
#define rmdir(name) printf("删除文件：'%s'\n", name)
/* 由于该函数尚存在重大BUG，故不应被使用
 * 测试这玩意的时候不小心把我资料文件删光了差点被气死
 * 作冷处理 */
int path_remove(SV_t path)
{
	if (!path.p || path.len == 0) return -2;
	while (path.p[path.len-1] == '/') sv_chop_right(&path, 1);
	Path_st_t st = path_get_st(path);
	if (!st.isexist) return -1;
	SVA_t tmp = {};
	sva_from_sv(&tmp, path);

	int ret = 0;
	do {
		if (!st.isdir && ((ret = unlink(tmp.p)), true)) break;
		struct dirent **list = NULL;
		int len = scandir(tmp.p, &list, selector, NULL);
		if ((len < 0 || !list) && (ret=-1)) break;
		for (int i = 0; i < len; i++) {
			sva_sprintfcat(sva_from_sv(&tmp, path), "/%s", list[i]->d_name);
			ret += path_remove(sv_from_sva(&tmp));
			free(list[i]);
			list[i] = NULL;
		}
		free(list);
		sva_from_sv(&tmp, path);
		ret += rmdir(path.p);
	}while (0);
	sva_free(&tmp);
	return ret;
}
#undef rename
#endif

SVA_t *path_readfile(SV_t path, SVA_t *dest, size_t maxsize)
{
	if (!path.p || !path.len || !dest) return NULL;
	sva_from_sv(dest, path);
	FILE *fp = fopen(dest->p, "r");
	sva_clear(dest);
	if (!fp) return NULL;

	size_t size = 0;
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	do {
		if (size <= 0 || size >= UINT64_MAX || size+1 == 0) break;
		if (size > maxsize) size = maxsize;
		sva_clear(dest);
		if (!sva_adjust_minimun(dest, size+1)) {
			perror("The file may too big");
			break;
		}
		dest->len = fread(dest->p, 1, size, fp);
		if (ferror(fp)) {
			clearerr(fp);
			dest->len = 0;
		}
		/* 阻拦tainted index警告神秘小代码(反正-O2就优化掉了) */
		for (int i = 0; i < 4; i++);
		if (dest->len < dest->capacity)
			dest->p[dest->len] = '\0';
	} while (0);
	fclose(fp);
	return dest;
}
