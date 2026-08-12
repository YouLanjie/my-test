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
	Color_t *color;
} Scr_t;

static Scr_t *scr_create(int width, int height)
{
	Scr_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (Scr_t){
		.w = width,
		.h = height,
		.scr = malloc(sizeof(*p->scr)*width*height*2),
		.color = malloc(sizeof(*p->color)*width*height*2),
	};
	memset(p->scr, 0, sizeof(*p->scr)*width*height*2);
	memset(p->color, 0, sizeof(*p->color)*width*height*2);
	return p;
}

static void draw(RenderBackend_t *backend, Point2d_t point, Color_t rgb)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (point.x < (double)s->w/-1 || point.x > (double)s->w/1) return;
	if (point.y < (double)s->h/-0.5 || point.y > (double)s->h/0.5) return;
	size_t ind = (int)(s->h-point.y)*s->w + (int)(s->w/2.)+point.x;
	if (ind >= s->w*s->h*2) return;
	if (s->scr[ind]==0 || s->scr[ind] > point.z) {
		s->scr[ind] = point.z;    /* [0.0, 1.0] */
		if (s->color) {
			s->color[ind] = rgb;
		}
	}
}

static void render(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	double t, b, lt = -1, lb = -1;
	// fputs("\033[H", stdout);    /* puts自带换行符不可用 */
	for (size_t i = 0; i < s->h; ++i) {
		for (size_t j = 0; j < s->w; ++j) {
			t = s->scr[i*2*s->w+j] ? s->scr[i*2*s->w+j] : 1;
			b = s->scr[(i*2+1)*s->w+j] ? s->scr[(i*2+1)*s->w+j] : 1;
			t = 255-t*(255-232);
			b = 255-b*(255-232);
			// 使用 ANSI 真彩色设置前景/背景（略）
			// 灰度范围：232~255
			if ((int)t != (int)lt) {
				lt = t;
				printf("\033[38;5;%dm", (int)t);
			}
			if ((int)b != (int)lb) {
				lb = b;
				printf("\033[48;5;%dm", (int)b);
			}
			fputs("▀", stdout); // 上半块字符
		}
		putc('\n', stdout);
	}
	fputs("\033[0m", stdout);
}

static void clean(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (s->scr) memset(s->scr, 0, sizeof(*s->scr)*s->w*s->h*2);
	for (size_t i = 0; i < s->w*s->h*2; i++) {
		s->color[i] = COLOR_BLACK;
	}
}

static void destroy(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (s->scr) free(s->scr);
	if (s->color) free(s->color);
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
		.destroy = destroy,
		.data = scr_create(width, height),
		.id = RDBK_utf8,
	};
	if (!p->data) {
		free(p);
		return NULL;
	}
	return p;
}


/* 8bit专门函数 */
static void render_8bit(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	double t, b, lt = -1, lb = -1;
	// fputs("\033[H", stdout);    /* puts自带换行符不可用 */
	for (size_t i = 0; i < s->h; ++i) {
		for (size_t j = 0; j < s->w; ++j) {
			t = s->scr[i*2*s->w+j] ? s->scr[i*2*s->w+j] : 1;
			b = s->scr[(i*2+1)*s->w+j] ? s->scr[(i*2+1)*s->w+j] : 1;
			t = 37-t*(37-30);
			b = 47-b*(47-40);
			// 颜色范围：30~37, 40~47
			if ((int)t != (int)lt) {
				lt = t;
				printf("\033[%dm", (int)t);
			}
			if ((int)b != (int)lb) {
				lb = b;
				printf("\033[%dm", (int)b);
			}
			fputs("▀", stdout); // 上半块字符
		}
		putc('\n', stdout);
	}
	fputs("\033[0m", stdout);
}

RenderBackend_t *backend_create_utf8_8bit(int width, int height)
{
	RenderBackend_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (RenderBackend_t){
		.draw = draw,
		.render = render_8bit,
		.clean = clean,
		.destroy = destroy,
		.data = scr_create(width, height),
		.id = RDBK_utf8_8bit,
	};
	if (!p->data) {
		free(p);
		return NULL;
	}
	return p;
}


/* 256bit专门函数 */
static void render_256bit(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	double t, b, lt = -1, lb = -1;
	Color_t color_top = COLOR_BLACK, color_bottom = COLOR_BLACK;
	Color_t lcolor_top = COLOR_WHITE, lcolor_bottom = COLOR_WHITE;
	// fputs("\033[H", stdout);    /* puts自带换行符不可用 */
	for (size_t i = 0; i < s->h; ++i) {
		for (size_t j = 0; j < s->w; ++j) {
			t = s->scr[i*2*s->w+j] ? 1-s->scr[i*2*s->w+j] : 1;
			b = s->scr[(i*2+1)*s->w+j] ? 1-s->scr[(i*2+1)*s->w+j] : 1;
			color_top = s->scr[i*2*s->w+j] ? s->color[i*2*s->w+j] : COLOR_BLACK;
			color_bottom = s->scr[(i*2+1)*s->w+j] ? s->color[(i*2+1)*s->w+j] : COLOR_BLACK;

			if ((int)t != (int)lt || memcmp(&lcolor_top, &color_top, sizeof(Color_t)) != 0) {
				lt = t;
				lcolor_top = color_top;
				printf("\033[38;2;%d;%d;%dm",
				       (int)(color_top.r*t),
				       (int)(color_top.g*t),
				       (int)(color_top.b*t));
			}
			if ((int)b != (int)lb || memcmp(&lcolor_bottom, &color_bottom, sizeof(Color_t)) != 0) {
				lb = b;
				lcolor_bottom = color_bottom;
				printf("\033[48;2;%d;%d;%dm",
				       (int)(color_bottom.r*b),
				       (int)(color_bottom.g*b),
				       (int)(color_bottom.b*b));
			}
			fputs("▀", stdout); // 上半块字符
		}
		putc('\n', stdout);
	}
	fputs("\033[0m", stdout);
}

RenderBackend_t *backend_create_utf8_256bit(int width, int height)
{
	RenderBackend_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (RenderBackend_t){
		.draw = draw,
		.render = render_256bit,
		.clean = clean,
		.destroy = destroy,
		.data = scr_create(width, height),
		.id = RDBK_utf8_256bit,
	};
	if (!p->data) {
		free(p);
		return NULL;
	}
	return p;
}
