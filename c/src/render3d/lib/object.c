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
	/* 允许创建空物体 */
	/*if (point_num <= 0 && line_num <= 0 && surface_num <= 0) return NULL;*/
	if (point_num && !points) return NULL;
	if (line_num && !lines) return NULL;
	if (surface_num && !surfaces) return NULL;
	Obj_t *p = malloc(sizeof(Obj_t));
	assert(p != NULL);
	*p = (Obj_t){
		.center = initial_position,
		.count_point = point_num,
		.count_line = line_num,
		.count_surface = surface_num,
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
	if ((*obj)->points) free((*obj)->points);
	if ((*obj)->lines) free((*obj)->lines);
	if ((*obj)->surfaces) free((*obj)->surfaces);
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
	size_t j = 0;
	for (j = 0 ; j < obj->count_point; j++,p++) {
		*p = vec_add(*p, v);
	}
	for (j = 0 ; j < obj->count_line; j++) {
		obj->lines[j].start = vec_add(obj->lines[j].start, v);
		obj->lines[j].end = vec_add(obj->lines[j].end, v);
	}
	return obj;
}

/* 绕指定轴旋转 */
void obj_rotate(Obj_t *obj, Vec_t direction, double theta)
{
	if (!obj || !vec_len(direction)) return;
	Point_t *p = NULL;
	double s = sin(theta), c = cos(theta), rc = 1-c;
	size_t j = 0;
	direction = vec_direct(direction);
	for (j=0, p=obj->points; p && j < obj->count_point; j++,p++) {
		/* 不使用vec_rotate，因为sin(theta)和cos(theta)都相同,避免多重计算 */
		*p = vec_adds(vec_mul(direction, vec_point_product(direction, *p)*rc),
			      vec_mul(vec_cross_product(direction, *p), s),
			      vec_mul(*p, c));
	}
	for (j = 0 ; obj->count_line && j < obj->count_line; j++) {
		p = &obj->lines[j].start;
		*p = vec_adds(vec_mul(direction, vec_point_product(direction, *p)*rc),
			      vec_mul(vec_cross_product(direction, *p), s),
			      vec_mul(*p, c));
		p = &obj->lines[j].end;
		*p = vec_adds(vec_mul(direction, vec_point_product(direction, *p)*rc),
			      vec_mul(vec_cross_product(direction, *p), s),
			      vec_mul(*p, c));
	}
}

void obj_scale(Obj_t *obj, double k)
{
	if (!obj) return;
	size_t i;
	for (i = 0; obj->count_point && i < obj->count_point; i++) {
		obj->points[i] = vec_mul(obj->points[i], k);
	}
	for (i = 0; obj->count_line && i < obj->count_line; i++) {
		obj->lines[i].start = vec_mul(obj->lines[i].start, k);
		obj->lines[i].end = vec_mul(obj->lines[i].end, k);
	}
}

/* 会自动重新申请内存 */
bool obj_merge(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;

	if (from->count_point && from->points) {
		Point_t *points = realloc(obj->points, sizeof(*points)*(obj->count_point+from->count_point));
		if (!points) return false;
		memcpy(points+obj->count_point, from->points, sizeof(*points)*from->count_point);
		obj->points = points;
		obj->count_point += from->count_point;
	}

	if (from->count_line && from->lines) {
		Line_t *lines = realloc(obj->lines, sizeof(*lines)*(obj->count_line+from->count_line));
		if (!lines) return false;
		memcpy(lines+obj->count_line, from->lines, sizeof(*lines)*from->count_line);
		obj->lines = lines;
		obj->count_line += from->count_line;
	}

	if (from->count_surface && from->surfaces) {
		Surface_t *surfaces = realloc(obj->surfaces, sizeof(*surfaces)*(obj->count_surface+from->count_surface));
		if (!surfaces) return false;
		memcpy(surfaces+obj->count_surface, from->surfaces, sizeof(*surfaces)*from->count_surface);
		obj->surfaces = surfaces;
		obj->count_surface += from->count_surface;
	}
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
Obj_t *obj_create_line_from_point(Point_t t1, Point_t t2)
{
	Point_t center = vec_mul(vec_add(t1, t2), 0.5);
	return obj_create(center, 0, 0, 1, (Line_t[]){{t1, t2}}, 0, NULL);
}

/* 从8顶点创建立方体
 * 前四|后四：两对立面的四个顶点
 * 边：p[n]与p[n+1]相连(n<=3) */
Obj_t *obj_create_box_from_point(Point_t points[8])
{
#define oclfp(i1, i2) obj_create_line_from_point(points[i1], points[i2])
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

Obj_t *obj_create_image_from_str(Point_t center, double k, const char *p, char ch)
{
	if (!p || ch == '\n') return NULL;
	size_t count = 0;
	for (const char *tmp=p;*tmp;tmp++) if(*tmp == ch) count++;
	if (count == 0) return NULL;
	Point_t *points = malloc(count*sizeof(*points));
	if (!points) return NULL;
	// obj_create((Point_t){0,0,0},0,NULL,0,NULL,0,NULL)
	// O   ^  y
	//     |
	// ----+----> x
	//     |
	//     |
	int x = 0, y = 0, max_x = 0;
	size_t i = 0;
	while (*p && i < count) {
		if (*p == ch) {
			points[i] = (Point_t){x, y, 0};
			i++;
			x++;
		} else if (*p == '\n') y--, x=0;
		else x++;

		if (x > max_x) max_x = x;
		p++;
	}
	Obj_t *obj = obj_create(center, 0, NULL, 0,NULL,0,NULL);    /* 先创建空物体 */
	if (!obj) {
		free(points);
		return NULL;
	}
	obj->count_point = count;
	obj->points = points;
	obj_transform_shift(obj, (Vec_t){max_x/-2., y/-2., 0});
	obj_scale(obj, k);
	return obj;
}

/* 投影物体 */
void obj_cast(Obj_t *obj, Camera_t *camera, RenderBackend_t *backend)
{
	if (!camera || !backend) return;
	size_t i = 0;
	Point2d_t p;
	for (i = 0; i < obj->count_point; i++) {
		p = camera_cast(camera, vec_add(obj->points[i], obj->center));
		if (p.z <= 0) continue;
		backend->draw(backend, (Point2d_t){p.x, p.y, p.z/camera->dept});
	}
	int step;
	Vec_t v, dv;
	for (i = 0; i < obj->count_line; i++) {
		v = obj->lines[i].start;
		dv = vec_sub(obj->lines[i].end, v);
		step = 100*vec_len(dv);
		dv = vec_mul(dv, 1./step);
		for (int j = 0; j < step; j++) {
			p = camera_cast(camera, vec_adds(obj->center, v, vec_mul(dv, j)));
			if (p.z <= 0) continue;
			backend->draw(backend, (Point2d_t){p.x, p.y, p.z/camera->dept});
		}
	}
}

