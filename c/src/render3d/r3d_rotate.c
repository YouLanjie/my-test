/**
 * @file        r3d_rotate.c
 * @author      Chglish
 * @date        2026-07-12
 * @brief       一个简单的3d N体运动模拟程序，
 * 可任意选择天体跟踪操纵，内置仿真简易太阳系
 */

#include "lib/render3d.h"
#include "../../include/tools.h"
#include "../../include/string_view.h"
#include "../../include/dynamic_array.h"
#include <time.h>

/* 引力常量 N*(m^2)/(kg^2) || (m^3)/(kg*s^2) */
const double G = 6.6743e-11;
const double SCALE = 1e3;    /* 将距离换算成 1单位 = 1km */
#define pow2(x) ((x)*(x))
#define syslog(rt, fmt, ...) sva_sprintfcat(&(rt)->logs, "[T+%8.3fd] "fmt"\n", (rt)->gtime/(24.*60*60) __VA_OPT__(,) __VA_ARGS__)

typedef struct {
	const char *name;
	Obj_t *obj;
	double radius;
	double mass;          /* 质量(kg) */
	double self_omiga;    /* 自转速度(rad/s) */
	Vec_t self_rotate;    /* 自转方向 */
	Vec_t speed;          /* 速度(km/s) */
	Camera_t cam;         /* 随身相机 */
} Star_t;

typedef struct {
	RenderBackend_t *backend;
	Camera_t *camera;
	Camera_t *active_cam;
	SVA_t logs;    /* 日志文本 */
	DA_t objs;
	Star_t *destination_to;
	Star_t *look_to;
	Star_t *follow;
	Star_t *about_point;
	Obj_t  *axis_helper;
	double dv;
	double gtime;
	double time_scale;
	double time_scale_limit;
	int  inp;
	uint8_t throttle;    /* 1% = 0.1m/s^2 */
	int8_t throttle_on;    /* <<0位表开关，<<1位表反方向推力 */
	uint8_t fps;
	bool rotate_cam_with_spd;
	bool axis;
	bool guidline;
	bool pause;
	bool print_busy;
} Runtimedata_t;

/* 分页器 */
static void print_pager(const char *headline, SV_t content, int mode)
{
	if (!content.len||!content.p) return;
	SV_t line = {};
	SV_t left = content;
	headline = headline?headline:"请输入文本";
	printf("\e[0m\n\e[2K==== %s ====\n", headline);
	const int pager_lines = 25;
	int total_lines = sv_countlines(content);
	int count = 0, start = 1;
	while (sv_forline(&line, &left)) {
		if (mode!=-1 || total_lines-count < pager_lines)
			printf("> %.*s\n", (int)line.len, line.p);
		count++;
		if ((mode<0||count%pager_lines != 0) && left.len != 0) continue;
		if (left.len == 0) printf("\e[32m-- 内容结束\e[0m\n");
		printf("\e[2m-- [分页器] %d-%d/%d 回车继续,c不翻页打印,p/u上翻,q退出\e[0m\n",
		       start, count, total_lines);
		kbhitGetchar();
		int ch = _getch();
		if (ch == 'q') break;
		if (ch == 'c') mode=-2;
		else if (ch == 'p' || ch == 'u') {
			mode = 0;
			count -= (count-1)%pager_lines+1+pager_lines;
			left = sv_merge(content, sv_seekline(content, content, count), left);
			if (count <= 0) {
				left = content;
				count = 0;
				printf("\e[31m-- 已经到顶咯\e[0m\n==== %s ====\n", headline);
			}
		}
		start = count+1;
	}
}

static Star_t star_create(char *name, double mass, double radius, Vec_t position, Vec_t speed, Star_t *about_point)
{
	Point_t center = about_point&&about_point->obj ? about_point->obj->center : (Vec_t){};
	Star_t star = {
		.obj = obj_shift(obj_create_cube/*_with_surface*/(2*radius), vec_add(center, position)),
		.speed = vec_add(about_point ? about_point->speed : (Vec_t){}, speed),
		.mass = mass > 1e-4 ? mass : 1e-4,    /* 负质量是非法的！ */
		.radius = radius,
		.name = name ? name : "UNKNOW",
	};
	return star;
}

static void star_free(void *p)
{
	Star_t *star = p;
	if (!star) return;
	if (!star->obj) return;
	obj_free(star->obj);
	star->obj = NULL;
}

static void star_pop(Runtimedata_t *rt, Star_t *star, const char *desc)
{
	if (!rt || !star) return;
	Star_t *stars = rt->objs.ptr;
	size_t idx = star - stars;
	if (idx >= rt->objs.len) return;
	syslog(rt, "天体'%s'掉出了这个世界，凶手是'%s'",
	       star->name ? star->name : "未知天体",
	       desc ? desc : "虚空");
	/* 修正各指针 */
	Star_t **objs[] = {&rt->follow, &rt->look_to, &rt->destination_to};
	size_t offsets[countof(objs)] = {};
	for (size_t i = 0; i < countof(objs); i++) {
		if (*objs[i] == star) offsets[i] = -1;
		else offsets[i] = *objs[i] ? *objs[i] - stars : -1;
	}
	da_pop(&rt->objs, idx, star_free);
	for (size_t i = 0; i < countof(objs); i++) {
		*objs[i] = da_get(&rt->objs, offsets[i]);
	}
	rt->active_cam = rt->follow ? &rt->follow->cam : rt->camera;
	rt->pause = true;
	print_pager("发生事件", sv_from_sva(&rt->logs), -1);
}

static void cleanup(Runtimedata_t *rt)
{
	printf("\e[0m\n");
	if (!rt) return;
	if (rt->backend) rt->backend->destroy(rt->backend);
	if (rt->camera) camera_free(rt->camera);
	if (rt->axis_helper) obj_free(rt->axis_helper);
	rt->backend = NULL;
	rt->camera  = NULL;
	sva_free(&rt->logs);
	da_free(&rt->objs, star_free);
}

static void sync_cam_size_scale(Runtimedata_t *rt)
{
	if (!rt || !rt->active_cam) return;
	int term_w = get_winsize_col() - 0;
	int term_h = get_winsize_row() - 2;
	if (rt->backend->get_size) {
		rt->backend->get_size(rt->backend, &term_w, &term_h);
	} else term_h *= 2;
	rt->active_cam->width = term_w;
	rt->active_cam->height = term_h;
	rt->active_cam->scale = fmax(term_w, term_h) / 2;
}

static bool setup(Runtimedata_t *rt, int mode)
{
	if (!rt) return false;
	int term_w = get_winsize_col() - 0;
	int term_h = get_winsize_row() - 2;
	if (rt->backend) {
		/* 轮换后端 */
#define BACKEND(name) backend_create_##name,
		static RenderBackend_t *(*backend_list[])(int width, int height) = {BACKEND_LIST};
#undef BACKEND
		enum Backend_id id = rt->backend->id;
		if (mode) id = (id+1) % countof(backend_list);
		rt->backend->destroy(rt->backend);
		while (!(rt->backend = backend_list[id%countof(backend_list)](term_w, term_h)))
			id++;
		sync_cam_size_scale(rt);
		return true;
	}
	rt->backend = backend_create_utf8_256bit(term_w, term_h);
	rt->camera = camera_create();
#define CREATE_LINE(x,y,z, r,g,b) obj_set_color(obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){x,y,z})), (Color_t){r,g,b,100})
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
	rt->objs.size = sizeof(Star_t);
	/* 设置帧率、运行倍率 */
	rt->fps = 40;
	rt->time_scale_limit = 1024;
	rt->time_scale = 1;
	return true;
}

