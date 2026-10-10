/**
 * @file        physics.c
 * @author      Chglish
 * @date        2026-10-10
 * @brief       从原r3d_rotate.c分离出的文件，负责物理计算、二体近似模拟等
 */

#include "./nbody.h"

/* 计算加速度，顺便调用碰撞处理 */
void phy_compute_acceleration(Runtimedata_t *rt)
{
	if (!rt) return;
	while (rt->accel.len < rt->objs.len) {
		da_append(&rt->accel, &(Vec_t){});
		rt->accel_last_upeate_time = -1;
	}
	if (rt->accel_last_upeate_time > rt->gtime || !rt->accel.ptr)
		return;
	Vec_t *acc = rt->accel.ptr;
	if (!acc) return;

	Star_t * const objs = rt->objs.ptr;
	const size_t len = rt->objs.len;
	Vec_t diff;
	double r2 = 0;
	double a = 0;
	memset(acc, 0, rt->accel.len*rt->accel.size);
	for (size_t i = 0; i < len; i++) {
		if (!objs[i].obj) continue;
		// 对于每个天体
		for (size_t j = i+1; j < len; j++) {
			if (!objs[j].obj) continue;
			// 计算它与它往后所有天体的加速度
			diff = vec_sub(objs[i].center, objs[j].center);    /* j -> i */
			r2 = (pow2(diff.x) + pow2(diff.y) + pow2(diff.z));
			if (r2 > 0) a = G_unit/r2;
			diff = vec_direct(diff);
			acc[i] = vec_add(acc[i], vec_mul(diff, -a * objs[j].mass));
			acc[j] = vec_add(acc[j], vec_mul(diff,  a * objs[i].mass));

			/* 检查碰撞 */
			if (r2 >= pow(objs[i].radius + objs[j].radius, 2))
				continue;
			star_star_impact(rt, objs+i, objs+j);
			phy_compute_acceleration(rt);
			return;
		}
	}
	return;
}

/* ai生成的RK4用辅助代码 */
static void phy_compute_acceleration_rk4(Runtimedata_t *rt, Vec_t *pos, Vec_t *acc)
{
	int n = (int)rt->objs.len;
	Star_t *objs = rt->objs.ptr;
	memset(acc, 0, sizeof(Vec_t) * n);	// 清空

	// 1. 引力加速度（两两相互作用）
	for (int i = 0; i < n; i++) {
		for (int j = i + 1; j < n; j++) {
			Vec_t diff = vec_sub(pos[i], pos[j]);	// 从j指向i的向量 (unit)
			double r2 = (pow2(diff.x) + pow2(diff.y) + pow2(diff.z));
			if (r2 < 1e-18)
				continue;
			// double r = sqrt(r2);
			Vec_t dir = vec_direct(diff);	// 单位方向向量
			double a = G_unit / r2;	// 与原始公式一致
			acc[i] = vec_add(acc[i], vec_mul(dir, -a * objs[j].mass));
			acc[j] = vec_add(acc[j], vec_mul(dir, a * objs[i].mass));
		}
	}

	// 2. 推力加速度（仅作用于跟随天体）
	if ((rt->throttle_on & 1) && rt->follow) {
		int idx = rt->follow - objs;
		if (idx >= 0 && idx < n) {
			double thrust_acc = throttle2accel(rt->throttle);
			if (rt->throttle_on & 0b10) thrust_acc = -thrust_acc;	// 反向
			Vec_t dir = vec_direct(rt->active_cam->forward);
			acc[idx] = vec_add(acc[idx], vec_mul(dir, thrust_acc));
		}
	}
}

