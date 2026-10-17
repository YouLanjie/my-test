/**
 * @file        rt_saves.c
 * @author      Chglish
 * @date        2026-10-17
 * @brief       处理二进制存档（提高存档精度）
 */

#include "./nbody.h"
#include <endian.h>

#define MAGIC_HEADER {'N','B',0,'r','3','d',0,-5,}
#define MAGIC_STAR {'S','T','3','d'}
#define MAGIC_LOG {'r','3','d',0,'L','o','g',0}

union u64f64c8_t {
	int64_t  i64;
	uint64_t u64;
	double   f64;
	uint8_t  c8[8];
};
#define f64u64(v) (((union u64f64c8_t){.f64=(v)}).u64)
#define u64f64(v) (((union u64f64c8_t){.u64=(v)}).f64)
#define h2lef(v) (u64f64(htole64(f64u64(v))))
#define lef2h(v) (u64f64(le64toh(f64u64(v))))

/* 存档头 */
static struct save_header {
	char magic[8];
	uint64_t save_time;
	double gtime;
	double dv;
	uint64_t seed;
	uint64_t star_num;
} create_save_header(Runtimedata_t *rt)
{
	// static const size_t size = sizeof(struct save_header);
	return (struct save_header){
		.magic = MAGIC_HEADER,
		.gtime = h2lef(rt ? rt->gtime : 0),
		.dv    = h2lef(rt ? unit2km(rt->dv) : 0),
		.seed  = htole64(rt ? rt->seed : 0),
		.save_time = htole64(time(NULL)),
		.star_num  = htole64(rt ? rt->objs.len : 0),
	};
}

static bool read_header(Runtimedata_t *rt, struct save_header *header)
{
	if (!rt || !header) return false;
	if (memcmp(header->magic, (char[])MAGIC_HEADER, sizeof(header->magic)) != 0)
		return false;
	if (header->star_num <= 0)
		return false;
	rt->gtime = lef2h(header->gtime);
	rt->dv = km2unit(lef2h(header->dv));
	rt->seed = le64toh(header->seed);
	header->star_num = le64toh(header->star_num);
	header->save_time = le64toh(header->save_time);
	return true;
}

static struct save_star {
	double m;    /* 质量 */
	double r;    /* 半径 */
	double p_x;    /* 位置 */
	double p_y;
	double p_z;
	double v_x;    /* 速度 */
	double v_y;
	double v_z;
	double u_x;    /* 自转轴方向 */
	double u_y;
	double u_z;
	double omiga;    /* 自转速度 */
	uint64_t name_len;
	uint8_t color[4];    /* 颜色码 */
	uint8_t padding[4];    /* 显式对齐余下4字节 */
} create_save_star(Star_t *star)
{
	struct save_star dat = {
		.color = {-1,-1,-1,-1},
		.padding = MAGIC_STAR,
	};
	// static const size_t size = sizeof(dat);
	if (!star) return dat;
	dat.m = h2lef(star->mass);
	dat.r = h2lef(unit2km(star->radius));
	dat.p_x = h2lef(unit2km(star->center.x));
	dat.p_y = h2lef(unit2km(star->center.y));
	dat.p_z = h2lef(unit2km(star->center.z));
	dat.v_x = h2lef(unit2km(star->speed.x));
	dat.v_y = h2lef(unit2km(star->speed.y));
	dat.v_z = h2lef(unit2km(star->speed.z));
	dat.u_x = h2lef(star->self_rotate.x);
	dat.u_y = h2lef(star->self_rotate.y);
	dat.u_z = h2lef(star->self_rotate.z);
	dat.omiga = h2lef(star->self_omiga);
	Color_t c = star->obj && star->obj->colors ? star->obj->colors[0] : COLOR_WHITE;
	dat.color[0] = c.r;
	dat.color[1] = c.g;
	dat.color[2] = c.b;
	dat.color[3] = c.a;
	dat.name_len = htole64(star->name.len);
	return dat;
}

