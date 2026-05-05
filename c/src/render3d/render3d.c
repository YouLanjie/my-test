/**
 * @file        render3d.c
 * @author      u0_a221
 * @date        2026-04-06
 * @brief       通过ascii艺术画形式“渲染”伪3D画面
 */

#include "lib/render3d.h"

#define MAX_FRAME 100000
uint8_t FPS = 40;

/* 由于日益严重的延迟因而使用特定的时钟避免usleep的额外等待 */
void my_sleep()
{
	static struct timespec t = {0};
	if (t.tv_sec == 0 && t.tv_nsec == 0) clock_gettime(CLOCK_MONOTONIC, &t);
	t.tv_nsec += 1e9 / FPS;
	if (t.tv_nsec >= 1e9) {
		t.tv_sec++;
		t.tv_nsec -= 1e9;
	}
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
}

static bool setup(RenderBackend_t **bk, Camera_t *ca, uint8_t num)
{
	if (!bk) return false;
	if (*bk) (*bk)->destory(*bk);
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

/* 单帧动画处理
 * 输入处理*/
static bool frame(RenderBackend_t **backend, Camera_t *camera, Obj_t *obj, double *theta, Vec_t *v)
{
	if (!obj || !theta || !v || !camera || !backend) return false;
	static bool suspend = false;
	static bool no_fiction = false;
	static bool focus = true;

	/* 输入处理 */
	switch (kbhitGetchar()) {
	case ' ': v->y += (v->y < 0 ? 1.5 : 0.5); break;
	case 'w': v->z -= 0.1; break;
	case 's': v->z += 0.1; break;
	case 'a': v->x -= 0.1; break;
	case 'd': v->x += 0.1; break;
	case 'j': *theta += 0.01*2*M_PI; break;
	case 'k': *theta -= 0.01*2*M_PI; break;
	case '+': *v=vec_mul(*v, 0), *theta=0; break;

	case '-': camera->scale-=1; break;
	case '=': camera->scale+=1; break;
	case ',': camera->dept-=1; break;
	case '.': camera->dept+=1; break;
	case 'W': camera->position.z-=0.01; break;
	case 'S': camera->position.z+=0.01; break;
	case 'A': camera->position.x-=0.01; break;
	case 'D': camera->position.x+=0.01; break;
	case 'J': camera_rotate(camera, (Vec_t){-1,0,0}, M_PI/180); break;
	case 'K': camera_rotate(camera, (Vec_t){1,0,0}, M_PI/180); break;
	case 'H': camera_rotate(camera, (Vec_t){0,0,1}, M_PI/180); break;
	case 'L': camera_rotate(camera, (Vec_t){0,0,-1}, M_PI/180); break;
	case 'F': camera_look(camera, (Point_t){0,0,-1}, (Vec_t){0,1,0}); break;

	case '1': if (FPS > 1) FPS--; break;
	case '2': if (FPS < (uint8_t)-1) FPS++; break;
	case 'f': focus=!focus; break;
	case '`': no_fiction=!no_fiction; break;
	case 'r': setup(backend, camera, (*backend)->id); break;
	case '\t': setup(backend, camera, (*backend)->id+1); break;
	case 'p': suspend=!suspend; break;
	case 'q': return true; break;
	}
	if (suspend) return false;

	obj_rotate(obj, (Vec_t){0, 1, 0}, *theta/FPS);
	obj_shift(obj, vec_mul(*v, 1./FPS));
	if (focus) camera_look(camera, (Point_t){
			       obj->center.x, obj->center.y+1, obj->center.z},
			       (Vec_t){0,1,0});

	if (!no_fiction) {
		/* 速度衰减 */
		v->x *= pow(0.99, 40./FPS);
		v->z *= pow(0.99, 40./FPS);
		*theta *= pow(0.99, 40./FPS);
	}
	/* 自然下落 */
	v->y -= 9.8/FPS;

	/* 类似碰撞处理 */
#define CP obj->center
#define BOX(var, min, max, rate) \
	if ((CP.var > (max) && v->var > 0) || (CP.var < (min) && v->var < 0)) v->var *= (rate)
	BOX(x, -5, 5, -0.7);
	BOX(z, -50, 10, -0.7);
	/* 地面碰撞处理 */
	static const int ground = -1;
	if (CP.y <= ground && v->y <= 0 && v->y > -0.5) {
		obj->center.y = -1;
		v->y = 0;
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

	Camera_t *camera = camera_create();
	if (!camera) return 0;
	RenderBackend_t *backend = NULL;
	setup(&backend, camera, chose_backend);
	if (!backend) return 0;

	Obj_t *line = obj_create((Point_t){0, 0, 0}, 0, NULL, 2,
				 (Line_t[]){ {{-2,-1,1},{-2,-1,-50}}, {{2,-1,1},  {2,-1,-50}}, },
				 0, NULL);
	for (int i = 0; i < 50; i++) {
		obj_merge_and_free(line, obj_create_line_from_point((Point_t){2,-1,i+2}, (Point_t){-2,-1,i+2}));
		obj_merge_and_free(line, obj_create_line_from_point((Point_t){2,-1,i+2}, (Point_t){10,5,i+2}));
		obj_merge_and_free(line, obj_create_line_from_point((Point_t){-2,-1,i+2}, (Point_t){-10,5,i+2}));
	}
	Obj_t *block = obj_create_image_from_str((Point_t){0, 1, -7}, 0.1,
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
	obj_rotate(block, (Vec_t){0, 1, 0}, M_PI*5);

	double theta = -2*M_PI/5;    /* 每秒转动的角度(rad/s) */
	Vec_t v = (Vec_t){
		.x = 0,
		.y = 2.,
		.z = 3.5,
	};

	printf("\x1b[2J");
	size_t i = 0;
	for (i = 0; i < MAX_FRAME; ++i) {
		if (frame(&backend, camera, block, &theta, &v)) break;
		if (!backend) break;
		obj_cast(line, camera, backend);
		obj_cast(block, camera, backend);
		backend->render(backend);
		backend->clean(backend);

		printf("=> FPS: %d, VS: %.1f, VD: %.1f, F: %ld \n",
		       FPS, camera->scale, camera->dept, i);
		printf("=> Center xyz: %.3f, %.3f, %.3f \n",
		       block->center.x,
		       block->center.y,
		       block->center.z
		       );
		printf("=> Speed xyzr: %.3f, %6.3f, %.3f, %.3fr/s \n",
		       v.x, v.y, v.z, (theta)/(2*M_PI));
		my_sleep();
	}
	obj_free(&line);
	obj_free(&block);
	if (backend) backend->destory(backend);
	printf("[INFO] 退出程序\n");
	return 0;
}

