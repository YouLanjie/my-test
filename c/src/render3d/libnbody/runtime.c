/**
 * @file        runtime.c
 * @author      Chglish
 * @date        2026-10-10
 * @brief       分离自r3d_rotate.c，运行时大结构体管理有关
 */

#include "./nbody.h"

/* 初始化后端、相机等资源 */
bool rt_setup(Runtimedata_t *rt, int mode)
{
	if (!rt) return false;
	int term_w = get_winsize_col() - 0;
	int term_h = get_winsize_row() - 2;
	if (term_w <= 0) term_w = 10;
	if (term_h <= 0) term_h = 10;
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
		rt_sync_cam_size_scale(rt);
		return true;
	}
	rt->backend = backend_create_utf8_256bit(term_w, term_h);
	rt->camera = camera_create();
#define CREATE_LINE(x,y,z, r,g,b) obj_set_color(obj_apply_shift(obj_create_line_from_point((Point_t){0,0,0}, (Point_t){x,y,z})), (Color_t){r,g,b,UI_ALPHA})
	rt->axis_helper = CREATE_LINE(kunit2unit(10),0,0, -1,0,0);
	obj_merge_and_free(rt->axis_helper, CREATE_LINE(0,kunit2unit(6),0, 0,-1,0));
	obj_merge_and_free(rt->axis_helper, CREATE_LINE(0,0,kunit2unit(3), 0,0,-1));
#undef CREATE_LINE

	if (!rt->backend || !rt->camera || !rt->axis_helper) {
		rt_cleanup(rt);
		return false;
	}
	rt->camera->width = term_w;
	rt->camera->height = term_h*2;
	rt->camera->position = (Vec_t){0, 0, km2unit(kunit2unit(20))};
	rt->camera->dept = km2unit(kunit2unit(100));
	rt->active_cam = rt->camera;
	rt->objs.size = sizeof(Star_t);
	rt->accel.size = sizeof(Vec_t);
	/* 设置帧率、运行倍率 */
	rt->fps = 40;
	rt->time_scale_limit = 2048;
	rt->time_scale = 1;
	return true;
}

/* 清理释放天体、后端、相机资源 */
void rt_cleanup(Runtimedata_t *rt)
{
	printf("\e[0m\n");
	if (!rt) return;
	if (rt->backend) rt->backend->destroy(rt->backend);
	if (rt->camera) camera_free(rt->camera);
	if (rt->axis_helper) obj_free(rt->axis_helper);
	rt->backend = NULL;
	rt->camera  = NULL;
	sva_free(&rt->logs);
	sva_free(&rt->buff);
	da_free(&rt->objs, star_free);
	da_free(&rt->accel, NULL);
}

void rt_sync_cam_size_scale(Runtimedata_t *rt)
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

void rt_switch_camera(Runtimedata_t *rt, Camera_t *ca)
{
	if (!rt || !ca) return;
	rt_sync_cam_size_scale(rt);
	ca->width = rt->active_cam->width;
	ca->height = rt->active_cam->height;
	ca->scale = rt->active_cam->scale;
	rt->active_cam = ca;
	return;
}

