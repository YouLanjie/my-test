/**
 * @file        dynamic_array.c
 * @author      Chglish
 * @date        2026-07-19
 * @brief       简陋的动态数组实现
 */

#include "../include/dynamic_array.h"
#include <stdlib.h>
#include <string.h>

#define DAP(da, offset) (da->ptr+(da->size*(offset)))

DA_t *da_create(DA_t *da, size_t cap)
{
	if (!da || da->size == 0) return NULL;
	if (cap == 0) cap = 10;
	if (da->ptr && da->cap >= cap) return da;
	if (da->len > da->cap) da->len = 0;
	void *p = da->ptr;
	da->cap = cap;
	if (da->ptr) da->ptr = realloc(da->ptr, da->size*da->cap);
	else da->ptr = malloc(da->size*da->cap);

	if (!da->ptr) {
		if (p) free(p);
		da->cap = 0;
		da->len = 0;
		return NULL;
	}
	memset(DAP(da, da->len), 0, da->size*(da->cap-da->len));
	return da;
}

DA_t *da_insert(DA_t *da, size_t idx, void *ptr)
{
	if (!da || da->size == 0) return NULL;
	if (idx >= da->cap) {
		if (!da_create(da, idx)) return NULL;
	}
	if (da->len+1 > da->cap) {
		if (!da_create(da, da->cap*2)) return NULL;
	}
	if (idx < da->len) {
		memmove(DAP(da, idx+1), DAP(da, idx), da->size*(da->len-idx));
	}
	memcpy(DAP(da, idx), ptr, da->size);
	da->len++;
	return da;
}

DA_t *da_append(DA_t *da, void *ptr)
{
	if (!da || da->size == 0) return NULL;
	da_insert(da, da->len, ptr);
	return da;
}

DA_t *da_free(DA_t *da, void (*free_function)(void *ptr))
{
	if (!da || da->size == 0) return NULL;
	if (da->ptr) {
		for (size_t i = 0; i < da->len && free_function; i++) {
			if (DAP(da, i)) free_function(DAP(da, i));
		}
		free(da->ptr);
	}
	da->len = 0;
	da->cap = 0;
	da->ptr = NULL;
	return da;
}

DA_t *da_pop(DA_t *da, size_t idx, void (*free_function)(void *ptr))
{
	if (!da || !da->ptr || da->size == 0) return NULL;
	if (idx >= da->len) return da;
	if (free_function && DAP(da, idx)) free_function(DAP(da, idx));
	if (idx < da->len-1) memmove(DAP(da, idx), DAP(da, idx+1), da->size*(da->len-idx-1));
	da->len--;
	memset(DAP(da, da->len), 0, da->size);
	return da;
}

void *da_get(DA_t *da, size_t idx)
{
	if (!da || !da->ptr || da->size == 0 || idx >= da->len) return NULL;
	return DAP(da, idx);
}

#undef DAP

