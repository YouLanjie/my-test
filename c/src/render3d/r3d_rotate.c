/**
 * @file        r3d_rotate.c
 * @author      Chglish
 * @date        2026-07-12
 * @brief       空观旋转.elf
 */

#include "./lib/render3d.h"
#include <math.h>
#include <stdbool.h>
#include <stdcountof.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_FRAME 100000
#define FPS 40
/* 时间缩放倍率，一秒等于1h */
double TIME_SCALE = 60.*60/FPS;

typedef struct {
	const char *name;
	Obj_t *obj;
	double mass;          /* 质量(kg) */
	double self_omiga;    /* 自转速度(rad/s) */
	Vec_t self_rotate;    /* 自转方向 */
	Vec_t speed;          /* 速度(km/s) */
	Camera_t cam;         /* 随身相机 */
} Star_t;

const double G = 6.672e-11;
const double SCALE = 1e3;    /* 将距离换算成 1单位 = 1km */
#define pow2(x) ((x)*(x))

typedef struct {
	RenderBackend_t *backend;
	Camera_t *camera;
	Camera_t *active_cam;
	size_t obj_count;
	Star_t *objs;
	Star_t *destination_to;
	Star_t *look_to;
	Star_t *follow;
	Obj_t  *axis_helper;
	double fuel_consumption;
	double gtime;
	int  inp;
	bool axis;
	bool pause;
} Runtimedata_t;

static void cleanup(Runtimedata_t *rt)
{
	printf("\e[0m\n");
	if (!rt) return;
	if (rt->backend) rt->backend->destroy(rt->backend);
	if (rt->camera) camera_free(rt->camera);
	if (rt->axis_helper) obj_free(rt->axis_helper);
	rt->backend = NULL;
	rt->camera  = NULL;
	if (!rt->objs) return;
	for (size_t i = 0; i < rt->obj_count; i++) {
		if (rt->objs[i].obj) obj_free(rt->objs[i].obj);
	}
}

static bool setup(Runtimedata_t *rt)
{
	if (!rt) return false;
	const int term_w = get_winsize_col() - 0;
	const int term_h = get_winsize_row() - 1;
	rt->backend = backend_create_utf8(term_w, term_h);
	// rt->backend = backend_create_ascii_8bit(term_w, term_h);
	rt->camera = camera_create();
	rt->axis_helper = obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){10*SCALE,0,0}));
	obj_merge_and_free(rt->axis_helper, obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){0,6*SCALE,0})));
	obj_merge_and_free(rt->axis_helper, obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){0,0,3*SCALE})));
	if (!rt->backend || !rt->camera || !rt->axis_helper) {
		cleanup(rt);
		return false;
	}
	rt->camera->width = term_w;
	rt->camera->height = term_h*2;
	rt->camera->position = (Vec_t){0, 0, 20*SCALE};
	rt->camera->dept = 100*SCALE;
	rt->active_cam = rt->camera;
	return true;
}

static double physics_update(Runtimedata_t *rt)
{
	if (!rt || rt->obj_count == 0) return 0;
	Star_t *objs = rt->objs;
	const size_t len = rt->obj_count % 1024;
	Vec_t acc[len] = {};
	Vec_t diff;
	double r2 = 0;
	double a = 0;
	for (size_t i = 0; i < len; i++) {
		if (!objs[i].obj) continue;
		// 对于每个天体
		for (size_t j = i+1; j < len; j++) {
			if (!objs[j].obj) continue;
			// 计算它与它往后所有天体的加速度
			diff = vec_sub(objs[i].obj->center, objs[j].obj->center);    /* j -> i */
			r2 = (pow2(diff.x) + pow2(diff.y) + pow2(diff.z)) * pow2(SCALE);
			if (r2 > 0) a = G/r2/SCALE;
			diff = vec_direct(diff);
			// LOG("\e[0m[%ld] a = %.2lf Tm/(kg * s^2)\n", j, r2);
			// a = G*M/(r^2)
			acc[i] = vec_add(acc[i], vec_mul(diff, -a * objs[j].mass));
			acc[j] = vec_add(acc[j], vec_mul(diff,  a * objs[i].mass));
		}
	}
	for (size_t i = 0; i < len; i++) {
		objs[i].speed = vec_add(objs[i].speed, vec_mul(acc[i], TIME_SCALE));
		diff = vec_mul(objs[i].speed, TIME_SCALE);
		obj_shift(objs[i].obj, diff);
		objs[i].cam.position = vec_add(objs[i].cam.position, diff);
		obj_rotate(objs[i].obj, objs[i].self_rotate, objs[i].self_omiga*TIME_SCALE);
	}
	return TIME_SCALE;
}