/* ai生成的RK4法物理更新 */
static void phy_update_step_rk4(Runtimedata_t *rt, double time_scale)
{
	if (!rt || rt->objs.len == 0 || time_scale == 0)
		return;

	double dt = time_scale / rt->fps;	// 单步物理时间（秒）
	int n = (int)rt->objs.len;
	Star_t *objs = rt->objs.ptr;

	// 保存当前状态
	Vec_t pos0[n], vel0[n];
	for (int i = 0; i < n; i++) {
		pos0[i] = objs[i].center;
		vel0[i] = objs[i].speed;
	}

	// 分配临时数组（用于RK4各阶段）
	Vec_t k1v[n], k1p[n], k2v[n], k2p[n], k3v[n], k3p[n], k4v[n], k4p[n];
	Vec_t tmp_pos[n], tmp_vel[n];

	// ---- 阶段1 ----
	phy_compute_acceleration_rk4(rt, pos0, k1v);	// k1v = a(t0, pos0, vel0)
	for (int i = 0; i < n; i++)
		k1p[i] = vel0[i];	// k1p = vel0

	// ---- 阶段2 ----
	for (int i = 0; i < n; i++) {
		tmp_pos[i] = vec_add(pos0[i], vec_mul(k1p[i], 0.5 * dt));
		tmp_vel[i] = vec_add(vel0[i], vec_mul(k1v[i], 0.5 * dt));
	}
	phy_compute_acceleration_rk4(rt, tmp_pos, k2v);
	for (int i = 0; i < n; i++)
		k2p[i] = tmp_vel[i];

	// ---- 阶段3 ----
	for (int i = 0; i < n; i++) {
		tmp_pos[i] = vec_add(pos0[i], vec_mul(k2p[i], 0.5 * dt));
		tmp_vel[i] = vec_add(vel0[i], vec_mul(k2v[i], 0.5 * dt));
	}
	phy_compute_acceleration_rk4(rt, tmp_pos, k3v);
	for (int i = 0; i < n; i++)
		k3p[i] = tmp_vel[i];

	// ---- 阶段4 ----
	for (int i = 0; i < n; i++) {
		tmp_pos[i] = vec_add(pos0[i], vec_mul(k3p[i], dt));
		tmp_vel[i] = vec_add(vel0[i], vec_mul(k3v[i], dt));
	}
	phy_compute_acceleration_rk4(rt, tmp_pos, k4v);
	for (int i = 0; i < n; i++)
		k4p[i] = tmp_vel[i];

	// ---- 最终更新位置和速度 ----
	for (int i = 0; i < n; i++) {
		Vec_t dp = vec_add(vec_add(k1p[i], vec_mul(k2p[i], 2.0)),
				   vec_add(vec_mul(k3p[i], 2.0), k4p[i])
		    );
		objs[i].center = vec_add(pos0[i], vec_mul(dp, dt / 6.0));

		Vec_t dv = vec_add(vec_add(k1v[i], vec_mul(k2v[i], 2.0)),
				   vec_add(vec_mul(k3v[i], 2.0), k4v[i])
		    );
		objs[i].speed = vec_add(vel0[i], vec_mul(dv, dt / 6.0));
	}

	// ---- 自转（欧拉更新，不影响平动） ----
	for (int i = 0; i < n; i++) {
		obj_rotate(objs[i].obj, objs[i].self_rotate,
			   objs[i].self_omiga * dt);
	}

	// ---- 累计推进Δv（用于显示） ----
	if ((rt->throttle_on & 1) && rt->follow) {
		double thrust_acc = throttle2accel(rt->throttle);
		if (rt->throttle_on & 0b10)
			thrust_acc = -thrust_acc;
		rt->dv += fabs(thrust_acc * dt);
	}
	// ---- 碰撞检测与合并（与原逻辑相同） ----
	for (int i = 0; i < n; i++) {
		for (int j = i + 1; j < n; j++) {
			Vec_t diff =
			    vec_sub(objs[i].center, objs[j].center);
			double r2 = pow2(diff.x) + pow2(diff.y) + pow2(diff.z);
			double sum_r = objs[i].radius + objs[j].radius;
			if (r2 < sum_r * sum_r) {
				star_star_impact(rt, &objs[i], &objs[j]);
				return;
			}
		}
	}
}

static void phy_update_step(Runtimedata_t *rt, double time_scale)
{
	if (!rt || rt->objs.len == 0 || time_scale == 0) return;
	time_scale /= rt->fps;
	phy_compute_acceleration(rt);
	if (!rt->accel.ptr || rt->accel.len < rt->objs.len) return;
	Star_t *objs = rt->objs.ptr;
	Vec_t *acc = rt->accel.ptr;
	Vec_t diff;
	for (size_t i = 0; i < rt->objs.len; i++) {
		objs[i].speed = vec_add(objs[i].speed, vec_mul(acc[i], time_scale));
		diff = vec_mul(objs[i].speed, time_scale);
		objs[i].center = vec_add(objs[i].center, diff);
		objs[i].cam.position = vec_add(objs[i].cam.position, diff);
		obj_rotate(objs[i].obj, objs[i].self_rotate, objs[i].self_omiga*time_scale);
	}
	if (rt->throttle_on&1 && rt->throttle && rt->follow) {
		double accel = throttle2accel(rt->throttle) * time_scale * (rt->throttle_on&0b10?-1:1);
		rt->follow->speed = vec_add(rt->follow->speed, vec_mul(vec_direct(rt->active_cam->forward), accel));
		rt->dv += fabs(accel);
	}
}

