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
	sva_free(&rt->huds);
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

void rt_dump_stars(Runtimedata_t *rt, FILE *output, bool no_inp)
{
	if (!rt || !rt->objs.ptr) return;
	if (!output) output = stdout;
	Star_t *objs = rt->objs.ptr;
	bool flg = !no_inp && isatty(fileno(output));
	if (flg) fprintf(output, "\e[0m\n\e[2K");
	fprintf(output, "===== 数据导出：各星体基本参数 =====\n");
	Color_t c = COLOR_WHITE;
	for (size_t i = 0; i < rt->objs.len; i++) {
		if (!objs[i].obj) continue;
		if (objs[i].obj->count_point > 1 && objs[i].obj->colors) {
			c = objs[i].obj->colors[0];
		}
		fprintf(output, " [%lu] %s (%gkg/r=%gkm) "
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
	fprintf(output, "游戏时间: T+%.1f s, 折合约 T+%.1f d\n", rt->gtime, sec2day(rt->gtime));
	fprintf(output, "操作累计dv: %.3f km/s\n", unit2km(rt->dv));
	fprintf(output, "初始化种子: %d\n", rt->seed);
	if (flg) {
		fprintf(output, "（回车返回）\n");
		kbhitGetchar();
		_getch();
	}
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
		if (sscanf(buf, "游戏时间: T+%lf s, 折合约 T+%*f d", &gtime)) {
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
	rt_dump_stars(rt, NULL, false);
	return true;
}

/* 期望的命令格式: 'key=value;' */
void rt_exec_cmd(Runtimedata_t *rt, char *cmd)
{
	if (!rt || !cmd) return;
	/* 期望运行时间 */
	double t = 0;
	/* 期望的速度改变量 */
	Vec_t dv = {};
	/* 速度、目标天体序号，顺序排列(与存档所带数字无关) */
	int idx_follow = 0, idx_dest = 0;
	char *cur = cmd;
	rt->throttle = -1;
	while ((cmd = cur)) {
		for (char *p = cur; p && *p; p++) {
			if (*p != ';' && *p != '\n') continue;
			*p = 0;
			cur = p + 1;
			break;
		}
		if (sscanf(cmd, " ts = %lg ", &rt->time_scale) == 1) {
			if (rt->time_scale <= 1e-4) rt->time_scale = 1;
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
	if (t <= 0) return;
	printf("预期运行时间：["GT2YHMS_FMT"]\n", gt2yhms_fmt(t));
	// throttle2accel
	return;
}