static Star_t *choose_star(Runtimedata_t *rt, const char *hint, Star_t *old)
{
	if (!rt) return NULL;
	printf("\e[0m\n可选天体：\n [0] 空选择\n");
	size_t len = 0;
	int choice = 0;
	for (size_t i = 0; i < rt->obj_count; i++) {
		len = i;
		if (!rt->objs[i].obj) break;
		printf(" [%lu] %s (%gkg)\n", i+1,
		       rt->objs[i].name ? rt->objs[i].name : "{未命名星体}",
		       rt->objs[i].mass);
		if (rt->objs+i == old) choice = i;
	}
	printf("(当前：%d)请输入要%s物体的id[0~%lu]：",
	       choice + 1, hint ? hint : "选择", len);
	if (scanf("%d", &choice) == 0) {
		kbhitGetchar();
		printf("输入错误，未作任何更改(回车返回)\n");
		_getch();
		return NULL;
	}
	choice--;
	if (choice == -1) return NULL;
	if (choice < 0 || (size_t)choice >= len) {
		printf("选择非法（回车返回）\n");
		kbhitGetchar();
		_getch();
		return NULL;
	}
	return rt->objs+choice;
}

static void voyage_helper(Runtimedata_t *rt)
{
	if (!rt) return;
	Star_t *from = NULL, *to = NULL;
	while ((from = choose_star(rt, "正在驾驶的", NULL)) == NULL)
		printf("重试...\n");
	while ((to = choose_star(rt, "要驶入的", NULL)) == NULL)
		printf("重试...\n");
	const Vec_t direct = vec_sub(to->obj->center, from->obj->center);
	double distance = vec_len(direct);
	if (distance <= 0) distance = 1e-20;
	const double speed = sqrt(G*to->mass/(distance*SCALE)) / SCALE;
	printf("========== 结果 ==========\n");
	printf("'%s' -> '%s'\n",
	       from->name ? from->name : "Unknow",
	       to->name ? to->name : "Unknow");
	printf("距离：%.1f km\n", distance);
	printf("航向：{%.1f, %.1f, %.1f}\n", direct.x, direct.y, direct.z);
	printf("目标线速度：%g km/s, 角速度：%g rad/s\n", speed, speed/distance);
	printf("周期：%.1f s | %.1f d\n", 2*M_PI/(speed/distance), 2*M_PI/(speed/distance)/(24*60*60));
	printf("（回车返回）\n");
	kbhitGetchar();
	_getch();
	return;
}

