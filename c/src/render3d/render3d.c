/**
 * @file        render3d.c
 * @author      u0_a221
 * @date        2026-04-06
 * @brief       通过ascii艺术画形式“渲染”伪3D画面
 */

#include "lib/render3d.h"
#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_FRAME 100000
uint8_t FPS = 40;

static bool setup(RenderBackend_t **bk, Camera_t *ca, uint8_t num)
{
	if (!bk) return false;
	if (*bk) (*bk)->destroy(*bk);
	*bk = NULL;
#define BACKEND(name) backend_create_##name,
	static RenderBackend_t *(*backend_list[])(int width, int height) = {BACKEND_LIST};
#undef BACKEND
	int scr_h = get_winsize_row() - 5,
	    scr_w = get_winsize_col() - 1;
	if (scr_h <= 10 || scr_w <= 10) {
		LOG("终端太小(当前可用尺寸：%dx%d)", scr_w, scr_h);
		return false;
	}
	*bk = backend_list[num%(sizeof(backend_list)/sizeof(backend_list[0]))](scr_w, scr_h);
	if (ca) {
		ca->width  = scr_w;      /* 需要让相机捕捉到的画面与终端大小相匹配减少无效运算 */
		ca->height = scr_h*2;    /* 每行能显示的实际大小为每列的两倍 */
	}
	return false;
}

typedef struct {
	RenderBackend_t *backend;
	Camera_t        *camera;
	Obj_t           *obj;
	double          theta;
	Vec_t           v;      /* 物体速度 */
	Vec_t           cav;    /* 相机速度 */
	bool            rlt_accelr;    /* 相对镜头加速度 */
	bool            suspend;
	bool            no_fiction;
	bool            no_focus;
	bool            no_box;
} Runtimedata_t;

/* 单帧动画处理
 * 输入处理*/
