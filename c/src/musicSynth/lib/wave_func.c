/**
 * @file        wave_func.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       声波函数集
 */

#include "../core.h"

/**
 * 方波函数（占空比50%）
 * 周期 T，幅值 A
 * 在 [0, T/2) 输出 A，[-T/2, 0) 输出 -A
 */
double square_wave(double t)
{
	double x = fmod(t, M_PI) - M_PI/2;
	return x >= 0 ? 1 : -1;
}

/**
 * 三角波函数（对称，峰峰值 2A）
 * 周期 T，幅值 A
 * 线性上升：0 → T/2 时从 -A → A
 * 线性下降：T/2 → T 时从 A → -A
 */
double triangle_wave(double t)
{
	const static double T = M_PI, A = 1;
	double x = fmod(t, T);
	if (x < 0)
		x += T;
	double u = 2.0 * x / T;	// 归一化到 [0, 2)
	if (u < 1.0) {
		return 2.0 * A * u - A;	// 上升段：-A 到 A
	} else {
		return 2.0 * A * (2.0 - u) - A;	// 下降段：A 到 -A
	}
}

/**
 * 锯齿波（上升沿）
 * x: [-pi/2, pi/2) -> [-1, 1) (线性上升)
 */
double sawtooth_wave(double t)
{
	double x = fmod(t, M_PI) - M_PI/2;
	return 2.0 * (x/M_PI - floor(x/M_PI+0.5));
}

double noise_wave(double t)
{
	(void)t;
	return 2*(double)rand()/RAND_MAX-1;
}

