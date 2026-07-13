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

#define MAX_FRAME 100000
#define FPS 40
/* 时间缩放倍率，一秒等于1h */
#define TIME_SCALE (60*60/FPS)

typedef struct {
	const char *name;
	Obj_t *obj;
	double mass;    /* 质量(kg) */
	double self_omiga;    /* 自转速度(rad/s) */
	Vec_t self_rotate;    /* 自转方向 */
	Vec_t speed;    /* 速度(km/s) */
} Star_t;

const double G = 6.672e-11;
const double SCALE = 1e3;    /* 将距离换算成 1单位 = 1km */
#define pow2(x) ((x)*(x))

static void physics_update(Star_t *stars, size_t len)
{
	if (!stars || len == 0) return;
	len %= 1024;
	Vec_t acc[len] = {};
	Vec_t diff;
	double r2 = 0;
	double a = 0;
	for (size_t i = 0; i < len; i++) {
		if (!stars[i].obj) continue;
		// 对于每个天体
		for (size_t j = i+1; j < len; j++) {
			if (!stars[j].obj) continue;
			// 计算它与它往后所有天体的加速度
			diff = vec_sub(stars[i].obj->center, stars[j].obj->center);    /* j -> i */
			r2 = (pow2(diff.x) + pow2(diff.y) + pow2(diff.z)) * pow2(SCALE);
			if (r2 > 0) a = G/r2/SCALE;
			diff = vec_direct(diff);
			// LOG("\e[0m[%ld] a = %.2lf Tm/(kg * s^2)\n", j, r2);
			// a = G*M/(r^2)
			acc[i] = vec_add(acc[i], vec_mul(diff, -a * stars[j].mass));
			acc[j] = vec_add(acc[j], vec_mul(diff,  a * stars[i].mass));
		}
	}
	for (size_t i = 0; i < len; i++) {
		stars[i].speed = vec_add(stars[i].speed, vec_mul(acc[i], TIME_SCALE));
		obj_shift(stars[i].obj, vec_mul(stars[i].speed, TIME_SCALE));
		obj_rotate(stars[i].obj, stars[i].self_rotate, stars[i].self_omiga*TIME_SCALE);
	}
	return;
}

static int choose_star(Star_t *stars, size_t len, int *choice, const char *hint)
{
	if (!stars || !len || !choice) return -1;
	printf("\e[0m\n可选天体：\n [0] 空选择\n");
	for (size_t i = 0; i < len; i++) {
		if (!stars[i].obj) continue;
		printf(" [%lu] %s\n", i+1, stars[i].name ? stars[i].name : "{未命名星体}");
	}
	printf("(当前：%d)请输入要%s物体的id[0~%lu]：",
	       *choice + 1, hint ? hint : "选择", len);
	if (scanf("%d", choice) == 0) {
		kbhitGetchar();
		printf("输入错误，未作任何更改(回车返回)\n");
		_getch();
	} else (*choice)--;
	return 0;
}

static void input_handle(int inp, Star_t *star, Camera_t *camera)
{
	if (!camera) return;
#define v_forward vec_direct(camera->forward)
#define v_up vec_direct(camera->up)
#define v_right vec_direct(vec_cross_product(camera->forward, camera->up))
	switch (inp) {
        case '-': camera_shift(camera, vec_mul(v_forward, -0.5*SCALE)); break;
        case '=': camera_shift(camera, vec_mul(v_forward, 0.5*SCALE)); break;
	case 'W': camera_shift(camera, vec_mul(v_up, 0.5*SCALE)); break;
	case 'S': camera_shift(camera, vec_mul(v_up, -0.5*SCALE)); break;
	case 'A': camera_shift(camera, vec_mul(v_right, -0.5*SCALE)); break;
	case 'D': camera_shift(camera, vec_mul(v_right, 0.5*SCALE)); break;
	default:
		if (!star || !star->obj) return;
		break;
	}

	static const double accelerate = 10/pow2(SCALE)*TIME_SCALE;
#define accelerate(var) star->speed = vec_add(star->speed, (var))
	switch (inp) {
	case '_': star->speed = vec_mul(star->speed, 0.1); break;
	case ' ': accelerate(vec_mul(v_forward, 5*accelerate)); break;
	case 'b': accelerate(vec_mul(v_forward, -5*accelerate)); break;
	case 'w': accelerate(vec_mul(v_up, accelerate)); break;
	case 's': accelerate(vec_mul(v_up, -accelerate)); break;
	case 'a': accelerate(vec_mul(v_right, -accelerate)); break;
	case 'd': accelerate(vec_mul(v_right, accelerate)); break;
#undef accelerate
	}
#undef v_forward
#undef v_up
#undef v_right
	return;
}