SVA_t *rt_dump_stars(Runtimedata_t *rt, SVA_t *dest)
{
	if (!rt || !rt->objs.ptr || !dest) return NULL;
#define printf(fmt, ...) sva_sprintfcat(dest, (fmt) __VA_OPT__(,) __VA_ARGS__)
	Star_t *objs = rt->objs.ptr;
	sva_clear(dest);
	printf("===== 数据导出：各星体基本参数 =====\n");
	Color_t c = COLOR_WHITE;
	for (size_t i = 0; i < rt->objs.len; i++) {
		if (!objs[i].obj) continue;
		if (objs[i].obj->count_point > 1 && objs[i].obj->colors) {
			c = objs[i].obj->colors[0];
		}
		printf(" [%lu] %s (%gkg/r=%gkm) "
		       "位置(km): {%.3f,%.3f,%.3f} "
		       "速度(km/s): {%.3f,%.3f,%.3f} "
		       "(#%.2X%.2X%.2X%.2X)\n", i+1,
		       objs[i].name.p ? objs[i].name.p : "{未命名星体}",
		       objs[i].mass, unit2km(objs[i].radius),
		       unit2km(objs[i].center.x),
		       unit2km(objs[i].center.y),
		       unit2km(objs[i].center.z),
		       unit2km(objs[i].speed.x),
		       unit2km(objs[i].speed.y),
		       unit2km(objs[i].speed.z),
		       c.r, c.g, c.b, c.a);
	}
	printf("游戏时间: T+%lfs, [T+"GT2YHMS_FMT"]\n",
	       rt->gtime, gt2yhms_fmt(rt->gtime));
	printf("操作累计dv: %.3f km/s\n", unit2km(rt->dv));
	printf("初始化种子: %d\n", rt->seed);
#undef printf
	return dest;
}

/* 导入天体场景（暂缺失自转、颜色等配置） */
bool rt_load_from_dumped_text(Runtimedata_t *rt, const char *filename)
{
	int ret = 0, count = 0;
	char buf[1024] = {};
	FILE *fp = fopen(filename, "r");
	if (!fp) return false;

	size_t idx = 0;
	char name[1024] = {};
	double mass = 0;
	double radius = 0;
	double Px = 0, Py = 0, Pz = 0;
	double Vx = 0, Vy = 0, Vz = 0;
	double gtime = 0;
	double dv = 0;
	uint32_t seed = 0;
	Color_t c;
	Star_t star;
	while (fgets(buf, sizeof(buf), fp)) {
		c = COLOR_WHITE;
		ret = sscanf(buf, " [%lu] %s (%lgkg/r=%lgkm) 位置(km): {%lf,%lf,%lf} 速度(km/s): {%lf,%lf,%lf} (#%2hhX%2hhX%2hhX%2hhX)",
			     &idx, name, &mass, &radius, &Px, &Py, &Pz, &Vx, &Vy, &Vz,
			     &c.r, &c.g, &c.b, &c.a);
		if (ret >= 10) {
			count++;
			star = star_create(name, mass, radius, (Vec_t){Px,Py,Pz}, (Vec_t){Vx,Vy,Vz}, NULL);
			if (ret >= 14) obj_set_color(star.obj, c);
			da_append(&rt->objs, &star);
			continue;
		}
		if (sscanf(buf, "游戏时间: T+ %lf s", &gtime)) {
		} else if (sscanf(buf, "操作累计dv: %lf km/s", &dv)) {
		} else sscanf(buf, "初始化种子: %u", &seed);
	}
	fclose(fp);
	if (count <= 0) return false;

	if (seed) rt->seed = seed;
	rt->dv = km2unit(dv);
	rt->gtime = gtime;
	rt->pause = true;

	star_sync_cam_config(rt);
	rtlog(rt, "成功载入存档(seed=%u,dv=%gkm/s)", seed, dv);
	printf("[INFO] 共导入%d个天体\n", count);
	rt_dump_stars(rt, &rt->buff);
	printf("%.*s\n", (int)rt->buff.len, rt->buff.p);
	return true;
}

