/**
 * @file        nbody.h
 * @author      Chglish
 * @date        2026-10-10
 * @brief       多体模拟器，分离自原r3d_rotate.c
 */

#pragma once

#ifndef _NBODY_H
#define _NBODY_H

#include "../lib/render3d.h"
#include "../../../include/tools.h"
#include "../../../include/string_view.h"
#include "../../../include/dynamic_array.h"

/* 将距离换算成 1单位 = 1km
 * 可换成其他数字 */
#define SCALE 1.0
/* 引力常量 N*(m^2)/(kg^2) || (m^3)/(kg*s^2) */
#define G 6.6743e-11
/* (km^3)/(kg*s^2) */
#define G_km (G/1e9)
/* 实际使用值 (unit^3)/(kg*s^2) */
#define G_unit (G_km/SCALE/SCALE/SCALE)

#define pow2(x) ((x)*(x))
/* 系列单位数值转换宏 */
/* 距离转换(除法转乘法、编译期常量优化) */
#define unit2km(n) ((n)*SCALE)
#define km2unit(n) ((n)*(1/SCALE))
#define unit2m(n) ((n)*(unit2km(1)*1e3))
#define m2unit(n) ((n)*(km2unit(1)/1e3))
#define kunit2unit(n) ((n)*1e3)
/* 时间转换 */
#define sec2day(sec) ((sec)/60./60./24.)
#define GT2YHMS_FMT "%3dd %02d:%02d:%04.1f"
#define gt2yhms_fmt(gt)\
	(int)sec2day(gt), \
	((isnormal(gt) && (gt)>=0) ? (int)fmod((gt)/60./60., 24): 0), \
	((isnormal(gt) && (gt)>=0) ? (int)fmod((gt)/60., 60) : 0), \
	((isnormal(gt) && (gt)>=0) ? fmod((gt), 60) : 0)
/* 将推力值转换为加速度(km/s^2)
 * 1% = 0.1m/s^2 = 0.1/1e3 km/s^2 */
#define throttle2accel(thr) m2unit(thr*0.1)

#define rtlog(rt, fmt, ...) \
	sva_sprintfcat(&(rt)->logs, "[T+" GT2YHMS_FMT "] " fmt "\n", \
		       gt2yhms_fmt((rt)->gtime) __VA_OPT__(,) __VA_ARGS__)
#define UI_ALPHA 200


struct orbital_parameters {
	char *typ;    /* 轨道类型字符串 */
	double a;     /* 半轴长 */
	double e;     /* 偏心率 */
	double rp;    /* 近地点 */
	double ra;    /* 远地点 */
	double r;     /* 当前半径 */
	double d_rp;  /* 距近地点 */
	double d_ra;  /* 距远地点 */
	double T;     /* 周期(秒) */
	double Tp;    /* 距离抵达近地点时间 */
	double Ta;    /* 距离抵达远地点时间 */
	Vec_t point_rp;
	Vec_t point_ra;
	Vec_t u;    /* 轨道平面法向量(r * v) */
};

struct hohmann_orbital_parameters {
	double theta;    /* 剩余角度，为0时最佳(rad) */
	double expect_theta;    /* 期望角度提前量(rad) */
	double T;    /* 半周期 */
	double v;    /* 期望速度unit/s */
};

typedef struct {
	SVA_t name;
	Obj_t *obj;
	double radius;
	double mass;          /* 质量(kg) */
	double self_omiga;    /* 自转速度(rad/s) */
	Vec_t self_rotate;    /* 自转方向 */
	Vec_t speed;          /* 速度(unit/s) */
	Vec_t center;         /* 中心位置 */
	Camera_t cam;         /* 随身相机 */
} Star_t;