void phy_update(Runtimedata_t *rt)
{
	if (!rt || rt->objs.len == 0) return;
	Vec_t v1 = rt->follow&&rt->rotate_cam_with_spd ? rt->follow->speed : (Vec_t){};
	double time_scale = rt->time_scale;
	while ((time_scale-=rt->time_scale_limit) > 0) {
		if (rt->use_rk4) phy_update_step_rk4(rt, rt->time_scale_limit);
		else phy_update_step(rt, rt->time_scale_limit);
	}
	if (rt->use_rk4) phy_update_step_rk4(rt, time_scale+rt->time_scale_limit);
	else phy_update_step(rt, time_scale+rt->time_scale_limit);

	if (rt->follow) rt->about_point = star_get_about_point(rt, rt->follow);
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
						  rt->follow->center,
						  vec_cross_product(v1, v2),
						  acos(vec_point_product(v1, v2)));
		}
	}
	star_sync_position(rt);
	rt->accel_last_upeate_time = rt->gtime;
	rt->gtime += rt->time_scale/rt->fps;
	return;
}

struct orbital_parameters phy_get_orbital_parameters(Star_t *ship, Star_t *center)
{
	struct orbital_parameters dat = {.typ="NULL"};
	if (!ship || !center || !ship->obj || !center->obj) return dat;

	const Vec_t v = vec_sub(ship->speed, center->speed);
	const Vec_t r = vec_sub(ship->center, center->center);
	const double mu = center->mass*G_unit;
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
	dat.point_rp = vec_add(vec_mul(vec_direct(e), dat.rp), center->center);
	dat.point_ra = vec_add(vec_mul(vec_direct(e), -dat.ra), center->center);
	dat.d_rp = vec_len(vec_sub(dat.point_rp, ship->center));
	dat.d_ra = vec_len(vec_sub(dat.point_ra, ship->center));
	dat.u = vec_direct(vec_cross_product(r, v));
	dat.T = 2*M_PI*(dat.a)*sqrt(dat.a/(G_unit*(ship->mass+center->mass)));

	char *typ = "椭圆轨道";
	if (dat.e > 1) typ = "双曲线轨道";
	else if (fabs(dat.e - 1) < 1e-5) typ = "抛物线轨道";
	else if (dat.e < 1e-5) typ = "圆轨道";
	dat.typ = typ;

	if (dat.T != dat.T) {
		dat.T = INFINITY;
		dat.Tp = INFINITY;
		dat.Ta = INFINITY;
		// dat.ra = INFINITY;
	}
	/* 舍弃正圆 */
	if (dat.e < 1e-12) return dat;

	/* 下面是ai给的算法(计算到达近地点和远地点的时间) */
	const double h = vec_len(vec_cross_product(r, v));	// 角动量大小
	const double v_r = vec_point_product(r, v) / dat.r;	// 径向速度

	/* 双曲线轨道 */
	if (dat.e > 1.0) {
		/* 舍弃正远离 */
		if (v_r > 0) return dat;
		const double a_h = -dat.a;	// 取正半长轴
		// 1. 计算双曲偏近点角 F（利用 asinh 直接确定符号）
		const double sinhF = v_r * dat.r / (dat.e * sqrt(mu * a_h));
		const double F = asinh(sinhF);	// 反双曲正弦，保留正负号
		// 2. 计算双曲平近点角 M_h
		const double M_h = dat.e * sinhF - F;
		// 3. 双曲平均角速度 n
		const double n = sqrt(mu / (a_h * a_h * a_h));
		// 4. 判断未来是否会经过近地点
		dat.Tp = -M_h / n;	// M_h为负，取反得正
		return dat;
	}

