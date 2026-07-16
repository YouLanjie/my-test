/**
 * @file        path.h
 * @author      Chglish
 * @date        2026-07-14
 * @brief       简要描述该文件的作用
 */

#pragma once

#ifndef _PATH_H
#define _PATH_H

#include "string_view.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

typedef SVA_t Path_t;

typedef struct {
	struct stat st;
	bool isexist;
	bool islink;
	bool isdir;
	bool isfile;
} Path_st_t;

#define path_from_cstr sva_from_cstr
#define path_from_sv sva_from_sv

/**
 * @brief 获得文件名
 *
 * @param path 要判断的字符串
 * @return 截取后的字符串
 */
SV_t path_basename(SV_t path);
/* @brief 获得文件前缀
 * @return 不存在`.`时长度为0 */
SV_t path_stemname(SV_t path);
/* @brief 获得文件后缀(保留`.`)
 * @return 不存在`.`时长度为0 */
SV_t path_suffixname(SV_t path);
/**
 * @brief 获得上级路径
 *
 * @param path 要判断的字符串
 * @return 截取后的字符串
 */
SV_t path_father(SV_t path);
/**
 * @brief 规范化path路径并保存到path
 *
 * @param path 要规范化的路径（兼保存地址）
 * @return 规范化后的路径（一般与传入的path相同）
 */
Path_t * path_normalize(Path_t *path);
/**
 * @brief 拼接路径到path
 *
 * @param path 前缀、保存地址
 * @param child 尾缀
 * @return 拼接后路径
 */
Path_t *path_join(Path_t *path, SV_t child);
Path_st_t path_get_st(Path_t f);
/* 可递归创建文件夹 */
int path_mkdir(SV_t path, int mode);
/* @brief 读取文件内容最多maxsize并保存到dest
 * @return 保存地址dest，出错为NULL */
SVA_t *path_readfile(SV_t path, SVA_t *dest, size_t maxsize);

#endif //PATH_H

