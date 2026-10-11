/**
 * @file        phy_get_closest.c
 * @author      Chglish
 * @date        2026-10-11
 * @brief       天体交会距离计算，二次拆分自physics.c（下面代码均由ai生成）
 */

#include "./nbody.h"

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
