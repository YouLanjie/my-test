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
#include <stddef.h>
#include <stdint.h>
#include <stdcountof.h>
#include <math.h>

/* 向量 */
typedef struct {
	double x;
	double y;
	double z;
} Vec_t;
/* 点、线、面（仅支持三点面） */
typedef Vec_t Point_t;
/* 二维平面点 */
typedef Vec_t Point2d_t;

/* 向量操作函数 */

/* 向量加法 */
Vec_t vec_add(Vec_t v1, Vec_t v2);
/**
 * @brief 向量减法
 *
 * @param v1 被减数
 * @param v2 减数
 * @return v2->v1的向量
 */
Vec_t vec_sub(Vec_t v1, Vec_t v2);
/* 向量数乘 */
Vec_t vec_mul(Vec_t v, double k);
/* 向量点乘 */
double vec_point_product(Vec_t a, Vec_t b);
/* 向量叉乘(a,b的顺序很重要) */
Vec_t vec_cross_product(Vec_t a, Vec_t b);
/* 向量长度 */
double vec_len(Vec_t v);
/* 获取该方向的单位向量 */
Vec_t vec_direct(Vec_t v);
/* 旋转向量 */
Vec_t vec_rotate(Vec_t v,Vec_t direction, double theta);
/* 便捷多向量加法 */
Vec_t vec_add3(Vec_t v1, Vec_t v2, Vec_t v3);
Vec_t vec_addn_(Vec_t vecs[], size_t size);
/* 不定项多向量加法 */
#define vec_adds(...) vec_addn_((Vec_t[]){__VA_ARGS__}, sizeof((Vec_t[]){__VA_ARGS__})/sizeof(Vec_t))


/* 相机 */
typedef struct {
	double height;     /* 投影平面高度(影响投影后返回值范围,一般需要设置为与输出后端大小相等或者比它大) */
	double width;      /* 投影平面宽度(影响投影后返回值范围) */
	double scale;      /* 观测原点到投影平面的距离(可当作焦距/缩放、视场角) */
	double dept;       /* 可视深度 */
	double z_near;     /* 最小可视距离 */
	double offset_x;   /* 投影中轴水平偏移(左右斜着看) */
	double offset_y;   /* 投影中轴垂直偏移(上下斜着看) */
	Point_t position;  /* 观测原点位置 */
	Vec_t forward;     /* 相机朝向(相机坐标系z轴负方向) */
	Vec_t up;          /* 相机向上方向(相机坐标系y轴正方向) */
	void *private_data;
} Camera_t;
/* 仅申请内存并初始化 */
Camera_t *camera_create();
void camera_free(Camera_t *p);
void camera_lock(Camera_t *camera);
void camera_unlock(Camera_t *camera);
/**
 * @brief 将点投影到虚拟屏幕(相机)上
 *
 * @param camera 相机
 * @param p 需要投影的单个点
 * @return 投影到相机后的二维点，正常时z轴作为深度>0
 * （若z<=0则点无法投影或在屏幕外）
 */
Point_t camera_cast(Camera_t *camera, Point_t p);
int camera_cast_line(Camera_t *camera, Point_t p1, Point_t p2, Point2d_t *ret_p1, Point2d_t *ret_p2);
void camera_shift(Camera_t *camera, Vec_t direction);
void camera_rotate(Camera_t *camera, Vec_t direction, double theta);
void camera_look(Camera_t *camera, Point_t point, Vec_t hold);


/* 渲染后端 */
#ifndef BACKEND_LIST
#define BACKEND_LIST \
	BACKEND(ascii) \
	BACKEND(ascii_8bit) \
	BACKEND(ascii_grey) \
	BACKEND(utf8) \
	BACKEND(utf8_8bit)
#endif

#define BACKEND(name) RDBK_##name,
enum Backend_id {BACKEND_LIST};
#undef BACKEND
typedef struct RenderBackend_t RenderBackend_t;
struct RenderBackend_t {
	void (*draw)(RenderBackend_t *backend, Point2d_t p);  /* 绘制(p的取值范围：xy:-1~1,z:0~1) */
	void (*render)(RenderBackend_t *backend);             /* 输出一帧 */
	void (*clean)(RenderBackend_t *backend);              /* 清理上一帧的数据 */
	void (*destroy)(RenderBackend_t *backend);            /* 释放内存 */
	void *data;
	enum Backend_id id;
};
#define BACKEND(name) RenderBackend_t *backend_create_##name(int width, int height);
BACKEND_LIST
#undef BACKEND


/* 线引用id */
typedef size_t Line_t[2];
/* 面引用id */
typedef size_t Surface_t[3];
/* 物体 */
typedef struct {
	Point_t    center;
	Point_t   *points;    /* 点 */
	Line_t    *lines;     /* 线(引用点的id) */
	Surface_t *surfaces;  /* 面(引用点的id) */
	/* 点线面的计数 */
	size_t count_point;
	size_t count_line;
	size_t count_surface;
} Obj_t;
/* 创建物体 */
Obj_t *obj_create(Point_t initial_position, size_t point_num, Point_t *points, size_t line_num, Line_t *lines, size_t surface_num, Surface_t *surfaces);
/* 释放对象 */
void obj_free(Obj_t *obj);
/* 将物体沿给定的Vec方向移动 */
Obj_t *obj_shift(Obj_t *obj, Vec_t v);
/* 将物体的各点相对于中心点沿给定的Vec方向移动(点多时耗性能) */
Obj_t *obj_transform_shift(Obj_t *obj, Vec_t v);
/* 应用移动(将物体原点搬回(0,0,0)但形状留在那个位置) */
#define obj_apply_shift(obj) obj_shift(obj_transform_shift((obj), (obj)->center), vec_mul((obj)->center, -1))
/* 绕指定轴旋转 */
Obj_t *obj_rotate(Obj_t *obj, Vec_t direction, double theta);
/* 对所有点应用向量乘法（缩放） */
void obj_scale(Obj_t *obj, double k);
/* 合并from的物体数据到obj */
bool obj_merge(Obj_t *obj, Obj_t *from);
/* 会自动释放from对象的内存（方便传参） */
bool obj_merge_and_free(Obj_t *obj, Obj_t *from);
/**
 * @brief 从两端点创建线段，物体中心在两点连线中心
 *
 * @param p1 端点1绝对坐标
 * @param p2 端点2绝对坐标
 * @return 线段形状或者NULL
 */
Obj_t *obj_create_line_from_point(Point_t p1, Point_t p2);
/**
 * @brief 从8个顶点创建立方体
 * 前四|后四：两对立面的四个顶点
 * 边：p[n]与p[n+1]相连(0<=0<8)
 *     p[n]与p[n+4]相连(0<=n<4)
 *
 * @param points 需要8个点
 * @return 返回创建后的物体
 */
Obj_t *obj_create_box_from_point(Point_t points[8]);
/**
 * @brief 创建正立方体
 *
 * @param edge_len 棱长
 * @return 创建后的物体
 */
Obj_t *obj_create_cube(double edge_len);
/**
 * @brief 通过字符串字符画生成物体
 * (画布垂直于z轴,中心在边界框中心)
 *
 * @param center 中心坐标
 * @param k 大小系数
 * @param p 字符画字符串
 * @param ch 要识别为点的符号
 * @return 创建成功的物体
 */
Obj_t *obj_create_image_from_str(Point_t center, double k, const char *p, char ch);
/* 投影物体 */
void obj_cast(Obj_t *obj, Camera_t *camera, RenderBackend_t *backend);

#endif //RENDER3D_H

