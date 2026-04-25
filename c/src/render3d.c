/**
 * @file        render3d.c
 * @author      u0_a221
 * @date        2026-04-06
 * @brief       通过ascii艺术画形式“渲染”伪3D画面
 */

#include "tools.h"
#include <math.h>

/* 辅助程序(Py)：
str(list([(k/10,i,j), (i, j, k/10), (i, k/10, j)] for k in range(-10, 10) for j in range(-1, 2, 2) for i in range(-1, 2, 2))).replace("(","{").repla ce(")","}").replace("[","").replace("]","").replace(" ","")
*/

#define SECOND 1000000
#ifndef FPS
#define FPS 40
#endif
#define MAX_FRAME 100000
/* 虚拟投影屏幕边界大小(非实际显示)，效果类似视差？
 * 值越大视角越窄(除非终端够大)透视效果越不明显 */
uint16_t VIEW_SCALE = 70;
uint16_t VIEW_DEPT  = 7;
double VIEW_OFFSETY = 0;
/* 宽高比校正因子。
 * 终端字符通常高度大于宽度（例如一个字符单元格宽高比约 1:2），
 * 若不校正，渲染出的圆形会变成纵向压扁的椭圆。
 * 该值用于缩放投影后的 Y 坐标，使视觉上形状符合预期。
 * 值越大，Y 方向压缩越强；值越小，Y 方向拉伸越强。
 * 通常设为 2.0 可大致校正常见终端的字符宽高比。(ai生成)
 */
double VIEW_AR = 2.;

static const char CHRTABLE[] = {
	"$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft"
	"/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. "
};
static const int MAXCHR = sizeof(CHRTABLE)-2;
/*#define LENOF(var) (sizeof(var)/sizeof(*var))*/

typedef struct {
	char *scr;
	size_t h;
	size_t w;
	size_t size;
} Scr_t;

Scr_t *scr_create(size_t width, size_t height)
{
	if (width == 0 || height == 0) return NULL;
	Scr_t *p = malloc(sizeof(*p));
	size_t size = width * height + 1;
	*p = (Scr_t){
		.scr = malloc(size),
		.h = height,
		.w = width,
		.size = size,
	};
	memset(p->scr, MAXCHR, p->size);
	return p;
}

void scr_free(Scr_t **p)
{
	if (!p || !*p) return;
	free((*p)->scr);
	free(*p);
	*p = NULL;
}

/* x,y : [0, MAX)
 * return: 屏幕字符指针或NULL */
char *scr_p(Scr_t *s, int x, int y)
{
	if (!s) return NULL;
	if (x < 0 || x >= s->w || y < 0 || y >= s->h) return NULL;
	return s->scr + y*s->w + x;
}
/* 中央原点屏幕坐标系 转换成 左上角原点屏幕坐标系 */
char *scr_c(Scr_t *s, int x, int y)
{
	if (!s) return NULL;
	return scr_p(s, s->w/2+(int)(x), s->h/2-(int)(y));
}

/* 灰度映射 打印前安全处理 打印 重置屏幕 */
void scr_print(Scr_t *s)
{
	if (!s) return;
	static int j = 0;
	static int c = 0;
	for (j = 0; j < s->size; j++) {
		c = s->scr[j] > MAXCHR ? MAXCHR : (uint32_t)s->scr[j];
		s->scr[j] = CHRTABLE[c];
	}
	for (j = 0; j < s->h; j++) s->scr[s->w*(j+1)-1] = '\n';
	s->scr[s->size-1] = 0;
	printf("\033[H%s", s->scr);
	memset(s->scr, MAXCHR, s->size);
}


/* 向量 | 点 | 物体 */
typedef struct {
	double x;
	double y;
	double z;
} Vec_t, Point_t;

/* 向量加法 */
static inline Vec_t vec_add(Vec_t v1, Vec_t v2)
{
	return (Vec_t){
		.x = v1.x+v2.x,
		.y = v1.y+v2.y,
		.z = v1.z+v2.z,
	};
}

/* 向量减法 */
static inline Vec_t vec_sub(Vec_t v1, Vec_t v2)
{
	return (Vec_t){
		.x = v1.x-v2.x,
		.y = v1.y-v2.y,
		.z = v1.z-v2.z,
	};
}

