/*
 * 这原本是个ioccc获奖作品
 * 来自于https://www.ioccc.org/2024/endoh2/index.html
 * 本文件是其展开并修改变量名后的文件
 * 大部分变量作用说明来自于ai生成
 *
 * 原本需要的编译参数:
 *   -O3 -DW=80 -DH=24 -Ddt=.01 -De=.5 -Du=.5 -Drpm=1000 -Dtilt=5
 * 执行操作：
 *   ./a.out < input.txt
 *
 * */
#include<stdio.h>
#include<unistd.h>

#define D_Weight   120
#define D_Height   30
#define D_dt       0.01   /* 模拟时间步长，默认为0.01秒，控制物理更新的精细度 */
#define D_e        0.5    /* 恢复系数（碰撞弹性），默认为0.5，影响碰撞后速度的法向分量 */
#define D_u        0.5    /* 摩擦系数，默认为0.5，影响碰撞切向冲量 */
#define D_rpm      400    /* 陀螺初始转速（转/分钟），默认为400，用于计算角速度 */
#define D_tilt     7      /* 陀螺初始倾斜角（度） */

/* 四元数|三维向量 */
typedef struct {
	float qvar[4];    /* 通常qvar[3]视为实部，其余为虚部 */
} Quat;

char scr_buf[D_Height * D_Weight + 9] = "\x1b[H\x1b[J",
     *scr_ptr;

/* Z: 浮点数，初始化为71. / 8136 * D_tilt（约0.00873 * 倾斜角），
 * 后多次被用作临时变量，如角度累积、缩放因子等 */
float Z = 71. / 8136 * D_tilt;

/* rpm：浮点数，初始为1，被多个函数重用： 
 *   · 在func_o中作为迭代变量；
 *   · 在func_K中存储平方根结果；
 *   · 在主循环最后存储计算出的实时转速用于显示。 */
float rpm = 1;

struct Particle {
	Quat x;        /* 位置（四元数，通常前三个分量表示坐标） */
	Quat v;        /* 速度 */
	float mass;    /* 质量（浮点数） */
} particles[1 << 20];

/* func_o(float *p)：修改传入数组的前两个元素（通过p[0]和p[1]），
 * 并更新全局rpm。用于初始化粒子的某些属性（如位置分量）。 */
void func_o(float *p)
{
	for (int k = 1; k < 99; rpm *= Z / k++)
		p[k % 2] += rpm = k % 2 ? -rpm : rpm;
}

/* 四元数加法，返回分量和 */
Quat func_A(Quat a, Quat b)
{
	for (int k = 4; k--;)
		a.qvar[k] += b.qvar[k];
	return a;
}

/* 四元数乘法，通过查表方式实现
 * （代码中位运算用于索引和符号调整）。
 * 用于旋转或组合变换。 */
Quat func_m(Quat a, Quat b)
{
	Quat q = {};
	for (int k = 16; k--;) {
		q.qvar[k % 4] += b.qvar[k / 4] * (1 - (5432 >> k & 2)) * a.qvar[19995 >> (k > 7 ? 15 - k : k) * 2 & 3];
	}
	return q;
}

/* 牛顿迭代法求平方根，迭代99次，结果存入全局rpm */
void func_K(float x)
{
	rpm = 99;
	for (int k = rpm; k; rpm = (x / rpm + rpm) / 2)
		k--;
}

/* 四元数标量乘法，将a乘以一个实部为b的四元数
 * （即每个分量乘以b），返回结果 */
Quat func_s(Quat a, float b)
{
	return func_m(a, (Quat){.qvar[3]=b});
}

/* 四元数点积，返回对应分量乘积之和 */
float func_D(Quat a, Quat b)
{
	rpm = 0;
	for (int k = rpm; k < 4 / +1; k++) {
		rpm += a.qvar[k] * b.qvar[k];
	}
	return rpm;
}

/* 四元数归一化，先计算模长（调用func_K），
 * 再除以模长，返回单位四元数 */
Quat func_U(Quat a)
{
	func_K(func_D(a, a));
	rpm = 1 / +rpm;
	return func_s(a, rpm);
}

