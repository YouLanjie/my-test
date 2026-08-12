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
	p->colors = malloc(point_num*sizeof(*p->colors));
	assert(p->points != NULL && p->colors != NULL);
	memcpy(p->points, points, point_num*sizeof(*p->points));
	for (size_t i = 0; i < point_num; i++) {
		p->colors[i] = COLOR_WHITE;
	}

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
	if (obj->colors) free(obj->colors);
	if (obj->lines) free(obj->lines);
	if (obj->surfaces) free(obj->surfaces);
	free(obj);
}

/* 将物体沿给定的Vec方向移动 */
Obj_t *obj_shift(Obj_t *obj, Vec_t v)
{
	if (!obj) return obj;
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
Obj_t *obj_rotate(Obj_t *obj, Vec_t direction, double theta)
{
	if (!obj || !vec_len(direction)) return NULL;
	Point_t *p = NULL;
	double s = sin(theta), c = cos(theta), rc = 1-c;
	size_t j = 0;
	direction = vec_direct(direction);    /* 归一化是必要的 */
	for (j=0, p=obj->points; p && j < obj->count_point; j++,p++) {
		/* 不使用vec_rotate，因为sin(theta)和cos(theta)都相同,避免多重计算 */
		*p = vec_add3(vec_mul(direction, vec_point_product(direction, *p)*rc),
			      vec_mul(vec_cross_product(direction, *p), s),
			      vec_mul(*p, c));
	}
	return obj;
}

void obj_scale(Obj_t *obj, double k)
{
	if (!obj) return;
	size_t i;
	for (i = 0; obj->count_point && i < obj->count_point; i++) {
		obj->points[i] = vec_mul(obj->points[i], k);
	}
}

Obj_t *obj_set_point_color(Obj_t *obj, size_t ind, Color_t color)
{
	if (!obj) return NULL;
	if (ind >= obj->count_point) return obj;
	obj->colors[ind] = color;
	return obj;
}

Obj_t *obj_set_color(Obj_t *obj, Color_t color)
{
	if (!obj) return NULL;
	for (size_t i = 0; i < obj->count_point; i++) {
		obj->colors[i] = color;
	}
	return obj;
}

Obj_t *obj_add_line(Obj_t *obj, size_t p1, size_t p2)
{
	if (!obj) return NULL;
	if (p1 >= obj->count_point || p2 >= obj->count_point)
		return obj;
	Line_t *lines;
	if (obj->count_line && obj->lines) {
		lines = realloc(obj->lines, (obj->count_line+1)*sizeof(*lines));
	} else {
		lines = malloc((obj->count_line+1)*sizeof(*lines));
	}
	if (!lines) return obj;
	obj->lines = lines;
	obj->count_line++;
	return obj;
}

Obj_t *obj_add_surface(Obj_t *obj, size_t p1, size_t p2, size_t p3)
{
	if (!obj) return NULL;
	if (p1 >= obj->count_point || p2 >= obj->count_point || p3 >= obj->count_point)
		return obj;
	Surface_t *surfaces;
	if (obj->count_surface && obj->surfaces) {
		surfaces = realloc(obj->surfaces, (obj->count_surface+1)*sizeof(*surfaces));
	} else {
		surfaces = malloc((obj->count_surface+1)*sizeof(*surfaces));
	}
	if (!surfaces) return obj;
	surfaces[obj->count_surface][0] = p1;
	surfaces[obj->count_surface][1] = p2;
	surfaces[obj->count_surface][2] = p3;
	obj->surfaces = surfaces;
	obj->count_surface++;
	return obj;
}

/* 会自动重新申请内存 */
bool obj_merge(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;

	size_t ori_count_points = obj->count_point;
	if (from->count_point && from->colors) {
		Color_t *colors = realloc(obj->colors, sizeof(*colors)*(obj->count_point+from->count_point));
		if (!colors) return false;
		memcpy(colors+obj->count_point, from->colors, sizeof(*colors)*from->count_point);
		obj->colors = colors;
	}
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
			surfaces[i][2] += ori_count_points;
		}
	}
	return true;
}

