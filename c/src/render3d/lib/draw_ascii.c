/**
 * @file        draw_ascii.c
 * @author      u0_a221
 * @date        2026-05-03
 * @brief       以ascii形式绘画
 */

#include "render3d.h"

typedef struct Scr_t {
	size_t h;
	size_t w;
	size_t size;
	double *scr;    /* 屏幕 */
	char *pixels;
} Scr_t;

static const char CHRTABLE[] = {
	"$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft"
	"/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. "
};
static const int MAXCHR = sizeof(CHRTABLE)-2;

static Scr_t *scr_create(size_t width, size_t height)
{
	if (width == 0 || height == 0) return NULL;
	Scr_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	size_t size = width * height + 1;
	*p = (Scr_t){
		.scr = malloc(size*sizeof(*p->scr)),
		.pixels = malloc(size),
		.h = height,
		.w = width,
		.size = size,
	};
	memset(p->pixels, MAXCHR, p->size);
	memset(p->scr, MAXCHR, p->size*sizeof(*p->scr));
	return p;
}

static void scr_free(Scr_t *p)
{
	if (!p) return;
	free(p->scr);
	free(p->pixels);
	free(p);
}

/* x,y : [0, MAX)
 * return: 屏幕字符指针或NULL */
static double *scr_p(Scr_t *s, int x, int y)
{
	if (!s) return NULL;
	if (x < 0 || x >= (int)s->w || y < 0 || y >= (int)s->h) return NULL;
	return s->scr + y*s->w + x;
}
/* 中央原点屏幕坐标系 转换成 左上角原点屏幕坐标系 */
static double *scr_c(Scr_t *s, int x, int y)
{
	if (!s) return NULL;
	return scr_p(s, s->w/2+(int)(x), s->h/2-(int)(y));
}

/* 灰度映射 打印前安全处理 打印 重置屏幕 */
static void scr_print(Scr_t *s)
{
	if (!s) return;
	static size_t j = 0;
	static size_t c = 0;
	for (j = 0; j < s->size; j++) {
		c = s->scr[j]*MAXCHR;
		s->pixels[j] = c ? CHRTABLE[c] : CHRTABLE[MAXCHR];
	}
	for (j = 0; j < s->h; j++) s->pixels[s->w*(j+1)-1] = '\n';
	s->pixels[s->size-1] = 0;
	printf("\033[H%s", s->pixels);
}

static void scr_clean(Scr_t *s)
{
	if (!s) return;
	memset(s->scr, 0, s->size*sizeof(*s->scr));
}

/* 绘制 */
static void draw(RenderBackend_t *backend, Point2d_t p)
{
	if (!backend || !backend->data) return;
	Scr_t *scr = backend->data;
	double *i = scr_c(scr, p.x, p.y/2);
	if (!i) return;
	if (*i > 1) return;
	/* p.z => (0, 1] */
	if (*i == 0 || *i > p.z) *i = p.z;    /* 若原本该处的距离远于新点的距离 */
}
/* 输出一帧 */
static void render(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	scr_print(backend->data);
}
/* 清理上一帧的数据 */
static void clean(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	scr_clean(backend->data);
}
/* 释放内存 */
static void destory(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	scr_free(backend->data);
	free(backend);
}

RenderBackend_t *backend_create_ascii(int width, int height)
{
	RenderBackend_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (RenderBackend_t){
		.draw = draw,
		.render = render,
		.clean = clean,
		.destory = destory,
		.data = scr_create(width, height)
	};
	if (!p->data) {
		free(p);
		return NULL;
	}
	return p;
}

