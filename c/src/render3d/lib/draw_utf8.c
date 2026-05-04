/**
 * @file        draw_utf8.c
 * @author      u0_a221
 * @date        2026-05-04
 * @brief       使用unicode字符进行渲染(延迟可能比较大)
 *              如果Termux跑不动这个后端可以装一个ttyd用较新的浏览器打开效果比较好
 */

#include "render3d.h"

typedef struct {
	size_t w;
	size_t h;
	double *scr;
} Scr_t;

Scr_t *scr_create(int width, int height)
{
	Scr_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (Scr_t){
		.w = width,
		.h = height,
		.scr = malloc(sizeof(*p->scr)*width*height*2),
	};
	memset(p->scr, 0, sizeof(p->scr)*width*height*2);
	return p;
}

static void draw(RenderBackend_t *backend, Point2d_t point)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (point.x < (double)s->w/-1 || point.x > (double)s->w/1) return;
	if (point.y < (double)s->h/-0.5 || point.y > (double)s->h/0.5) return;
	size_t ind = (int)(s->h-point.y)*s->w + (int)(s->w/2.)+point.x;
	if (ind >= s->w*s->h*2) return;
	if (s->scr[ind]==0 || s->scr[ind] > point.z) {
		s->scr[ind] = point.z;    /* [0.0, 1.0] */
	}
}

static void render(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	double t, b;
	fputs("\033[H", stdout);    /* puts自带换行符不可用 */
	for (size_t i = 0; i < s->h; ++i) {
		for (size_t j = 0; j < s->w; ++j) {
			// 灰度范围：232~255
			t = s->scr[i*2*s->w+j] ? s->scr[i*2*s->w+j] : 1;
			b = s->scr[(i*2+1)*s->w+j] ? s->scr[(i*2+1)*s->w+j] : 1;
			t *= (255-232);
			b *= (255-232);
			// 使用 ANSI 真彩色设置前景/背景（略）
			printf("\033[38;5;%dm\033[48;5;%dm▀", 255-(int)t, 255-(int)b);
			// fputs("▀", stdout); // 上半块字符
		}
		putc('\n', stdout);
	}
	fputs("\033[0m", stdout);
}

static void clean(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	memset(s->scr, 0, sizeof(*s->scr)*s->w*s->h*2);
}

static void destory(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (s->scr) free(s->scr);
	free(s);
	free(backend);
}

RenderBackend_t *backend_create_utf8(int width, int height)
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