static void voyage_helper(Star_t *stars, size_t len)
{
	if (!stars || !len) return;
	int from = -1, to = -1;
	choose_star(stars, len, &from, "正在驾驶的");
	choose_star(stars, len, &to, "要驶入的");
	if (!(from >= 0 && (size_t)from < len && stars[from].obj) || 
	    !(to >= 0 && (size_t)to < len && stars[to].obj)) {
		printf("选择非法（回车返回）\n");
		kbhitGetchar();
		_getch();
		return;
	}
	const Vec_t direct = vec_sub(stars[to].obj->center, stars[from].obj->center);
	double distance = vec_len(direct);
	if (distance <= 0) distance = 1e-20;
	const double speed = sqrt(G*stars[to].mass/(distance*SCALE)) / SCALE;
	printf("========== 结果 ==========\n");
	printf("'%s' -> '%s'\n",
	       stars[from].name ? stars[from].name : "Unknow",
	       stars[to].name ? stars[to].name : "Unknow");
	printf("距离：%.1f km\n", distance);
	printf("航向：{%.1f, %.1f, %.1f}\n", direct.x, direct.y, direct.z);
	printf("目标线速度：%g km/s, 角速度：%g rad/s\n", speed, speed/distance);
	printf("周期：%.1f s | %.1f d\n", 2*M_PI/(speed/distance), 2*M_PI/(speed/distance)/(24*60*60));
	printf("（回车返回）\n");
	kbhitGetchar();
	_getch();
	return;
}

int main(void)
{
	const int term_w = get_winsize_col() - 0;
	const int term_h = get_winsize_row() - 1;
	RenderBackend_t *backend = backend_create_utf8(term_w, term_h);
	Camera_t        *camera = camera_create();
	Star_t           objs[] = {
		(Star_t){
			.name = "地球",
			.obj = obj_rotate(obj_create_cube(6371), (Vec_t){1, 1, -1}, M_PI/4),
			.mass = 5.965e24,
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(24*60*60),
			// 主星（地球）
			// 逃逸速度：
			// 7.9km/s  11.2km/s
		}, (Star_t){
			.name = "地球小卫星",
			.obj = obj_shift(obj_create_cube(1), (Vec_t){7000, 0, 0}),
			.mass = 1,
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0.8}), 7.9),
		}, (Star_t){
			.name = "地球大卫星",
			.obj = obj_shift(obj_create_cube(2000), (Vec_t){-11000, 0, 0}),
			.mass = 1e10,
			// GM = Rv^2
			// > sqrt((6.67*10^-11) * (5.965*10^24) / (11000*1000))/1000
			// 6.0141159707
			.speed = vec_mul(vec_direct((Vec_t){0, 1, -0.5}), 6.0141159707),
			.self_rotate = (Vec_t){1, 1, -1},
			.self_omiga = 2*M_PI/(24*60*60),
		}, (Star_t){
			.name = "月球",
			.obj = obj_shift(obj_create_cube(1737.4), (Vec_t){0, 384400, 0}),
			.mass = 7.342e22,
			.speed = vec_mul(vec_direct((Vec_t){-1, 0, 0}), 1.022),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(30.5*24*60*60),
		},
	};
	Obj_t *axis_helper = obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){10*SCALE,0,0}));
	obj_merge_and_free(axis_helper, obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){0,6*SCALE,0})));
	obj_merge_and_free(axis_helper, obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){0,0,3*SCALE})));
	if (!backend || !camera || !axis_helper) goto EXIT_AND_CLEANUP;

	camera->width = term_w;
	camera->height = term_h*2;
	camera->position = (Vec_t){0, 0, 20*SCALE};
	camera->dept = 100*SCALE;
	const Camera_t orig_cam_dat = *camera;
	// camera->scale = 10;
	// double G = 

	printf("\e[2J");
	int destination_to = -1;
	int look_to = -1;
	int follow = -1;
	bool axis = false;
	bool pause = false;
	size_t i = 0;
	int inp = 0;
