/**
 * @file        render3d.h
 * @author      u0_a221
 * @date        2026-05-03
 * @brief       拆分render3d.c准备的头文件
 */

#pragma once

#ifndef _RENDER3D_H
#define _RENDER3D_H

#include "../../../include/tools.h"
#include <math.h>

/* 参数声明 */
#define SECOND 1000000
#define MAX_FRAME 100000


/* 向量 */
typedef struct {
	double x;
	double y;
	double z;
} Vec_t;
/* 点、线、面（仅支持三点面） */
typedef Vec_t Point_t;
typedef Vec_t *Line_t;            /* 指向一系列点 */
typedef Vec_t (*Surface_t)[3];    /* 三个指向点的指针的数组 */
/* 二维平面点 */
typedef Vec_t Point2d_t;

/* 向量操作函数 */
Vec_t vec_add(Vec_t v1, Vec_t v2);
Vec_t vec_sub(Vec_t v1, Vec_t v2);
Vec_t vec_mul(Vec_t v, double k);
double vec_point_product(Vec_t a, Vec_t b);
Vec_t vec_cross_product(Vec_t a, Vec_t b);
double vec_len(Vec_t v);
Vec_t vec_direct(Vec_t v);
Vec_t vec_addn_(Vec_t vecs[], size_t size);
Vec_t vec_rotate(Vec_t v,Vec_t direction, double theta);
#define vec_adds(...) vec_addn_((Vec_t[]){__VA_ARGS__}, sizeof((Vec_t[]){__VA_ARGS__})/sizeof(Vec_t))


/* 相机 */
typedef struct {
	double height;     /* 投影平面高度(影响投影后返回值范围) */
	double width;      /* 投影平面宽度(影响投影后返回值范围) */
	double scale;      /* 观测原点到投影平面的距离(可当作焦距/缩放、视场角) */
	double dept;       /* 可视深度 */
	double offset_x;   /* 投影中轴水平偏移(左右斜着看) */
	double offset_y;   /* 投影中轴垂直偏移(上下斜着看) */
	Point_t position;  /* 观测原点位置 */
	Vec_t forward;     /* 相机朝向(相机坐标系z轴负方向) */
	Vec_t up;          /* 相机向上方向(相机坐标系y轴正方向) */
} Camera_t;
Camera_t *camera_create();
void camera_free(Camera_t **p);
Point_t camera_cast(Camera_t *camera, Point_t p);
void camera_shift(Camera_t *camera, Vec_t direction);
void camera_rotate(Camera_t *camera, Vec_t direction, double theta);


/* 渲染后端 */
typedef struct RenderBackend_t RenderBackend_t;
struct RenderBackend_t {
	void (*draw)(RenderBackend_t *backend, Point2d_t p);  /* 绘制(p的取值范围：xy:-1~1,z:0~1) */
	void (*render)(RenderBackend_t *backend);             /* 输出一帧 */
	void (*clean)(RenderBackend_t *backend);              /* 清理上一帧的数据 */
	void (*destory)(RenderBackend_t *backend);            /* 释放内存 */
	void *data;
};
#define BACKEND_LIST \
	BACKEND(ascii) \
	BACKEND(utf8)

#define BACKEND(name) RenderBackend_t *backend_create_##name(int width, int height);
BACKEND_LIST
#undef BACKEND
#define BACKEND(name) Backend_##name,
enum Backend_list {BACKEND_LIST};
#undef BACKEND


/* 物体 */
typedef struct {
	Point_t center;
	/* 点、线、面之间内存相互独立、不互相引用 */
	Point_t *points;
	Line_t  *lines;
	Surface_t *surfaces;
	/* 点线面的计数 */
	size_t count_point;
	size_t count_line;
	size_t count_suface;
} Obj_t;
Obj_t *obj_create(Point_t initial_position, size_t point_num, Point_t *points, size_t line_num, Line_t *lines, size_t surface_num, Surface_t *surfaces);
void obj_free(Obj_t **obj);
Obj_t *obj_shift(Obj_t *obj, Vec_t v);
Obj_t *obj_transform_shift(Obj_t *obj, Vec_t v);
/* 应用移动(将物体原点搬回(0,0,0)但形状留在那个位置) */
#define obj_apply_shift(obj) obj_shift(obj_transform_shift((obj), (obj)->center), vec_mul((obj)->center, -1))
void obj_rotate(Obj_t *obj, Vec_t direction, double theta);
bool obj_merge(Obj_t *obj, Obj_t *from);
bool obj_merge_and_free(Obj_t *obj, Obj_t *from);
Obj_t *obj_create_line_from_point(Point_t t1, Point_t t2, size_t count);
Obj_t *obj_create_box_from_point(size_t count, Point_t points[8]);
void obj_cast(Obj_t *obj, Camera_t *camera, RenderBackend_t *backend);

#endif //RENDER3D_H

