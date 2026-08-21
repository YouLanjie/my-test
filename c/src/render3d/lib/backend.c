/**
 * @file        backend.c
 * @author      Chglish
 * @date        2026-08-15
 * @brief       后端的一些辅助接触公共函数
 */

#include "render3d.h"

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

Color_t color_add(Color_t dest, Color_t src)
{
	double k = src.a/(double)UINT8_MAX;
	dest.r = dest.r*(1-k) + src.r*k;
	dest.g = dest.g*(1-k) + src.g*k;
	dest.b = dest.b*(1-k) + src.b*k;
	return dest;
}

Color_t color_mix(Color_t c1, Color_t c2, double k)
{
	k = fmin(fabs(k), 1);
	c1.r = c1.r*(1-k) + c2.r*k;
	c1.g = c1.g*(1-k) + c2.g*k;
	c1.b = c1.b*(1-k) + c2.b*k;
	c1.a = c1.a*(1-k) + c2.a*k;
	return c1;
}

Color_t color_mul(Color_t c, double k)
{
	c.a *= k;
	c.r *= k;
	c.g *= k;
	c.b *= k;
	return c;
}

void backend_draw_line(RenderBackend_t *backend, Camera_t *camera,
		       Point_t p1, Point_t p2,
		       Color_t c1, Color_t c2)
{
	if (p1.z <= 0 || p2.z <= 0) return;
	double dx, dy, dz;
	double z = 0;
	int8_t step = 0;
	Color_t color;
	dx = p2.x-p1.x, dy = p2.y-p1.y, dz = p2.z-p1.z;
	double inv_z1 = p1.z ? 1/p1.z : INFINITY;
	double inv_z2 = p2.z ? 1/p2.z : INFINITY;
	if (fabs(dx) >= fabs(dy)) {
		if (dx == 0) return;
		step = dx < 0 ? -1 : 1;
		for (int x = p1.x; (x-p2.x)*step <= 0; x+=step) {
			z = (dz? 1./(inv_z1+(inv_z2-inv_z1)*(x-p1.x)/dx) :p1.z)/fabs(camera->dept);
			color = dz ? color_mix(c1, c2, (z-p1.z)/dz) : c1;
			backend->draw(backend, (Point2d_t){x, p1.y+(x-p1.x)/dx*dy, z}, color);
		}
	} else {
		step = dy < 0 ? -1 : 1;
		for (int y = p1.y; (y-p2.y)*step <= 0; y+=step) {
			z = (dz? 1./(inv_z1+(inv_z2-inv_z1)*(y-p1.y)/dy) :p1.z)/fabs(camera->dept);
			color = dz ? color_mix(c1, c2, (z-p1.z)/dz) : c1;
			backend->draw(backend, (Point2d_t){p1.x+(y-p1.y)/dy*dx, y, z}, color);
		}
	}
}

void backend_draw_surface(RenderBackend_t *backend, Camera_t *camera,
			  Point_t p1, Point_t p2, Point_t p3,
			  Color_t c1, Color_t c2, Color_t c3)
{
	if (!backend || !camera) return;
	Color_t color[3] = {c1, c2, c3};
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

