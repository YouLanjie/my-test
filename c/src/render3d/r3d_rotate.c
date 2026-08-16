/**
 * @file        r3d_rotate.c
 * @author      Chglish
 * @date        2026-07-12
 * @brief       空观旋转.elf
 */

#include "lib/render3d.h"
#include "../../include/tools.h"

#define MAX_FRAME INT64_MAX
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

/* 引力常量 N*(m^2)/(kg^2) || (m^3)/(kg*s^2) */
const double G = 6.6743e-11;
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
	bool guidline;
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

static void sync_cam_size_scale(Runtimedata_t *rt)
{
	if (!rt || !rt->active_cam) return;
	int term_w = get_winsize_col() - 0;
	int term_h = get_winsize_row() - 1;
	if (rt->backend->get_size) {
		rt->backend->get_size(rt->backend, &term_w, &term_h);
	} else term_h *= 2;
	rt->active_cam->width = term_w;
	rt->active_cam->height = term_h;
	rt->active_cam->scale = fmax(term_w, term_h) / 2;
}

static bool setup(Runtimedata_t *rt)
{
	if (!rt) return false;
	int term_w = get_winsize_col() - 0;
	int term_h = get_winsize_row() - 1;
	if (!rt->backend) {
		rt->backend = backend_create_utf8_256bit(term_w, term_h);
		rt->camera = camera_create();
#define CREATE_LINE(x,y,z, r,g,b) obj_set_color(obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){x,y,z})), (Color_t){r,g,b,200})
		rt->axis_helper = CREATE_LINE(10*SCALE,0,0, -1,0,0);
		obj_merge_and_free(rt->axis_helper, CREATE_LINE(0,6*SCALE,0, 0,-1,0));
		obj_merge_and_free(rt->axis_helper, CREATE_LINE(0,0,3*SCALE, 0,0,-1));
#undef CREATE_LINE

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
#define BACKEND(name) backend_create_##name,
	static RenderBackend_t *(*backend_list[])(int width, int height) = {BACKEND_LIST};
#undef BACKEND
	enum Backend_id id = rt->backend->id;
	id = (id+1) % countof(backend_list);
	rt->backend->destroy(rt->backend);
	rt->backend = backend_list[id%countof(backend_list)](term_w, term_h);
	sync_cam_size_scale(rt);
	return true;
}

static void physics_update_step(Runtimedata_t *rt, double time_scale)
{
	if (!rt || rt->obj_count == 0 || time_scale == 0) return;
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
		objs[i].speed = vec_add(objs[i].speed, vec_mul(acc[i], time_scale));
		diff = vec_mul(objs[i].speed, time_scale);
		obj_shift(objs[i].obj, diff);
		objs[i].cam.position = vec_add(objs[i].cam.position, diff);
		obj_rotate(objs[i].obj, objs[i].self_rotate, objs[i].self_omiga*time_scale);
	}
}

