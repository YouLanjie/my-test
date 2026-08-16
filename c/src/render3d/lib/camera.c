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
		.z_near = 1e-6,
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
	*((PrivatCamera_t*)p->private_data) = (PrivatCamera_t){
		(Vec_t){}, (Vec_t){}, (Vec_t){}, false
	};
	return p;
}

void camera_free(Camera_t *p)
{
	if (!p) return;
	if (p->private_data) free(p->private_data);
	free(p);
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

static void camera_update_direct(Camera_t *camera)
{
	if (!camera) return;
	PrivatCamera_t *pc = camera->private_data;
	if (pc->locked) {
		/* 一旦锁定即停止同步运算,减少计算量 */
		return;
	}
	/* 得到相机坐标系的x,y,z三轴 */
	pc->z = vec_direct(vec_mul(camera->forward, -1));
	pc->y = vec_direct(camera->up);
	pc->x = vec_direct(vec_cross_product(pc->y, pc->z));
}

Point_t camera_world2camera(Camera_t *camera, Point_t p)
{
	if (!camera || !camera->private_data) return (Point_t){0,0,INFINITY};
	PrivatCamera_t *pc = camera->private_data;
	/* 转换坐标系 */
	p = vec_sub(p, camera->position);
	p = (Vec_t){vec_point_product(p,pc->x), vec_point_product(p,pc->y), vec_point_product(p,pc->z)};
	p.z = -p.z;    /* 负数方向转为正数深度 */
	return p;
}

Point2d_t camera_cast(Camera_t *camera, Point_t p)
{
	if (!camera || !camera->private_data || camera->scale <= 0) return (Point_t){0,0,-1};
	camera_update_direct(camera);
	p = camera_world2camera(camera, p);
	if (p.z <= 0 || p.z > camera->dept) return (Point_t){0,0,-2};

	p.x = camera->scale*p.x/p.z - camera->offset_x;
	p.y = camera->scale*p.y/p.z - camera->offset_y;
	if (p.y > camera->height/2 || p.y < camera->height/-2 ||
	    p.x > camera->width/2  || p.x < camera->width/-2)
		return (Point_t){0,0,0};    /* 超出可视范围检测 */
	return p;
}

int camera_cast_line(Camera_t *camera, Point_t p1, Point_t p2, Point2d_t *ret_p1, Point2d_t *ret_p2)
{
	if (!camera || !camera->private_data || camera->scale <= 0 || !ret_p1 || !ret_p2) return -1;
	camera_update_direct(camera);
	p1 = camera_world2camera(camera, p1);
	p2 = camera_world2camera(camera, p2);

	/* 如果两端都同侧越界，则认为非法跳过(若dept小于零则忽略) */
	if ((p1.z <= 0 && p2.z <= 0) || (camera->dept > 0 && p1.z > camera->dept && p2.z > camera->dept)) return -2;
	if (p1.z < 0 || p2.z < 0) {    /* 确保深度都为正 */
		Point_t *pp1 = p1.z < 0 ? &p1 : &p2,
			*pp2 = p1.z > 0 ? &p1 : &p2;
		*pp1 = vec_add(*pp1, vec_mul(vec_sub(*pp2, *pp1), (camera->z_near-pp1->z)/(pp2->z-pp1->z)));
	}
	if (camera->dept > 0 && (p1.z > camera->dept || p2.z > camera->dept)) {    /* 确保深度不超纲 */
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

int camera_cast_surface(Camera_t *camera,
			Point_t *p1, Point_t *p2, Point_t *p3,
			Point_t *p4, Point_t *p5, Point_t *p6)
{
	if (!camera || !p1 || !p2 || !p3 || !p4 || !p5 || !p6) return -1;
	camera_update_direct(camera);
	Point_t *points[3] = {p1, p2, p3};
	Point_t *positive[3] = {};
	Point_t *nagative[3] = {};
	size_t count = 0;
	for (size_t i = 0; i < countof(points); i++) {
		*points[i] = camera_world2camera(camera, *points[i]);
		if (points[i]->z > camera->z_near) {
			positive[count] = points[i];
			count++;
		} else {
			nagative[i-count] = points[i];
		}
	}
	if (count == 0) return 0;
	switch (count) {
	case 2:
		*p4 = *positive[0];
		*p5 = *positive[1];
		*p6 = vec_add(*p4, vec_mul(vec_sub(*nagative[0], *p4), (p4->z-camera->z_near)/(p4->z-nagative[0]->z)));
		*positive[0] = *p6;
		*nagative[0] = vec_add(*p5, vec_mul(vec_sub(*nagative[0], *p5), (p5->z-camera->z_near)/(p5->z-nagative[0]->z)));
		count = 6;
		break;
	case 1:
		p4 = positive[0];
		*nagative[0] = vec_add(*p4, vec_mul(vec_sub(*nagative[0], *p4), (p4->z-camera->z_near)/(p4->z-nagative[0]->z)));
		*nagative[1] = vec_add(*p4, vec_mul(vec_sub(*nagative[1], *p4), (p4->z-camera->z_near)/(p4->z-nagative[1]->z)));
		count = 3;
		break;
	}
	Point_t *p[6] = {p1, p2, p3, p4, p5, p6};
	for (size_t i = 0; i < countof(p) && i < count; i++) {
		if (p[i]->z == 0) p[i]->z = 1e-9;
		p[i]->x = camera->scale*p[i]->x/fabs(p[i]->z) - camera->offset_x;
		p[i]->y = camera->scale*p[i]->y/fabs(p[i]->z) - camera->offset_y;
	}
	return count / 3;
}

void camera_shift(Camera_t *camera, Vec_t direction)
{
	if (!camera) return;
	camera->position = vec_add(camera->position, direction);
}

void camera_rotate(Camera_t *camera, Vec_t direction, double theta)
{
	if (!camera) return;
	direction = vec_direct(direction);
	camera->forward = vec_rotate(camera->forward, direction, theta);
	camera->up = vec_rotate(camera->up, direction, theta);
}

void camera_rotate_about_point(Camera_t *camera, Point_t about_point, Vec_t direction, double theta)
{
	if (!camera) return;
	direction = vec_direct(direction);

	Vec_t diff = vec_sub(camera->position, about_point);
	diff = vec_rotate(diff, direction, theta);
	camera->position = vec_add(about_point, diff);

	camera->forward = vec_rotate(camera->forward, direction, theta);
	camera->up = vec_rotate(camera->up, direction, theta);
}

void camera_look(Camera_t *camera, Point_t point, Vec_t hold)
{
	if (!camera) return;
	if (vec_len(hold) == 0) hold = (Vec_t){0, 1, 0};    /* 默认向上保持 */
	double len = vec_len(vec_sub(point, camera->position));
	if (len <= 0) return;    /* 忽略非法请求 */
	const Vec_t forward = vec_mul(vec_sub(point, camera->position), 1./len);
	Vec_t right = vec_cross_product(forward, hold);
	if (right.x == 0 && right.y == 0 && right.z == 0)    /* 避免万向节锁错误 */
		right = vec_cross_product(camera->forward, hold);
	camera->forward = forward;
	camera->up = vec_direct(vec_cross_product(right, forward));
}

void camera_look_no_hold(Camera_t *camera, Point_t point)
{
	if (!camera) return;
	double len = vec_len(vec_sub(point, camera->position));
	if (len <= 0) return;    /* 忽略非法请求 */
	const Vec_t forward = vec_mul(vec_sub(point, camera->position), 1./len);
	Vec_t mu = vec_cross_product(camera->forward, forward);
	if ((len = vec_len(mu)) < 1e-12) {    /* 共线 */
		if (vec_point_product(forward, camera->forward) > 0) return;    /* 同向不变 */
		camera->up = vec_mul(camera->up, -1);
		return;
	}
	mu = vec_mul(mu, 1./len);
	// 复习： ^a * ^b == |^a| * |^b| * cos(theta)
	double cos_angele =
		vec_point_product(camera->forward, forward) /
		(vec_len(forward)*vec_len(camera->forward));
	if (cos_angele > 1.0) cos_angele = 1.0;
	if (cos_angele < -1.0) cos_angele = -1.0;
	camera->forward = forward;
	camera->up = vec_direct(vec_rotate(camera->up, mu, acos(cos_angele)));
}

