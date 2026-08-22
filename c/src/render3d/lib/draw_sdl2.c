/**
 * @file        draw_sdl2.c
 * @author      Chglish
 * @date        2026-08-15
 * @brief       ai给的基于SDL2的后端
 */

#include "render3d.h"

#if !defined(__has_include) || !__has_include(<SDL2/SDL.h>)
/* 避免无sdl2报错的空实现 */
RenderBackend_t *backend_create_sdl2(int width, int height)
{
	(void)width;
	(void)height;
	return NULL;
}
#else

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 屏幕数据结构 */
typedef struct {
	SDL_Window *window;	/* 窗口 */
	SDL_Renderer *renderer;	/* 渲染器 */
	SDL_Texture *texture;	/* 纹理（存储像素） */
	uint32_t *pixels;	/* 像素缓冲区（ABGR 或 ARGB，与纹理格式一致） */
	double *depth;		/* 深度缓冲（z值，范围 0~1） */
	int w, h;		/* 窗口宽高（像素） */
	int pitch;		/* 纹理每行字节数 */
	uint32_t format;	/* 纹理像素格式（如 SDL_PIXELFORMAT_ARGB8888） */
} Scr_t;

/* 将 Color_t 转换为 SDL 像素格式（这里按 ARGB 顺序，可根据系统调整） */
static inline uint32_t color_to_pixel(Color_t c, double z)
{
	z = pow(fmax(1 - z, 1e-8), 1/2.2);
	c.r *= z;
	c.g *= z;
	c.b *= z;
	/* 简单假设为 ARGB8888 或 ABGR8888，但为通用，直接按字节组装 */
	/* 实际 SDL 纹理可使用 SDL_PIXELFORMAT_ARGB8888，我们按该顺序填充 */
	return (uint32_t) ((c.a << 24) | (c.r << 16) | (c.g << 8) | c.b);
}

static inline Color_t pixel_to_color(uint32_t c)
{
	return (Color_t){
		.r = (c>>16) & UINT8_MAX,
		.g = (c>>8) & UINT8_MAX,
		.b = c & UINT8_MAX,
		.a = (c>>24) & UINT8_MAX,
	};
}

/* 创建屏幕数据 */
static Scr_t *scr_create(int width, int height)
{
	if (width <= 0 || height <= 0)
		return NULL;

	/* 初始化 SDL 视频子系统（若尚未初始化） */
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
			return NULL;
		}
	}

	Scr_t *s = calloc(1, sizeof(Scr_t));
	if (!s)
		return NULL;

	/* 创建窗口（居中，可调整大小标志可选） */
	s->window = SDL_CreateWindow("3D Renderer (SDL2)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);	/* 可调整大小方便查看 */
	if (!s->window) {
		fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
		goto error;
	}

	/* 创建渲染器（使用硬件加速） */
	s->renderer = SDL_CreateRenderer(s->window, -1,
					 SDL_RENDERER_ACCELERATED |
					 SDL_RENDERER_PRESENTVSYNC);
	if (!s->renderer) {
		fprintf(stderr, "SDL_CreateRenderer Error: %s\n",
			SDL_GetError());
		goto error;
	}

	/* 获取实际窗口大小（可能与请求不同，如高DPI） */
	SDL_GetWindowSize(s->window, &s->w, &s->h);

	/* 选择纹理格式（ARGB8888 通用） */
	s->format = SDL_PIXELFORMAT_ARGB8888;
	s->texture = SDL_CreateTexture(s->renderer, s->format,
				       SDL_TEXTUREACCESS_STREAMING, s->w, s->h);
	if (!s->texture) {
		fprintf(stderr, "SDL_CreateTexture Error: %s\n",
			SDL_GetError());
		goto error;
	}

	/* 分配像素缓冲区（与纹理格式匹配） */
	// int pitch;
	SDL_QueryTexture(s->texture, NULL, NULL, NULL, NULL);
	s->pixels = malloc(s->w * s->h * sizeof(uint32_t));
	s->depth = malloc(s->w * s->h * sizeof(double));
	if (!s->pixels || !s->depth)
		goto error;

	/* 清空初始数据 */
	memset(s->pixels, 0, s->w * s->h * sizeof(uint32_t));
	for (int i = 0; i < s->w * s->h; ++i)
		s->depth[i] = 1.0;	/* 远处为1，近处为0 */

	return s;

 error:
	if (s->pixels)
		free(s->pixels);
	if (s->depth)
		free(s->depth);
	if (s->texture)
		SDL_DestroyTexture(s->texture);
	if (s->renderer)
		SDL_DestroyRenderer(s->renderer);
	if (s->window)
		SDL_DestroyWindow(s->window);
	free(s);
	return NULL;
}

/* 获取窗口实际尺寸（可选） */
static void scr_getsize(RenderBackend_t *backend, int *w, int *h)
{
	if (!backend || !backend->data || !w || !h)
		return;
	Scr_t *s = backend->data;
	*w = s->w;
	*h = s->h;
}