	/* 真近点角 θ */
	const double theta_cos = vec_point_product(r, e) / (dat.r * dat.e);
	const double theta_sin = v_r * h / (mu * dat.e);
	/* 偏近点角 E */
	const double cosE = (dat.e + theta_cos) / (1.0 + dat.e * theta_cos);
	const double sinE = (sqrt(1.0 - dat.e * dat.e) * theta_sin) / (1.0 + dat.e * theta_cos);
	const double E = fmod(atan2(sinE, cosE)+2*M_PI, 2*M_PI);
	/* 平近点角 M */
	double M = fmod((E - dat.e*sinE) + 2*M_PI, 2*M_PI);

	/* 平均角速度 n 和周期 T */
	const double n = sqrt(mu / (dat.a * dat.a * dat.a));
	const double T = 2.0 * M_PI / n;
	dat.T = T;		// 覆盖原 T 计算（更准确）

	/* 到下一个近地点的时间 */
	if (fabs(M) < 1e-12)
		dat.Tp = T;
	else
		dat.Tp = (2.0 * M_PI - M) / n;

	/* 到下一个远地点的时间 */
	if (fabs(M - M_PI) < 1e-12)
		dat.Ta = T;
	else if (M < M_PI)
		dat.Ta = (M_PI - M) / n;
	else
		dat.Ta = (3.0 * M_PI - M) / n;

	return dat;
}

/**
 * @brief 计算距离霍曼转移点火点提前量角度
 *
 * @param follow 操纵天体
 * @param destination_to 目标天体
 * @param dest_about_point 目标天体环绕中心
 * @param about_point 操纵天体环绕中心
 * @return 轨道参数结构体
 */
struct hohmann_orbital_parameters
phy_get_hohmann_orbit_theta(Star_t *follow, Star_t *destination_to,
			    Star_t *dest_about_point, Star_t *about_point)
{
	if (!follow || !follow->obj || !destination_to || !destination_to->obj
	    || !dest_about_point || !dest_about_point->obj) return (struct hohmann_orbital_parameters){};
	/* 假使以follow(卫)，about_point(地)，dest_about_point(日)，destination_to(火)进行讲解 */
	/* 日火向量 */
	const Vec_t r2 = vec_sub(destination_to->center, dest_about_point->center);
	/* 日卫向量 */
	const Vec_t r1 = vec_sub(follow->center, dest_about_point->center);
	const double r1f = vec_len(r1);
	const double r2f = vec_len(r2);
	const double a = (r1f + r2f) / 2;
	const double T = 2*M_PI*(a)*sqrt(a/(G_unit*(follow->mass+dest_about_point->mass)));
	const double mu = G_unit*dest_about_point->mass;
	const double omiga = M_PI - (sqrt(mu/r2f) / r2f)*T/2;
	/* 使用了跟踪天体平面 */
	const double rtheta = vec_angle2d(r1, r2, vec_sub(follow->speed, dest_about_point->speed));
	const double theta = fmod(rtheta - fmod(omiga, 2*M_PI) + M_PI, 2*M_PI);
	struct hohmann_orbital_parameters ret = {
		.expect_theta = omiga,
		.theta = (theta<0?theta+2*M_PI:theta)-M_PI,
		.T = T/2,
		.v = sqrt(2*mu*r2f/r1f/(r1f+r2f)),
	};
	if (!about_point || about_point->mass <= 0 || about_point == dest_about_point)
		return ret;

	const double mu2 = G_unit*about_point->mass;
	/* 日卫向量沿日卫速度方向的垂直（径向）向量 */
	const Vec_t v4 = vec_cross_product(vec_cross_product(r1, vec_sub(follow->speed, dest_about_point->speed)), r1);
	/* 计算霍曼转移理想速度(对日速度)（速度乘以方向） */
	const Vec_t v3 = vec_mul(vec_direct(v4), ret.v);
	/* 日心速度转地心速度 */
	ret.v = vec_len(vec_sub(vec_add(v3, dest_about_point->speed), about_point->speed));
	/* 根据预期剩余速度计算期望速度(对地速度) */
	const Vec_t r3 = vec_sub(follow->center, about_point->center);
	const double speed_need = pow2(ret.v)+2*mu2/vec_len(r3);
	if (speed_need > 0) {
		ret.v = sqrt(speed_need);
	}
	/* 算法借用自get_orbital_parameters()
	 * 计算在霍曼轨道速度下的双曲线曲率 */
	const Vec_t v1 = vec_direct(vec_sub(about_point->speed, dest_about_point->speed));
	const Vec_t v2 = vec_mul(v1, ret.v);
	const double e = vec_len(vec_mul(vec_sub(vec_mul(r3, vec_point_product(v2,v2)-mu2/vec_len(r3)),
						 vec_mul(v2, vec_point_product(r3,v2))), 1/mu2));
	ret.expect_theta = asin(1/e);
	ret.theta = fmod(vec_angle2d(vec_sub(follow->speed, about_point->speed), v1, vec_mul(r3,-1))
			 -asin(1/e)+M_PI, 2*M_PI)-M_PI;
	return ret;
}



