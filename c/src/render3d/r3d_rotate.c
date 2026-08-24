/**
 * @file        r3d_rotate.c
 * @author      Chglish
 * @date        2026-07-12
 * @brief       一个简单的3d N体运动模拟程序，
 * 可任意选择天体跟踪操纵，内置仿真简易太阳系
 */

#include "lib/render3d.h"
#include "../../include/tools.h"
#include <time.h>

#define MAX_FRAME INT64_MAX
#define FPS 40
/* 时间缩放倍率，默认x1 */
double TIME_SCALE = 1./FPS;

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
	double dv;
	double gtime;
	int  inp;
	uint8_t throttle;    /* 1% = 0.1m/s^2 */
	int8_t throttle_on;
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
	int term_h = get_winsize_row() - 2;
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
	int term_h = get_winsize_row() - 2;
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
	if (rt->throttle_on&1 && rt->throttle && rt->follow) {
		/* throttle_on<0时朝反方向推力 */
		double accel = rt->throttle * 0.1 / SCALE * time_scale * (rt->throttle_on<0?-1:1);
		rt->follow->speed = vec_add(rt->follow->speed, vec_mul(vec_direct(rt->active_cam->forward), accel));
		rt->dv += fabs(accel);
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
static Star_t *get_about_point(Runtimedata_t *rt, Star_t *follow)
{
	static Obj_t base_obj = { };
	static Star_t base = {
		.obj = &base_obj,
		.name = "绝对坐标",
	};
	if (!rt || !rt->objs || rt->obj_count < 2)
		return &base;
	if (!follow) follow = rt->follow;
	if (!follow || (size_t)(follow-rt->objs) > rt->obj_count)
		return &base;

	Star_t *objs = rt->objs;
	size_t n = rt->obj_count;
	size_t idx_follow = follow - objs;	// 目标索引

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

struct hohmann_orbital_parameters {
	double theta;    /* 剩余角度，为0时最佳(rad) */
	double expect_theta;    /* 期望角度提前量(rad) */
	double T;    /* 半周期 */
};

/* 计算距离霍曼转移点火点提前量角度 */
static struct hohmann_orbital_parameters get_hohmann_orbit_theta(Star_t *follow, Star_t *destination_to, Star_t *about_point)
{
	if (!follow || !follow->obj || !destination_to || !destination_to->obj
	    || !about_point || !about_point->obj) return (struct hohmann_orbital_parameters){};
	const Vec_t r2 = vec_sub(destination_to->obj->center, about_point->obj->center);
	const Vec_t r1 = vec_sub(follow->obj->center, about_point->obj->center);
	const double r2f = vec_len(r2);
	const double a = (vec_len(r1) + r2f) / 2;
	const double T = 2*M_PI*(a*SCALE)*sqrt(a*SCALE/(G*(follow->mass+about_point->mass)));
	const double omiga = M_PI - (sqrt(G*about_point->mass/(r2f*SCALE)) / SCALE / r2f)*T/2;
	const double theta = acos(vec_point_product(vec_direct(r1), vec_direct(r2))) * \
			     (vec_point_product(vec_cross_product(r1, vec_sub(follow->speed, about_point->speed)),
					       vec_cross_product(r1, r2)) < 0 ? -1 : 1);
	const struct hohmann_orbital_parameters ret = {
		.expect_theta = omiga,
		.theta = theta - fmod(omiga, 2*M_PI),
		.T = T/2,
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
		printf("霍曼转移轨半周期: %.1f d\n", ret.T/(24*60*60));
		printf("霍曼转移角度提前: %.1f deg\n", ret.expect_theta/M_PI*180.);
		printf("距霍曼转移点火点: %.1f deg\n", ret.theta/M_PI*180.);
	}

	printf("（回车返回）\n");
	kbhitGetchar();
	_getch();
	return;
}

static void dump_stars(Runtimedata_t *rt)
{
	if (!rt || !rt->objs) return;
	printf("\e[0m\n\e[2K===== 数据导出：各星体基本参数 =====\n");
	for (size_t i = 0; i < rt->obj_count; i++) {
		if (!rt->objs[i].obj) continue;
		printf(" [%lu] %s (%gkg) 位置(km): {%.3f,%.3f,%.3f} 速度(km/s): {%.3f,%.3f,%.3f}\n", i+1,
		       rt->objs[i].name ? rt->objs[i].name : "{未命名星体}",
		       rt->objs[i].mass,
		       rt->objs[i].obj->center.x,
		       rt->objs[i].obj->center.y,
		       rt->objs[i].obj->center.z,
		       rt->objs[i].speed.x,
		       rt->objs[i].speed.y,
		       rt->objs[i].speed.z);
	}
	printf("游戏时间: T+%.1f s, 折合约 T+%.1f d\n", rt->gtime, rt->gtime/(24*60*60));
	printf("操作累计dv: %.3f km/s\n", rt->dv);
	printf("（回车返回）\n");
	kbhitGetchar();
	_getch();
}

static void print_qrh()
{
	printf("\e[0m\n\e[2K这里是高级操作教程，下面是一些常见操作方法\n"
	       "1. 改变轨道倾角：使用f,t设置目标后，若目标与操纵天体围绕同一天体公转，则会在状态\n"
	       "   栏右端显示形如`0.7(-21)°/11.4`的数据，最左边显示的是操纵天体与目标天体的轨道\n"
	       "   倾角。当括号内的角度读数接近0时表明你运行到了两个轨道平面的升/降交点。此时先\n"
	       "   使用p暂停，使用I打开参考线，转动相机使中心天体-自己-目标天体的连线（青线和灰\n"
	       "   线）处于同一条直线。观察黄色矢量方向（如果看不见就用+放大），旋转相机使得黄线\n"
	       "   基本竖直于屏幕，连续按两次>或者<以朝着黄线相对于青灰线的一侧旋转，此时视线方\n"
	       "   向基本指向速度的法向方向。使用zx设定推力并按下空格启动引擎，还有要记得取消暂\n"
	       "   停。等待引擎加速改变速度方向。角度每改变5°左右就需要反方向按两次<或>旋转相机\n"
	       "   让黄线重新竖直。重复该动作并持续观察轨道相差角度直到接近0。但由于误差等原因很\n"
	       "   多时候数值无法完全归零，状态栏显示精度又不足以观察最小值，可在临界范围内改为\n"
	       "   观察括号内数值，一般而言，其值最大时一般轨道夹角最小。\n"
	       "2. 变轨操作：使用f,t设置目标后，若目标为自身环绕天体，则会显示近地点(Rp)和远地点\n"
	       "   (Ra)高度。一般而言，近地点加减速和在远地点改变轨道倾角最省dv。若远地点值为负\n"
	       "   数则说明当前天体未能被目标天体捕获需要在近地点附近进行减速。加速减速都需要带\n"
	       "   有一定提前量以免错过最佳点火点。\n"
	       "3. 霍曼转移：霍曼转移的逻辑就是预估好目标天体在转移之后的预期位置（点火位置与中\n"
	       "   心天体的连线方向上）并反推当前位置判断点火时机，然后点火加速减速改变近远地点\n"
	       "   高度使其中一个达到或略微超过目标天体轨道高度，途中些许修正轨道并在最后减速泊\n"
	       "   入目标天体。比方说拖地球到木星。首先t设定好目标（木星），此时应当会出现第一点\n"
	       "   提到的仪表信息。(如果目标中心天体不同请先变轨脱离或者f到中心天体代为观察)第三\n"
	       "   个数就是距离最佳点火点的角度，值越接近0位置越好（算法原因绕圈过程可能存在数值\n"
	       "   跳变）。等待读数接近0后t改变目标为中心天体（太阳）以观察Ra,Rp。使用? 查询木星\n"
	       "   的轨道高度自己记下来。按下F选择和f相同的天体（地球）以锁定当前的速度方向（减\n"
	       "   速需要使用r改为减速）。设定好油门并空格启动引擎变轨，观察近地点（减速）或远地\n"
	       "   点（加速）直到达到目标轨道高度。然后就是等待天体移动靠近。接近目标天体时记得\n"
	       "   观察中心天体是否有改变为目标天体改变后降低倍速等待到近地点进行减速入轨（入轨\n"
	       "   时若远地点为负数时绝对值越大则越接近入轨状态）\n"
	       );
	printf("（回车返回）\n");
	kbhitGetchar();
	_getch();
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
	case '\t': setup(rt); break;
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
	case '|': dump_stars(rt); break;
	case '?': voyage_helper(rt); break;
	case 'M': print_qrh(); break;
	case 'i': rt->axis = !rt->axis; break;
	case 'I': rt->guidline = !rt->guidline; break;
	case '7': rt->active_cam->scale-=1; break;
	case '8': rt->active_cam->scale+=1; break;
	case '9': rt->active_cam->dept/=2; break;
	case '0': rt->active_cam->dept*=2; break;
	case '{': TIME_SCALE=1./FPS; break;
	case '}': TIME_SCALE=(60.*60/FPS); break;
	case '[': TIME_SCALE/=2; break;
	case ']': TIME_SCALE*=2; break;
	case 'Z': rt->throttle=255; break;
	case 'X': rt->throttle=0; break;
	case 'z': rt->throttle+= rt->throttle<255?1:0; break;
	case 'x': rt->throttle-= rt->throttle>0?1:0; break;
	case 'r': rt->throttle_on^=1<<7; break;
	case ' ':
		rt->throttle_on ^= 1;
		if (rt->throttle_on&1 && TIME_SCALE*FPS >= 32) {
			TIME_SCALE = 1./FPS;
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
		if (!rt->pause && rt->throttle_on&1 && TIME_SCALE*FPS >= 32) {
			rt->pause = true;
			rt->throttle_on &= ~1;
		}
		break;
	case 'c':
		printf("\e[2J");
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
	const double accel = !rt->pause ? rt->throttle/SCALE*TIME_SCALE : 0;
#define accelerate(var, k) rt->follow->speed = vec_add(rt->follow->speed, vec_mul((var), (k))), rt->dv += accel
	switch (rt->inp) {
	case 'n': accelerate(v_forward, 5*accel); break;
	case 'N': accelerate(v_forward, 50*accel); break;
	case 'b': accelerate(v_forward, -5*accel); break;
	case 'B': accelerate(v_forward, -50*accel); break;
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

int main(void)
{
	Runtimedata_t rt = {0};
	if (!setup(&rt)) {
		return EXIT_FAILURE;
	}
	srand(time(NULL));
	/* 日地距离 */
	const double Dx_SE = -149.6e6;
	/* 地月系相对太阳速度 */
	const double Vy_SE = -29.78;
	Star_t objs[] = {
// #define THREE_BODY
#ifdef THREE_BODY
#define RAND01 ((double)rand()/RAND_MAX)
#define RAND12 (1+RAND01)
#define RAND_VEC(k) vec_mul((Vec_t){1-2*RAND01, 1-2*RAND01, 1-2*RAND01}, RAND12*(k))
#define RAND_COLOR ((Color_t){100+RAND01*155,100+RAND01*155,100+RAND01*155,-1})
		(Star_t){
			.name = "日1",
			.obj = obj_set_color(obj_shift(obj_create_cube(RAND12*7e5*2), RAND_VEC(RAND12*1e8)), RAND_COLOR),
			.mass = 1e30*(1+10*RAND01),
			.speed = RAND_VEC(5),
			.self_rotate = vec_direct(RAND_VEC(1)),
			.self_omiga = RAND01*2*M_PI/(24*60*60),
		}, (Star_t){
			.name = "日2",
			.obj = obj_set_color(obj_shift(obj_create_cube(RAND12*7e5*2), RAND_VEC(RAND12*1e8)), RAND_COLOR),
			.mass = 1e30*(1+10*RAND01),
			.speed = RAND_VEC(5),
			.self_rotate = vec_direct(RAND_VEC(1)),
			.self_omiga = RAND01*2*M_PI/(24*60*60),
		}, (Star_t){
			.name = "日3",
			.obj = obj_set_color(obj_shift(obj_create_cube(RAND12*7e5*2), RAND_VEC(RAND12*1e8)), RAND_COLOR),
			.mass = 1e30*(1+10*RAND01),
			.speed = RAND_VEC(5),
			.self_rotate = vec_direct(RAND_VEC(1)),
			.self_omiga = RAND01*2*M_PI/(24*60*60),
		}, (Star_t){
			.name = "!?小小?!",
			.obj = obj_set_color(obj_create_cube(6371*2), RAND_COLOR),
			.mass = ((void)Dx_SE, (void)Vy_SE, 5.965e24*(RAND01+0.5)),
			.speed = RAND_VEC(5),
			.self_rotate = vec_direct(RAND_VEC(1)),
			.self_omiga = RAND01*2*M_PI/(24*60*60),
		}, (Star_t){ .name = "列表结束", },
#undef RAND_VEC
#undef RAND12
#undef RAND01
#else
		(Star_t){
			.name = "地球",
			.obj = obj_set_color(obj_rotate(obj_shift(obj_create_cube/*_with_surface*/(6371*2),
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
			.obj = obj_set_color(obj_shift(obj_create_cube(1), (Vec_t){12000+Dx_SE, 0, 0}),
					     (Color_t){-1,30,30,-1}),
			.mass = 1,
			.speed = vec_add(vec_mul(vec_direct((Vec_t){0, 1, 0.8}), 5.75993), (Vec_t){0, Vy_SE, 0}),
		}, (Star_t){
			.name = "地球大卫星",
			.obj = obj_shift(obj_create_cube(900), (Vec_t){-42164+Dx_SE, 0, 0}),
			.mass = 1e10,
			// GM = Rv^2
			// > sqrt((6.67*10^-11) * (5.965*10^24) / (11000*1000))/1000
			// 6.0141159707
			.speed = vec_add(vec_mul(vec_direct((Vec_t){0, -1, 0.1}), 3.07282), (Vec_t){0, Vy_SE, 0}),
			.self_rotate = (Vec_t){1, 1, -1},
			.self_omiga = 2*M_PI/(24*60*60),
		}, (Star_t){
			.name = "月球",
			.obj = obj_shift(obj_create_cube(1737.4*2),
					 vec_add((Vec_t){Dx_SE, 0, 0},
						 (vec_rotate((Vec_t){0, 384400, 0},
							     (Vec_t){1,0,0}, 5.14*M_PI/180)))),
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
			.obj = obj_set_color(obj_shift(obj_create_cube(3389.5*2), (Vec_t){227.94e6, 0, 0}), (Color_t){227,124,93,-1}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 24.07),
			.self_rotate = (Vec_t){0, 0, 1},
			.self_omiga = 2*M_PI/(24.6*60*60),
		}, (Star_t){
			.name = "木星",
			.mass = 1.898e27,
			.obj = obj_set_color(obj_shift(obj_create_cube(69911*2), (Vec_t){778.57e6, 0, 0}), (Color_t){169,105,49,-1}),
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
			.obj = obj_set_color(obj_shift(obj_create_cube(25362*2), (Vec_t){2872.46e6, 0, 0}), (Color_t){190,227,230,-1}),
			.speed = vec_mul(vec_direct((Vec_t){0, 1, 0}), 6.81),
			.self_rotate = (Vec_t){0, 0, -1},
			.self_omiga = 2*M_PI/(17.24*60*60),
		}, (Star_t){
			.name = "海王星",
			.mass = 1.024e26,
			.obj = obj_set_color(obj_shift(obj_create_cube(24622*2), (Vec_t){4495.06e6, 0, 0}), (Color_t){45,55,140,-1}),
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
#endif
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
	Star_t *about_point;
	for (i = 0; i < MAX_FRAME; ++i) {
		if ((rt.inp = kbhitGetchar()))
			if (!input_handle(&rt)) break;
		if (!rt.pause) rt.gtime += physics_update(&rt);
		about_point = get_about_point(&rt, NULL);
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
		}
		if ((rt.guidline || rt.axis) && rt.follow) {
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
		if (rt.guidline && rt.follow && rt.destination_to) {
			Point_t p1, p2;
			/* 目的地方向 */
			camera_cast_line(rt.active_cam,
					 rt.follow->obj->center,
					 rt.destination_to->obj->center,
					 &p1, &p2);
			backend_draw_line(rt.backend, rt.active_cam, p1, p2,
					  (Color_t){0,-1,-1,-1},
					  (Color_t){0,-1,-1,-1});
			/* 当前环绕中心方向 */
			camera_cast_line(rt.active_cam,
					 rt.follow->obj->center,
					 about_point->obj->center,
					 &p1, &p2);
			backend_draw_line(rt.backend, rt.active_cam, p1, p2,
					  (Color_t){-1,-1,-1,0.3*225},
					  (Color_t){-1,-1,-1,0.3*225});
		}
		for (size_t i = 0; i < countof(objs); i++) {
			if (!objs[i].obj) continue;
			obj_cast(objs[i].obj, rt.active_cam, rt.backend);
		}
		printf("\e[H");
		rt.backend->render(rt.backend);
		rt.backend->clean(rt.backend);
		printf("\e[0m\e[2K\r[T+%.1fd, x%g, %c%s%d%%%c, dv:%.3gkm/s]",
		       rt.gtime/(24.*60*60), TIME_SCALE*FPS,
		       rt.throttle_on&1?'[':':',
		       rt.throttle_on>=0?"":"-",
		       rt.throttle,
		       rt.throttle_on&1?']':':',
		       rt.dv);
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
			struct orbital_parameters ret = get_orbital_parameters(rt.follow, about_point);
			if (about_point == rt.destination_to) {
				/* 环绕状态 */
				printf(" Rp:%.1fkm Ra:%.1fkm", ret.rp, ret.ra);
			} else if (get_about_point(&rt, rt.destination_to) == about_point) {
				/* 共心状态 */
				struct orbital_parameters ret2 = get_orbital_parameters(rt.destination_to, about_point);
				Vec_t u = vec_direct(vec_cross_product(ret.u, ret2.u));
				double deg = vec_point_product(vec_direct(vec_sub(rt.follow->speed, about_point->speed)), u);
				deg = 90 - acos(deg)/M_PI*180.;
				printf(" %.1f(%.1f)°/%.1f",
				       acos(vec_point_product(ret.u, ret2.u))/(M_PI)*180.,
				       deg,
				       get_hohmann_orbit_theta(rt.follow, rt.destination_to, about_point).theta/M_PI*180.);
			}
		}
		if (rt.pause) printf(" [已暂停]");
		sleep_fixed_step(1./FPS);
	}

	cleanup(&rt);
	return EXIT_SUCCESS;
}