#define f2vec(var_prefix) (Vec_t){lef2h(var_prefix##_x),lef2h(var_prefix##_y),lef2h(var_prefix##_z)}
static bool read_star(Runtimedata_t *rt, struct save_star *star, char *name)
{
	if (!rt || !star || !name) return false;
	if (memcmp(star->padding, (char[])MAGIC_STAR, sizeof(star->padding)) != 0)
		return false;
	Star_t s = star_create(name, lef2h(star->m), lef2h(star->r), f2vec(star->p), f2vec(star->v), NULL);
	s.self_rotate = f2vec(star->u);
	s.self_omiga = lef2h(star->omiga);
	Color_t c = {
		.r = star->color[0],
		.g = star->color[1],
		.b = star->color[2],
		.a = star->color[3],
	};
	obj_set_color(s.obj, c);
	da_append(&rt->objs, &s);
	return true;
}
#undef f2vec

bool rt_bin_save(Runtimedata_t *rt, const char *filename)
{
	if (!rt || !filename) return false;
	FILE *fp = fopen(filename, "wb");
	if (!fp) return false;
	struct save_header header = create_save_header(rt);
	fwrite(&header, 1, sizeof(header), fp);
	Star_t *objs = rt->objs.ptr;
	struct save_star star = {};
	for (size_t i = 0; i < rt->objs.len; i++) {
		star = create_save_star(&objs[i]);
		fwrite(&star, 1, sizeof(star), fp);
		fwrite(objs[i].name.p, 1, objs[i].name.len, fp);
	}
	if (rt->logs.p && rt->logs.len) {
		char magic[] = MAGIC_LOG;
		fwrite(magic, 1, sizeof(magic), fp);
		uint64_t tmp = htole64(rt->logs.len);
		fwrite(&tmp, 1, sizeof(tmp), fp);
		fwrite(rt->logs.p, 1, rt->logs.len, fp);
	}
	fclose(fp);
	return true;
}

bool rt_bin_load(Runtimedata_t *rt, const char *filename)
{
	if (!rt || !filename) return false;
	FILE *fp = fopen(filename, "rb");
	if (!fp) return false;

	bool ret = false;
	struct save_header header = {};
	do {
		if (fread(&header, 1, sizeof(header), fp) < sizeof(header)) break;
		if (!read_header(rt, &header)) break;
		da_free(&rt->objs, star_free);

		struct save_star star = {};
		for (uint64_t i = 0; i < header.star_num && fread(&star, 1, sizeof(star), fp) == sizeof(star); i++) {
			ret = false;
			/* 限制名字长度 */
			const uint64_t len = le64toh(star.name_len) % 1024;
			char buf[len+20];
			sprintf(buf, "未命名天体%lu", i);
			if (len && (fread(buf, 1, len, fp) < len || (buf[len]=0))) break;
			if (!read_star(rt, &star, buf)) break;
			ret = true;
		}
		if (!ret || rt->objs.len != header.star_num) {
			ret = false;
			break;
		}
		char buf[] = MAGIC_LOG;
		/* 若无有效日志块则直接跳过 */
		if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)
		    || memcmp(buf, (char[])MAGIC_LOG, sizeof(buf)) != 0)
			break;
		ret = false;
		uint64_t size;
		if (fread(&size, 1, sizeof(size), fp) != sizeof(size)
		    || !(size=le64toh(size))) {
			break;
		}
		if (!sva_adjust_minimun(&rt->logs, size+2)) break;
		if (fread(rt->logs.p, 1, size, fp) != size) {
			rt->logs.len = 0;
			if (rt->logs.p) rt->logs.p[0] = 0;
			break;
		}
		rt->logs.len = size;
		rt->logs.p[size] = 0;
		ret = true;
	}while (false);
	if (ret) {
		star_sync_cam_config(rt);
		char buf[40] = "";
		struct tm tp = {};
		time_t timer = header.save_time;
		localtime_r(&timer, &tp);
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tp);
		rtlog(rt, "成功载入二进制存档，存档时间: %s", buf);
		rt->pause = true;
	}
	fclose(fp);
	return ret;
}