static bool input_handle(Runtimedata_t *rt)
{
	if (!rt) return false;
	double accel = 10/pow2(SCALE)*TIME_SCALE;

#define v_forward vec_direct(rt->active_cam->forward)
#define v_up      vec_direct(rt->active_cam->up)
#define v_right   vec_direct(vec_cross_product(rt->active_cam->forward, rt->active_cam->up))
	switch (rt->inp) {
	case '[': TIME_SCALE/=2; break;
	case ']': TIME_SCALE*=2; break;
	case '{': TIME_SCALE=1./FPS; break;
	case '}': TIME_SCALE=(60.*60/FPS); break;
	case 'f':
		rt->follow = choose_star(rt, "跟随", rt->follow);
		if (!rt->follow) {
			rt->active_cam = rt->camera;
			break;
		}
		rt->active_cam = &rt->follow->cam;
		rt->active_cam->position = 
			vec_add(rt->follow->obj->center,
				vec_mul(vec_direct(rt->follow->speed),
					-vec_len(vec_sub(rt->active_cam->position,
							 rt->follow->obj->center))));
		camera_look_no_hold(rt->active_cam,
				    vec_add(rt->active_cam->position,
					    rt->follow->speed));
		break;
	case 'F': rt->look_to = choose_star(rt, "看向", rt->look_to); break;
	case 't': rt->destination_to = choose_star(rt, "测距", rt->destination_to); break;
	case '?': voyage_helper(rt); break;
	case 'i': rt->axis = !rt->axis; break;
	case 'p': rt->pause = !rt->pause; break;
	case '7': rt->active_cam->scale-=1; break;
	case '8': rt->active_cam->scale+=1; break;
	case '9': rt->active_cam->dept/=2; break;
	case '0': rt->active_cam->dept*=2; break;
	case '.':
		rt->pause = true;
		rt->gtime += physics_update(rt);
		break;
	case 'q':
	case 'Q':
		return false;
		break;
#define cam_shift(vec, k) camera_shift(rt->active_cam, vec_mul(vec_direct(vec), (k)))
        case '-': cam_shift(v_forward, -0.5*SCALE); break;
        case '=': cam_shift(v_forward, 0.5*SCALE); break;
	case '_': cam_shift(v_forward, -5e2*SCALE); break;
        case '+': cam_shift(v_forward, 5e2*SCALE); break;
	case 'W': cam_shift(v_up, 0.5*SCALE); break;
	case 'S': cam_shift(v_up, -0.5*SCALE); break;
	case 'A': cam_shift(v_right, -0.5*SCALE); break;
	case 'D': cam_shift(v_right, 0.5*SCALE); break;
#undef cam_shift
#define cam_rotate(vec, theta) \
		  rt->follow?\
		  camera_rotate_about_point(rt->active_cam, rt->follow->obj->center, (vec), -(theta)):\
		  camera_rotate(rt->active_cam, (vec), (theta))
	case 'h': cam_rotate(v_up, M_PI/180);break;
	case 'j': cam_rotate(v_right, -M_PI/180);break;
	case 'k': cam_rotate(v_right, M_PI/180);break;
	case 'l': cam_rotate(v_up, -M_PI/180);break;
	case 'J': cam_rotate(v_forward, M_PI/180);break;
	case 'K': cam_rotate(v_forward, -M_PI/180);break;
#undef cam_rotate
	}
	if (!rt->follow) return true;

	const double speed1 = vec_len(rt->follow->speed) * SCALE;
#define accelerate(var, k) rt->follow->speed = vec_add(rt->follow->speed, vec_mul((var), (k))), accel = k
	switch (rt->inp) {
	// case '_': rt->follow->speed = vec_mul(rt->follow->speed, 0.1); break;
	case ' ': accelerate(v_forward, 5*accel); break;
	case 'N': accelerate(v_forward, 50*accel); break;
	case 'b': accelerate(v_forward, -5*accel); break;
	case 'B': accelerate(v_forward, -50*accel); break;
	case 'w': accelerate(v_up, accel); break;
	case 's': accelerate(v_up, -accel); break;
	case 'a': accelerate(v_right, -accel); break;
	case 'd': accelerate(v_right, accel); break;
#undef accelerate
	}
	const double speed2 = vec_len(rt->follow->speed) * SCALE;
	// 计算能量损耗
	rt->fuel_consumption += rt->follow->mass * fabs(pow2(speed2) - pow2(speed1)) / 2;
#undef v_forward
#undef v_up
#undef v_right
	return fabs(accel);
}

