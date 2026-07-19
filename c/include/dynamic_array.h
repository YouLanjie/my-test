/**
 * @file        dynamic_array.h
 * @author      Chglish
 * @date        2026-07-19
 * @brief       简陋的动态数组声明
 */

#pragma once

#ifndef _DYNAMIC_ARRAY_H
#define _DYNAMIC_ARRAY_H

#include <stddef.h>

typedef struct {
	void  *ptr;
	size_t cap;
	size_t len;
	const size_t size;
} DA_t;

/* 需要提前初始化好.size字段
 * 申请内存，至少可用长度cap(不会释放多出的空间) */
DA_t *da_create(DA_t *da, size_t cap);
/* 在idx处插入含有指向ptr指针的元素(让da->ptr[idx] = ptr) */
DA_t *da_insert(DA_t *da, size_t idx, void *ptr);
/* 将ptr里的内容（长度da->size）追加到末尾数组 */
DA_t *da_append(DA_t *da, void *ptr);
DA_t *da_free(DA_t *da, void (*free_function)(void *ptr));
DA_t *da_pop(DA_t *da, size_t idx, void (*free_function)(void *ptr));
/* 通过索引id获取对应元素指针 */
void *da_get(DA_t *da, size_t idx);

#endif //DYNAMIC_ARRAY_H