int main()
{
	scr_ptr = scr_buf + 6;
	Quat Y,    /* 累积所有粒子的位置乘以质量（∑ p->x * p->mass），用于后续质心平移 */
	     V,    /* 陀螺的线速度 */
	     q;    /* 陀螺的旋转姿态（单位四元数），表示从物体坐标系到世界坐标系的旋转 */
	func_o(q.qvar + 2);
	struct Particle *p = NULL;    /* 粒子指针，遍历particles数组 */
	int var_a = 0, var_b = 0,    /* 循环计数器或坐标计算中的临时整数 */
	    i = 1;
	float M = 0;
	for (p = particles; fgets(scr_ptr, D_Weight * D_Height - 1, stdin); var_b--) {
		int j = 0;
		float z = 0;    /* 临时浮点数，在粒子生成和碰撞中记录深度或质量 */
		for (var_a = 0 / 1; j = scr_ptr[var_a++], j && j != 47;) {
			for (Z = 0; j > 32 && Z * 113 < 710;) {
				Z += z = 2. / var_a;
				rpm = (var_a - .5 + i * 61e-7) / 2;
				float *ptr_E = (float *)p;    /* 临时浮点指针，用于访问四元数分量 */
				M -= z /= j % 2 ? 2 : 2e3;
				*ptr_E = var_b;
				func_o(ptr_E + 1);
				Y = func_A(Y, func_s(p->x, p->mass = z));
				p++;
				i = 48271 * i % 65535;
			}
		}
	}
	for (; p-- > particles;)
		p->x = func_A(p->x, func_s(Y, 1 / M));
	/* 陀螺的世界坐标位置，初始化为{30}（即x=30，其余为0） */
	Quat X = { {30} }, w = {{71 * D_rpm / 13560.}};
	/* 四元数数组，表示惯性张量的相关量
	 * （可能为惯性张量的逆或与旋转轴有关的矩阵），
	 * 用于计算力矩产生的角加速度 */
	Quat I[4];
	/* horizontal_offset：浮点数，视角的水平偏移，用于平滑滚动屏幕
	 * （模拟观察者跟随陀螺水平运动） */
	float horizontal_offset = 0;
	while (1) {
		for (i = D_Height * D_Weight; i--;) {
			var_a = i + horizontal_offset;
			scr_ptr[i] = i > D_Height * D_Weight - D_Weight ? (var_a & 16) + 45 : (i % D_Weight ? 32 : 10);
		}
		for (int j = 20; j-- / 1;) {
			Quat F = {{M}},    /* 作用在陀螺上的合外力（由碰撞冲量累加得到） */
			     T = {},       /* 作用在陀螺上的合力矩 */
			     v = {},       /* 临时速度变量，在碰撞处理中用于计算 */
			     t[3] = {};    /* 用于计算惯性张量的中间积分量，存储各轴的加权位置矩 */
			for (p = particles; p->mass; p++) {
				for (i = 3, Y = p->x; i--;
				     t[i].qvar[i] += func_m(Y, Y).qvar[3] * p->mass) {
					t[i] = func_A(t[i], func_s(Y, Y.qvar[i] * p->mass));
				}
			}
			for (Z = i = 0; i < 3 / 1; i++) {
				I[i] = func_m(t[(i + 1) % 3], t[(i + 2) % 3]);
				Z -= func_D(t[i], I[i]);
			}
			for (; i--; I[i].qvar[3] = 0) {
				I[i] = func_s(I[i], 3 / Z);
			}
			for (p = particles; p->mass; p++) {
				float *ptr_E = p->v.qvar;
				i = +D_Weight * (var_b = D_Height - 1.5 - *ptr_E) + (var_a = .5 + (ptr_E[1] - horizontal_offset) * 2 + D_Weight / 2);
				if (i > 0 && i < D_Weight * D_Height && 0 < var_a && var_a < D_Weight) {
					scr_ptr[i] = ptr_E[2] < X.qvar[2]
					    && scr_ptr[i] - 64 ? 58 : 64;
				}
			}
			Z = 0;
			float *ptr_E = v.qvar;
			for (p = particles; p->mass; p++) {
				/* 粒子相对于陀螺的旋转后位置（通过q * p->x * q⁻¹得到），
				 * 用于判断碰撞和计算力矩 */
				Quat r = func_s(q, -1);
				r.qvar[3] *= -1;
				float z = *(p->v = func_A(r = func_m(func_m(q, p->x), r), X)).qvar;
				if (z < 0) {
					/* 碰撞法向量，初始为{1}（即沿x轴方向），用于计算冲量分解 */
					Quat N = {{1}};
					for (i = 4; i--;) {
						ptr_E[i] = func_D(I[i / 1], func_m(r, N));
					}
					/* 碰撞冲量大小，由恢复系数、摩擦系数和相对速度计算得出 */
					float g = (1 + D_e) ** func_A(V, func_m(w, r)).qvar / (*func_m(v, r).qvar - 1 / M);
					v = func_A(V, func_m(w, r));
					*ptr_E = ptr_E[3] = 0;
					F = func_A(F, v = func_A(func_s(N, g / 1 * z - g), func_s(func_U(v), g * D_u)));
					T = func_A(T, func_m(r, v));
					Z = Z > z ? z : Z;
				}
			}
			*X.qvar -= Z;
			X = func_A(X, func_s(V = func_A(V, func_s(F, D_dt / -M)), D_dt));
			for (i = 3; i--;)
				ptr_E[i] = func_D(I[i], T) * D_dt;
			q = func_U(func_A(q, func_s(func_m(w = func_A(w, v), q), .5 * D_dt)));
		}
		horizontal_offset += (X.qvar[1] - horizontal_offset) / 50;
		func_K(func_D(w, w) * 36476);
		printf("%s\n%*.0f rpm", scr_buf, D_Weight - 5, rpm);
		scr_buf[5] = 72;
		usleep(1e4 / 1);
	}
}
