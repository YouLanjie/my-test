/**
 * @file        backend.c
 * @author      Chglish
 * @date        2026-08-15
 * @brief       后端的一些辅助接触公共函数
 */

#include "render3d.h"

bool triangle_check_in(Point_t p1, Point_t p2, Point_t p3, Point_t check_point, Vec_t *result)
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
	if (src.a == UINT8_MAX) return src;
	if (src.a == 0) return dest;
	double k = src.a/(double)UINT8_MAX;
	dest.r = dest.r*(1-k) + src.r*k;
	dest.g = dest.g*(1-k) + src.g*k;
	dest.b = dest.b*(1-k) + src.b*k;
	dest.a = src.a;
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
		int x = fmin(fmax(p1.x, camera->width/-2), camera->width/2);
		int x_stop = fmin(fmax(p2.x, camera->width/-2), camera->width/2);
		for (; (x-x_stop)*step <= 0; x+=step) {
			z = (dz? 1./(inv_z1+(inv_z2-inv_z1)*(x-p1.x)/dx) :p1.z)/fabs(camera->dept);
			color = dz ? color_mix(c1, c2, (z-p1.z)/dz) : c1;
			backend->draw(backend, (Point2d_t){x, p1.y+(x-p1.x)/dx*dy, z}, color);
		}
	} else {
		step = dy < 0 ? -1 : 1;
		int y = fmin(fmax(p1.y, camera->height/-2), camera->height/2);
		int y_stop = fmin(fmax(p2.y, camera->height/-2), camera->height/2);
		for (; (y-y_stop)*step <= 0; y+=step) {
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
	if (camera->dept > 0 && p1.z > camera->dept && p2.z > camera->dept && p3.z > camera->dept)
		return;
	Color_t color[3] = {c1, c2, c3};
	Point2d_t p[3] = {p1, p2, p3};

	/* 冒泡排序 */
	for (size_t i = 0; i < countof(p); i++) {
		for (size_t j = 0; j < countof(p)-1; j++) {
			if (p[j].y <= p[j+1].y) continue;
			Point2d_t tmp_p = p[j+1];
			p[j+1] = p[j];
			p[j] = tmp_p;
			Color_t tmp_c = color[j+1];
			color[j+1] = color[j];
			color[j] = tmp_c;
		}
	}
	if (p[0].y >= p[2].y) return;

	/* 边缘裁切 */
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

	double z = 0;
	Color_t rgb;
	Point_t ret;
#define LERP(start, end, k) ((start) + (k) * ((end) - (start)))
	for (int y = y_min; y < y_max; y++) {
		double dy[2] = {
			(y-p[0].y)/(p[2].y-p[0].y),
			y < p[1].y ? (y-p[0].y)/(p[1].y-p[0].y) : (y-p[1].y)/(p[2].y-p[1].y),
		};
		double x_range[2] = {
			LERP(p[0].x, p[2].x, dy[0]),
			y < p[1].y ? LERP(p[0].x, p[1].x, dy[1]) : LERP(p[1].x, p[2].x, dy[1]),

		};
		int x_left = x_range[0];
		int x_right = x_range[1];
		if (x_left > x_right) {
			x_left ^= x_right;
			x_right ^= x_left;
			x_left ^= x_right;
		}
		for (int x = fmax(x_left, x_min); x < fmin(x_right, x_max); x++) {
			/* 向量法计算重心坐标 */
			/* 数学考虑：
			 * 三角形ABC，动点D在射线AB上，动点E在线段AC上，动点P在线段DE上且在ABC内。
			 * λ=|AD|/|AB|, μ=|AE|/|AC|, k=|DP|/|ED|，用OA,OB,OC表示OP
			 * 特别的，额外考虑DE平行于AB时，另立解法如下：
			 * 三角形ABC，动点D在线段AC上，动点E在线段BC上且满足ED//AB，动点P在线段DE上且在ABC内。
			 * μ=|AD|/|AC|, k=|DP|/|DE|，用OA,OB,OC表示OP
			 * */
			z = (x-x_range[1])/(x_range[0]-x_range[1]);
			ret.x = p[0].y<p[1].y ? 1+dy[1]*z-z*dy[0]-dy[1]	: (1-dy[0])*z;
			ret.y = p[0].y<p[1].y ? (1-z)*dy[1]			: (1-dy[0])*(1-z);
			ret.z = p[0].y<p[1].y ? z*dy[0]			: dy[0];
			if (fabs(ret.x+ret.y+ret.z-1) > 1e-5) continue;
			/* 透视插值 */
			const double inv_z[4] = {1./p[0].z, 1./p[1].z, 1./p[2].z};
			z = 1./(ret.x*inv_z[0] + ret.y*inv_z[1] + ret.z*inv_z[2]);
#define interpolation(var, field) ((ret.x*inv_z[0]*var[0].field + ret.y*inv_z[1]*var[1].field + ret.z*inv_z[2]*var[2].field)*z)
			rgb.r = interpolation(color, r);
			rgb.g = interpolation(color, g);
			rgb.b = interpolation(color, b);
			rgb.a = interpolation(color, a);
			if (z < 0 || (camera->dept > 0 && z > camera->dept))
				continue;
			backend->draw(backend, (Point2d_t){
				      x, y,
				      z/fabs(camera->dept)},
				      rgb);
		}
	}
	return;
}