/* 向量数乘 */
static inline Vec_t vec_mul(Vec_t v, double k)
{
	return (Vec_t){
		.x = v.x*k,
		.y = v.y*k,
		.z = v.z*k,
	};
}

/* 向量点乘 */
static inline double vec_point_product(Vec_t a, Vec_t b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

/* 向量叉乘 */
static inline Vec_t vec_cross_product(Vec_t a, Vec_t b)
{
	return (Vec_t){
		.x = a.y*b.z - a.z*b.y,
		.y = a.z*b.x - a.x*b.z,
		.z = a.x*b.y - a.y*b.x,
	};
;
}

/* 向量长度 */
static inline double vec_len(Vec_t v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

/* 将点投影到虚拟屏幕上 */
void point_cast(Point_t p, Scr_t *s)
{
	if (p.z >= 0) return;
	char *scrp = scr_c(s,
			   p.x/-p.z*VIEW_SCALE,
			   (p.y+VIEW_OFFSETY)/VIEW_AR/-p.z*VIEW_SCALE);
	if (!scrp) return;
	uint8_t dept = -p.z*MAXCHR/VIEW_DEPT;
	if (dept> MAXCHR) dept = MAXCHR;
	if (*scrp > dept) *scrp = dept;
	return;
}

typedef struct {
	Point_t center;
	size_t count_point;
	Point_t *points;
} Obj_t;

/* 创建物体
 * size: 点数
 * center: 初始位置
 * points: 点云 */
Obj_t *obj_create(int size, Point_t center, Point_t *points)
{
	if (size <= 0 || !points) return NULL;
	Obj_t *p = malloc(sizeof(Obj_t));
	*p = (Obj_t){
		.center = center,
		.count_point = size,
		.points = malloc(size*sizeof(Point_t)),
	};
	memcpy(p->points, points, size*sizeof(Point_t));
	return p;
}

/* 绕指定轴旋转 */
void obj_rotate(Obj_t *obj, Vec_t direction, double theta)
{
#ifdef NO_ROTATE
	return;
#endif
	if (!obj || !obj->points || !theta || !vec_len(direction)) return;
	Point_t *p = obj->points;
	direction = vec_mul(direction, 1/vec_len(direction));
	double s = sin(theta), c = cos(theta);
	for (int j = 0 ; j < obj->count_point; j++,p++) {
		*p = vec_add(vec_add(vec_mul(*p, c),
				     vec_mul(vec_cross_product(*p, direction), s)),
			     vec_mul(direction, vec_point_product(direction, *p)*(1-c)));
	}
}

/* 将物体的各点相对于中心点沿给定的Vec方向移动 */
static inline Obj_t *obj_transform_shift(Obj_t *obj, Vec_t v)
{
	if (!obj || !obj->points) return obj;
	Point_t *p = obj->points;
	for (int j = 0 ; j < (int64_t)obj->count_point; j++,p++) {
		*p = (Point_t){
			.x = p->x + v.x,
			.y = p->y + v.y,
			.z = p->z + v.z,
		};
	}
	return obj;
}
#define obj_apply_shift(obj) obj_transform_shift((obj), (obj)->center)

/* 将物体沿给定的Vec方向移动 */
static inline Obj_t *obj_shift(Obj_t *obj, Vec_t v)
{
	if (!obj || !obj->points) return obj;
	obj->center = vec_add(obj->center, v);
	return obj;
}

/* 投影物体 */
void obj_cast(Obj_t *obj, Scr_t *scr)
{
	static int j = 0;
	for (j = 0 ; j < obj->count_point; j++)
		point_cast(vec_add(obj->points[j], obj->center), scr);
}

bool obj_merge(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;
	Point_t *points = realloc(obj->points,
				  sizeof(*points)*(obj->count_point+from->count_point));
	if (!points) return false;
	memcpy(points+obj->count_point, from->points, sizeof(*points)*from->count_point);
	/* realloc已经释放内存了 */
	/*free(obj->points);*/
	obj->points = points;
	obj->count_point += from->count_point;
	return true;
}

void obj_free(Obj_t **obj)
{
	if (!obj || !*obj) return;
	free((*obj)->points);
	free(*obj);
	*obj = NULL;
}

bool obj_merge_and_free(Obj_t *obj, Obj_t *from)
{
	if (!obj || !from) return false;
	if (!obj_merge(obj, from)) return false;
	obj_free(&from);
	return true;
}

Obj_t *obj_create_line_from_point(Point_t t1, Point_t t2, size_t count)
{
	if (count == 0 || count > 1024) return NULL;
	Point_t center = vec_mul(vec_add(t1, t2), 0.5),
		dt = vec_sub(t2, t1),
		t[count];
	for (int i = 0; i < count && i < (sizeof(t)/sizeof(*t)); i++) {
		t[i] = vec_mul(dt, -0.5+(double)i/count);
	}
	return obj_create(count, center, t);
}

/* 从8顶点创建立方体
 * 前四|后四：两对立面的四个顶点
 * 边：p[n]与p[n+1]相连(n<=3) */
Obj_t *obj_create_box_from_point(size_t count, Point_t points[8])
{
#define oclfp(i1, i2) obj_create_line_from_point(points[i1], points[i2], count)
	Obj_t *box = oclfp(0, 4), *line = NULL;
	obj_apply_shift(box);
	for (int i = 1; i < 12; i++) {
		if (i < 4) line = oclfp(i, i+4);    /* 1~3 */
		else if (i < 8) line = oclfp(i-4, (i-3)%4);    /* 4~7 */
		else line = oclfp(i-4, (i-3)%4+4);    /* 8~11 */

		if (line == NULL) break;
		obj_apply_shift(line);
		obj_merge(box, line);
		obj_free(&line);
		line = NULL;
	}
	return box;
#undef oclfp
}

/* 单帧动画处理
 * 输入处理*/
bool frame(Obj_t *obj, double *theta, Vec_t *v)
{
	if (!obj) return false;
	static bool suspend = false;

	/* 输入处理 */
	switch (kbhitGetchar()) {
	case ' ': v->y += (v->y < 0 ? 1.5 : 0.5)/FPS; break;
	case 'w': v->z -= 0.1/FPS; break;
	case 's': v->z += 0.1/FPS; break;
	case 'a': v->x -= 0.1/FPS; break;
	case 'd': v->x += 0.1/FPS; break;
	case 'k': *theta += 0.01*2*M_PI/FPS; break;
	case 'j': *theta -= 0.01*2*M_PI/FPS; break;
	case '-': VIEW_SCALE -= 1; break;
	case '=': VIEW_SCALE += 1; break;
	case '+': *v=vec_mul(*v, 0), *theta=0; break;
	case 'W': VIEW_OFFSETY-=0.01; break;
	case 'S': VIEW_OFFSETY+=0.01; break;
	case '9': VIEW_DEPT--; break;
	case '0': VIEW_DEPT++; break;
	case ',': if (VIEW_AR >= 0.05) VIEW_AR-=0.01; break;
	case '.': VIEW_AR+=0.01; break;
	case 'p': suspend=!suspend; break;
	case 'q': return true; break;
	}
	if (suspend) return false;

	obj_rotate(obj, (Vec_t){0, 1, 0}, *theta);
	obj_shift(obj, *v);

	/* 速度衰减 */
	v->x *= pow(0.99, 40./FPS);
	v->z *= pow(0.99, 40./FPS);
	*theta *= pow(0.99, 40./FPS);
	/* 自然下落 */
	v->y -= 0.098/FPS;

	/* 类似碰撞处理 */
#define CP obj->center
#define BOX(var, min, max, rate) \
	if ((CP.var > (max) && v->var > 0) || (CP.var < (min) && v->var < 0)) \
	v->var *= (rate);
	BOX(x, -5, 5, -0.7);
	BOX(z, -50, -2, -0.7);
	BOX(y, -1, 10, -0.5);
#undef BOX
#undef CP
	return false;
}

Scr_t *base_init()
{
	int scr_h = get_winsize_row() - 5,
	    scr_w = get_winsize_col() - 1;
	if (scr_h <= 10 || scr_w <= 10) {
		LOG("终端太小(当前可用尺寸：%dx%d)", scr_w, scr_h);
		return NULL;
	}
	return scr_create(scr_w, scr_h);
}

int main(void)
{
	Scr_t *scr = base_init();
	if (!scr) return 0;

	Obj_t *line[] = {
		obj_create_line_from_point((Point_t){-2,-1,1}, (Point_t){-2,-1,-50}, 100),
		obj_create_line_from_point((Point_t){2,-1,1}, (Point_t){2,-1,-50}, 100)
	};
	Obj_t *block = obj_create(5*17-1,
			(Point_t){0, 1, -7},
			(Point_t[]){
{-0.6,0.9,0},{-0.5,0.9,0},{-0.4,0.9,0},{-0.3,0.9,0},{0.1,0.9,0},
{0.2,0.9,0},{0.3,0.9,0},{0.4,0.9,0},{-0.7,0.8,0},{0.1,0.8,0},
{0.5,0.8,0},{-0.6,0.7,0},{-0.5,0.7,0},{-0.4,0.7,0},{0.1,0.7,0},
{0.2,0.7,0},{0.3,0.7,0},{0.4,0.7,0},{0.5,0.7,0},{-0.3,0.6,0},
{0.1,0.6,0},{0.5,0.6,0},{-0.7,0.5,0},{-0.6,0.5,0},{-0.5,0.5,0},
{-0.4,0.5,0},{0.1,0.5,0},{0.2,0.5,0},{0.3,0.5,0},{0.4,0.5,0},
{-0.9,0.2,0},{-0.1,0.2,0},{0.3,0.2,0},{0.4,0.2,0},{0.5,0.2,0},
{0.6,0.2,0},{-0.9,0.1,0},{-0.1,0.1,0},{0.3,0.1,0},{0.7,0.1,0},
{-0.9,0,0},{-0.1,0,0},{0.3,0,0},{0.4,0,0},{0.5,0,0},
{-0.9,-0.1,0},{-0.4,-0.1,0},{-0.1,-0.1,0},{0.3,-0.1,0},{0.6,-0.1,0},
{-0.9,-0.2,0},{-0.8,-0.2,0},{-0.7,-0.2,0},{-0.3,-0.2,0},{-0.2,-0.2,0},
{-0.1,-0.2,0},{0.3,-0.2,0},{0.7,-0.2,0},{-0.9,-0.5,0},{-0.7,-0.5,0},
{-0.5,-0.5,0},{-0.3,-0.5,0},{-0.1,-0.5,0},{0.2,-0.5,0},{0.3,-0.5,0},
{0.5,-0.5,0},{0.7,-0.5,0},{-0.9,-0.6,0},{-0.8,-0.6,0},{-0.7,-0.6,0},
{-0.4,-0.6,0},{-0.3,-0.6,0},{-0.2,-0.6,0},{0.1,-0.6,0},{0.2,-0.6,0},
{0.6,-0.6,0},{-0.9,-0.7,0},{-0.7,-0.7,0},{-0.4,-0.7,0},{-0.2,-0.7,0},
{0.2,-0.7,0},{0.5,-0.7,0},{0.7,-0.7,0},{0.1,-0.8,0},
			});
	obj_merge_and_free(block, obj_create_box_from_point(20, (Point_t[]){
{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1},{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1} }));
	obj_transform_shift(block, (Vec_t){.x=0,.y=1,.z=0});
	obj_rotate(block, (Vec_t){0, 1, 0}, -M_PI*5/FPS);

	double theta = M_PI/8/FPS;
	Vec_t v = (Vec_t){
		.x = 0,
		.y = 2./FPS,
		.z = 3.2/FPS,
	};

	printf("\x1b[2J");
	int i = 0, j = 0;
	for (i = 0; i < MAX_FRAME; ++i) {
		if (frame(block, &theta, &v)) break;
		for (j = 0 ; j < sizeof(line)/sizeof(*line); j++) {
			obj_cast(line[j], scr);
		}
		obj_cast(block, scr);
		scr_print(scr);

		printf("=> FPS: %d, VS: %d, VD: %d, VA: %.2lf, F: %d \n",
		       FPS, VIEW_SCALE, VIEW_DEPT, VIEW_AR, i);
		printf("=> Center xyz: %.3f, %.3f, %.3f \n",
		       block->center.x,
		       block->center.y,
		       block->center.z
		       );
		printf("=> Speed xyzr: %.3f, %6.3f, %.3f, %.3fr/s \n",
		       v.x, v.y, v.z, (theta*FPS)/(2*M_PI));
		usleep(SECOND/FPS);
	}
	for (j = 0 ; j < sizeof(line)/sizeof(*line); j++) {
		obj_free(&line[j]);
	}
	obj_free(&block);
	scr_free(&scr);
	printf("[INFO] 退出程序\n");
	return 0;
}

