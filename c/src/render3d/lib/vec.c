/**
 * @file        vec.c
 * @author      u0_a221
 * @date        2026-05-03
 * @brief       向量的包装
 */

#include "render3d.h"

Vec_t vec_add(Vec_t v1, Vec_t v2)
{
	return (Vec_t){
		.x = v1.x+v2.x,
		.y = v1.y+v2.y,
		.z = v1.z+v2.z,
	};
}

Vec_t vec_sub(Vec_t v1, Vec_t v2)
{
	return (Vec_t){
		.x = v1.x-v2.x,
		.y = v1.y-v2.y,
		.z = v1.z-v2.z,
	};
}

Vec_t vec_mul(Vec_t v, double k)
{
	return (Vec_t){
		.x = v.x*k,
		.y = v.y*k,
		.z = v.z*k,
	};
}

double vec_point_product(Vec_t a, Vec_t b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

Vec_t vec_cross_product(Vec_t a, Vec_t b)
{
	return (Vec_t){
		.x = a.y*b.z - a.z*b.y,
		.y = a.z*b.x - a.x*b.z,
		.z = a.x*b.y - a.y*b.x,
	};
;
}

double vec_len(Vec_t v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

Vec_t vec_direct(Vec_t v)
{
	double len = vec_len(v);
	if (len == 1) return v;
	else if (len == 0) return (Vec_t){0, 0, 0};
	return vec_mul(v, 1/len);
}

Vec_t vec_rotate(Vec_t v,Vec_t direction, double theta)
{
	return vec_add3(vec_mul(direction, vec_point_product(direction, v)*(1-cos(theta))),
			vec_mul(vec_cross_product(direction, v), sin(theta)),
			vec_mul(v, cos(theta)));
}

Vec_t vec_add3(Vec_t v1, Vec_t v2, Vec_t v3)
{
	return (Vec_t){
		.x = v1.x+v2.x+v3.x,
		.y = v1.y+v2.y+v3.y,
		.z = v1.z+v2.z+v3.z,
	};
}
Vec_t vec_addn_(Vec_t vecs[], size_t size)
{
	if (!vecs || size == 0) return (Vec_t){0, 0, 0};
	Vec_t v = *vecs;
	size_t i = 1;
	for (; i < size; ++i) v = vec_add(v, vecs[i]);
	return v;
}
