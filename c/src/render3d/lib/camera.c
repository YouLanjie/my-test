/**
 * @file        camera.c
 * @author      u0_a221
 * @date        2026-05-03
 * @brief       做相机
 */

#include "render3d.h"

typedef struct {
	Vec_t x;
	Vec_t y;
	Vec_t z;
	bool locked;
} PrivatCamera_t;

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
		.private_data = malloc(sizeof(PrivatCamera_t)),
	};
	if (p->private_data == NULL) {
		free(p);
		return NULL;
	}
	*((PrivatCamera_t*)p->private_data) = (PrivatCamera_t){{0},{0},{0}, false};
	return p;
}

void camera_free(Camera_t **p)
{
	if (!p || !*p) return;
	if ((*p)->private_data) free((*p)->private_data);
	free(*p);
	*p = NULL;
}

void camera_lock(Camera_t *camera)
{
	if (!camera || !camera->private_data) return;
	PrivatCamera_t *pc = camera->private_data;
	pc->locked = true;
	/* 更新相机坐标系的x,y,z三轴 */
	pc->z = vec_direct(vec_mul(camera->forward, -1));
	pc->y = vec_direct(camera->up);
	pc->x = vec_direct(vec_cross_product(pc->y, pc->z));
}

void camera_unlock(Camera_t *camera)
{
	if (!camera || !camera->private_data) return;
	((PrivatCamera_t*)camera->private_data)->locked = false;
}

/* 将点投影到虚拟屏幕(相机)上
 * 返回：投影到相机后的二维点（z轴作为深度>0）
 * 若z<=0则为无法投影点或屏幕外点 */
Point2d_t camera_cast(Camera_t *camera, Point_t p)
{
	if (!camera || !camera->private_data || camera->scale <= 0) return (Point_t){0,0,-1};
	PrivatCamera_t *pc = camera->private_data;
	if (!pc->locked) {    /* 一旦锁定即停止同步运算,减少计算量 */
		/* 得到相机坐标系的x,y,z三轴 */
		pc->z = vec_direct(vec_mul(camera->forward, -1));
		pc->y = vec_direct(camera->up);
		pc->x = vec_direct(vec_cross_product(pc->y, pc->z));
	}
	/* 转换坐标系 */
	p = vec_sub(p, camera->position);
	p = (Vec_t){vec_point_product(p,pc->x), vec_point_product(p,pc->y), vec_point_product(p,pc->z)};
	p.z = -p.z;    /* 负数方向转为正数深度 */
	if (p.z <= 0 || p.z > camera->dept) return (Point_t){0,0,-2};

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

void camera_look(Camera_t *camera, Point_t point, Vec_t hold)
{
	if (!camera) return;
	if (vec_len(hold) == 0) hold = (Vec_t){0, 1, 0};    /* 默认向上保持 */
	camera->forward = vec_direct(vec_sub(point, camera->position));
	camera->up = vec_direct(vec_cross_product(vec_cross_product(camera->forward, hold), camera->forward));
}