void rt_logevent(Runtimedata_t *rt)
{
	if (!rt) return;
	rt->loop_stat.ev_swap_rpra = false;
	rt->loop_stat.obp = (typeof(rt->loop_stat.obp)){};

	if (rt->follow) rt->loop_stat.obp = phy_get_orbital_parameters(rt->follow, rt->about_point);
	if (rt->follow && rt->loop_stat.follow == rt->follow
	    && rt->loop_stat.about_point && rt->loop_stat.about_point->name.p
	    && rt->loop_stat.about_point != rt->about_point) {
		format_orbital_parameters(rt, &rt->buff, rt->loop_stat.obp);
		rtlog(rt, "天体'%s'被'%s'捕获(%s)(原运行在'%s'),累计dv:%.3gkm/s",
		      rt->follow->name.p, rt->about_point->name.p, rt->buff.p,
		      rt->loop_stat.about_point->name.p, unit2km(rt->dv));
		rt->need_notification++;
	}
	if (!rt->about_point) return;
	const bool cond1 = rt->follow && rt->loop_stat.follow == rt->follow && rt->about_point;
	const bool cond2 = cond1 ?    /* 变轨时近远地点高度交换(e变化方向变化) */
		(rt->throttle_on&1)
		&& (rt->loop_stat.obp.e < 1 && rt->loop_stat.e1 < 1)
		&& (rt->loop_stat.e2 > rt->loop_stat.e1 && rt->loop_stat.e1 < rt->loop_stat.obp.e) : false;
	const bool cond3 = (rt->loop_stat.throttle_on^rt->throttle_on)&1      /* 油门开关 */
		|| (rt->throttle_on&1 && (rt->loop_stat.throttle_on^rt->throttle_on)&0b10);
	const bool cond4 = cond1 ? cond2    /* 触发以下任意事件 */
		|| cond3
		|| ((rt->loop_stat.e1-1)*(rt->loop_stat.obp.e-1)<0) : false;    /* 轨道类型改变 */
	if (cond1 && cond4) {
		if (cond3) {
			rtlog(rt, "油门切换至: %s向推力 %d%% %s",
			      rt->throttle_on&0b10?"反":"正", rt->throttle, rt->throttle_on&1?"开":"关");
		}
		if (cond2) {
			rtlog(rt, "近远地点高度交换");
			if (rt->time_scale >= 2) rt->need_notification++;
		}
		format_orbital_parameters(rt, &rt->buff, rt->loop_stat.obp);
		rtlog(rt, "%s[%s]:%s(%s),dv:%.3gkm/s",
		      rt->follow->name.p, rt->about_point->name.p,
		      rt->loop_stat.obp.typ, rt->buff.p, unit2km(rt->dv));
	}
	rt->loop_stat.e2 = rt->loop_stat.e1;
	rt->loop_stat.e1 = rt->loop_stat.obp.e;
	rt->loop_stat.throttle_on = rt->throttle_on;
	rt->loop_stat.ev_swap_rpra = cond2;

	rt->loop_stat.about_point = rt->about_point;
	rt->loop_stat.follow = rt->follow;
}

/* 返回值：
 * 1<<0 倍速减半
 * 1<<1 倍速降为1
 * 1<<2 暂停游戏 */