typedef struct {
	RenderBackend_t *backend;
	Camera_t *camera;
	Camera_t *active_cam;
	SVA_t logs;    /* 日志文本 */
	SVA_t buff;    /* HUD文本缓存 */
	DA_t objs;
	DA_t accel;
	Star_t *destination_to;
	Star_t *look_to;
	Star_t *follow;
	Star_t *about_point;
	Obj_t  *axis_helper;
	double dv;
	double gtime;
	double time_scale;
	double time_scale_limit;
	double accel_last_upeate_time;
	uint32_t seed;
	int32_t  inp;
	uint8_t throttle;    /* 1% = 0.1m/s^2, 取决于throttle2accel() */
	int8_t throttle_on;    /* <<0位表开关，<<1位表反方向推力 */
	uint8_t fps;
	bool rotate_cam_with_spd;
	bool axis;
	bool guidline;
	bool pause;
	bool print_busy;
	bool use_rk4;
	bool fixed_about_point;
	bool hud_time_simple_format;
	bool ignore_safety;
	uint8_t need_notification;
	struct {
		Star_t *about_point;
		Star_t *follow;
		double e1;
		double e2;
		int8_t throttle_on;
		bool ev_swap_rpra;
		struct orbital_parameters obp;    /* 存储环绕参数 */
	} loop_stat;    /* 存储游戏循环的上次、当前状态，日志用 */
} Runtimedata_t;


/* 辅助函数 */
double get_nowtime();
void print_pager(const char *headline, SV_t content, int mode);
void format_orbital_parameters(Runtimedata_t *rt, SVA_t *dest, struct orbital_parameters ret);
SVA_t *voyage_helper(Runtimedata_t *rt, SVA_t *dest, Star_t *to);
void set_camera_forward(Runtimedata_t *rt);

/* 运行时管理 */
bool rt_setup(Runtimedata_t *rt, int mode);
void rt_cleanup(Runtimedata_t *rt);
void rt_sync_cam_size_scale(Runtimedata_t *rt);
void rt_switch_camera(Runtimedata_t *rt, Camera_t *ca);
SVA_t *rt_dump_stars(Runtimedata_t *rt, SVA_t *dest);
bool rt_load_from_dumped_text(Runtimedata_t *rt, const char *filename);
void rt_logevent(Runtimedata_t *rt);
int rt_format_huds(Runtimedata_t *rt, SVA_t *dest, struct orbital_parameters ret);
void rt_exec_cmd(Runtimedata_t *rt, char *cmd);
bool rt_bin_save(Runtimedata_t *rt, const char *filename);
bool rt_bin_load(Runtimedata_t *rt, const char *filename);

/* 天体相关 */
Star_t star_create(char *name, double mass, double radius, Vec_t position, Vec_t speed, Star_t *about_point);
void star_free(void *p);
void star_pop(Runtimedata_t *rt, Star_t *star, const char *desc);
void star_sync_position(Runtimedata_t *rt);
void star_sync_cam_config(Runtimedata_t *rt);
void star_star_impact(Runtimedata_t *rt, Star_t *s1, Star_t *s2);
Star_t *star_get_about_point(Runtimedata_t *rt, Star_t *follow);
Star_t *star_choose(Runtimedata_t *rt, const char *hint, Star_t *old);

/* 物理模拟 */
void phy_compute_acceleration(Runtimedata_t *rt);
Star_t *phy_get_about_point(Runtimedata_t *rt, Star_t *follow);
void phy_update(Runtimedata_t *rt);
struct orbital_parameters phy_get_orbital_parameters(Star_t *ship, Star_t *center);
struct hohmann_orbital_parameters phy_get_hohmann_orbit_theta(Star_t *follow, Star_t *destination_to, Star_t *dest_about_point, Star_t *about_point);

/* 特别的，二体运动模拟计算最近距离，ai生成 */
/* 求解结果 */
struct close_approach_t {
	double time;      /* 距离当前时刻的时间（秒） */
	double distance;  /* 最近距离（km） */
	bool   valid;     /* 是否求出有效结果 */
};
struct close_approach_t phy_two_body_closest_approach(Star_t *star_a, Star_t *star_b, Star_t *center);

#endif //NBODY_H