/* 根据引力影响范围自动获取速度参考系星体
 * (ai生成)
 * 根据潮汐摄动比自动获取速度参考系星体 */
static Star_t *get_about_point(Runtimedata_t *rt, Star_t *follow)
{
	static Obj_t base_obj = { };
	static Star_t base = {
		.obj = &base_obj,
		.name = "绝对坐标",
	};
	Star_t *objs = rt->objs.ptr;
	if (!rt || !objs || rt->objs.len < 2)
		return &base;
	if (!follow) follow = rt->follow;
	if (!follow || (size_t)(follow-objs) > rt->objs.len)
		return &base;

	size_t n = rt->objs.len;
	size_t idx_follow = follow - objs;	// 目标索引

	// 1. 预先计算每个天体受到的总引力加速度（矢量）
	const size_t len = rt->objs.len % 1024;
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
		if (objs+j == follow || !objs[j].obj)
			continue;

		// 候选天体 j 对 follow 的引力加速度
		Vec_t diff =
		    vec_sub(objs[j].obj->center, follow->obj->center);
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

static void physics_update_step(Runtimedata_t *rt, double time_scale)
{
	if (!rt || rt->objs.len == 0 || time_scale == 0) return;
	time_scale /= rt->fps;
	Star_t *objs = rt->objs.ptr;
	Star_t *crash[2] = {NULL};
	const size_t len = rt->objs.len % 1024;
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
			if (r2 < pow(objs[i].radius + objs[j].radius, 2) * pow2(SCALE)) {
				crash[0] = objs+i;
				crash[1] = objs+j;
			}
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
	if (crash[0] && crash[1] && crash[0] != crash[1]) {
		if (crash[0]->mass <= crash[1]->mass) {
			objs = crash[1];
			crash[1] = crash[0];
			crash[0] = objs;
		}
		crash[0]->mass += crash[1]->mass;
#define star_impact_xyz(xyz) (crash[0]->mass*crash[0]->speed.xyz + crash[1]->mass*crash[1]->speed.xyz)/(crash[0]->mass+crash[1]->mass)
		crash[0]->speed = (Vec_t){
			.x = star_impact_xyz(x),
			.y = star_impact_xyz(y),
			.z = star_impact_xyz(z),
		};
#undef star_impact_xyz
		SVA_t buf = {};
		sva_sprintf(&buf, "来自`%s`(+%gkg)大地的爱",
			    crash[0]->name?crash[0]->name:"未知天体",
			    crash[1]->mass);
		star_pop(rt, crash[1], buf.p);
		sva_free(&buf);
	}
	if (rt->throttle_on&1 && rt->throttle && rt->follow) {
		double accel = rt->throttle * 0.1 / SCALE * time_scale * (rt->throttle_on&0b10?-1:1);
		rt->follow->speed = vec_add(rt->follow->speed, vec_mul(vec_direct(rt->active_cam->forward), accel));
		rt->dv += fabs(accel);
	}
}

static double physics_update(Runtimedata_t *rt)
{
	if (!rt || rt->objs.len == 0) return 0;
	Vec_t v1 = rt->follow&&rt->rotate_cam_with_spd ? rt->follow->speed : (Vec_t){};
	double time_scale = rt->time_scale;
	while ((time_scale-=rt->time_scale_limit) > 0) {
		physics_update_step(rt, rt->time_scale_limit);
	}
	physics_update_step(rt, time_scale+rt->time_scale_limit);
	if (rt->follow) rt->about_point = get_about_point(rt, rt->follow);
	if (rt->follow && rt->rotate_cam_with_spd) {
		Vec_t v2 = rt->follow->speed;
		if (rt->about_point) {
			v1 = vec_sub(v1, rt->about_point->speed);
			v2 = vec_sub(v2, rt->about_point->speed);
		}
		v1 = vec_direct(v1);
		v2 = vec_direct(v2);
		if (vec_len(vec_cross_product(v1, v2)) > 1e-5) {
			camera_rotate_about_point(rt->active_cam,
						  rt->follow->obj->center,
						  vec_cross_product(v1, v2),
						  acos(vec_point_product(v1, v2)));
		}
	}
	return rt->time_scale/rt->fps;
}

static Star_t *choose_star(Runtimedata_t *rt, const char *hint, Star_t *old)
{
	if (!rt) return NULL;
	printf("\e[0m\n可选天体：\n [0] 空选择\n");
	int choice = 0;
	Star_t *objs = rt->objs.ptr;
	for (size_t i = 0; i < rt->objs.len; i++) {
		printf(" [%lu] %s (%g kg/%g km)%s\n", i+1,
		       objs[i].name ? objs[i].name : "{未命名星体}",
		       objs[i].mass, objs[i].radius, !objs[i].obj?"(不可用)":"");
		if (objs+i == old) choice = i;
	}
	printf("(当前：%d)请输入要%s物体的id[0~%lu]：",
	       choice + 1, hint ? hint : "选择", rt->objs.len);
	if (scanf("%d", &choice) == 0) {
		kbhitGetchar();
		printf("输入错误，未作任何更改(回车返回)\n");
		_getch();
		return NULL;
	}
	choice--;
	if (choice == -1) return NULL;
	objs = da_get(&rt->objs, choice);
	if (choice < 0 || (size_t)choice >= rt->objs.len || !objs || !objs->obj) {
		printf("选择非法（回车返回）\n");
		kbhitGetchar();
		_getch();
		return NULL;
	}
	return objs;
}

struct orbital_parameters {
	char *typ;    /* 轨道类型字符串 */
	double a;    /* 半轴长 */
	double e;    /* 偏心率 */
	double rp;    /* 近地点 */
	double ra;    /* 远地点 */
	double r;    /* 当前半径 */
	double d_rp;    /* 距近地点 */
	double d_ra;    /* 距远地点 */
	double T;    /* 周期(秒) */
	Vec_t point_rp;
	Vec_t point_ra;
	Vec_t u;    /* 轨道平面法向量(r * v) */
};

static struct orbital_parameters get_orbital_parameters(Star_t *ship, Star_t *center)
{
	struct orbital_parameters dat = {.typ="NULL"};
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
	dat.r = vec_len(r);
	dat.a = 1 / (2/dat.r - vec_point_product(v, v)/mu);
	dat.e = vec_len(e);
	dat.rp = dat.a*(1-dat.e);
	dat.ra = dat.a*(1+dat.e);
	dat.point_rp = vec_add(vec_mul(vec_direct(e), dat.rp), center->obj->center);
	dat.point_ra = vec_add(vec_mul(vec_direct(e), -dat.ra), center->obj->center);
	dat.d_rp = vec_len(vec_sub(dat.point_rp, ship->obj->center));
	dat.d_ra = vec_len(vec_sub(dat.point_ra, ship->obj->center));
	dat.u = vec_direct(vec_cross_product(r, v));
	dat.T = 2*M_PI*(dat.a*SCALE)*sqrt(dat.a*SCALE/(G*(ship->mass+center->mass)));

	char *typ = "椭圆轨道";
	if (dat.e > 1) typ = "双曲线轨道";
	else if (fabs(dat.e - 1) < 1e-5) typ = "抛物线轨道";
	else if (dat.e < 1e-5) typ = "圆轨道";
	dat.typ = typ;
	return dat;
}

static void format_orbital_parameters(Runtimedata_t *rt, SVA_t *dest, struct orbital_parameters ret)
{
	if (!rt || !rt->follow || !dest) return;
	const Vec_t dv = vec_sub(rt->follow->speed, rt->about_point?rt->about_point->speed:(Point_t){});
	const Point_t center = rt->about_point&&rt->about_point->obj?rt->about_point->obj->center:(Point_t){};
	sva_sprintf(dest, "e=%.3g,a=%.3gkm,θ=%.3g,r=%.3gkm,v=%.3gkm/s,⟂v=%.3gkm/s",
		    ret.e, ret.a, acos(vec_point_product(ret.u, (Vec_t){0,0,1}))/M_PI*180., ret.r,
		    vec_len(dv),
		    -vec_point_product(vec_direct(vec_sub(center, rt->follow->obj->center)), dv)
		    );
	return;
}

struct hohmann_orbital_parameters {
	double theta;    /* 剩余角度，为0时最佳(rad) */
	double expect_theta;    /* 期望角度提前量(rad) */
	double T;    /* 半周期 */
	double v;    /* 期望速度km/s */
};

/* 计算距离霍曼转移点火点提前量角度 */
static struct hohmann_orbital_parameters get_hohmann_orbit_theta(Star_t *follow, Star_t *destination_to, Star_t *about_point)
{
	if (!follow || !follow->obj || !destination_to || !destination_to->obj
	    || !about_point || !about_point->obj) return (struct hohmann_orbital_parameters){};
	const Vec_t r2 = vec_sub(destination_to->obj->center, about_point->obj->center);
	const Vec_t r1 = vec_sub(follow->obj->center, about_point->obj->center);
	const double r1f = vec_len(r1);
	const double r2f = vec_len(r2);
	const double a = (r1f + r2f) / 2;
	const double T = 2*M_PI*(a*SCALE)*sqrt(a*SCALE/(G*(follow->mass+about_point->mass)));
	const double mu = G*about_point->mass/SCALE/SCALE/SCALE;
	const double omiga = M_PI - (sqrt(mu/r2f) / r2f)*T/2;
	/* 使用了跟踪天体平面 */
	const double rtheta = vec_angle2d(r1, r2, vec_sub(follow->speed, about_point->speed));
	const double theta = fmod(rtheta - fmod(omiga, 2*M_PI) + M_PI, 2*M_PI);
	const struct hohmann_orbital_parameters ret = {
		.expect_theta = omiga,
		.theta = (theta<0?theta+2*M_PI:theta)-M_PI,
		.T = T/2,
		.v = sqrt(2*mu*r2f/r1f/(r1f+r2f)),
	};
	return ret;
}

static void print_starinfo(Star_t *star, struct orbital_parameters dat)
{
	if (!star || !star->obj) return;
	printf("围绕天体: %s\n", star->name);
	printf("轨道类型: %s (%.3f) \t| 周期: %.1f d\n", dat.typ, dat.e, dat.T/(24*60*60));
	printf("当前高度: %.1f km \t| 半长轴: %.1f km\n", dat.r, dat.a);
	printf("近地点: %.1f km \t| 远地点: %.1f km\n", dat.rp, dat.ra);
	printf("距离近地点: %.1f km\n", dat.d_rp);
	printf("距离远地点: %.1f km\n", dat.d_ra);
	printf("倾角: %.3f deg\n", acos(vec_point_product(dat.u, (Vec_t){0,0,1}))/(2*M_PI)*360.);
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
	if (from == to)
		printf("[WARN] 怎么选了个一样的？\n");
	const Vec_t direct = vec_sub(to->obj->center, from->obj->center);
	double distance = vec_len(direct);
	if (distance <= 0) distance = 1e-20;
	const double speed = sqrt(G*to->mass/(distance*SCALE)) / SCALE;
	printf("========== 基础信息 ==========\n");
	printf("'%s' -> '%s'\n",
	       from->name ? from->name : "Unknow",
	       to->name ? to->name : "Unknow");
	printf("距离：%.1f km\n", distance);
	printf("航向：{%.1f, %.1f, %.1f}\n", direct.x, direct.y, direct.z);
	printf("理想圆轨线速度：%g km/s, 角速度：%g rad/s\n", speed, speed/distance);
	printf("理想圆轨周期：%.1f s | %.1f d\n", 2*M_PI/(speed/distance), 2*M_PI/(speed/distance)/(24*60*60));

	Star_t *s1 = get_about_point(rt, from);
	if (!s1) s1 = to;
	struct orbital_parameters dat = get_orbital_parameters(from, to);
	if (s1 != to) {
		printf("====== 假设(%s -> %s)轨道情况 ======\n", from->name, to->name);
		print_starinfo(to, dat);
		dat = get_orbital_parameters(from, s1);
	}
	printf("====== 当前(%s)轨道情况 ======\n", from->name);
	print_starinfo(s1, dat);

	Star_t *s2 = get_about_point(rt, to);
	if (s1 == s2 && s1 != to && s2 != from) {
		struct orbital_parameters dat2 = get_orbital_parameters(to, s2);
		printf("====== 目标(%s)共轨情况 ======\n", to->name);
		print_starinfo(s2, dat2);
		printf("相对倾角: %.4f deg\n", acos(vec_point_product(dat2.u, dat.u))/M_PI*180.);
		struct hohmann_orbital_parameters ret = get_hohmann_orbit_theta(from, to, s2);
		printf("====== 霍曼转移轨道数据 ======\n");
		printf("半周期: %.3g d\n", ret.T/(24*60*60));
		printf("提前角度: %.3g deg\n", ret.expect_theta/M_PI*180.);
		printf("距点火点: %.3g deg\n", ret.theta/M_PI*180.);
		printf("期望速度: %.3g km/s\n", ret.v);
	}

	printf("（回车返回）\n");
	kbhitGetchar();
	_getch();
	return;
}

static void dump_stars(Runtimedata_t *rt)
{
	if (!rt || !rt->objs.ptr) return;
	Star_t *objs = rt->objs.ptr;
	printf("\e[0m\n\e[2K===== 数据导出：各星体基本参数 =====\n");
	for (size_t i = 0; i < rt->objs.len; i++) {
		if (!objs[i].obj) continue;
		printf(" [%lu] %s (%gkg/r=%gkm) 位置(km): {%.3f,%.3f,%.3f} 速度(km/s): {%.3f,%.3f,%.3f}\n", i+1,
		       objs[i].name ? objs[i].name : "{未命名星体}",
		       objs[i].mass, objs[i].radius,
		       objs[i].obj->center.x,
		       objs[i].obj->center.y,
		       objs[i].obj->center.z,
		       objs[i].speed.x,
		       objs[i].speed.y,
		       objs[i].speed.z);
	}
	printf("游戏时间: T+%.1f s, 折合约 T+%.1f d\n", rt->gtime, rt->gtime/(24*60*60));
	printf("操作累计dv: %.3f km/s\n", rt->dv);
	printf("（回车返回）\n");
	kbhitGetchar();
	_getch();
}

static void print_qrh()
{
	(void)R"(
	(()"; // "  /* 由于vim语法高亮匹配问题，需要这个东西修正括号匹配 */
	const char *content = R"(（以下内容由AI生成，人类监制修正）
================================================================================
                   【 轨道飞行快速参考手册 QRH v1.0 】
================================================================================
索引 | 1. 全键位速查  2. UI信息解释与使用  3. 变轨黄金律  4. 霍曼转移自动化流程
================================================================================

[1] 全键位速查 (CAM/ENG/NAV/SYS)
--------------------------------------------------------------------------------
【视角控制 CAM】
 平移  : W(上) S(下) A(左) D(右)
 旋转  : h(左转) l(右转) j(下转) k(上转)   ← 1° 小幅度转动视线
 滚转  : H(左滚45°) L(右滚45°)             ← 大角度歪头（改变画面上方向）
         J(左滚1°)  K(右滚1°)              ← 小角度歪头（微调水平基准）
 大旋  : < (左摇头45°) > (右摇头45°)       ← 快速环顾四周
 推拉  : - (后退1) = (前进1)               ← 线性移动视点(天文尺度下不常用)
         + (瞬移至半距视角)                ← 使相机与目标距离减半（放大）
         _ (瞬移至倍距视角)                ← 使相机与目标距离翻倍（缩小）
 焦距  : 7 (缩小视场角) 8 (放大视场角)     ← 改变透视畸变(一般不建议调整)
 深度  : 9 (减半可视距离) 0 (加倍可视距离)

【引擎控制 ENG】 (须先按 f 锁定跟随天体)
 油门  : z(+1%) x(-1%)  Z(满油门/255)  X(急停/0)
 点火  : 空格(开关)  r(切换反向推力)
 手动机动 (沿视角方向，不常用，不推荐用) :
         n/N (加速 1x/10x)  b/B (减速 1x/10x)
 手动机动 (沿上下左右方向，用于微小径向/法向修正，不常用) :
         w(上) s(下) a(左) d(右)

【目标与导航 NAV】
 f : 选择驾驶/跟随天体 (自动切换视角并锁定)
 t : 选择最终目标天体 (**核心导航键**：设定后自动显示 Rp/Ra 及霍曼数据)
 F : 选择看向天体 (仅锁定视线方向，不参与导航计算。若看向自身则看向自身速度方向)
 T : 切换开关“视角随相对速度方向自动旋转”    ← 用于轨道面内自动对向飞行
 ? : 打开轨道计算辅助面板，计算查看更详细的轨道参数

【系统与调试 SYS】
 时间  : [ (减半) ] (加倍)  { (重置1倍) } (设定为3600倍)
 暂停  : p (切换)  . (单帧步进，一般不常用)
 后端  : Tab (轮换渲染后端)  ` (相机匹配屏幕尺寸)  ~ (重载当前渲染后端)
 显示  : i (坐标轴开/关)  I (辅助参考线开/关)
 日志  : " (分页查看航行日志)  | (导出所有天体原始数据)
 手册  : M (打开本手册)
 调试  : ' (帧率/负载显示开/关)
 清屏  : c (强制刷新画面) 
 退出  : q/Q

================================================================================
[2] UI界面信息解释与使用
--------------------------------------------------------------------------------
【三条核心线的颜色含义】 (按 I 开启)
 黄线 (相对速度矢量) : 指向当前天体相对于参考系中心的速度方向。
 青线 (目标指向)     : 指向你按 t 设定的目标天体。
 灰线 (引力中心)     : 指向当前捕获你的参考系中心天体。

【主状态栏/HUD的解释】
-------------------------------
[T+0.1d, x1, :80%:, dv:0km/s] | 地球小卫星[地球] (5.729 km/s) Rp:11934.2km Ra:12066.5km
距月球 389861.1 km (-3.976 km/s) 33.5°(-29.6)°/8.5° L:283.81s [已暂停] (  2.1%/40fps)
-------------------------------
 - T+0.1d : 当前游戏持续时间
 - x1     : 当前游戏倍速，超过一定数值后会使用科学记数法
 - :80%:  : 油门显示，`:255%:`表示设定255油门但未启动，`[255%]`表示最大推力推进中
            若显示为`:-10%:`或`[-10%]`则表示推力与视线方向相反(使用r反转)
 - dv:0km/s     : 表示当前累计手动变速速度变化量，衡量机动高效程度
 - 地球小卫星   : 当前跟踪/操纵天体
 - [地球]       : 当前天体环绕天体，若变为`{地球}`则表示启用了T跟随速度方向旋转视角
 - (5.729 km/s) : 相对中心天体速度大小
 - Rp:11934.2km : 近地点，仅在设置了`t`目标后显示
 - Ra:12066.5km : 远地点，显示同近地点，油门启动时与近地点交换高度时会自动暂停
 - 距月球       : 当前设定的目标(该行的内容若无说明，均需要设置目标才显示)
 - 389861.1 km  : 相对目标的直线距离
 - (-3.976 km/s): 相对目标垂直速度，负数表示接近正数表示远离
 - 33.5°(-29.6)°/8.5° : 见下小节
 - L:283.81s    : 霍曼转移剩余加速时间 (仅在 |C|<10° 时显示，油门为0时示数为0)
 - [已暂停]     : 暂停状态，若没有设置t则会在第一行显示
 - (  2.1%/40fps) : 负载状况，用'打开查看，若超过100%表示超载，帧率会降低(暂不支持跳帧)

【霍曼转移角度参数诊断 (状态栏第二行中右端) 】 (仅当 t 目标与飞船共心时显示)
 显示格式 : `A(B)°/C°`
   A = 当前轨道面与目标天体轨道面的 绝对倾角 (越小越共面)。
   B = 速度方向与两轨道面交线方向的 夹角 (取值范围 0°~90°)。
   C = 距霍曼转移最佳点火点的 剩余角度 (0° 即为最佳时机)。

【倾角修正的几何原理与视角对齐法】
 改变倾角的最小 Δv 方向，是“当前速度向量”与“目标轨道面内速度向量”的向量差方向，
 机动前后的速度大小（速率）应当尽量保持不变。
 > 形象地说：若两速度向量等长，Δv 方向即等腰三角形的**底边方向**。
 
 ① 视角对齐轨道面（使用 hjkl）：
    利用 `h/j/k/l` 旋转视角，直到**屏幕边缘大致平行于灰线（引力中心）与青线（目标方向）所张成的平面**。
    （直观判断：让青灰线看起来处于同一条直线（水平或竖直最易判断），而不张开一定角度。）
    技巧：
    - 先转动视角尽可能让两条线对称分布在屏幕两侧
    - 再使用hj调整角度让两条线角度逐渐减小直至近似为一条直线即可

 ② 歪头构造等腰三角形（使用 HJKL 滚转）：
    使用 `H/L` 或 `J/K` 进行滚转（歪头），仔细调整，使得屏幕上的黄线与
    青线或灰线**在屏幕上恰好形成一个锐角等腰三角形**。
    （或者让"黄线"与"和黄线构成锐角的另一条线"在屏幕上对称分布在屏幕两侧）

 ③ 90°转向锁定最优推力方向（使用 `>` 或 `<`）：
    保持当前视角不变，**连续按两次 `>`（或两次 `<`）**。
    具体选择方向取决于黄线方位，黄线在左就按`<<`，在右就按`>>`
    该操作将视线在水平面内旋转 90°，此时：
    > 此时你的视线方向就已经精确指向了最小 Δv 的推力方向
    如果黄线与另一条线几乎重叠分不出来方向，可任选一边旋转后进入下一步试错

 ④ 启动推进与趋势验证：
    视线方向即推力方向。设定好推力，启动油门开始修正。
    如果不确定推力方向，可以：
    1. 先暂停，使用?观察相对倾角，记录数值
    2. 快速取消又恢复暂停，再用?观察，相对倾角减小了表明方向对了，否则使用r逆转推力方向
    修正时观察状态栏(`A(B)°/C°`)：
    - B值 增大 → 倾角 A 减小 → 方向正确，继续推进！
    - B值 减小 → 倾角 A 增大 → 方向反了，立即按 r 切换推力方向。
    当 B 值持续增大并趋近 90° 时，表明已将倾角压至本次修正的极限最小值。
    ⚠️ 超过 90° 继续推进会导致倾角反弹增大，必须在 B ≈ 85°~90° 时果断关火。
    需要说明越接近修正极限B的变化速度就越快，届时需要逐步降低推力调整
    > 若修正极限值仍旧过大（>1°），可环绕等待B降低为0然后重复本程序

【安全保护机制 (自动化)】
 ① 高倍速接近预警：当时间倍速 > 2x 且预测即将飞越目标天体时，系统会**自动暂停游戏**，防止错过捕获窗口。
 ② 变轨自动化停止：在霍曼转移加速中，若倍速 > 1 且预测 1 秒后速度将超过目标速度，系统会**自动暂停并降低倍速**至 1x，便于精确控制。

================================================================================
[3] 变轨黄金法则与捕获时机 (基于 Rp/Ra 及径向速度)
--------------------------------------------------------------------------------
法则 A (改变轨道形状，效率最高点) :
   * 在 远地点 (Ra) 加速 → 抬升 近地点 (Rp)  (改变轨道形状最省燃料)。
   * 在 近地点 (Rp) 减速 → 降低 远地点 (Ra)  (捕获入轨最省燃料)。

法则 B (坠入判据与捕获窗口) :
   * 坠入双重判据：只有在 **Ra 显示为负数** 且 **径向速度（接近速度）为负数** 时，
     才表示天体正在真正坠向中心。请用 `?` 查阅目标天体半径，确保 Rp 大于该半径。
   * 捕获最佳窗口：等待**径向速度大小达到最大值后开始减小并趋近 0 时**
     (即飞船即将到达近地点)，此时按法则 A 在近地点进行减速制动，捕获效率最高。
   * 注意：Rp 本身不会为负，若观测到 Rp 跨越 0 值，表示轨道方向发生了反向，
     此时在 `?` 面板中倾角显示会从 <90° 跳变至 >90°。

法则 C (微小径向调整) :
   * 若在到达目标天体近地点前需要微量调整近地点高度（确保不撞入目标天体），
     应使用 `wasd` 进行小幅度径向速度修正，而不是动用主引擎大油门。
     目标天体半径可在使用?选择天体时查看。若近地点处于安全高度，则建议接近
     近地点后再根据法则 AB 进行变轨机动入轨。

================================================================================
[4] 霍曼转移标准化执行检查单 (含自动化特性说明)
--------------------------------------------------------------------------------
前置条件 : 已按 f 锁定飞船，按 t 设定最终目标天体。
           **注意**：只需设 t，Rp/Ra 及霍曼数据即自动显示，无需按 F。
 
 [ ] 第一步 检查倾角 (共面性) ：
           观察 A(B)°/C°。若 A 值较大（>5°），应先按第2节方法修正倾角。
           非共面轨道盲目转移将直接导致错过目标。
 
 [ ] 第二步 观察 C 值与自动化提示：
           状态栏显示 `A(B)°/C°` 即代表共心。当 C 值趋近 0° 时准备点火。
           若当前油门 > 0 且 |C| < 10°，状态栏会显示“加速剩余时间: XX秒”。
 
 [ ] 第三步 启动点火与自动停止：
           按 Z 设定油门，按 空格 点火。
           系统会自动监控：若倍速 > 1 且预测 1 秒后速度超过所需霍曼速度，
           **系统将自动暂停并降倍速至 1x**，避免过度加速。

 [ ] 第四步 等待与安全暂停：
           若前往外行星，观察 Ra 抬升；若前往内行星，观察 Ra 降低。
           在目标接近过程中，若高倍速运行，**安全保护机制会自动暂停**防止飞越。

 [ ] 第五步 入轨捕获：
           接近目标时，参考法则 B 的捕获窗口，在近地点执行减速泊入。

================================================================================
【写在最后】 倾角修正如打靶，视线垂直黄灰夹；B 增倾减方向对，B 近九十即收兵。
            捕获要看径向速，峰值回零快减速；高倍暂停是保护，自动降速显神通。
            黄线是命根，灰线是老板，青线是KPI。
================================================================================
)";
	print_pager("QRH", sv_from_cstr(content), 0);
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
#define v_forward vec_direct(rt->active_cam->forward)
#define v_up      vec_direct(rt->active_cam->up)
#define v_right   vec_direct(vec_cross_product(rt->active_cam->forward, rt->active_cam->up))
	switch (rt->inp) {
	case '\t': setup(rt, 1); break;
	case '~': setup(rt, 0); break;
	case '`': sync_cam_size_scale(rt); break;
	case 'f':
		rt->follow = choose_star(rt, "跟随", rt->follow);
		if (!rt->follow) {
			switch_camera(rt, rt->camera);
			break;
		}
		switch_camera(rt, &rt->follow->cam);
		rt->about_point = get_about_point(rt, rt->follow);
		Vec_t direct = rt->about_point ? vec_sub(rt->follow->speed, rt->about_point->speed) : rt->follow->speed;
		if (vec_len(direct) == 0) direct = rt->active_cam->forward;
		direct = vec_direct(direct);
		const double distance = vec_len(vec_sub(rt->active_cam->position, rt->follow->obj->center));
		rt->active_cam->position = vec_add(rt->follow->obj->center, vec_mul(direct, -distance));
		camera_look_no_hold(rt->active_cam, rt->follow->obj->center);
		break;
	case 't': rt->destination_to = choose_star(rt, "驶向", rt->destination_to); break;
	case 'F': rt->look_to = choose_star(rt, "看向", rt->look_to); break;
	case 'T': rt->rotate_cam_with_spd = !rt->rotate_cam_with_spd; break;
	case '|': dump_stars(rt); break;
	case '?': voyage_helper(rt); break;
	case 'M': print_qrh(); break;
	case '"': print_pager("航行日志", sv_from_sva(&rt->logs), -1); break;
	case '\'': rt->print_busy = !rt->print_busy; break;
	case 'i': rt->axis = !rt->axis; break;
	case 'I': rt->guidline = !rt->guidline; break;
	case '7': rt->active_cam->scale-=1; break;
	case '8': rt->active_cam->scale+=1; break;
	case '9': rt->active_cam->dept/=2; break;
	case '0': rt->active_cam->dept*=2; break;
	case '{': rt->time_scale=1.; break;
	case '}': rt->time_scale=(60.*60); break;
	case '[': rt->time_scale/=2; break;
	case ']': rt->time_scale*=2; break;
	case 'Z': rt->throttle=255; break;
	case 'X': rt->throttle=0; break;
	case 'z': rt->throttle+= rt->throttle<255?1:0; break;
	case 'x': rt->throttle-= rt->throttle>0?1:0; break;
	case 'r': rt->throttle_on^=0b10; break;
	case ' ':
		rt->throttle_on ^= 1;
		if (rt->throttle_on&1 && rt->time_scale >= 32) {
			rt->time_scale = 1.;
		}
		break;
	case '.':
		rt->pause = true;
		rt->gtime += physics_update(rt);
		break;
	case 'P':
	case 'p':
		rt->pause = !rt->pause;
		/* 意外油门保护 */
		if (!rt->pause && rt->throttle_on&1 && rt->time_scale >= 32) {
			rt->pause = true;
			rt->throttle_on &= ~1;
		}
		break;
	case 'c':
		printf("\e[4l\e[2J");
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
	case 'H': cam_rotate(v_forward, M_PI/4);break;
	case 'L': cam_rotate(v_forward, -M_PI/4);break;
	case '<': cam_rotate(v_up, M_PI/4);break;
	case '>': cam_rotate(v_up, -M_PI/4);break;
#undef cam_rotate
	}
	if (!rt->follow) return true;

	/* 手动加速,并记录操作dv(禁止暂停加速) */
	const double accel = !rt->pause ? 0.1*rt->throttle/SCALE/rt->fps : 0;
#define accelerate(var, k) rt->follow->speed = vec_add(rt->follow->speed, vec_mul((var), (k))), rt->dv += accel
	switch (rt->inp) {
	case 'n': accelerate(v_forward, accel); break;
	case 'N': accelerate(v_forward, 10*accel); break;
	case 'b': accelerate(v_forward, -accel); break;
	case 'B': accelerate(v_forward, -10*accel); break;
	case 'w': accelerate(v_up, accel); break;
	case 's': accelerate(v_up, -accel); break;
	case 'a': accelerate(v_right, -accel); break;
	case 'd': accelerate(v_right, accel); break;
#undef accelerate
	}
#undef v_forward
#undef v_up
#undef v_right
	return true;
}

void scene_init(Runtimedata_t *rt)
{
	if (!rt) return;
	double rand_num = 0;
#define RAND01 ((double)rand()/RAND_MAX)
#define RAND12 (1+RAND01)
#define RAND_VEC(k) vec_mul((Vec_t){1-2*RAND01, 1-2*RAND01, 1-2*RAND01}, RAND12*(k))
#define RAND_COLOR ((Color_t){100+RAND01*155,100+RAND01*155,100+RAND01*155,-1})
#define l_star_create(name, mass, radius, r, v, u)	\
	do {								\
		rand_num = 2*M_PI*RAND01;				\
		star = star_create(name, mass, radius,			\
				   vec_rotate((Vec_t){r,0,0}, u, rand_num),\
				   vec_rotate((Vec_t){0,v,0}, u, rand_num),\
				   NULL);			\
	} while(0)

	Star_t star = {};
	Star_t *center = NULL;
	l_star_create("地球", 5.965e24, 6371, -149.6e6, -29.78, ((Vec_t){0,0,1}));
	star.self_rotate = (Vec_t){0, 0, 1};
	star.self_omiga = 2*M_PI/(24*60*60);
	obj_rotate(obj_set_color(star.obj, (Color_t){29,153,243,-1}), (Vec_t){1, 1, -1}, M_PI/3.8);
	da_append(&rt->objs, &star);
	center = da_get(&rt->objs, rt->objs.len-1);

	star = star_create("地球小卫星", 1, 0.5, (Vec_t){12000,0,0}, vec_xyzl(0, 1, 0.8, 5.75993), center);
	obj_set_color(star.obj, (Color_t){-1,30,30,50});
	da_append(&rt->objs, &star);

	// GM = Rv^2
	// > sqrt((6.67*10^-11) * (5.965*10^24) / (11000*1000))/1000
	// 6.0141159707
	star = star_create("地球大卫星", 1e10, 450, (Vec_t){-42164,0,0}, vec_xyzl(0, -1, 0.1, 3.07282), center);
	obj_set_color(star.obj, (Color_t){0,-1,30,50});
	da_append(&rt->objs, &star);

	star = star_create("月球", 7.342e22, 1737.4,
			   vec_rotate((Vec_t){0, 384400, 0}, (Vec_t){1,0,0}, 5.14*M_PI/180),
			   vec_xyzl(-1, 0, 0, 1.022), center);
	obj_set_color(star.obj, (Color_t){100,100,100,-1});
	star.self_rotate = (Vec_t){0, 0, 1};
	star.self_omiga = 2*M_PI/(30.5*24*60*60);
	da_append(&rt->objs, &star);

	l_star_create("水星", 3.301e23, 2439.7, 57.91e6, 47.87, ((Vec_t){0,0,1}));
	star.self_rotate = (Vec_t){0, 0, 1};
	star.self_omiga = 2*M_PI/(1407.6*60*60);
	da_append(&rt->objs, &star);

	l_star_create("金星", 4.867e24, 6051.8, 108.21e6, 35.02, ((Vec_t){0,0,1}));
	star.self_rotate = (Vec_t){0, 0, -1};
	star.self_omiga = 2*M_PI/(5832.6*60*60);
	obj_set_color(star.obj, (Color_t){191,128,33,-1});
	da_append(&rt->objs, &star);

	l_star_create("火星", 6.417e23, 3389.5, 227.94e6, 24.07, ((Vec_t){0,0,1}));
	star.self_rotate = (Vec_t){0, 0, 1};
	star.self_omiga = 2*M_PI/(24.6*60*60);
	obj_set_color(star.obj, (Color_t){227,124,93,-1});
	da_append(&rt->objs, &star);

	l_star_create("木星", 1.898e27, 69911, 778.57e6, 13.07, ((Vec_t){0,0,1}));
	star.self_rotate = (Vec_t){0, 0, 1};
	star.self_omiga = 2*M_PI/(9.93*60*60);
	obj_set_color(star.obj, (Color_t){169,105,49,-1});
	da_append(&rt->objs, &star);

	l_star_create("土星", 5.683e26, 58232, 1433.53e6, 9.69, ((Vec_t){0,0,1}));
	star.self_rotate = (Vec_t){0, 0, 1};
	star.self_omiga = 2*M_PI/(10.66*60*60);
	obj_set_color(star.obj, (Color_t){237,191,116,-1});
	da_append(&rt->objs, &star);

	l_star_create("天王星", 8.681e25, 25362, 2872.46e6, 6.81, ((Vec_t){0,0,1}));
	star.self_rotate = vec_rotate((Vec_t){0, 0, 1}, (Vec_t){0, -1, 0}, 97.77);
	star.self_omiga = 2*M_PI/(17.24*60*60);
	obj_set_color(star.obj, (Color_t){190,227,230,-1});
	da_append(&rt->objs, &star);

	l_star_create("海王星", 1.024e26, 24622, 4495.06e6, 5.43, ((Vec_t){0,0,1}));
	star.self_rotate = (Vec_t){0, 0, 1};
	star.self_omiga = 2*M_PI/(16.11*60*60);
	obj_set_color(star.obj, (Color_t){45,55,140,-1});
	da_append(&rt->objs, &star);

	star = star_create("太阳", 1.989e30, 695700, (Vec_t){0,0,0}, (Vec_t){0,0,0}, NULL);
	star.self_rotate = (Vec_t){0, 0, 1};
	star.self_omiga = 2*M_PI/(25.4*60*60);
	obj_set_color(star.obj, (Color_t){-1,-1,0,-1});
	da_append(&rt->objs, &star);

// #define THREE_BODY
#ifdef THREE_BODY
	/* 安置在太阳系外4光年 */
	l_star_create("!?强强?!", 5.965e24*(10*RAND01+0.3), 6371*RAND12, 4*365*24*60*60*3e5, RAND12*5, ((Vec_t){0,0,1}));
	star.self_rotate = vec_direct(RAND_VEC(1));
	star.self_omiga = 2*M_PI/(24*RAND12*60*60);
	obj_set_color(star.obj, RAND_COLOR);
	da_append(&rt->objs, &star);
	center = da_get(&rt->objs, rt->objs.len-1);

	star = star_create("sun1", 1e31*(RAND12 - 0.5), RAND12*7e5, RAND_VEC(RAND12*1e8), RAND_VEC(5), center);
	star.self_rotate = vec_direct(RAND_VEC(1));
	star.self_omiga = 2*M_PI/(100*RAND12*60*60);
	obj_set_color(star.obj, RAND_COLOR);
	da_append(&rt->objs, &star);

	star = star_create("sun2", 5e31*(RAND12 - 0.5), RAND12*7e5, RAND_VEC(RAND12*1e8), RAND_VEC(5), center);
	star.self_rotate = vec_direct(RAND_VEC(1));
	star.self_omiga = 2*M_PI/(100*RAND12*60*60);
	obj_set_color(star.obj, RAND_COLOR);
	da_append(&rt->objs, &star);

	star = star_create("sun3", 1e32*(RAND12 - 0.5), RAND12*7e5, RAND_VEC(RAND12*1e8), RAND_VEC(5), center);
	star.self_rotate = vec_direct(RAND_VEC(1));
	star.self_omiga = 2*M_PI/(100*RAND12*60*60);
	obj_set_color(star.obj, RAND_COLOR);
	da_append(&rt->objs, &star);
#endif
#undef RAND_VEC
#undef RAND12
#undef RAND01
	Star_t *objs = rt->objs.ptr;
	for (size_t i = 0; i < rt->objs.len; i++) {
		if (!objs[i].obj) continue;
		objs[i].cam = *rt->camera;    /* 同步相机配置 */
		const double distance = objs[i].radius*10;
		const Vec_t direct = vec_mul(vec_direct(objs[i].cam.forward), -distance);
		objs[i].cam.dept = 5*distance;
		objs[i].cam.position = vec_add(objs[i].obj->center, direct);
	}
}

int main(void)
{
	Runtimedata_t rt = {0};
	if (!setup(&rt, 0)) {
		return EXIT_FAILURE;
	}
	srand(time(NULL));
	scene_init(&rt);
	rt.camera->position = (Vec_t){0, 0, 1e6*SCALE};
	rt.camera->dept = 1e8*SCALE;

	printf("按键说明：\n"
	       "zx 增减推力 空格开关油门（固定朝视线方向加速）\n"
	       "(每1%%推力每秒提供0.1m/s的dv)\n"
	       "bB 减速 Nn加速 wasd偏转加速\n"
	       "WASD 控制镜头平移 -=_+ 控制镜头远近\n"
	       "hjkl 控制镜头摇头抬头 <>左右大摇(45°)\n"
	       "HJKL 控制镜头歪头 JK小转 HL大转(45°)\n"
	       "7/8 控制焦距 9/0 控制可视距离\n"
	       "f 跟随  F 看向某物体  t 设定目标\n"
	       "[]{} 控制时间流速 p暂停 .逐帧运行\n"
	       "' 显示负载/帧率  \" 查看航行日志  | 导出场景数据\n"
	       "? 进行数学辅助计算\n"
	       "f跟随时视角会调整方向为绝对速度\n"
	       "F选择看向自身视角会追踪该速度方向\n"
	       "i打开绝对坐标轴(红绿蓝)+相对速度矢量显示(黄)\n"
	       "I打开目标参考线（青）+环绕天体方向参考线（灰）\n"
	       " 以及相对速度矢量显示(黄)\n"
	       );
	rt.inp = 'f';
	input_handle(&rt);

	printf("\e[2J");
	size_t i = 0;
	SVA_t buf = {};
	double busy = 0;
	struct orbital_parameters ret = {}, last_ret = {};
	Star_t *last_about_point = NULL,
	       *last_follow = NULL;
	int8_t last_throttle_on = false;
	for (i = 0; i < INT64_MAX; ++i) {
		last_about_point = rt.about_point;
		last_follow = rt.follow;
		if ((rt.inp = kbhitGetchar()))
			if (!input_handle(&rt)) break;
		if (!rt.pause) rt.gtime += physics_update(&rt);
		if (rt.follow) ret = get_orbital_parameters(rt.follow, rt.about_point);
		if (rt.follow && last_follow == rt.follow && last_about_point && last_about_point != rt.about_point) {
			format_orbital_parameters(&rt, &buf, ret);
			syslog(&rt, "天体'%s'被'%s'捕获(%s)(原运行在'%s'),累计dv:%.3gkm/s",
			       rt.follow->name, rt.about_point->name, buf.p,
			       last_about_point->name, rt.dv);
		}
		if (!rt.about_point) break;
		if (rt.follow && last_follow == rt.follow && rt.about_point &&
		    (((last_ret.e-1)*(ret.e-1)<0) || (last_throttle_on^rt.throttle_on)&1 ||
		     (rt.throttle_on&1 && (ret.ra<last_ret.rp || ret.rp>last_ret.ra)))) {
			if (ret.ra+10<last_ret.rp || ret.rp>last_ret.ra+10) {
				syslog(&rt, "近远地点高度交换");
				if (rt.time_scale >= 32) rt.pause = true;
			}
			format_orbital_parameters(&rt, &buf, ret);
			syslog(&rt, "'%s'->'%s':%s(%s,dv:%.3gkm/s)(油门%d%%%s)",
			       rt.follow->name, rt.about_point->name,
			       ret.typ, buf.p, rt.dv, rt.throttle, rt.throttle_on&1?"开":"关");
			last_ret = ret;
		}
		last_throttle_on = rt.throttle_on;

		if (rt.follow && rt.look_to) {
			Vec_t direct = rt.look_to == rt.follow ? \
				       vec_sub(rt.follow->speed, rt.about_point->speed) : \
				       vec_sub(rt.look_to->obj->center, rt.follow->obj->center);
			double dist = vec_len(vec_sub(rt.follow->obj->center, rt.active_cam->position));
			rt.active_cam->position = 
				vec_add(rt.follow->obj->center,
					vec_mul(vec_direct(direct), -dist));
			camera_look_no_hold(rt.active_cam, rt.look_to->obj->center);
		}

		if (rt.axis && rt.follow) {
			rt.axis_helper->center = rt.follow->obj->center;
			obj_cast(rt.axis_helper, rt.active_cam, rt.backend);
		}
		if ((rt.guidline || rt.axis) && rt.follow) {
			Point_t p1, p2;
			camera_cast_line(rt.active_cam,
					 rt.follow->obj->center,
					 vec_add(rt.follow->obj->center,
						 vec_sub(rt.follow->speed, rt.about_point->speed)),
					 &p1, &p2);
			backend_draw_line(rt.backend, rt.active_cam, p1, p2,
					  (Color_t){-1,-1,0,100},
					  (Color_t){-1,-1,0,100});
		}
		if (rt.guidline && rt.follow && rt.destination_to) {
			Point_t p1, p2;
			/* 目的地方向 */
			camera_cast_line(rt.active_cam,
					 rt.follow->obj->center,
					 rt.destination_to->obj->center,
					 &p1, &p2);
			backend_draw_line(rt.backend, rt.active_cam, p1, p2,
					  (Color_t){0,-1,-1,100},
					  (Color_t){0,-1,-1,100});
			/* 当前环绕中心方向 */
			camera_cast_line(rt.active_cam,
					 rt.follow->obj->center,
					 rt.about_point->obj->center,
					 &p1, &p2);
			backend_draw_line(rt.backend, rt.active_cam, p1, p2,
					  (Color_t){-1,-1,-1,100},
					  (Color_t){-1,-1,-1,100});
		}
		for (size_t i = 0; i < rt.objs.len; i++) {
			Star_t *star = da_get(&rt.objs, i);
			if (!star->obj) continue;
			obj_cast(star->obj, rt.active_cam, rt.backend);
		}
		printf("\e[H");
		rt.backend->render(rt.backend);
		rt.backend->clean(rt.backend);
		printf("\e[0m\e[2K\e[B\e[2K\e[A\r[T+%.1fd, x%g, %c%s%d%%%c, dv:%.3gkm/s]",
		       rt.gtime/(24.*60*60), rt.time_scale,
		       rt.throttle_on&1?'[':':',
		       rt.throttle_on&0b10?"-":"",
		       rt.throttle,
		       rt.throttle_on&1?']':':',
		       rt.dv);
		if (rt.follow) {
			printf(" | %s%c%s%c (%.3f km/s)",
			       rt.follow->name ? rt.follow->name : "Unknow",
			       rt.rotate_cam_with_spd ? '{' : '[',
			       rt.about_point->name ? rt.about_point->name : "Unknow",
			       rt.rotate_cam_with_spd ? '}' : ']',
			       vec_len(vec_sub(rt.follow->speed, rt.about_point->speed)));
		}
		if (rt.follow && rt.destination_to) {
			printf(" Rp:%.1fkm Ra:%.1fkm\r\e[B", ret.rp, ret.ra);
			const Vec_t dist = vec_sub(rt.destination_to->obj->center, rt.follow->obj->center);
			const Vec_t dv = vec_sub(rt.follow->speed, rt.destination_to->speed);
			const double vertical_speed = vec_point_product(vec_direct(dist), dv);
			// 速度 <0 表靠近， >0 表远离
			printf("距%s %.1f km (%.3f km/s)",
			       rt.destination_to->name ? rt.destination_to->name : "Unknow",
			       vec_len(dist),
			       -vertical_speed);
			if (rt.about_point != rt.destination_to && get_about_point(&rt, rt.destination_to) == rt.about_point) {
				/* 共心状态 */
				struct orbital_parameters ret2 = get_orbital_parameters(rt.destination_to, rt.about_point);
				Vec_t u = vec_direct(vec_cross_product(ret.u, ret2.u));
				Vec_t v = vec_sub(rt.follow->speed, rt.about_point->speed);
				double deg_cross = vec_point_product(vec_direct(v),u);
				deg_cross = 90 - acos(deg_cross)/M_PI*180.;
				struct hohmann_orbital_parameters ret3 =
					get_hohmann_orbit_theta(rt.follow, rt.destination_to, rt.about_point);
				double time_left = rt.throttle?(rt.throttle_on&0b10?-1:1)*(ret3.v-vec_len(v))/(0.1*rt.throttle/SCALE):0;
				printf(" %.1f°(%.1f)°/%.1f°",
				       acos(vec_point_product(ret.u, ret2.u))/(M_PI)*180.,
				       deg_cross,
				       ret3.theta/M_PI*180.);
				if (fabs(ret3.theta/M_PI*180.) < 10) {
					printf(" L:%.2fs", time_left);
					/* 自动暂停 */
					if (rt.throttle_on&1 && rt.time_scale>2 && time_left-rt.time_scale < 0) {
						rt.pause = true;
						rt.time_scale = 1;
					}
				}
			}
			if (rt.time_scale >= 32 && ret.e > 1 && vertical_speed > 10 &&
			    vec_len(dist)<vertical_speed*rt.time_scale) {
				rt.time_scale = 1;
				rt.pause = true;
			}
		}
		if (rt.pause) printf(" [已暂停]");
		if (rt.print_busy) printf(" (%5.1f%%/%dfps)", busy, (int)(busy<100?rt.fps:rt.fps*100/busy));
		busy = (busy + (1-(sleep_fixed_step(1./rt.fps))/(1./rt.fps)) * 100)/2;
	}

	if (rt.logs.p)
		printf("\e[0m\n航行日志：\n%s", rt.logs.p);

	sva_free(&buf);
	cleanup(&rt);
	return EXIT_SUCCESS;
}