/* 天体交会距离计算（下面代码均由ai生成）
 *
 * 提示词1：
 * 编写两个函数，分别根据给定天体与中心天体，使用二体简化模型计算，
 * 返回一定时间后的相对中心天体位置向量，以及转动一定角度（0~inf rad）所需的时间
 *
 * 提示词2：
 * 写函数，给定两天体与指定中心天体，
 * 分别使用二体近似计算二者会在最近一次的什么时候达到多大的最近距离
 * */

/* 缓存二体运动所需的轨道量 */
struct two_body_t {
	Vec_t  r_vec;   /* 相对位置向量 (unit) */
	Vec_t  v_vec;   /* 相对速度向量 (unit/s) */
	double mu;      /* 中心天体引力参数 GM (unit^3/s^2) */
	double r;       /* 当前距离 (unit) */
	double a;       /* 半长轴 (unit)，双曲线时<0 */
	double e;       /* 偏心率 */
	Vec_t  e_hat;   /* 近地点方向单位向量（近圆轨道取当前径向） */
	Vec_t  p_hat;   /* 轨道面内前进方向单位向量 (h_hat × e_hat) */
	bool   valid;
};

static struct two_body_t two_body_setup(Star_t *star, Star_t *center)
{
	struct two_body_t tb = {0};
	if (!star || !center || !star->obj || !center->obj) return tb;
	if (center->mass <= 0) return tb;

	tb.r_vec = vec_sub(star->center, center->center);
	tb.v_vec = vec_sub(star->speed, center->speed);
	tb.mu    = G_unit * center->mass;
	tb.r     = vec_len(tb.r_vec);
	if (tb.r < 1e-12 || tb.mu <= 0) return tb;

	const double v2 = vec_point_product(tb.v_vec, tb.v_vec);
	const double inv_a = 2.0 / tb.r - v2 / tb.mu;
	tb.a = (fabs(inv_a) > 1e-30) ? 1.0 / inv_a : INFINITY;

	/* 偏心率向量 e = ((v²-GM/r)·r - (r·v)·v) / GM */
	const Vec_t e_vec = vec_mul(
		vec_sub(vec_mul(tb.r_vec, v2 - tb.mu / tb.r),
		        vec_mul(tb.v_vec, vec_point_product(tb.r_vec, tb.v_vec))),
		1.0 / tb.mu);
	tb.e = vec_len(e_vec);

	/* 轨道平面法向 */
	const Vec_t h_vec = vec_cross_product(tb.r_vec, tb.v_vec);
	const double h_len = vec_len(h_vec);
	if (h_len < 1e-20) return tb;    /* 径向轨道暂不支持 */

	/* 近地点方向（近圆轨道用当前径向作参考） */
	if (tb.e > 1e-10)
		tb.e_hat = vec_mul(e_vec, 1.0 / tb.e);
	else
		tb.e_hat = vec_mul(tb.r_vec, 1.0 / tb.r);
	/* 前进方向 = h_hat × e_hat */
	tb.p_hat = vec_cross_product(vec_mul(h_vec, 1.0 / h_len), tb.e_hat);

	tb.valid = true;
	return tb;
}

/**
 * @brief 使用二体简化模型计算一段时间后天体相对于中心天体的位置
 *
 * 支持椭圆轨道（e<1）与双曲线轨道（e>1）。抛物线退化情形（|e-1|<1e-10）
 * 以及纯径向轨道暂不支持，函数返回 false。
 *
 * @param star          运动天体
 * @param center        中心天体（引力源）
 * @param time          经过的时间（秒），可为负值表示回退
 * @param out_position  输出：相对中心天体的位置向量（km）
 * @return true 成功；false 参数非法或轨道类型不支持
 */