int rt_format_huds(Runtimedata_t *rt, SVA_t *dest, struct orbital_parameters ret)
{
	if (!rt || !dest) return 0;
	sva_clear(dest);

#define printf(fmt, ...) sva_sprintfcat(dest, (fmt) __VA_OPT__(,) __VA_ARGS__)
	/* 基础信息 */
	if (rt->hud_time_simple_format) {
		printf("[T+%.1fd,", sec2day(rt->gtime));
	} else {
		printf("[T+"GT2YHMS_FMT" |", gt2yhms_fmt(rt->gtime));
	}
	printf(" x%g, %c%s%d%%%c, dv:%.3gkm/s]",
	       rt->time_scale,
	       rt->throttle_on&1?'[':':',
	       rt->throttle_on&0b10?"-":"",
	       rt->throttle,
	       rt->throttle_on&1?']':':',
	       unit2km(rt->dv));
	if (rt->follow) {
		/* 控制天体基本信息 */
		const char hint[2][4] = { "<{[", ">}]", };
		const uint8_t hintc = rt->follow==rt->look_to?0:(rt->rotate_cam_with_spd?1:2);
		printf(" | %s%c%s%c (%.3f km/s)",
		       rt->follow->name.p ? rt->follow->name.p : "Unknow",
		       hint[0][hintc],
		       rt->about_point->name.p ? rt->about_point->name.p : "Unknow",
		       hint[1][hintc],
		       unit2km(vec_len(vec_sub(rt->follow->speed, rt->about_point->speed))));
	}
	if (!rt->follow || !rt->destination_to) return 0;

	/* 控制天体Ra,Rp情况 */
	const char *hint1 =
		ret.rp<=rt->about_point->radius ? "\e[31m"
		: (ret.rp<=1.2*rt->about_point->radius ? "\e[33m" : "");
	const char *hint2 = hint1[0] ? "\e[0m" : "";
	sva_replace(dest, sv_from_lstr("\n"), sv_from_lstr("\\n"));
	printf(" Rp:%s%.1fkm%s Ra:%.1fkm\n", hint1, unit2km(ret.rp), hint2, unit2km(ret.ra));

	/* 距离目标天体情况 */
	const Vec_t dist = vec_sub(rt->destination_to->center, rt->follow->center);
	const Vec_t dv = vec_sub(rt->follow->speed, rt->destination_to->speed);
	const double vertical_speed = vec_point_product(vec_direct(dist), dv);
	// 对于打速度印值， <0 表靠近， >0 表远离
	printf("距%s %.1f km (%.3f km/s)",
	       rt->destination_to->name.p ? rt->destination_to->name.p : "Unknow",
	       unit2km(vec_len(dist)),
	       unit2km(-vertical_speed));

	/* 安全保护 */
	int ret_code = 0;
	if (!rt->ignore_safety && !rt->pause
	    && (ret.rp<=rt->about_point->radius && vertical_speed > 0 && rt->time_scale > 1))
		ret_code |= 0b110;

	/* 星际航行参数计算 */
	Star_t *dest_about_point = NULL;
	int8_t mode = rt->about_point != rt->destination_to && rt->about_point->mass > 1e-3 &&
		(dest_about_point=star_get_about_point(rt, rt->destination_to)) == rt->about_point;

	if (!mode && dest_about_point && dest_about_point->mass > 1e-3 &&
	    star_get_about_point(rt, rt->about_point) == dest_about_point) {
		/* 启用二阶霍曼轨道计算 */
		mode = 2;
	}
	if (!mode && ret.e > 1 && ret.Tp < 2*30*24*60*60) {
		/* 正在环绕目标天体 */
		printf(" L:["GT2YHMS_FMT"]", gt2yhms_fmt(ret.Tp));
		if (!rt->ignore_safety
		    && rt->time_scale > 1
		    && rt->destination_to==rt->about_point && ret.e > 1
		    && vertical_speed > 0 && ret.Tp - rt->time_scale < 0) {
			ret_code |= 1;
			if (ret.Tp <= 30) ret_code |= 0b111;
		}
	}
	if (!mode) return ret_code;

	/* 简易霍曼转移轨道计算 */
	/* 共心状态 */
	struct orbital_parameters ret2 = phy_get_orbital_parameters(rt->destination_to, dest_about_point);
	Vec_t u = vec_direct(vec_cross_product(ret.u, ret2.u));
	/* 相对当前中心速度 */
	Vec_t v = vec_sub(rt->follow->speed, rt->about_point->speed);
	double deg_cross = vec_point_product(vec_direct(v),u);
	deg_cross = 90 - acos(deg_cross)/M_PI*180.;
	struct hohmann_orbital_parameters ret3 =
		phy_get_hohmann_orbit_theta(rt->follow, rt->destination_to, dest_about_point, NULL);
	printf(" %.1f°(%.1f)°/%.1f°",
	       acos(vec_point_product(ret.u, ret2.u))/(M_PI)*180.,
	       deg_cross,
	       ret3.theta/M_PI*180.);
	if (mode == 2) {
		/* 二阶偏角 */
		ret3 = phy_get_hohmann_orbit_theta(rt->follow, rt->destination_to, dest_about_point, rt->about_point);
		printf(" [%.1f°]", ret3.theta/M_PI*180.);
	}
	const double time_left = rt->throttle ?
		(rt->throttle_on&0b10?-1:1)*(ret3.v-vec_len(v))/throttle2accel(rt->throttle): 0;
	if (fabs(ret3.theta/M_PI*180.) < 10) {
		printf(" L:%.2fs", time_left);
		if (rt->throttle_on&1 && rt->time_scale>1 && time_left-rt->time_scale < 0) {
			/* 需要降低倍速(减半) */
			ret_code |= 1;
		}
	}

	/* 二体简化模型近似计算最近值(ai生成，仅供参考) */
	struct close_approach_t ca;
	if (mode != 2 && (ca=phy_two_body_closest_approach(rt->follow, rt->destination_to, rt->about_point)).valid) {
		printf(" ETA:%.2fd(%gkm)", sec2day(ca.time), unit2km(ca.distance));
	}
	return ret_code;
#undef printf
}