static int scr_resize(Scr_t *s, int new_w, int new_h)
{
	if (!s || new_w <= 0 || new_h <= 0)
		return -1;

	/* 释放旧的纹理、像素和深度缓冲 */
	if (s->texture) {
		SDL_DestroyTexture(s->texture);
		s->texture = NULL;
	}
	if (s->pixels) {
		free(s->pixels);
		s->pixels = NULL;
	}
	if (s->depth) {
		free(s->depth);
		s->depth = NULL;
	}

	/* 更新宽高 */
	s->w = new_w;
	s->h = new_h;

	/* 重新创建纹理（保持原有格式） */
	s->texture = SDL_CreateTexture(s->renderer, s->format,
				       SDL_TEXTUREACCESS_STREAMING, s->w, s->h);
	if (!s->texture) {
		fprintf(stderr, "SDL_CreateTexture (resize) Error: %s\n",
			SDL_GetError());
		return -1;
	}

	/* 重新分配像素缓冲和深度缓冲 */
	s->pixels = malloc(s->w * s->h * sizeof(uint32_t));
	s->depth = malloc(s->w * s->h * sizeof(double));
	if (!s->pixels || !s->depth) {
		fprintf(stderr, "malloc failed during resize\n");
		return -1;
	}

	/* 清空为新缓冲区（黑屏，深度置最远） */
	memset(s->pixels, 0, s->w * s->h * sizeof(uint32_t));
	for (int i = 0; i < s->w * s->h; ++i)
		s->depth[i] = 1.0;

	return 0;
}

/* 绘制一个点（带深度和颜色） */
static void draw(RenderBackend_t *backend, Point2d_t point, Color_t rgb)
{
	if (!backend || !backend->data)
		return;
	Scr_t *s = backend->data;

	/* 将像素偏移量转为屏幕坐标 (原点在中心，y向上为正) */
	int px = (int)(s->w / 2.0 + point.x);
	int py = (int)(s->h / 2.0 - point.y);
	if (px < 0 || px >= s->w || py < 0 || py >= s->h)
		return;

	int index = py * s->w + px;
	double z = point.z;
	if (z < 0.0)
		z = 0.0;
	if (z > 1.0)
		z = 1.0;

	/* 深度测试：更近（z更小）则更新 */
	if (z < s->depth[index]) {
		s->depth[index] = z;
		s->pixels[index] = color_to_pixel(color_add(pixel_to_color(s->pixels[index]), rgb), z);
	}
}

/* 渲染一帧（更新纹理并绘制到窗口，同时处理窗口事件） */
static void render(RenderBackend_t *backend)
{
	if (!backend || !backend->data)
		return;
	Scr_t *s = backend->data;

	/* 处理事件（保持窗口响应） */
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			/* 可由外部检测窗口关闭 */
			break;
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
				int new_w = e.window.data1;
				int new_h = e.window.data2;
				if (scr_resize(s, new_w, new_h) == 0) {
					/* 尺寸调整成功，后续纹理和缓冲已经是新尺寸 */
				} else {
					fprintf(stderr,
						"Failed to resize SDL2 backend\n");
					assert(false && "Failed to resize SDL2 backend");
				}
			}
			break;
		default:
			break;
		}
	}

	/* 更新纹理并绘制 */
	SDL_UpdateTexture(s->texture, NULL, s->pixels, s->w * sizeof(uint32_t));
	SDL_RenderClear(s->renderer);
	SDL_RenderCopy(s->renderer, s->texture, NULL, NULL);
	SDL_RenderPresent(s->renderer);
}

/* 清空深度和颜色缓冲 */
static void clean(RenderBackend_t *backend)
{
	if (!backend || !backend->data)
		return;
	Scr_t *s = backend->data;
	/* 将像素置黑，深度置为1（最远） */
	memset(s->pixels, 0, s->w * s->h * sizeof(uint32_t));
	for (int i = 0; i < s->w * s->h; ++i)
		s->depth[i] = 1.0;
}

/* 释放后端资源 */
static void destroy(RenderBackend_t *backend)
{
	if (!backend)
		return;
	Scr_t *s = backend->data;
	if (s) {
		if (s->pixels)
			free(s->pixels);
		if (s->depth)
			free(s->depth);
		if (s->texture)
			SDL_DestroyTexture(s->texture);
		if (s->renderer)
			SDL_DestroyRenderer(s->renderer);
		if (s->window)
			SDL_DestroyWindow(s->window);
		free(s);
	}
	free(backend);

	/* 由于可能有多个后端同时存在，不全局退出 SDL，由主程序决定 */
	/* 若确定不再使用，可调用 SDL_Quit()，但此处不调用，交给使用者 */
}

/* 创建 SDL2 后端（外部调用） */
RenderBackend_t *backend_create_sdl2(int width, int height)
{
	RenderBackend_t *p = malloc(sizeof(RenderBackend_t));
	if (!p)
		return NULL;

	Scr_t *data = scr_create(width, height);
	if (!data) {
		free(p);
		return NULL;
	}

	*p = (RenderBackend_t) {
		.draw = draw,
		.render = render,
		.clean = clean,
		.destroy = destroy,
		.get_size = scr_getsize,
		.data = data,
		.id = RDBK_sdl2,
	};
	return p;
}
#endif