static bool two_body_position_after(Star_t *star, Star_t *center,
                                    double time, Vec_t *out_position)
{
	if (!out_position) return false;
	struct two_body_t tb = two_body_setup(star, center);
	if (!tb.valid) return false;

	const double e = tb.e;
	const double cos_t0 = vec_point_product(tb.r_vec, tb.e_hat) / tb.r;
	const double sin_t0 = vec_point_product(tb.r_vec, tb.p_hat) / tb.r;

	double r_new, theta_new;

	if (e < 1.0 - 1e-10) {
		/* ---------- 椭圆轨道 ---------- */
		if (!(tb.a > 0)) return false;
		const double n  = sqrt(tb.mu / (tb.a * tb.a * tb.a));
		const double E0 = atan2(sqrt(1 - e * e) * sin_t0, e + cos_t0);
		const double M0 = E0 - e * sin(E0);
		const double M  = M0 + n * time;

		/* 牛顿迭代解 Kepler 方程 E - e·sinE = M */
		double E = M;
		for (int i = 0; i < 32; i++) {
			const double dE = (E - e * sin(E) - M) / (1 - e * cos(E));
			E -= dE;
			if (fabs(dE) < 1e-13) break;
		}
		const double cosE = cos(E), sinE = sin(E);
		const double den  = 1.0 - e * cosE;
		r_new     = tb.a * den;
		theta_new = atan2(sqrt(1 - e * e) * sinE / den,
		                  (cosE - e) / den);
	} else if (e > 1.0 + 1e-10) {
		/* ---------- 双曲线轨道 ---------- */
		if (!(tb.a < 0)) return false;
		const double a_h = -tb.a;
		const double n   = sqrt(tb.mu / (a_h * a_h * a_h));
		const double H0  = asinh(sqrt(e * e - 1) * sin_t0 /
		                         (1 + e * cos_t0));
		const double M0  = e * sinh(H0) - H0;
		const double M   = M0 + n * time;

		/* 牛顿迭代解 e·sinhH - H = M */
		double H = asinh(M / e);
		for (int i = 0; i < 64; i++) {
			const double f  = e * sinh(H) - H - M;
			const double fp = e * cosh(H) - 1;
			if (fp <= 0) return false;
			const double dH = f / fp;
			H -= dH;
			if (fabs(dH) < 1e-13) break;
		}
		const double coshH = cosh(H), sinhH = sinh(H);
		const double den   = e * coshH - 1;
		r_new     = a_h * den;
		theta_new = atan2(sqrt(e * e - 1) * sinhH / den,
		                  (e - coshH) / den);
	} else {
		/* 抛物线退化情形暂不支持 */
		return false;
	}

	*out_position = vec_add(vec_mul(tb.e_hat, r_new * cos(theta_new)),
	                        vec_mul(tb.p_hat, r_new * sin(theta_new)));
	return true;
}

#if 0
/* 归一化角度到 [0, 2π) */
static double angle_norm_2pi(double a)
{
	a = fmod(a, 2.0 * M_PI);
	if (a < 0) a += 2.0 * M_PI;
	return a;
}

/* 由真近点角θ求椭圆平近点角M，两者均在 [0, 2π) 内 */
static double ellipse_M_from_theta(double theta, double e)
{
	double E = atan2(sqrt(1 - e * e) * sin(theta), e + cos(theta));
	if (E < 0) E += 2.0 * M_PI;
	return E - e * sin(E);
}

/**
 * @brief 使用二体简化模型计算天体在轨道上前进指定角度所需的时间
 *
 * 椭圆轨道：角度可为任意 >=0 的值（自动累加整周期）
 * 双曲线轨道：角度不能超过从当前真近点角到渐近线剩余的角度，
 *             否则永远无法到达，返回 *out_time = INFINITY 且返回 true
 *
 * @param star      运动天体
 * @param center    中心天体（引力源）
 * @param angle     前进角度（弧度），要求 >= 0
 * @param out_time  输出：所需时间（秒）
 * @return true 成功；false 参数非法或轨道类型不支持
 */
