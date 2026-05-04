/**
 * @file        object.c
 * @author      u0_a221
 * @date        2026-05-03
 * @brief       物体对象
 */

#include "render3d.h"
#include <assert.h>

/* 创建物体 */
Obj_t *obj_create(Point_t initial_position,
		  size_t point_num,   Point_t *points,
		  size_t line_num,    Line_t  *lines,
		  size_t surface_num, Surface_t *surfaces)
{
	if (point_num <= 0 && line_num <= 0 && surface_num <= 0) return NULL;
	if (point_num && !points) return NULL;
	if (line_num && !lines) return NULL;
	if (surface_num && !surfaces) return NULL;
	Obj_t *p = malloc(sizeof(Obj_t));
	assert(p != NULL);
	*p = (Obj_t){
		.center = initial_position,
		.count_point = point_num,
		.count_line = line_num,
		.count_suface = surface_num,
		.points = NULL, .lines = NULL, .surfaces = NULL,
	};
	if (point_num) {
		p->points = malloc(point_num*sizeof(*p->points));
		assert(p->points != NULL);
		memcpy(p->points, points, point_num*sizeof(*p->points));
	}
	if (line_num) {
		p->lines = malloc(line_num*sizeof(*p->lines));
		assert(p->lines != NULL);
		memcpy(p->lines, lines, line_num*sizeof(*p->lines));
	}
	if (surface_num) {
		p->surfaces = malloc(surface_num*sizeof(*p->surfaces));
		assert(p->surfaces != NULL);
		memcpy(p->surfaces, surfaces, surface_num*sizeof(*p->surfaces));
	}
	return p;
}

/* 释放对象同时设置指针为NULL */
void obj_free(Obj_t **obj)
{
	if (!obj || !*obj) return;
	free((*obj)->points);
	free(*obj);
	*obj = NULL;
}

/* 将物体沿给定的Vec方向移动 */
Obj_t *obj_shift(Obj_t *obj, Vec_t v)
{
	if (!obj || !obj->points) return obj;
	obj->center = vec_add(obj->center, v);
	return obj;
}

/* 将物体的各点相对于中心点沿给定的Vec方向移动 */
Obj_t *obj_transform_shift(Obj_t *obj, Vec_t v)
{
	if (!obj || !obj->points) return obj;
	Point_t *p = obj->points;
	for (int j = 0 ; j < (int64_t)obj->count_point; j++,p++) {
		*p = (Point_t){
			.x = p->x + v.x,
			.y = p->y + v.y,
			.z = p->z + v.z,
		};
	}
	return obj;
}

/* 绕指定轴旋转 */
void obj_rotate(Obj_t *obj, Vec_t direction, double theta)
{
	if (!obj || !obj->points || !vec_len(direction)) return;
	Point_t *p = obj->points;
	direction = vec_mul(direction, 1/vec_len(direction));
	double s = sin(theta), c = cos(theta), rc = 1-c;
	for (size_t j = 0 ; j < obj->count_point; j++,p++) {
		/* 不使用vec_rotate，因为sin(theta)和cos(theta)都相同,避免多重计算 */
		*p = vec_adds(vec_mul(direction, vec_point_product(direction, *p)*rc),
			      vec_mul(vec_cross_product(direction, *p), s),
			      vec_mul(*p, c));
	}
}

/* 会自动重新申请内存 */
bool obj_merge(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;
	Point_t *points = realloc(obj->points,
				  sizeof(*points)*(obj->count_point+from->count_point));
	if (!points) return false;
	memcpy(points+obj->count_point, from->points, sizeof(*points)*from->count_point);
	/* realloc已经释放内存了 */
	/*free(obj->points);*/
	obj->points = points;
	obj->count_point += from->count_point;
	return true;
}

/* 会自动释放from对象的内存（方便传参） */
bool obj_merge_and_free(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;
	if (!obj_merge(obj, from)) return false;
	obj_free(&from);
	return true;
}

/* count: 点云密度 */
Obj_t *obj_create_line_from_point(Point_t t1, Point_t t2, size_t count)
{
	if (count == 0 || count > 1024) return NULL;
	Point_t center = vec_mul(vec_add(t1, t2), 0.5),
		dt = vec_sub(t2, t1),
		t[count];
	for (size_t i = 0; i < count && i < (sizeof(t)/sizeof(*t)); i++) {
		t[i] = vec_mul(dt, -0.5+(double)i/count);
	}
	return obj_create(center, count, t, 0, NULL, 0, NULL);
}

/* 从8顶点创建立方体
 * 前四|后四：两对立面的四个顶点
 * 边：p[n]与p[n+1]相连(n<=3) */
Obj_t *obj_create_box_from_point(size_t count, Point_t points[8])
{
#define oclfp(i1, i2) obj_create_line_from_point(points[i1], points[i2], count)
	Obj_t *box = oclfp(0, 4), *line = NULL;
	obj_apply_shift(box);
	for (int i = 1; i < 12; i++) {
		if (i < 4) line = oclfp(i, i+4);    /* 1~3 */
		else if (i < 8) line = oclfp(i-4, (i-3)%4);    /* 4~7 */
		else line = oclfp(i-4, (i-3)%4+4);    /* 8~11 */

		if (line == NULL) break;
		obj_apply_shift(line);
		obj_merge(box, line);
		obj_free(&line);
		line = NULL;
	}
	return box;
#undef oclfp
}

/* 投影物体 */
void obj_cast(Obj_t *obj, Camera_t *camera, RenderBackend_t *backend)
{
	if (!camera || !backend) return;
	Point2d_t p;
	for (size_t j = 0; j < obj->count_point; j++) {
		p = camera_cast(camera, vec_add(obj->points[j], obj->center));
		if (p.z <= 0) continue;
		backend->draw(backend, (Point2d_t){p.x, p.y, p.z/camera->dept});
	}
}

