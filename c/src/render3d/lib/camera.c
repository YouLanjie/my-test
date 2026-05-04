/**
 * @file        camera.c
 * @author      u0_a221
 * @date        2026-05-03
 * @brief       做相机
 */

#include "render3d.h"

/* 仅申请内存并初始化 */
Camera_t *camera_create()
{
	Camera_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (Camera_t){
		.height = 9*100,
		.width = 16*100,
		.scale = 70,
		.dept = 20,
		.offset_x = 0,
		.offset_y = 0,
		.position = (Point_t){0,0,0},
		.forward = (Vec_t){0, 0, -1},
		.up = (Vec_t){0, 1, 0},
	};
	return p;
}

void camera_free(Camera_t **p)
{
	if (!p || !*p) return;
	free(*p);
	*p = NULL;
}

/* 将点投影到虚拟屏幕(相机)上
 * 返回：投影到相机后的二维点（z轴作为深度>0）
 * 若z<=0则为无法投影点或屏幕外点 */
Point2d_t camera_cast(Camera_t *camera, Point_t p)
{
	if (!camera) return (Point_t){0,0,-1};
	/* 得到相机坐标系的i,j,k三轴 */
	Vec_t k = vec_direct(vec_mul(camera->forward, -1)),
	      j = vec_direct(camera->up),               /* 纵坐标 */
	      i = vec_direct(vec_cross_product(j, k));  /* 横坐标 */
	p = vec_sub(p, camera->position);
	p = (Vec_t){vec_point_product(p,i), vec_point_product(p,j), vec_point_product(p,k)};
	p.z = -p.z;
	if (p.z <= 0 || p.z > camera->dept || camera->scale == 0) return (Point_t){0,0,-2};

	p.x = camera->scale*p.x/p.z - camera->offset_x;
	p.y = camera->scale*p.y/p.z - camera->offset_y;
	if (p.y > camera->height/2 || p.y < camera->height/-2 ||
	    p.x > camera->width/2  || p.x < camera->width/-2)
		return (Point_t){0,0,0};    /* 超出可视范围检测 */
	return p;
}

void camera_shift(Camera_t *camera, Vec_t direction)
{
	if (!camera) return;
	camera->position = vec_add(camera->position, direction);
}
void camera_rotate(Camera_t *camera, Vec_t direction, double theta)
{
	if (!camera) return;
	camera->forward = vec_rotate(camera->forward, direction, theta);
	camera->up = vec_rotate(camera->up, direction, theta);
}