static bool frame(Runtimedata_t *dat)
{
	if (!dat || !dat->obj || !dat->camera || !dat->backend) return false;
	Vec_t accelr = (Vec_t){};

	/* 输入处理 */
	switch (kbhitGetchar()) {
	case ' ': accelr.y = (dat->v.y < 0 ? 1.5 : 0.5); break;
	case 'w': accelr.z = -0.1; break;
	case 's': accelr.z = +0.1; break;
	case 'a': accelr.x = -0.1; break;
	case 'd': accelr.x = +0.1; break;
	case 'j': dat->theta += 0.01*2*M_PI; break;
	case 'k': dat->theta -= 0.01*2*M_PI; break;
	case '+':
		  dat->v=vec_mul(dat->v, 0);
		  dat->theta=0;
		  dat->cav=vec_mul(dat->cav, 0);
		  break;

	case '-': dat->camera->scale-=1; break;
	case '=': dat->camera->scale+=1; break;
	case ',': dat->camera->dept-=1; break;
	case '.': dat->camera->dept+=1; break;
	case 'W': dat->cav.y+=0.01; break;
	case 'S': dat->cav.y-=0.01; break;
	case 'A': dat->cav.x-=0.01; break;
	case 'D': dat->cav.x+=0.01; break;
	case 'Q': dat->cav.z-=0.01; break;
	case 'E': dat->cav.z+=0.01; break;
	case 'J': camera_rotate(dat->camera, (Vec_t){-1,0,0}, M_PI/180); break;
	case 'K': camera_rotate(dat->camera, (Vec_t){1,0,0}, M_PI/180); break;
	case 'H': camera_rotate(dat->camera, (Vec_t){0,0,1}, M_PI/180); break;
	case 'L': camera_rotate(dat->camera, (Vec_t){0,0,-1}, M_PI/180); break;
	case 'F': dat->camera->position = vec_mul(dat->camera->position, 0),
		  camera_look(dat->camera, (Point_t){0,0,-1}, (Vec_t){0,1,0}); break;

	case '1': if (FPS > 1) FPS--; break;
	case '2': if (FPS < (uint8_t)-1) FPS++; break;
	case '3': dat->no_fiction=!dat->no_fiction; break;
	case '4': dat->rlt_accelr=!dat->rlt_accelr; break;
	case '5': dat->no_box=!dat->no_box; break;
	case 'f': dat->no_focus=!dat->no_focus; break;
	case 'r': setup(&dat->backend, dat->camera, dat->backend->id); break;
	case 'c': printf("\x1b[2J"); break;
	case '\t': setup(&dat->backend, dat->camera, dat->backend->id+1); break;
	case 'p': dat->suspend=!dat->suspend; break;
	case 'q': return true; break;
	}
	if (dat->rlt_accelr &&
	    (accelr.x != 0 || accelr.z != 0)) {
		Vec_t z = dat->camera->forward;
		z.y = 0;
		Vec_t x = vec_cross_product((Vec_t){0, 1, 0}, z);
		accelr = vec_add3((Vec_t){0, accelr.y, 0},
				  vec_mul(vec_direct(x), -accelr.x),
				  vec_mul(vec_direct(z), -accelr.z));
	}
	dat->v = vec_add(dat->v, accelr);
	if (dat->suspend) return false;

	camera_shift(dat->camera, dat->cav);
	obj_rotate(dat->obj, (Vec_t){0, 1, 0}, dat->theta/FPS);
	obj_shift(dat->obj, vec_mul(dat->v, 1./FPS));
	if (!dat->no_focus) camera_look(dat->camera, (Point_t){
		dat->obj->center.x, dat->obj->center.y+1, dat->obj->center.z},
		(Vec_t){0,1,0});

	if (!dat->no_fiction) {
		/* 速度衰减 */
		dat->v.x *= pow(0.99, 40./FPS);
		dat->v.z *= pow(0.99, 40./FPS);
		dat->theta *= pow(0.99, 40./FPS);
	}
	/* 自然下落 */
	dat->v.y -= 9.8/FPS;

	/* 类似碰撞处理 */
#define CP dat->obj->center
#define BOX(var, min, max, rate) \
	if ((CP.var > (max) && dat->v.var > 0) || (CP.var < (min) && dat->v.var < 0)) dat->v.var *= (rate)
	if (!dat->no_box) {
		BOX(x, -5, 5, -0.7);
		BOX(z, -50, 10, -0.7);
	}
	/* 地面碰撞处理 */
	static const int ground = -1;
	if (CP.y <= ground && dat->v.y <= 0 && dat->v.y > -0.5) {
		dat->obj->center.y = -1;
		dat->v.y = 0;
	} else BOX(y, ground, 10, -0.5);
#undef BOX
#undef CP
	return false;
}

void help(char *argv)
{
#define BACKEND(name) "  %2d : "#name"\n"
	printf("Usage: %s\n"
	       "    -h       打印此信息\n"
	       "    -b <NUM> 指定渲染输出后端\n"
	       "可用渲染后端:\n"
	       BACKEND_LIST,
#undef BACKEND
	       argv
#define BACKEND(name) ,RDBK_##name
	       BACKEND_LIST
	       );
#undef BACKEND
}