static double physics_update(Runtimedata_t *rt)
{
	if (!rt || rt->obj_count == 0) return 0;
	const double ts_limit = 10;
	if (TIME_SCALE <= ts_limit) {
		physics_update_step(rt, TIME_SCALE);
		return TIME_SCALE;
	}
	double time_scale = TIME_SCALE;
	while ((time_scale-=ts_limit) > 0) {
		physics_update_step(rt, ts_limit);
	}
	physics_update_step(rt, time_scale+ts_limit);
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

/* 根据引力影响范围自动获取速度参考系星体
 * (ai生成)
 * 根据潮汐摄动比自动获取速度参考系星体 */
static Star_t *get_about_point(Runtimedata_t *rt)
{
	static Obj_t base_obj = { };
	static Star_t base = {
		.obj = &base_obj,
		.name = "绝对坐标",
	};
	if (!rt || !rt->follow || !rt->objs || rt->obj_count < 2)
		return &base;

	Star_t *objs = rt->objs;
	size_t n = rt->obj_count;
	size_t idx_follow = rt->follow - objs;	// 目标索引

	// 1. 预先计算每个天体受到的总引力加速度（矢量）
	const size_t len = rt->obj_count % 1024;
	Vec_t acc_total[len] = {};

	for (size_t i = 0; i < n; i++) {
		if (!objs[i].obj)
			continue;
		Vec_t acc = { 0.0, 0.0, 0.0 };
		for (size_t k = 0; k < n; k++) {
			if (k == i || !objs[k].obj)
				continue;
			Vec_t diff =
			    vec_sub(objs[k].obj->center, objs[i].obj->center);
			double r2 =
			    (pow2(diff.x) + pow2(diff.y) +
			     pow2(diff.z)) * pow2(SCALE);
			if (r2 < 1e-18)
				continue;
			double r = sqrt(r2);
			double factor = G * objs[k].mass / (r2 * r);	// a = GM/r^3 * r_vec
			acc = vec_add(acc, vec_mul(diff, factor));
		}
		acc_total[i] = acc;
	}

	// 2. 寻找最小摄动比
	double min_ratio = 1e100;
	Star_t *best = &base;

	for (size_t j = 0; j < n; j++) {
		if (j == idx_follow || !objs[j].obj)
			continue;

		// 候选天体 j 对 follow 的引力加速度
		Vec_t diff =
		    vec_sub(objs[j].obj->center, rt->follow->obj->center);
		double r2 =
		    (pow2(diff.x) + pow2(diff.y) + pow2(diff.z)) * pow2(SCALE);
		if (r2 < 1e-18)
			continue;
		double r = sqrt(r2);
		double factor = G * objs[j].mass / (r2 * r);
		Vec_t acc_j_on_target = vec_mul(diff, factor);
		double a_j = vec_len(acc_j_on_target);
		if (a_j < 1e-30)
			continue;

		// 潮汐摄动：目标处其他天体的合力 - 候选天体处其他天体的合力
		Vec_t target_others =
		    vec_sub(acc_total[idx_follow], acc_j_on_target);
		Vec_t candidate_others = acc_total[j];	// 候选天体自身的总加速度（不包含自引力）
		Vec_t tidal = vec_sub(target_others, candidate_others);
		double a_tidal = vec_len(tidal);

		double ratio = a_tidal / a_j;
		if (ratio < min_ratio) {
			min_ratio = ratio;
			best = objs + j;
		}
	}

	// 3. 若最小比值大于 0.5（无显著主宰体），返回绝对坐标
	if (min_ratio > 0.5)
		return &base;
	return best;
}

struct orbital_parameters {
	double a;    /* 半轴长 */
	double e;    /* 偏心率 */
	double rp;    /* 近地点 */
	double ra;    /* 远地点 */
	Vec_t point_rp;
	Vec_t point_ra;
};

static struct orbital_parameters get_orbital_parameters(Star_t *ship, Star_t *center)
{
	struct orbital_parameters dat = {};
	if (!ship || !center || !ship->obj || !center->obj) return dat;

	const Vec_t v = vec_sub(ship->speed, center->speed);
	const Vec_t r = vec_sub(ship->obj->center, center->obj->center);
	const double mu = center->mass*G/SCALE/SCALE/SCALE;
	/* 比机械能 */
	// const double epsilon = vec_point_product(r, r)/2 - mu/distance;
	// const double a = -mu / (2*epsilon);
	/* 偏心率 */
	const Vec_t e = vec_mul(vec_sub(vec_mul(r, vec_point_product(v,v)-mu/vec_len(r)), vec_mul(v, vec_point_product(r, v))), 1/mu);
	/* 半长轴 */
	dat.a = 1 / (2/vec_len(r) - vec_point_product(v, v)/mu);
	dat.e = vec_len(e);
	dat.rp = dat.a*(1-dat.e);
	dat.ra = dat.a*(1+dat.e);
	dat.point_rp = vec_add(vec_mul(vec_direct(e), dat.rp), center->obj->center);
	dat.point_ra = vec_add(vec_mul(vec_direct(e), dat.ra), center->obj->center);
	return dat;
}

static void voyage_helper(Runtimedata_t *rt)
{
	if (!rt) return;
	Star_t *from = rt->follow, *to = NULL;
	while (!from && (from = choose_star(rt, "正在驾驶的", NULL)) == NULL)
		printf("重试...\n");
	while ((to = choose_star(rt, "要驶入的", NULL)) == NULL)
		printf("重试...\n");
	if (from->mass > to->mass)
		printf("[TIPS] from比to重，结果可能不正确\n");
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

	struct orbital_parameters dat = get_orbital_parameters(from, to);
	const char *typ = "椭圆轨道";
	if (dat.e > 1) typ = "双曲线轨道";
	else if (fabs(dat.e - 1) < 1e-5) typ = "抛物线轨道";
	else if (dat.e < 1e-5) typ = "圆轨道";

	printf("====== 当前轨道情况 ======\n");
	printf("轨道类型: %s (%.3f)\n", typ, dat.e);
	printf("近地点: %.1f km | 远地点: %.1f km\n", dat.rp, dat.ra);
	printf("距离近地点: %.1f km\n", vec_len(vec_sub(dat.point_rp, to->obj->center)));
	printf("距离远地点: %.1f km\n", vec_len(vec_sub(dat.point_ra, to->obj->center)));
	printf("（回车返回）\n");
	kbhitGetchar();
	_getch();
	return;
}

static void switch_camera(Runtimedata_t *rt, Camera_t *ca)
{
	if (!rt || !ca) return;
	sync_cam_size_scale(rt);
	ca->width = rt->active_cam->width;
	ca->height = rt->active_cam->height;
	ca->scale = rt->active_cam->scale;
	rt->active_cam = ca;
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
	case '\t': setup(rt); break;
	case '[': TIME_SCALE/=2; break;
	case ']': TIME_SCALE*=2; break;
	case '{': TIME_SCALE=1./FPS; break;
	case '}': TIME_SCALE=(60.*60/FPS); break;
	case 'f':
		rt->follow = choose_star(rt, "跟随", rt->follow);
		if (!rt->follow) {
			switch_camera(rt, rt->camera);
			break;
		}
		switch_camera(rt, &rt->follow->cam);
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
	case 'I': rt->guidline = !rt->guidline; break;
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
	case '_': cam_shift(v_forward, rt->follow?-vec_len(vec_sub(rt->follow->obj->center, rt->active_cam->position)):-5e2*SCALE); break;
        case '+': cam_shift(v_forward, rt->follow?vec_len(vec_sub(rt->follow->obj->center, rt->active_cam->position))/2:5e2*SCALE); break;
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
			.obj = obj_set_color(obj_rotate(obj_shift(obj_create_cube_with_surface(6371*2),
								  (Vec_t){Dx_SE, 0, 0}),
							(Vec_t){1, 1, -1}, M_PI/3.8), (Color_t){29,153,243,-1}),
			.mass = 5.965e24,
			.speed = (Vec_t){0, Vy_SE, 0},
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(24*60*60),
			// 主星（地球）
			// 逃逸速度：
			// 7.9km/s  11.2km/s
		}, (Star_t){
			.name = "地球小卫星",
			.obj = obj_set_color(obj_shift(obj_create_cube(1), (Vec_t){7000+Dx_SE, 0, 0}),
					     (Color_t){-1,30,30,-1}),
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
			.name = "水星",
			.mass = 3.301e23,
			.obj = obj_shift(obj_create_cube(2439.7*2), (Vec_t){57.91e6, 0, 0}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 47.87),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(1407.6*60*60),
		}, (Star_t){
			.name = "金星",
			.mass = 4.867e24,
			.obj = obj_shift(obj_create_cube(6051.8*2), (Vec_t){108.21e6, 0, 0}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 35.02),
			.self_rotate = (Vec_t){0, 0, -1},
			.self_omiga = 2*M_PI/(5832.6*60*60),
		}, (Star_t){
			.name = "火星",
			.mass = 6.417e23,
			.obj = obj_shift(obj_create_cube(3389.5*2), (Vec_t){227.94e6, 0, 0}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 24.07),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(24.6*60*60),
		}, (Star_t){
			.name = "木星",
			.mass = 1.898e27,
			.obj = obj_shift(obj_create_cube(69911*2), (Vec_t){778.57e6, 0, 0}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 13.07),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(9.93*60*60),
		}, (Star_t){
			.name = "土星",
			.mass = 5.683e26,
			.obj = obj_shift(obj_create_cube(58232*2), (Vec_t){1433.53e6, 0, 0}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 9.69),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(10.66*60*60),
		}, (Star_t){
			.name = "天王星",
			.mass = 8.681e25,
			.obj = obj_shift(obj_create_cube(25362*2), (Vec_t){2872.46e6, 0, 0}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 6.81),
			.self_rotate = (Vec_t){0, 0, -1},
			.self_omiga = 2*M_PI/(17.24*60*60),
		}, (Star_t){
			.name = "海王星",
			.mass = 1.024e26,
			.obj = obj_shift(obj_create_cube(24622*2), (Vec_t){4495.06e6, 0, 0}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 5.43),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(16.11*60*60),
		}, (Star_t){
			.name = "太阳",
			.obj = obj_set_color(obj_shift(obj_create_cube(695700*2), (Vec_t){0, 384400, 0}),
					     (Color_t){-1,-1,0,-1}),
			.mass = 1.989e30,
			.speed = vec_mul(vec_direct((Vec_t){0, 0, 0}), 1.022),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(25.4*60*60),
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
	rt.camera->position = (Vec_t){0, 0, 1e6*SCALE};
	rt.camera->dept = 1e8*SCALE;

	printf("按键说明：\nwasd 控制偏转\n"
	       "bB 减速 空格或N加速\n"
	       "WASD 控制镜头平移 -=_+ 控制镜头远近\n"
	       "hjkl 控制镜头方向 JK 控制镜头旋转\n"
	       "7/8 控制焦距 9/0 控制可视距离\n"
	       "f 跟随  F 看向某物体  t 测距\n"
	       "[]{} 控制时间流速\n"
	       "? 进行数学辅助计算\n"
	       "f跟随时视角会调整方向为绝对速度\n"
	       "F选择看向自身视角会追踪该速度方向\n"
	       "使用t进行测距视角可以追踪相对速度方向\n");
	rt.inp = 'f';
	input_handle(&rt);

	printf("\e[2J");
	size_t i = 0;
	Star_t *about_point;
	for (i = 0; i < MAX_FRAME; ++i) {
		if ((rt.inp = kbhitGetchar()))
			if (!input_handle(&rt)) break;
		if (!rt.pause) rt.gtime += physics_update(&rt);
		about_point = get_about_point(&rt);
		if (rt.look_to) {
			Vec_t direct = rt.look_to == rt.follow ? \
				       vec_sub(rt.follow->speed, about_point->speed) : \
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

			Point_t p1, p2;
			camera_cast_line(rt.active_cam,
					 rt.follow->obj->center,
					 vec_add(rt.follow->obj->center,
						 vec_sub(rt.follow->speed, about_point->speed)),
					 &p1, &p2);
			backend_draw_line(rt.backend, rt.active_cam, p1, p2,
					  (Color_t){-1,-1,0,-1},
					  (Color_t){-1,-1,0,-1});
		}
		for (size_t i = 0; i < countof(objs); i++) {
			if (!objs[i].obj) continue;
			obj_cast(objs[i].obj, rt.active_cam, rt.backend);
		}
		if (rt.guidline && rt.follow && rt.destination_to) {
			Point_t p1, p2;
			camera_cast_line(rt.active_cam,
					 rt.follow->obj->center,
					 rt.destination_to->obj->center,
					 &p1, &p2);
			backend_draw_line(rt.backend, rt.active_cam, p1, p2,
					  (Color_t){0,-1,-1,-1},
					  (Color_t){0,-1,-1,-1});
		}
		printf("\e[H");
		rt.backend->render(rt.backend);
		rt.backend->clean(rt.backend);
		printf("\e[0m\e[2K\r[TS:%g, GT:%.1fd, E:%gJ C:%gkm]",
		       TIME_SCALE*FPS, rt.gtime/(24.*60*60),
		       rt.fuel_consumption,
		       rt.follow?vec_len(vec_sub(rt.active_cam->position,
						 rt.follow->obj->center)):0);
		if (rt.follow) {
			printf(" | %s[%s] (%.3f km/s)",
			       rt.follow->name ? rt.follow->name : "Unknow",
			       about_point->name ? about_point->name : "Unknow",
			       vec_len(vec_sub(rt.follow->speed, about_point->speed)));
		}
		if (rt.follow && rt.destination_to) {
			const Vec_t dist = vec_sub(rt.destination_to->obj->center, rt.follow->obj->center);
			const Vec_t dv = vec_sub(rt.follow->speed, rt.destination_to->speed);
			// 速度 <0 表靠近， >0 表远离
			printf(" 距%s %.1f km (%.3f km/s)",
			       rt.destination_to->name ? rt.destination_to->name : "Unknow",
			       vec_len(dist),
			       -vec_point_product(vec_direct(dist), dv));
			if (about_point == rt.destination_to) {
				struct orbital_parameters ret = get_orbital_parameters(rt.follow, about_point);
				printf(" Rp:%.1fkm Ra:%.1fkm", ret.rp, ret.ra);
			}
		}
		sleep_fixed_step(1./FPS);
	}

	cleanup(&rt);
	return EXIT_SUCCESS;
}

