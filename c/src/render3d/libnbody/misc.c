/**
 * @file        misc.c
 * @author      Chglish
 * @date        2026-10-10
 * @brief       分离自r3d_rotate.c，杂项辅助函数
 */

#include "./nbody.h"

double get_nowtime()
{
	struct timespec t = {};
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec/1e9;
}

/**
 * @brief 分页器
 *
 * @param headline 标题
 * @param content 内容
 * @param mode 模式(-1显示最后一页，-2不翻页)
 */
void print_pager(const char *headline, SV_t content, int mode)
{
	if (!content.len||!content.p) return;
	SV_t line = {};
	SV_t left = content;
	headline = headline?headline:"请输入文本";
	printf("\e[0m\n\e[J==== %s ====\n", headline);
	const int pager_lines = 25;
	int total_lines = sv_countlines(content);
	int count = 0, start = 1;

	const double delay = 0.6;
	double t0 = get_nowtime(), t1;
	while (sv_forline(&line, &left)) {
		if (mode!=-1 || total_lines-count < pager_lines)
			printf("> %.*s\n", (int)line.len, line.p);
		else start++;
		count++;
		if ((mode<0||count%pager_lines != 0) && left.len != 0) continue;
		if (left.len == 0) printf("\e[32m-- 内容结束\e[0m\n");
		printf("\e[2m-- [分页器] %d-%d/%d 回车继续,c不翻页打印,p/u上翻,q退出\e[0m\n",
		       start, count, total_lines);
		kbhitGetchar();
		int ch = 0, ch2 = 0;
		/* 延迟一段时间防止误触
		 * 效果取决于终端设置的连按延迟 */
		while ((ch=_getch())>0 && (t1 = get_nowtime()) < t0+delay) {
			if (!ch2 || ch == ch2) t0 = t1;
			ch2 = ch;
		}
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

void format_orbital_parameters(Runtimedata_t *rt, SVA_t *dest, struct orbital_parameters ret)
{
	if (!rt || !rt->follow || !dest) return;
	const Vec_t dv = 
		vec_mul(vec_sub(rt->follow->speed, rt->about_point?rt->about_point->speed:(Point_t){}),
			unit2km(1));
	const Point_t center = rt->about_point&&rt->about_point->obj?rt->about_point->center:(Point_t){};
	sva_sprintf(dest, "e=%.3g,a=%.3gkm,θ=%.3g,r=%.3gkm,v=%.3gkm/s,⟂v=%.3gkm/s",
		    ret.e, unit2km(ret.a), acos(vec_point_product(ret.u, (Vec_t){0,0,1}))/M_PI*180.,
		    unit2km(ret.r), vec_len(dv),
		    -vec_point_product(vec_direct(vec_sub(center, rt->follow->center)), dv)
		    );
	return;
}

#define printf(fmt, ...) sva_sprintfcat(dest, (fmt) __VA_OPT__(,) __VA_ARGS__)
static void print_starinfo(SVA_t *dest, Star_t *star, struct orbital_parameters dat)
{
	if (!dest || !star || !star->obj) return;
	const double Tp = dat.Tp == INFINITY ? 0 : dat.Tp;
	const double Ta = dat.Ta == INFINITY ? 0 : dat.Ta;
	printf("围绕天体: %s\n", star->name.p);
	printf("轨道类型: %s (%.3f) \t| 周期: %g d\n", dat.typ, dat.e, sec2day(dat.T));
	printf("当前高度: %.1f km \t| 半长轴: %.1f km\n", unit2km(dat.r), unit2km(dat.a));
	printf("近地点: %.1f km \t| 远地点: %.1f km\n", unit2km(dat.rp), unit2km(dat.ra));
	printf("距离近地点: ["GT2YHMS_FMT"] %.1f km\n", gt2yhms_fmt(Tp), unit2km(dat.d_rp));
	printf("距离远地点: ["GT2YHMS_FMT"] %.1f km\n", gt2yhms_fmt(Ta), unit2km(dat.d_ra));
	printf("倾角: %.3f deg\n", acos(vec_point_product(dat.u, (Vec_t){0,0,1}))/(2*M_PI)*360.);
}

SVA_t *voyage_helper(Runtimedata_t *rt, SVA_t *dest, Star_t *to)
{
	if (!rt || !dest) return NULL;
	Star_t *from = rt->follow;
	while (!from && (from = star_choose(rt, "正在驾驶的", NULL)) == NULL)
		printf("重试...\n");
	while (!to && (to = star_choose(rt, "要驶入的", NULL)) == NULL)
		printf("重试...\n");

	sva_clear(dest);
	if (from->mass > to->mass)
		printf("[TIPS] from比to重，结果可能不正确\n");
	if (from == to)
		printf("[WARN] 怎么选了个一样的？\n");
	const Vec_t direct = vec_sub(to->center, from->center);
	double distance = vec_len(direct);
	if (distance <= 0) distance = 1e-20;
	const double speed = sqrt(G_unit*to->mass/distance);
	printf("========== 基础信息 ==========\n");
	printf("'%s' -> '%s'\n",
	       from->name.p ? from->name.p : "Unknow",
	       to->name.p ? to->name.p : "Unknow");
	printf("距离：%.1f km\n", unit2km(distance));
	const Vec_t dv = vec_sub(from->speed, to->speed);
	const double vertical_speed = vec_point_product(vec_direct(direct), dv);
	printf("相对速度：%.1f km | 垂直速度： %.1f km\n", unit2km(vec_len(dv)), unit2km(vertical_speed));
	// printf("航向：{%.1f, %.1f, %.1f}\n", direct.x, direct.y, direct.z);
	printf("理想圆轨线速度：%g km/s, 角速度：%g rad/s\n", unit2km(speed), speed/distance);
	const double T = 2*M_PI/(speed/distance);
	printf("理想圆轨周期：%.1f s | ["GT2YHMS_FMT"]\n", T, gt2yhms_fmt(T));

	/* 驾驶天体中心 */
	Star_t *s1 = star_get_about_point(rt, from);
	if (!s1) s1 = to;
	struct orbital_parameters dat = phy_get_orbital_parameters(from, s1);
	printf("====== 当前(%s)轨道情况 ======\n", from->name.p);
	print_starinfo(dest, s1, dat);

	/* 目标天体中心 */
	Star_t *s2 = star_get_about_point(rt, to);
	/* 驾驶天体中心的中心 */
	Star_t *s3 = s1 != s2 ? star_get_about_point(rt, s1) : NULL;
	if ((s1 == s2 || s3 == s2) && s1 != to && s2 != from) {
		struct orbital_parameters dat2 = phy_get_orbital_parameters(to, s2);
		printf("====== 目标(%s)共轨情况 ======\n", to->name.p);
		print_starinfo(dest, s2, dat2);
		printf("相对倾角: %.4f deg\n", acos(vec_point_product(dat2.u, dat.u))/M_PI*180.);
		struct hohmann_orbital_parameters ret = phy_get_hohmann_orbit_theta(from, to, s2, NULL);
		printf("====== 霍曼转移轨道数据 ======\n");
		printf("半周期: ["GT2YHMS_FMT"]\n", gt2yhms_fmt(ret.T));
		printf("提前角度: %.3g deg\n", ret.expect_theta/M_PI*180.);
		printf("距点火点: %.3g deg\n", ret.theta/M_PI*180.);
		printf("期望速度: %.3g km/s\n", unit2km(ret.v));
		struct close_approach_t ca;
		if (s1 != s2) {
			ret = phy_get_hohmann_orbit_theta(from, to, s2, s1);
			printf("====== 次级霍曼轨道数据 ======\n");
			printf("提前角度: %.3g deg\n", ret.expect_theta/M_PI*180.);
			printf("距点火点: %.3g deg\n", ret.theta/M_PI*180.);
			printf("期望速度: %.3g km/s\n", unit2km(ret.v));
		} else if ((ca = phy_two_body_closest_approach(from, to, s2)).valid) {
			printf("实验性计算: '%s' 与 '%s' 将在 %.2f 天后接近至 %.1f km\n",
			       from->name.p, to->name.p, sec2day(ca.time), unit2km(ca.distance));
			printf("[WARN] 该数值仅在二体、三体近似时较为准确\n");
		}
	}
	return dest;
}
#undef printf

void set_camera_forward(Runtimedata_t *rt)
{
	if (!rt) return;
	Camera_t *ca = rt->active_cam;
	if (!ca || !rt->follow) return;
	char buf[100] = {};
	Vec_t v1 = {}, v2 = {};
	printf("\e[0m\n==== 设定相机朝向 ====\n");
	// printf("参考数据:\n- [%f,%f,%f]\n", v1.x, v1.y, v1.z);
	printf("请输入相机要看向的方向，留空跳过(格式：x,y,z)：\n");
	if (!fgets(buf, sizeof(buf)-1, stdin)) return;
	for (char *p = buf; *p; p++) if (strchr("{}[]()<>,", *p)) *p = ' ';
	if (sscanf(buf, " %lg %lg %lg", &v2.x, &v2.y, &v2.z) == 3 && vec_len(v2) != 0) {
		v1 = vec_direct(ca->forward);
		v2 = vec_direct(v2);
		camera_rotate_about_point(ca, rt->follow->center, vec_cross_product(v1, v2),
					  acos(vec_point_product(v1, v2)));
	}

	printf("请输入相机竖直方向，留空跳过(格式：x,y,z)：\n");
	if (!fgets(buf, sizeof(buf)-1, stdin)) return;
	for (char *p = buf; *p; p++) if (strchr("{}[]()<>,", *p)) *p = ' ';
	if (sscanf(buf, " %lg %lg %lg", &v2.x, &v2.y, &v2.z) != 3) return;
	if (vec_len(v2) == 0) return;
	Vec_t v3 = vec_cross_product((v1=vec_direct(ca->up)), (v2=vec_direct(v2)));
	if (vec_len(v3) == 0) return;
	v2 = vec_direct(vec_cross_product(v2, v3));
	camera_rotate_about_point(ca, rt->follow->center, vec_cross_product(v1, v2),
				  acos(vec_point_product(v1, v2)));
}