int main(int argc, char *argv[])
{
	int ch = 0;
	uint8_t chose_backend = 0;
	while ((ch=getopt(argc, argv, "hb:")) != -1) {
		switch (ch) {
		case 'b':
			sscanf(optarg, "%hhu", &chose_backend);
			break;
		case 'h':
			help(argv[0]);
			return 0;
			break;
		case '?':
			help(argv[0]);
			return 1;
			break;
		}
	}

	Runtimedata_t data = {
		.backend = NULL,
		.obj = NULL,
		.camera = camera_create(),
		.theta = -2*M_PI/5,    /* 每秒转动的角度(rad/s) */
		.v = (Vec_t){
			.x = 0,
			.y = 2.,
			.z = 3.5,
		},
	};
	if (!data.camera) return 0;
	setup(&data.backend, data.camera, chose_backend);
	if (!data.backend) return 0;

	Obj_t *line = obj_create((Point_t){0, 0, 0},
				 4, (Point_t[]){ {-2,-1,50},{-2,-1,-50}, {2,-1,50},{2,-1,-50}, },
				 2, (Line_t[]){ {0,1}, {2,3}, },
				 0, NULL);
	for (int i = 0; i < 50; i++) {
		obj_merge_and_free(line, obj_apply_shift(obj_create_line_from_point((Point_t){2,-1,i+2}, (Point_t){-2,-1,i+2})));
	}

	Obj_t *block = obj_create_image_from_str((Point_t){0.3, 1, -7}, 0.1,
"####################\n"
"####....###....#####\n"
"###.#######.###.####\n"
"####...####.....####\n"
"#######.###.###.####\n"
"###....####....#####\n"
"####################\n"
"####################\n"
"#.#######.###....###\n"
"#.#######.###.###.##\n"
"#.#######.###...####\n"
"#.####.##.###.##.###\n"
"#...###...###.###.##\n"
"####################\n"
"####################\n"
"#.#.#.#.#.##..#.#.##\n"
"#...##...##..###.###\n"
"#.#.##.#.###.##.#.##\n"
"###########.########\n"
"####################\n", '.');
	obj_merge_and_free(block, obj_create_box_from_point((Point_t[]){
{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1},{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1} }));
	obj_transform_shift(block, (Vec_t){.x=0,.y=1,.z=0});    /* 让参考中心点下移一格 */
	/*obj_rotate(block, (Vec_t){0, 1, 0}, M_PI*5);*/
	data.obj = block;

	printf("\x1b[2J");
	size_t i = 0;
	double busy = 0, skip_farme = 0;
	for (i = 0; i < MAX_FRAME; ++i) {
		skip_farme = 0;
		while (busy - skip_farme >= 100) {
			if (frame(&data)) {
				i = MAX_FRAME;
				break;
			}
			skip_farme += 100.;
		}
		if (frame(&data)) break;
		if (!data.backend) break;
		obj_cast(line, data.camera, data.backend);
		obj_cast(block, data.camera, data.backend);

		printf("\033[H");
		data.backend->render(data.backend);
		data.backend->clean(data.backend);

		printf("=> FPS: %d, VS: %.0f, VD: %.0f, BS: %5.1f%%, F: %ld "
		       "%s%s%s%s%s %s\n",
		       FPS, data.camera->scale, data.camera->dept, busy, i,
		       data.rlt_accelr||data.no_fiction||data.no_box ? "[" : "",
		       data.no_fiction ? "F" : "",
		       data.rlt_accelr ? "R" : "",
		       data.no_box ? "B" : "",
		       data.rlt_accelr||data.no_fiction||data.no_box ? "]" : "",
		       busy < 100 ? "           ": "(OVERLOAD) ");
		printf("=> Center xyz: %.3f, %.3f, %.3f \n",
		       block->center.x,
		       block->center.y,
		       block->center.z
		       );
		printf("=> Speed xyzr: %.3f, %6.3f, %.3f, %.3fr/s \n",
		       data.v.x, data.v.y, data.v.z, (data.theta)/(2*M_PI));
		printf("=> Ca P/V xyz: (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f) \n",
		       data.camera->position.x, data.camera->position.y, data.camera->position.z,
		       data.cav.x, data.cav.y, data.cav.z);
#ifdef _WIN32
		Sleep(1./FPS * 1e3);
#else
		busy = (busy + (1-(sleep_fixed_step(1./FPS))/(1./FPS)) * 100)/2;
#endif
	}
	obj_free(line);
	obj_free(block);
	if (data.backend) data.backend->destroy(data.backend);
	camera_free(data.camera);
	printf("[INFO] 退出程序\n");
	return 0;
}

