/**
 * @file        star.c
 * @author      Chglish
 * @date        2026-10-10
 * @brief       分离自r3d_rotate.c，天体生命周期有关
 */

#include "./nbody.h"

Star_t star_create(char *name, double mass, double radius, Vec_t position, Vec_t speed, Star_t *about_point)
{
	Point_t center = about_point ? about_point->center : (Vec_t){};
	// obj_create_cube/*_with_surface*/
	position = vec_mul(position, km2unit(1));
	speed = vec_mul(speed, km2unit(1));
	radius = km2unit(radius);
	Star_t star = {
		.obj = obj_create_sphere(radius, 16, 16),
		.center = vec_add(center, position),
		.speed = vec_add(about_point ? about_point->speed : (Vec_t){}, speed),
		.mass = mass > 1e-4 ? mass : 1e-4,    /* 负质量是非法的！ */
		.radius = radius,
		.self_rotate = (Vec_t){0, 0, 1},
		.name = {},
	};
	sva_from_cstr(&star.name, name?name:"UNKNOW");
	return star;
}

void star_free(void *p)
{
	Star_t *star = p;
	if (!star) return;
	sva_free(&star->name);
	if (!star->obj) return;
	obj_free(star->obj);
	star->obj = NULL;
}

void star_pop(Runtimedata_t *rt, Star_t *star, const char *desc)
{
	if (!rt || !star) return;
	const Star_t *stars = rt->objs.ptr;
	const size_t idx = star - stars;
	if (idx >= rt->objs.len) return;
	char *hints[] = {
		"掉出了这个世界",
		"爱上了仙人球",
		"心跳停止了",
		"咬到舌头了",
		"无法无天了",
		"变成了大素",
		"对巨石一见钟情了",
		"以为他们会飞了",
		"求死的愿望实现了",
		"的鞭刑终于停止了",
		"的椎间盘突出了",
		"的头盖骨被拿来当碗使了",
		"的零件放错了位置",
		"的内脏变成了外脏",
		"被折成两半了",
		"被空气迎头痛击",
		"被送到了奥库瑞姆之家",
	};
	rtlog(rt, "天体'%s'%s，凶手是'%s'",
	      star->name.p ? star->name.p : "未知天体",
	      hints[rand()%countof(hints)],
	      desc ? desc : "虚空");
	/* 修正各指针 */
	Star_t **objs[] = {&rt->follow, &rt->look_to, &rt->destination_to, &rt->about_point};
	size_t idxes[countof(objs)] = {};
	for (size_t i = 0; i < countof(objs); i++) {
		if (*objs[i] == star) idxes[i] = -1;
		else idxes[i] = *objs[i] ? *objs[i] - stars : -1;
		/* 处理清理后的位移错位 */
		if (idxes[i] > idx) idxes[i]--;
	}
	da_pop(&rt->objs, idx, star_free);
	for (size_t i = 0; i < countof(objs); i++) {
		*objs[i] = da_get(&rt->objs, idxes[i]);
	}
	rt->active_cam = rt->follow ? &rt->follow->cam : rt->camera;
	rt->need_notification++;
}

void star_sync_position(Runtimedata_t *rt)
{
	if (!rt || !rt->objs.ptr) return;
	Star_t *objs = rt->objs.ptr;
	for (size_t i = 0; i < rt->objs.len; i++) {
		if (objs[i].obj) objs[i].obj->center = objs[i].center;
	}
}

void star_sync_cam_config(Runtimedata_t *rt)
{
	if (!rt || !rt->objs.ptr) return;
	Star_t *objs = rt->objs.ptr;
	for (size_t i = 0; i < rt->objs.len; i++) {
		if (!objs[i].obj) continue;
		objs[i].cam = *rt->camera;    /* 同步相机配置 */
		const double distance = objs[i].radius*10;
		const Vec_t direct = vec_mul(vec_direct(objs[i].cam.forward), -distance);
		objs[i].cam.dept = 5*distance;
		objs[i].cam.position = vec_add(objs[i].center, direct);
	}
}