/**
 * @brief 合并两个物体
 *
 * @param obj 合并后的目标物体
 * @param from 被合并物体，合并后会自动free
 * @return 成功返回true 失败返回false
 */
bool obj_merge_and_free(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;
	if (!obj_merge(obj, from)) return false;
	obj_free(from);
	return true;
}

Obj_t *obj_create_line_from_point(Point_t p1, Point_t p2)
{
	Point_t center = vec_mul(vec_add(p1, p2), 0.5);
	return obj_create(center, 2, (Point_t[]){vec_sub(p1,center), vec_sub(p2,center)}, 1, (Line_t[]){{0,1}}, 0, NULL);
}

Obj_t *obj_create_box_from_point(Point_t points[8])
{
	return obj_create((Point_t){0,0,0}, 8, points,
			  12,
			  (Line_t[]){ {0,4},{1,5},{2,6},{3,7}, {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4} },
			  0, (Surface_t[]){});
}

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
	for (size_t i = 0; i < countof(points); i++) {
		points[i] = vec_mul(points[i], edge_len/2);
	}
	return obj_create_box_from_point(points);
}

Obj_t *obj_create_cube_with_surface(double edge_len)
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
	for (size_t i = 0; i < countof(points); i++) {
		points[i] = vec_mul(points[i], edge_len/2);
	}
	Obj_t *obj = obj_create_box_from_point(points);
	if (!obj) return NULL;
	obj_add_surface(obj, 0, 1, 2);
	obj_add_surface(obj, 2, 3, 0);

	obj_add_surface(obj, 4, 5, 6);
	obj_add_surface(obj, 6, 7, 4);
	for (size_t i = 0; i < 4; i++) {
		obj_add_surface(obj, i, i+4, (i+1)%4);
		obj_add_surface(obj, (i+1)%4, (i+1)%4+4, i+4);
	}
	return obj;
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
	obj->colors = malloc(count*sizeof(*obj->colors));
	for (size_t i = 0; i < count; i++) {
		obj->colors[i] = COLOR_WHITE;
	}
	obj_transform_shift(obj, (Vec_t){max_x/-2., y/-2., 0});
	obj_scale(obj, k);
	return obj;
}

static bool triangle_check_in(Point_t p1, Point_t p2, Point_t p3, Point_t check_point, Vec_t *result)
{
	double total = vec2d_area(p1, p2, p3);
	if (fabs(total) < 1e-5) return false;
	Vec_t ret = {};
	ret.x = vec2d_area(p2, p3, check_point)/total;
	ret.y = vec2d_area(p3, p1, check_point)/total;
	ret.z = vec2d_area(p1, p2, check_point)/total;
	if (!(ret.x>0 && ret.y>0 && ret.z>0)) return false;
	if (result) *result = ret;
	return true;
}

static void draw_surface(Obj_t *obj, Camera_t *camera, RenderBackend_t *backend,
			 Point_t p1, Point_t p2, Point_t p3,
			 size_t surface_id)
{
	if (!obj || !camera || !backend) return;
	Color_t color[3] = {};
	memset(color, -1, sizeof(color));
	if (surface_id < obj->count_surface) {
		color[0] = obj->colors[obj->surfaces[surface_id][0]];
		color[1] = obj->colors[obj->surfaces[surface_id][1]];
		color[2] = obj->colors[obj->surfaces[surface_id][2]];
	}
	Point2d_t p[3] = {p1, p2, p3};
	double x_min = camera->width/2;
	double x_max = camera->width/-2;
	double y_min = camera->height/2;
	double y_max = camera->height/-2;
	for (int j = 0; j < 3; j++) {
		x_min = fmin(x_min, p[j].x);
		x_max = fmax(x_max, p[j].x);
		y_min = fmin(y_min, p[j].y);
		y_max = fmax(y_max, p[j].y);
	}
	x_min = fmax(camera->width/-2, x_min);
	x_max = fmin(camera->width/2, x_max);
	y_min = fmax(camera->height/-2, y_min);
	y_max = fmin(camera->height/2 +1, y_max);
	Point_t ret;
	double z = 0;
	Color_t rgb;
	// printf("[%.2f,%.2f,%.2f,%.2f]", x_min, x_max, y_min, y_max);
	for (int j = x_min; j < x_max; j++) {
		for (int k = y_min; k < y_max; k++) {
			if (!triangle_check_in(p[0], p[1], p[2], (Vec_t){j, k, 0}, &ret))
				continue;
#define interpolation(var, field) (1./(ret.x*(1./var[0].field) + ret.y*(1./var[1].field) + ret.z*(1./var[2].field)))
			z = interpolation(p, z);
			rgb.r = interpolation(color, r);
			rgb.g = interpolation(color, g);
			rgb.b = interpolation(color, b);
			rgb.a = interpolation(color, a);
			if (z < 0 || (camera->dept > 0 && z > camera->dept))
				continue;
			// assert(z <= 10);
			backend->draw(backend, (Point2d_t){
				      j, k,
				      z/fabs(camera->dept)},
				      rgb);
		}
	}
	return;
}

