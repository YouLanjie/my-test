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
		.near = 1e-6,
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

/* 投影线条(需要传入两个端点的地址)
 * ret < 0 : 无投影 */
int camera_cast_line(Camera_t *camera, Point_t p1, Point_t p2, Point2d_t *ret_p1, Point2d_t *ret_p2)
{
	if (!camera || !camera->private_data || camera->scale <= 0 || !ret_p1 || !ret_p2) return -1;
	PrivatCamera_t *pc = camera->private_data;
	if (!pc->locked) {    /* 一旦锁定即停止同步运算,减少计算量 */
		/* 得到相机坐标系的x,y,z三轴 */
		pc->z = vec_direct(vec_mul(camera->forward, -1));
		pc->y = vec_direct(camera->up);
		pc->x = vec_direct(vec_cross_product(pc->y, pc->z));
	}
	/* 转换坐标系 */
	p1 = vec_sub(p1, camera->position);
	p1 = (Vec_t){vec_point_product(p1,pc->x), vec_point_product(p1,pc->y), vec_point_product(p1,pc->z)};
	p1.z = -p1.z;    /* 负数方向转为正数深度 */

	p2 = vec_sub(p2, camera->position);
	p2 = (Vec_t){vec_point_product(p2,pc->x), vec_point_product(p2,pc->y), vec_point_product(p2,pc->z)};
	p2.z = -p2.z;

	/* 如果两端都同侧越界，则认为非法跳过 */
	if ((p1.z <= 0 && p2.z <= 0) || (p1.z > camera->dept && p2.z > camera->dept)) return -2;
	if (p1.z < 0 || p2.z < 0) {    /* 确保深度都为正 */
		Point_t *pp1 = p1.z < 0 ? &p1 : &p2,
			*pp2 = p1.z > 0 ? &p1 : &p2;
		*pp1 = vec_add(*pp1, vec_mul(vec_sub(*pp2, *pp1), (camera->near-pp1->z)/(pp2->z-pp1->z)));
	}
	if (p1.z > camera->dept || p2.z > camera->dept) {    /* 确保深度不超纲 */
		Point_t *pp1 = p1.z > camera->dept ? &p1 : &p2,
			*pp2 = p1.z < camera->dept ? &p1 : &p2;
		*pp1 = vec_add(*pp2, vec_mul(vec_sub(*pp1, *pp2), (camera->dept-pp2->z)/(pp1->z-pp2->z)));
	}
	p1.x = camera->scale*p1.x/p1.z - camera->offset_x;
	p1.y = camera->scale*p1.y/p1.z - camera->offset_y;
	p2.x = camera->scale*p2.x/p2.z - camera->offset_x;
	p2.y = camera->scale*p2.y/p2.z - camera->offset_y;

	const double
		left = camera->width/-2, right  = camera->width/2,
		top  = camera->height/2, bottom = camera->height/-2;
	/* 使用 Liang-Barsky 算法进行线段裁切 */
	const double dx = p2.x-p1.x, dy = p2.y-p1.y;
	/* p数组和q数组顺序: 左, 右, 下, 上
	 * p: 离开边框的速度
	 * q: 起始点到边框的内边距 */
	const double p[4] = {-dx, dx, -dy, dy};
	const double q[4] = {p1.x-left, right-p1.x, p1.y-bottom, top-p1.y};
	/* u1,u2: 向量（点）OQ=OA+u*(OB-OA) */
	double u1 = 0, u2 = 1;
	/* r: 对于两线(x0为边界,x1为被截线,u为所求)相交方程
	 *    x0=x1+u*dx, 解得 u = (x0-x1)/dx = q/p
	 *    (其中正负已经分配到p和q的值中)
	 *    故r的几何意义为与预定交线的u值*/
	double r = 0;
	for (size_t i = 0; i < sizeof(p)/sizeof(p[0]); ++i) {
		if (p[i] == 0) {    /* 平行边界 */
			if (q[i] < 0) return -3;    /* 还是在外面的 */
			continue;
		}
		r = q[i] / p[i];
		/* 挑更出更里面的端点(尽可能截得更短点) */
		if (p[i] < 0) u1 = u1 > r ? u1 : r;    /* 进入 */
		else          u2 = u2 < r ? u2 : r;    /* 离开 */
	}
	if (u1 > u2) return -4;
	*ret_p1 = (Point2d_t){p1.x+u1*dx, p1.y+u1*dy, 1/(1/p1.z+u1*(1/p2.z-1/p1.z))};
	*ret_p2 = (Point2d_t){p1.x+u2*dx, p1.y+u2*dy, 1/(1/p1.z+u2*(1/p2.z-1/p1.z))};
	return 0;
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