#define item_in_objs(ind) (follow >= 0 && (size_t)ind < countof(objs) && objs[ind].obj)
	for (i = 0; i < MAX_FRAME; ++i) {
		switch (inp = kbhitGetchar()) {
		case 'f': choose_star(objs, countof(objs), &follow, "跟随"); break;
		case 'F': choose_star(objs, countof(objs), &look_to, "看向"); break;
		case 't': choose_star(objs, countof(objs), &destination_to, "测距"); break;
		case '?': voyage_helper(objs, countof(objs)); break;
		case '+':
			*camera = orig_cam_dat;
			look_to = -1;
			follow = -1;
			destination_to = -1;
			break;
		case '-': case '=': case '_': case ' ': case 'b':
		case 'w': case 'a': case 's': case 'd':
		case 'W': case 'A': case 'S': case 'D':
			input_handle(inp, item_in_objs(follow) ? objs+follow : NULL, camera);
			break;
		case '9': camera->dept/=2; break;
		case '0': camera->dept*=2; break;
		case 'i': axis = !axis; break;
		case 'p': pause = !pause; break;
		case '7': camera->scale-=1; break;
		case '8': camera->scale+=1; break;
		case '.':
			pause = true;
			physics_update(objs, countof(objs));
			break;
		case 'q':
		case 'Q':
			goto EXIT_AND_CLEANUP;
			break;
		}
		if (!pause) physics_update(objs, countof(objs));
		if (item_in_objs(follow))
			camera->position = vec_add(objs[follow].obj->center,
						   vec_mul(vec_direct(camera->forward), -vec_len(orig_cam_dat.position)));
		if (item_in_objs(look_to))
			camera_look(camera, objs[look_to].obj->center, (Vec_t){0,1,0});
		else if (item_in_objs(follow) && vec_len(objs[follow].speed) > 0)
			camera_look(camera, vec_add(camera->position, objs[follow].speed),
				    (Vec_t){0,1,0});

		if (axis && item_in_objs(follow)) {
			axis_helper->center = objs[follow].obj->center;
			obj_cast(axis_helper, camera, backend);
		}
		for (size_t i = 0; i < countof(objs); i++) {
			if (!objs[i].obj) continue;
			obj_cast(objs[i].obj, camera, backend);
		}
		printf("\e[H");
		backend->render(backend);
		backend->clean(backend);
		printf("\e[0m\e[2K\rCamera[%.1f,%.1f,%.1f]",
		       camera->position.x / SCALE,
		       camera->position.y / SCALE,
		       camera->position.z / SCALE);
		if (item_in_objs(follow)) {
			printf(" | %s (%.1f,%.1f,%.1f) %.1f km/s",
			       objs[follow].name ? objs[follow].name : "Unknow",
			       objs[follow].obj->center.x / SCALE,
			       objs[follow].obj->center.y / SCALE,
			       objs[follow].obj->center.z / SCALE,
			       vec_len(objs[follow].speed));
			if (item_in_objs(destination_to)) {
				printf(" 距离'%s'还有 %.1f km",
				       objs[destination_to].name ? objs[destination_to].name : "Unknow",
				       vec_len(vec_sub(objs[destination_to].obj->center,
						       objs[follow].obj->center)));
			}
		}
		sleep_fixed_step(1./FPS);
	}

EXIT_AND_CLEANUP:
	printf("\e[0m\n");
	if (backend) backend->destroy(backend);
	if (camera) camera_free(camera);
	if (axis_helper) obj_free(axis_helper);
	for (size_t i = 0; i < countof(objs); i++) {
		if (objs[i].obj) obj_free(objs[i].obj);
	}
	return EXIT_SUCCESS;
}