/* 投影物体 */
void obj_cast(Obj_t *obj, Camera_t *camera, RenderBackend_t *backend)
{
	if (!obj || !camera || !backend) return;
	size_t i = 0;
	Point2d_t p[6] = {};
	camera_lock(camera);
	for (i = 0; i < obj->count_point; i++) {
		p[0] = camera_cast(camera, vec_add(obj->points[i], obj->center));
		if (p[0].z <= 0) continue;
		backend->draw(backend, (Point2d_t){p[0].x, p[0].y, p[0].z/fabs(camera->dept)},
			      obj->colors[i]);
	}

	double dx, dy, dz;
	int8_t step = 0;
	for (i = 0; i < obj->count_line; i++) {
		if (camera_cast_line(camera,
				     vec_add(obj->center, obj->points[obj->lines[i][0]]),
				     vec_add(obj->center, obj->points[obj->lines[i][1]]), p, p+1) < 0)
			continue;
		Color_t color = obj->colors[obj->lines[i][0]];
		color.r = (color.r + obj->colors[obj->lines[i][1]].r) / 2;
		color.g = (color.g + obj->colors[obj->lines[i][1]].g) / 2;
		color.b = (color.b + obj->colors[obj->lines[i][1]].b) / 2;
		if (p[0].z <= 0 || p[1].z <= 0) continue;
		dx = p[1].x-p[0].x, dy = p[1].y-p[0].y, dz = p[1].z-p[0].z;
		if (fabs(dx) >= fabs(dy)) {
			step = dx < 0 ? -1 : 1;
			for (int x = p[0].x; (x-p[1].x)*step <= 0; x+=step) {
				backend->draw(backend, (Point2d_t){
					      x, p[0].y+(x-p[0].x)*dy/dx,
					      (p[0].z+(x-p[0].x)*dz/dx)/fabs(camera->dept)
					      }, color);
			}
		} else {
			step = dy < 0 ? -1 : 1;
			for (int y = p[0].y; (y-p[1].y)*step <= 0; y+=step) {
				backend->draw(backend, (Point2d_t){
					      p[0].x+(y-p[0].y)*dx/dy, y,
					      (p[0].z+(y-p[0].y)*dz/dy)/fabs(camera->dept)
					      }, color);
			}
		}
		p[0] = (Point_t){0, 0, 0}, p[1] = (Point2d_t){0, 0, 0};
	}

	int count = 0;
	for (i = 0; i < obj->count_surface; i++) {
		for (int j = 0; j < 3; j++) {
			p[j] = vec_add(obj->center, obj->points[obj->surfaces[i][j]]);
		}
		count = camera_cast_surface(camera, p+0, p+1, p+2, p+3, p+4, p+5);
		for (int j = 0; j < count && j*3+2 < (int)countof(p); j++) {
			draw_surface(obj, camera, backend, p[j*3], p[j*3+1], p[j*3+2], i);
		}
	}
	camera_unlock(camera);
}