static bool two_body_advance_time(Star_t *star, Star_t *center,
                                  double angle, double *out_time)
{
	if (!out_time || angle < 0) return false;
	struct two_body_t tb = two_body_setup(star, center);
	if (!tb.valid) return false;

	const double e = tb.e;
	const double cos_t0 = vec_point_product(tb.r_vec, tb.e_hat) / tb.r;
	const double sin_t0 = vec_point_product(tb.r_vec, tb.p_hat) / tb.r;
	const double t0     = atan2(sin_t0, cos_t0);

	if (e < 1.0 - 1e-10) {
		/* ---------- 椭圆轨道 ---------- */
		if (!(tb.a > 0)) return false;
		const double n = sqrt(tb.mu / (tb.a * tb.a * tb.a));
		const double T = 2.0 * M_PI / n;

		/* 拆分成"整周期 + 剩余角度" */
		const double cycles = floor(angle / (2.0 * M_PI));
		const double rem    = angle - cycles * 2.0 * M_PI;

		/* 用平近点角做插值（在 [0,2π) 内 M 与 θ 单调对应） */
		const double M0 = ellipse_M_from_theta(angle_norm_2pi(t0), e);
		const double Mt = ellipse_M_from_theta(angle_norm_2pi(t0 + rem), e);

		double dM = Mt - M0;
		if (dM < 0) dM += 2.0 * M_PI;

		*out_time = cycles * T + dM / n;
		return true;
	} else if (e > 1.0 + 1e-10) {
		/* ---------- 双曲线轨道 ---------- */
		if (!(tb.a < 0)) return false;
		const double a_h = -tb.a;
		const double n   = sqrt(tb.mu / (a_h * a_h * a_h));
		const double theta_max = acos(-1.0 / e);   /* 渐近线真近点角 */

		const double theta_target = t0 + angle;
		if (theta_target >= theta_max) {
			*out_time = INFINITY;      /* 渐近线处耗时发散，实际不可达 */
			return true;
		}

		const double H0 = asinh(sqrt(e * e - 1) * sin_t0 /
		                        (1 + e * cos_t0));
		const double M0 = e * sinh(H0) - H0;

		const double Ht = asinh(sqrt(e * e - 1) * sin(theta_target) /
		                        (1 + e * cos(theta_target)));
		const double Mt = e * sinh(Ht) - Ht;

		*out_time = (Mt - M0) / n;
		return true;
	}
	/* 抛物线退化情形暂不支持 */
	return false;
}
#endif

/* 计算 t 时刻两天体相对中心天体的距离；失败时返回 INFINITY 并置 *ok=false */
static double two_body_sep_at(Star_t *a, Star_t *b, Star_t *center,
                              double t, bool *ok)
{
	Vec_t pa, pb;
	if (!two_body_position_after(a, center, t, &pa) ||
	    !two_body_position_after(b, center, t, &pb)) {
		if (ok) *ok = false;
		return INFINITY;
	}
	if (ok) *ok = true;
	return vec_len(vec_sub(pa, pb));
}

/* 在 [lo, hi] 上用黄金分割法找 two_body_sep_at 的极小值 */
static double golden_min_sep(Star_t *a, Star_t *b, Star_t *center,
                             double lo, double hi, double *out_val)
{
	const double phi = (sqrt(5.0) - 1.0) / 2.0;
	double x1 = hi - phi * (hi - lo);
	double x2 = lo + phi * (hi - lo);
	bool ok1, ok2;
	double f1 = two_body_sep_at(a, b, center, x1, &ok1);
	double f2 = two_body_sep_at(a, b, center, x2, &ok2);
	if (!ok1 || !ok2) {
		/* 区间内出现定义域失败，返回当前最优 */
		if (ok1 && !ok2) { *out_val = f1; return x1; }
		if (!ok1 && ok2) { *out_val = f2; return x2; }
		*out_val = INFINITY;
		return (ok1 ? x1 : x2);
	}
	for (int i = 0; i < 128 && (hi - lo) > 1e-3; i++) {
		if (f1 < f2) {
			hi = x2; x2 = x1; f2 = f1;
			x1 = hi - phi * (hi - lo);
			f1 = two_body_sep_at(a, b, center, x1, &ok1);
			if (!ok1) break;
		} else {
			lo = x1; x1 = x2; f1 = f2;
			x2 = lo + phi * (hi - lo);
			f2 = two_body_sep_at(a, b, center, x2, &ok2);
			if (!ok2) break;
		}
	}
	*out_val = fmin(f1, f2);
	return (f1 < f2) ? x1 : x2;
}