int main(void)
{
	Runtimedata_t rt = {0};
	if (!setup(&rt)) {
		return EXIT_FAILURE;
	}
	/* 日地距离 */
	const double Dx_SE = -149.6e6;
	/* 日月系相对太阳距离 */
	const double Vy_SE = -29.78;
	Star_t objs[] = {
		(Star_t){
			.name = "地球",
			.obj = obj_rotate(obj_shift(obj_create_cube(6371*2), (Vec_t){Dx_SE, 0, 0}),
					  (Vec_t){1, 1, -1}, M_PI/3.8),
			.mass = 5.965e24,
			.speed = (Vec_t){0, Vy_SE, 0},
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(24*60*60),
			// 主星（地球）
			// 逃逸速度：
			// 7.9km/s  11.2km/s
		}, (Star_t){
			.name = "地球小卫星",
			.obj = obj_shift(obj_create_cube(1), (Vec_t){7000+Dx_SE, 0, 0}),
			.mass = 1,
			.speed = vec_add(vec_mul(vec_direct((Vec_t){0, 1, 0.8}), 7.9), (Vec_t){0, Vy_SE, 0}),
		}, (Star_t){
			.name = "地球大卫星",
			.obj = obj_shift(obj_create_cube(900), (Vec_t){-11000+Dx_SE, 0, 0}),
			.mass = 1e10,
			// GM = Rv^2
			// > sqrt((6.67*10^-11) * (5.965*10^24) / (11000*1000))/1000
			// 6.0141159707
			.speed = vec_add(vec_mul(vec_direct((Vec_t){0, -1, 0.1}), 6.0141159707), (Vec_t){0, Vy_SE, 0}),
			.self_rotate = (Vec_t){1, 1, -1},
			.self_omiga = 2*M_PI/(24*60*60),
		}, (Star_t){
			.name = "月球",
			.obj = obj_shift(obj_create_cube(1737.4*2), (Vec_t){0+Dx_SE, 384400, 0}),
			.mass = 7.342e22,
			.speed = vec_add(vec_mul(vec_direct((Vec_t){-1, 0, 0}), 1.022), (Vec_t){0, Vy_SE, 0}),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(30.5*24*60*60),
		}, (Star_t){
			.name = "太阳",
			.obj = obj_shift(obj_create_cube(695700*2), (Vec_t){0, 384400, 0}),
			.mass = 1.989e30,
			.speed = vec_mul(vec_direct((Vec_t){0, 0, 0}), 1.022),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(30.5*24*60*60),
		}, (Star_t){ .name = "列表结束", },
	};

	for (size_t i = 0; i < countof(objs); i++) {
		if (!objs[i].obj) continue;
		objs[i].cam = *rt.camera;    /* 同步相机配置 */
		objs[i].cam.position = vec_add(objs[i].obj->center,
					       rt.camera->position);
	}
	rt.objs = objs;
	rt.obj_count = countof(objs);
	rt.follow = objs;

	printf("按键说明：\nwasd 控制偏转\n"
	       "bB 减速 空格或N加速\n"
	       "WASD 控制镜头平移 -=_+ 控制镜头远近\n"
	       "hjkl 控制镜头方向 JK 控制镜头旋转\n"
	       "7/8 控制焦距 9/0 控制可视距离\n"
	       "f 跟随  F 看向某物体  t 测距\n"
	       "[]{} 控制时间流速\n"
	       "? 进行数学辅助计算\n");
	rt.inp = 'f';
	input_handle(&rt);

	printf("\e[2J");
	size_t i = 0;
	for (i = 0; i < MAX_FRAME; ++i) {
		if ((rt.inp = kbhitGetchar()))
			if (!input_handle(&rt)) break;
		if (!rt.pause) rt.gtime += physics_update(&rt);
		if (rt.look_to) {
			Vec_t direct = rt.look_to == rt.follow ? \
				       rt.follow->speed : \
				       vec_sub(rt.look_to->obj->center, rt.follow->obj->center);
			double dist = vec_len(vec_sub(rt.follow->obj->center,
						      rt.active_cam->position));
			rt.active_cam->position = 
				vec_add(rt.follow->obj->center,
					vec_mul(vec_direct(direct), -dist));
			camera_look_no_hold(rt.active_cam, rt.look_to->obj->center);
		}

		if (rt.axis && rt.follow) {
			rt.axis_helper->center = rt.follow->obj->center;
			obj_cast(rt.axis_helper, rt.active_cam, rt.backend);
		}
		for (size_t i = 0; i < countof(objs); i++) {
			if (!objs[i].obj) continue;
			obj_cast(objs[i].obj, rt.active_cam, rt.backend);
		}
		printf("\e[H");
		rt.backend->render(rt.backend);
		rt.backend->clean(rt.backend);
		printf("\e[0m\e[2K\r[TS:%g, GT:%.1fd, E:%gJ D:%gkm]",
		       TIME_SCALE*FPS, rt.gtime/(24.*60*60),
		       rt.fuel_consumption,
		       rt.follow?vec_len(vec_sub(rt.active_cam->position,
						 rt.follow->obj->center)):0);
		if (rt.follow) {
			printf(" | %s (%g km/s)",
			       rt.follow->name ? rt.follow->name : "Unknow",
			       vec_len(rt.follow->speed));
		}
		if (rt.follow && rt.destination_to) {
			const Vec_t dist = vec_sub(rt.destination_to->obj->center, rt.follow->obj->center);
			const Vec_t dv = vec_sub(rt.follow->speed, rt.destination_to->speed);
			// 速度 <0 表靠近， >0 表远离
			printf(" 距离'%s'还有 %.1f km (%g km/s)",
			       rt.destination_to->name ? rt.destination_to->name : "Unknow",
			       vec_len(dist),
			       (vec_point_product(dist, dv)>=0?-1:1)*vec_len(dv));
		}
		sleep_fixed_step(1./FPS);
	}

	cleanup(&rt);
	return EXIT_SUCCESS;
}