void star_star_impact(Runtimedata_t *rt, Star_t *s1, Star_t *s2)
{
	if (!rt || !s1 || !s2) return;
	Star_t *stars = rt->objs.ptr;
	if (!stars || s1 == s2
	    || s1 < stars || s1 >= stars+rt->objs.len
	    || s2 < stars || s2 >= stars+rt->objs.len)
		return;

	/* 确保s2是被删除项(质量小) */
	if (s1->mass <= s2->mass) {
		Star_t *tmp = s2;
		s2 = s1;
		s1 = tmp;
	}
#define star_impact_xyz(xyz) (s1->mass*s1->speed.xyz + s2->mass*s2->speed.xyz)/(s1->mass+s2->mass)
	s1->speed = (Vec_t){
		.x = star_impact_xyz(x),
		.y = star_impact_xyz(y),
		.z = star_impact_xyz(z),
	};
#undef star_impact_xyz
	s1->mass += s2->mass;
	SVA_t buf = {};
	sva_sprintf(&buf, "来自`%s`(+%gkg)大地的爱",
		    s1->name.p?s1->name.p:"未知天体",
		    s2->mass);
	star_pop(rt, s2, buf.p);
	sva_free(&buf);
}

/* 根据引力影响范围自动获取速度参考系星体
 * (ai生成，原理我也不清楚，总之能用)
 * 根据潮汐摄动比自动获取速度参考系星体 */
Star_t *star_get_about_point(Runtimedata_t *rt, Star_t *follow)
{
	static Obj_t base_obj = { };
	static const SV_t name_sv = sv_from_lstr("绝对坐标");
	static const SVA_t name = {.p = (void*)name_sv.p, .len = name_sv.len, .capacity = 0};
	static Star_t base = {
		.obj = &base_obj,
		.name = name,
	};
	Star_t *objs = rt->objs.ptr;
	if (!rt || !objs || rt->objs.len < 2)
		return &base;
	if (!follow) follow = rt->follow;
	if (!follow || (size_t)(follow-objs) > rt->objs.len)
		return &base;
	if (rt->about_point && follow == rt->follow && rt->fixed_about_point)
		return rt->about_point;

	size_t n = rt->objs.len;
	size_t idx_follow = follow - objs;	// 目标索引

	// 1. 预先计算每个天体受到的总引力加速度（矢量）
	phy_compute_acceleration(rt);
	if (rt->accel.len < rt->objs.len) return &base;
	Vec_t *acc_total = rt->accel.ptr;
	if (!acc_total) return &base;

	// 2. 寻找最小摄动比
	double min_ratio = 1e100;
	Star_t *best = &base;

	for (size_t j = 0; j < n; j++) {
		if (objs+j == follow || !objs[j].obj)
			continue;

		// 候选天体 j 对 follow 的引力加速度 (unit/s^2)
		Vec_t diff = vec_sub(objs[j].center, follow->center);
		double r2 = (pow2(diff.x) + pow2(diff.y) + pow2(diff.z));
		if (r2 < 1e-18) continue;
		double r = sqrt(r2);
		double factor = G_unit * objs[j].mass / (r2 * r);
		Vec_t acc_j_on_target = vec_mul(diff, factor);
		double a_j = vec_len(acc_j_on_target);
		if (a_j < 1e-30) continue;

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

Star_t *star_choose(Runtimedata_t *rt, const char *hint, Star_t *old)
{
	if (!rt) return NULL;
	printf("\e[0m\n可选天体：\n [0] 空选择\n");
	int choice = 0;
	Star_t *objs = rt->objs.ptr;
	for (size_t i = 0; i < rt->objs.len; i++) {
		printf(" [%lu] %s (%g kg/%g km)%s\n", i+1,
		       objs[i].name.p ? objs[i].name.p : "{未命名星体}",
		       objs[i].mass, unit2km(objs[i].radius), !objs[i].obj?"(不可用)":"");
		if (objs+i == old) choice = i;
	}
	printf("(当前：%d)请输入要%s物体的id[0~%lu]：",
	       choice + 1, hint ? hint : "选择", rt->objs.len);
	char buf[100] = {};
	if (!fgets(buf, sizeof(buf)-1, stdin) || sscanf(buf, "%d", &choice) != 1) {
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