/**
 * @brief 二体近似下，计算两天体下一次最近接近的时刻与距离
 *
 * 假设两颗天体都各自只受指定中心天体的引力支配（彼此间的引力被忽略），
 * 采用与 two_body_position_after 相同的单位约定：
 *   - 位置 unit，速度 unit/s，时间 s，mu = G_unit*M (unit^3/s^2)
 *
 * 搜索窗口根据两轨道的会合周期自动决定：
 *   - 若两颗均为椭圆轨道：窗口 ≈ 2×min(T_syn, 50×T_max)，并保证至少覆盖 2 个较长周期
 *   - 若一颗为双曲线：窗口取另一颗周期的 4 倍
 *   - 若两颗均为双曲线：窗口取 10 年（双曲线接近只发生有限次）
 *
 * @param star_a  天体 A
 * @param star_b  天体 B
 * @param center  两者共同的中心天体（引力源）
 * @return struct close_approach_t { time, distance, valid }
 *         - time     从当前时刻起算的时间（秒），>= 0
 *         - distance 该时刻两天体的相对距离（km）
 *         - valid    求解是否成功（参数非法或不支持轨道时为 false）
 */
struct close_approach_t phy_two_body_closest_approach(Star_t *star_a, Star_t *star_b, Star_t *center)
{
	struct close_approach_t res = {
		.time = INFINITY,
		.distance = INFINITY,
		.valid = false
	};
	if (!star_a || !star_b || !center) return res;
	if (star_a == star_b || star_a == center || star_b == center) return res;
	if (!star_a->obj || !star_b->obj || !center->obj) return res;

	struct two_body_t tb_a = two_body_setup(star_a, center);
	struct two_body_t tb_b = two_body_setup(star_b, center);
	if (!tb_a.valid || !tb_b.valid) return res;

	/* 椭圆轨道周期；双曲线用 INFINITY 标记 */
	double T_a = INFINITY, T_b = INFINITY;
	if (tb_a.e < 1.0 && tb_a.a > 0)
		T_a = 2.0 * M_PI * sqrt(tb_a.a * tb_a.a * tb_a.a / tb_a.mu);
	if (tb_b.e < 1.0 && tb_b.a > 0)
		T_b = 2.0 * M_PI * sqrt(tb_b.a * tb_b.a * tb_b.a / tb_b.mu);

	/* 决定搜索窗口 T_scan */
	double T_scan;
	if (isfinite(T_a) && isfinite(T_b)) {
		const double T_max = fmax(T_a, T_b);
		const double T_min = fmin(T_a, T_b);
		double T_syn;
		if (fabs(T_a - T_b) > 1e-6 * T_max)
			T_syn = T_a * T_b / fabs(T_a - T_b);   /* 会合周期 */
		else
			T_syn = T_max;                          /* 周期相同，取单周期 */
		T_scan = 2.0 * fmin(T_syn, 50.0 * T_max);
		if (T_scan < 2.0 * T_max) T_scan = 2.0 * T_max;
		(void)T_min;
	} else if (isfinite(T_a)) {
		T_scan = 4.0 * T_a;
	} else if (isfinite(T_b)) {
		T_scan = 4.0 * T_b;
	} else {
		/* 两者皆双曲线：给一个固定窗口 */
		T_scan = 10.0 * 365.25 * 24.0 * 3600.0;
	}

	/* 粗扫描 */
	const int N_COARSE = 8192;
	const double step = T_scan / N_COARSE;
	double best_t = 0.0, best_d = INFINITY;
	bool any = false;
	for (int i = 0; i <= N_COARSE; i++) {
		const double t = step * i;
		bool ok;
		const double d = two_body_sep_at(star_a, star_b, center, t, &ok);
		if (!ok) continue;   /* 双曲线越界等，跳过 */
		any = true;
		if (d < best_d) { best_d = d; best_t = t; }
	}
	if (!any) return res;

	/* 若最优点落在左端点（t=0 附近即已是最小），直接返回 */
	if (best_t <= step * 0.5) {
		res.time     = best_t;
		res.distance = best_d;
		res.valid    = true;
		return res;
	}

	/* 在最小值附近精修 */
	const double lo = fmax(0.0, best_t - step);
	const double hi = best_t + step;
	double refined_d;
	const double refined_t = golden_min_sep(star_a, star_b, center,
	                                        lo, hi, &refined_d);
	if (isfinite(refined_d) && refined_d < best_d) {
		res.time     = refined_t;
		res.distance = refined_d;
	} else {
		res.time     = best_t;
		res.distance = best_d;
	}
	res.valid = true;
	return res;
}
/* 大段ai代码结束，
 * 这算法还是太又臭又长，一点也不优雅，好像还没有别的办法，真的不想管了
 * cpu烧了就烧了吧 */
