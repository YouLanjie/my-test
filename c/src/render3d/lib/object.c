/**
 * @file        object.c
 * @author      u0_a221
 * @date        2026-05-03
 * @brief       物体对象
 */

#include "render3d.h"
#include <assert.h>
#include <stdcountof.h>

/* 创建物体 */
Obj_t *obj_create(Point_t initial_position,
		  size_t point_num,   Point_t *points,
		  size_t line_num,    Line_t *lines,
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
		.count_line = 0,
		.count_surface = 0,
		.points = NULL, .lines = NULL, .surfaces = NULL,
	};
	if (!point_num) return p;
	p->points = malloc(point_num*sizeof(*p->points));
	assert(p->points != NULL);
	memcpy(p->points, points, point_num*sizeof(*p->points));

	if (line_num) {
		p->lines = malloc(line_num*sizeof(*p->lines));
		assert(p->lines != NULL);
		memcpy(p->lines, lines, line_num*sizeof(*p->lines));
		for (size_t i = 0; i < line_num; i++) {    /* 边界检查 */
			if (p->lines[i][0] < point_num && p->lines[i][1] < point_num) continue;
			p->lines[i][0] = 0;
			p->lines[i][1] = 0;
		}
		p->count_line = line_num;
	}
	if (surface_num) {
		p->surfaces = malloc(surface_num*sizeof(*p->surfaces));
		assert(p->surfaces != NULL);
		memcpy(p->surfaces, surfaces, surface_num*sizeof(*p->surfaces));
		for (size_t i = 0; i < surface_num; i++) {    /* 边界检查 */
			if (p->surfaces[i][0] < point_num && p->surfaces[i][1] < point_num) continue;
			p->surfaces[i][0] = 0;
			p->surfaces[i][1] = 0;
		}
		p->count_surface = surface_num;
	}
	return p;
}

/* 释放对象 */
void obj_free(Obj_t *obj)
{
	if (!obj) return;
	if (obj->points) free(obj->points);
	if (obj->lines) free(obj->lines);
	if (obj->surfaces) free(obj->surfaces);
	free(obj);
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
		*p = vec_add3(vec_mul(direction, vec_point_product(direction, *p)*rc),
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
}

/* 会自动重新申请内存 */
bool obj_merge(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;

	size_t ori_count_points = obj->count_point;
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
		size_t i = obj->count_line;
		obj->lines = lines;
		obj->count_line += from->count_line;
		for (;i < obj->count_line; i++) {    /* 更新索引 */
			lines[i][0] += ori_count_points;
			lines[i][1] += ori_count_points;
		}
	}

	if (from->count_surface && from->surfaces) {
		Surface_t *surfaces = realloc(obj->surfaces, sizeof(*surfaces)*(obj->count_surface+from->count_surface));
		if (!surfaces) return false;
		memcpy(surfaces+obj->count_surface, from->surfaces, sizeof(*surfaces)*from->count_surface);
		size_t i = obj->count_surface;
		obj->surfaces = surfaces;
		obj->count_surface += from->count_surface;
		for (; i < obj->count_surface; i++) {    /* 更新索引 */
			surfaces[i][0] += ori_count_points;
			surfaces[i][1] += ori_count_points;
		}
	}
	return true;
}

/* 会自动释放from对象的内存（方便传参） */
bool obj_merge_and_free(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;
	if (!obj_merge(obj, from)) return false;
	obj_free(from);
	return true;
}

/* 从两端点创建线段(p1,p2为绝对坐标) */
Obj_t *obj_create_line_from_point(Point_t p1, Point_t p2)
{
	Point_t center = vec_mul(vec_add(p1, p2), 0.5);
	return obj_create(center, 2, (Point_t[]){vec_sub(p1,center), vec_sub(p2,center)}, 1, (Line_t[]){{0,1}}, 0, NULL);
}

/* 从8顶点创建立方体
 * 前四|后四：两对立面的四个顶点
 * 边：p[n]与p[n+1]相连(0<=0<8)
 *     p[n]与p[n+4]相连(0<=n<4) */
Obj_t *obj_create_box_from_point(Point_t points[8])
{
	return obj_create((Point_t){0,0,0}, 8, points, 12,
			  (Line_t[]){ {0,4},{1,5},{2,6},{3,7}, {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4} },
			  0, NULL);
}

/* 创建正立方体 */
Obj_t *obj_create_cube(double edge_len)
{
	Point_t points[] = {
		{-1,-1,1},
		{1,-1,1},
		{1,1,1},
		{-1,1,1},
		{-1,-1,-1},
		{1,-1,-1},
		{1,1,-1},
		{-1,1,-1}
	};
	for (int i = 0; i < countof(points); i++) {
		points[i] = vec_mul(points[i], edge_len/2);
	}
	return obj_create_box_from_point(points);
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
	camera_lock(camera);
	for (i = 0; i < obj->count_point; i++) {
		p = camera_cast(camera, vec_add(obj->points[i], obj->center));
		if (p.z <= 0) continue;
		backend->draw(backend, (Point2d_t){p.x, p.y, p.z/camera->dept});
	}
	Point2d_t p2;
	double dx, dy, dz;
	int8_t step = 0;
	for (i = 0; i < obj->count_line; i++) {
		if (camera_cast_line(camera,
				     vec_add(obj->center, obj->points[obj->lines[i][0]]),
				     vec_add(obj->center, obj->points[obj->lines[i][1]]), &p, &p2) < 0)
			continue;
		if (p.z <= 0 || p2.z <= 0) continue;
		dx = p2.x-p.x, dy = p2.y-p.y, dz = p2.z-p.z;
		if (fabs(dx) >= fabs(dy)) {
			step = dx < 0 ? -1 : 1;
			for (int x = p.x; (x-p2.x)*step <= 0; x+=step) {
				backend->draw(backend, (Point2d_t){x, p.y+(x-p.x)*dy/dx, (p.z+(x-p.x)*dz/dx)/camera->dept});
			}
		} else {
			step = dy < 0 ? -1 : 1;
			for (int y = p.y; (y-p2.y)*step <= 0; y+=step) {
				backend->draw(backend, (Point2d_t){p.x+(y-p.y)*dx/dy, y, (p.z+(y-p.y)*dz/dy)/camera->dept});
			}
		}
		p = (Point_t){0, 0, 0}, p2 = (Point2d_t){0, 0, 0};
	}
	camera_unlock(camera);
}

