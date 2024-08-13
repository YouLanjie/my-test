/*
 *   Copyright (C) 2024 Chglish
 *
 *   文件名称：a.c
 *   创 建 者：Chglish
 *   创建日期：2024年08月11日
 *   描    述：
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define sigmoid(x) (1.0 / (1.0 + exp(-x)))

typedef struct node node;
struct node {
	int weight_len;    /* 权重长度 */
	double *weight;    /* 权重 */
	double bias;       /* 偏置 */
	double output;     /* 输出 */

	double *w_want;    /* 权重改变期望 */
	double b_want;     /* 偏置改变期望 */
	double o_want;     /* 结果改变期望 */
};

node *create_node(int last_layer)
{
	node *p = malloc(sizeof(node));
	p->bias = (rand() % 1000) / 1000.0;
	p->b_want = 0;
	p->output = 0;
	p->o_want = 0;
	p->weight_len = 0;

	if (last_layer <= 0) return p;
	p->weight_len = last_layer;
	p->weight = malloc(sizeof(double)*last_layer);
	p->w_want = malloc(sizeof(double)*last_layer);
	for (int i = 0; i < last_layer; i++) {
		p->weight[i] = (rand() % 2000 - 1000) / 1000.0;
	}
	memset(p->w_want, 0, sizeof(double)*last_layer);
	return p;
}

#define HX 16
#define HY 1
node *layerI[2], *layerH[HY][HX], *layerO[4];

int init()
{
	for (int i = 0; i < 2; i++) layerI[i] = create_node(0);
	for (int i = 0; i < HX; i++) layerH[0][i] = create_node(2);
	for (int i = 1; i < HY; i++)
		for (int j = 0; j < HX; j++) layerH[i][j] = create_node(HX);
	for (int i = 0; i < 4; i++) layerO[i] = create_node(HX);
	return 0;
}

int counting(node **lp, node *p)
{
	if (!p || !lp) return -1;
	p->output = 0;
	for(int i = 0; i < p->weight_len; i++, lp++) {
		if (!lp || !p->weight[i]) continue;
		p->output += (*lp)->output * p->weight[i];
	}
	p->output = sigmoid(p->output + p->bias);
	return 0;
}

int run()
{
	for (int i = 0; i < HX; i++) counting(&layerI[0], layerH[0][i]);
	for (int j = 1; j < HY; j++)
		for (int i = 0; i < HX; i++) counting(&layerH[j - 1][0], layerH[j][i]);
	for (int i = 0; i < 4; i++) counting(&layerH[0][0], layerO[i]);
	return 0;
}

int getcost(node **lp, node *p)
{
	if (!p || !lp) return -1;

	double cost = p->o_want;
	for(int i = 0; i < p->weight_len; i++, lp++) {
		p->w_want[i] += cost * (*lp)->output;
		(*lp)->o_want += sigmoid(cost * p->weight[i]);
	}
	p->b_want += cost * 0.001;
	p->o_want = 0;
	return 0;
}

int applycost(node *p)
{
	if (!p) return -1;
	for(int i = 0; i < p->weight_len; i++) {
		p->weight[i] += p->w_want[i];
		p->w_want[i] = 0;
	}
	p->bias += p->b_want;
	p->b_want = 0;
	return 0;
}

int train()
{
	run();
	layerO[0]->o_want = layerO[0]->o_want - layerO[0]->output;
	layerO[1]->o_want = layerO[1]->o_want - layerO[1]->output;
	layerO[2]->o_want = layerO[2]->o_want - layerO[2]->output;
	layerO[3]->o_want = layerO[3]->o_want - layerO[3]->output;
	for (int i = 0; i < 4; i++) getcost(&layerH[HY - 1][0], layerO[i]);
	for (int j = HY - 1; j > 0; j--)
		for (int i = 0; i < HX; i++) getcost(&layerH[j - 1][0], layerH[j][i]);
	for (int i = 0; i < HX; i++) getcost(&layerI[0], layerH[0][i]);
	return 0;
}

int prepare_train()
{
	layerI[0]->output = rand() % 2000 / 1000.0 - 1;
	layerI[1]->output = rand() % 2000 / 1000.0 - 1;
	layerO[0]->o_want = layerI[0]->output >= 0 ? 1 : 0;
	layerO[1]->o_want = layerI[1]->output >= 0 ? 1 : 0;
	layerO[2]->o_want = layerI[0]->output >= layerI[1]->output ? 1 : 0;
	layerO[3]->o_want = layerI[0]->output >= layerI[1]->output ? 0 : 1;
	return 0;
}

void input()
{
	double inp1 = 0, inp2 = 0;
	printf("Your input(double * 2):\n");
	scanf("%lf %lf", &inp1, &inp2);
	layerI[0]->output = inp1;
	layerI[1]->output = inp2;
}

