/**
 * @file        draw_fb0.c
 * @author      Chglish
 * @date        2026-08-13
 * @brief       尝试通过/dev/fb0渲染
 */

#include "render3d.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/fb.h>

typedef struct {
	size_t term_h;
	size_t w;
	size_t h;
	double *scr;
	Color_t *color;
	uint8_t *fbp;
	uint32_t line_length;
	int fd;
} Scr_t;

static Scr_t *scr_create(int width, int height)
{
	(void)width;
	Scr_t scr = {
		.fd = open("/dev/fb0", O_RDWR),
		.term_h = height,
	};
	if (scr.fd < 0) return NULL;
	bool flag = true;
	Scr_t *p = NULL;
	do {
		struct fb_fix_screeninfo finfo;
		struct fb_var_screeninfo vinfo;
		if (ioctl(scr.fd, FBIOGET_FSCREENINFO, &finfo) == -1) break;
		if (ioctl(scr.fd, FBIOGET_VSCREENINFO, &vinfo) == -1) break;
		if (vinfo.bits_per_pixel != 32) break;

		scr.w = vinfo.xres;
		scr.h = vinfo.yres;
		scr.line_length = finfo.line_length;
		scr.fbp = mmap(NULL, scr.line_length * scr.h,
			       PROT_READ | PROT_WRITE, MAP_SHARED, scr.fd, 0);
		if (scr.fbp == MAP_FAILED) break;

		scr.scr = malloc(sizeof(*p->scr)*scr.w*scr.h);
		scr.color = malloc(sizeof(*p->color)*scr.w*scr.h);
		p = malloc(sizeof(*p));
		flag = false;
	}while (false);
	if (flag || !p || !scr.scr || !scr.color) {
		if (scr.scr) free(scr.scr);
		if (scr.color) free(scr.color);
		if (scr.fbp && scr.fbp != MAP_FAILED)
			munmap(scr.fbp, scr.line_length*scr.h);
		if (p) free(p);
		close(scr.fd);
		return NULL;
	}
	*p = scr;
	memset(p->scr, 0, sizeof(*p->scr)*p->w*p->h);
	memset(p->color, 0, sizeof(*p->color)*p->w*p->h);
	return p;
}

static void scr_getsize(RenderBackend_t *backend, int *w, int *h)
{
	if (!backend || !w || !h) return;
	Scr_t *s = backend->data;
	if (!s) return;
	*w = s->w;
	*h = s->h;
}

static void draw(RenderBackend_t *backend, Point2d_t point, Color_t rgb)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (point.x < (double)s->w/-1 || point.x > (double)s->w/1) return;
	if (point.y < (double)s->h/-1 || point.y > (double)s->h/1) return;
	size_t ind = (int)(s->h/2.-point.y)*s->w + (int)(s->w/2.)+point.x;
	if (ind >= s->w*s->h) return;
	if (s->scr[ind]==0 || s->scr[ind] > point.z) {
		s->scr[ind] = point.z;    /* [0.0, 1.0] */
		if (s->color) {
			s->color[ind] = color_add(s->color[ind], rgb);
		}
	}
}

static void render(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (!s) return;
	Color_t color = COLOR_BLACK;
	double dept;
	uint8_t *p = NULL;
	// fputs("\033[H", stdout);    /* puts自带换行符不可用 */
	for (size_t i = 0; i < s->h; ++i) {
		for (size_t j = 0; j < s->w; ++j) {
			dept = s->scr[i*s->w+j] ? s->scr[i*s->w+j] : 1;
			color = s->scr[i*s->w+j] ? s->color[i*s->w+j] : COLOR_BLACK;
			p = s->fbp + s->line_length*i + j*4;
			dept = pow(fmax(1 - dept, 1e-8), 1/2.2);
			p[0] = color.b*dept;
			p[1] = color.g*dept;
			p[2] = color.r*dept;
			p[3] = color.a*dept;
		}
	}
	printf("\033[%zuH", s->term_h+1);
}

static void clean(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (s->scr) memset(s->scr, 0, sizeof(*s->scr)*s->w*s->h);
	for (size_t i = 0; i < s->w*s->h; i++) {
		s->color[i] = COLOR_BLACK;
	}
}

static void destroy(RenderBackend_t *backend)
{
	if (!backend || !backend->data) return;
	Scr_t *s = backend->data;
	if (s->scr) free(s->scr);
	if (s->color) free(s->color);
	if (s->fbp) {
		munmap(s->fbp, s->line_length*s->h);
	}
	if (s->fd >= 0) close(s->fd);
	free(s);
	free(backend);
}

RenderBackend_t *backend_create_fb0(int width, int height)
{
	RenderBackend_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (RenderBackend_t){
		.draw = draw,
		.render = render,
		.clean = clean,
		.destroy = destroy,
		.get_size = scr_getsize,
		.data = scr_create(width, height),
		.id = RDBK_fb0,
	};
	if (!p->data) {
		free(p);
		return NULL;
	}
	return p;
}