/* 期望的命令格式: 'key=value;' */
void rt_exec_cmd(Runtimedata_t *rt, char *cmd)
{
	if (!rt || !cmd) return;
	/* 期望运行时间 */
	double t = 0;
	double time_scale = 1;
	/* 期望的速度改变量 */
	Vec_t dv = {};
	/* 速度、目标天体序号，顺序排列(与存档所带数字无关) */
	int idx_follow = 0, idx_dest = 0;
	char *cur = cmd;
	rt->throttle_on = 0;
	rt->throttle = -1;
	while ((cmd = cur) && *cur) {
		for (char *p = cur; p && *p; p++) {
			if (*p != ';' && *p != '\n') continue;
			*p = 0;
			cur = p + 1;
			break;
		}
		if (sscanf(cmd, " ts = %lg ", &time_scale) == 1) {
			if (time_scale <= 1e-4) time_scale = 1;
		} else if (sscanf(cmd, " tsl = %lg ", &rt->time_scale_limit) == 1) {
			if (rt->time_scale_limit < 1) rt->time_scale_limit = 1024;
		} else if (sscanf(cmd, " t = %lg ", &t) == 1) {
			if (t < 0) t = 0;
		} else if (sscanf(cmd, " thr = %hhu ", &rt->throttle) == 1) {
		} else if (sscanf(cmd, " dv = { %lg , %lg , %lg } ", &dv.x, &dv.y, &dv.z) == 3) {
		} else if (sscanf(cmd, " F = %d ", &idx_follow) == 1) {
		} else if (sscanf(cmd, " D = %d ", &idx_dest) == 1) {
		}
	}
	// if (t <= 0) return;
	rt->follow = da_get(&rt->objs, idx_follow-1);
	rt->destination_to = da_get(&rt->objs, idx_dest-1);
	printf("预期运行时间：["GT2YHMS_FMT"] (倍速:x%g/%g)\n",
	       gt2yhms_fmt(t), time_scale, rt->time_scale_limit);
	t += rt->gtime;
	const double t0 = get_nowtime();
	if (vec_len(dv) > 0) {
		if (!rt->throttle) rt->throttle = 1;
		rt->time_scale = 1;
		rt->throttle_on = 1;
		const double T = vec_len(dv)/throttle2accel(rt->throttle) + rt->gtime;
		while (rt->gtime < T) phy_update(rt);
		rt->throttle_on = 0;
	}
	rt->time_scale = time_scale;
	while (rt->gtime < t) phy_update(rt);

	printf("[INFO] 模拟结束\n航行日志：\n%s", rt->logs.len ? rt->logs.p : "- 空 -\n");
	rt_dump_stars(rt, &rt->buff);
	printf("%.*s", (int)rt->buff.len, rt->buff.p);
	if (rt->follow && rt->destination_to) {
		voyage_helper(rt, &rt->buff, rt->destination_to);
		printf("航程信息：\n%.*s", (int)rt->buff.len, rt->buff.p);
	}
	printf("[INFO] 程序实际运行挂钟时间：%.1f s\n", get_nowtime()-t0);
	return;
}