/* 打印结果并计算错误数量 */
int print_result(int print)
{
	double i1 = layerI[0]->output,
	       i2 = layerI[1]->output,
	       o1 = layerO[0]->output,
	       o2 = layerO[1]->output,
	       o3 = layerO[2]->output,
	       o4 = layerO[3]->output;
	int error = 0;
	int f1 = o1 > 0.5,
	    f2 = o2 > 0.5,
	    f3 = o3 > o4;
	if (i1 < 0 && f1) error++;
	if (i2 < 0 && f2) error++;
	if (i1 < i2 && f3) error++;

	if (print == 1) {
		double e1 = layerO[0]->o_want + o1,
		       e2 = layerO[1]->o_want + o2,
		       e3 = layerO[2]->o_want + o3,
		       e4 = layerO[3]->o_want + o4;
		printf("input:%7.4lf,\t%7.4lf;\tresult:%lf,%lf,%lf,%lf;\texcpet:%lf,%lf,%lf,%lf\n", i1, i2, o1, o2, o3, o4, e1, e2, e3, e4);
	} else if(print == 2) {
		printf("%7.4lf %s零, %7.4lf %s零,且 %7.4lf %s %7.4lf, ERROR=%d\n",
		       i1, f1 ? "大于" : "小于",
		       i2, f2 ? "大于" : "小于",
		       i1, f3 ? "大于" : "小于", i2, error);
	}
	return error;
}

int write_mem()
{
	FILE *fp = fopen("output.txt", "w");
	if (!fp) return -1;
	for (int i = 0; i < HY; i++) {
		fprintf(fp, "==========================================================\n");
		fprintf(fp, "Hide Layer No.%d\n", i);
		for (int j = 0; j < HX; j++) {
			fprintf(fp, "=============================\n");
			fprintf(fp, "Hide No.%d\n", j);
			fprintf(fp, "Weight:");
			for (int k = 0; k < layerH[i][j]->weight_len; k++) {
				fprintf(fp, "%7.4lf ", layerH[i][j]->weight[k]);
			}
			fprintf(fp, "\nWeight want:");
			for (int k = 0; k < layerH[i][j]->weight_len; k++) {
				fprintf(fp, "%7.4lf ", layerH[i][j]->w_want[k]);
			}
			fprintf(fp, "\nBias:%7.4lf\n", layerH[i][j]->bias);
			fprintf(fp, "Output:%7.4lf\n", layerH[i][j]->output);
			fprintf(fp, "Output want:%7.4lf\n", layerH[i][j]->o_want);
		}
	}
	fprintf(fp, "==========================================================\n");
	fprintf(fp, "----------------------------------------------------------\n");
	fprintf(fp, "Output Layer\n");
	for (int i = 0; i < 4; i++) {
		fprintf(fp, "-----------------------------\n");
		fprintf(fp, "Output No.%d\n", i);
		fprintf(fp, "Weight:");
		for (int j = 0; j < HX; j++) {
			fprintf(fp, "%7.4lf ", layerO[i]->weight[j]);
		}
		fprintf(fp, "\nWeight want:");
		for (int j = 0; j < HX; j++) {
			fprintf(fp, "%7.4lf ", layerO[i]->w_want[j]);
		}
		fprintf(fp, "\nBias:%7.4lf\n", layerO[i]->bias);
		fprintf(fp, "Output:%7.4lf\n", layerO[i]->output);
		fprintf(fp, "Output want:%7.4lf\n", layerO[i]->o_want);
	}
	return 0;
}

int main()
{
	srand(time(NULL));
	init();
	for (int i = 1; i <= 2000000; i++) {
		prepare_train();
		train();
		if (i % 20 == 0) {     /* 1 */
			for (int i = 0; i < 4; i++) applycost(layerO[i]);
			for (int j = HY - 1; j > 0; j--)
				for (int i = 0; i < HX; i++) applycost(layerH[j][i]);
			for (int i = 0; i < HX; i++) applycost(layerH[0][i]);
			if (i % 10000 == 0) print_result(1);
		}
	}
	/* write_mem(); */
	printf("==========================================================\n");
	printf("Test:\n");
	int error = 0, max = 200000;
	for (int i = 1; i <= max; i++) {
		prepare_train();
		run();
		error += print_result(0);
		if (i % 500 == 0) print_result(2);
	}
	printf("Test result: Count:%d, Error:%d, Error present:%lf%%, Right present:%lf%%\n",
	       max, error, (double)error / max * 100, (double)(max - error) / max * 100);
	printf("==========================================================\n");
	printf("Test by user:\n");
	input();
	run();
	print_result(1);
	print_result(2);
	return 0;
}

