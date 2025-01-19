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
#include <unistd.h>

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

typedef struct layer layer;
struct layer {
	node **p;
	int node_number;
	layer *np;
	layer *lp;
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

layer *create_layer(int node_number, layer *lp)
{
	layer *p = malloc(sizeof(layer));
	p->p = malloc(sizeof(node**) * node_number);
	for (int i = 0; i < node_number; i++) {
		p->p[i] = create_node(lp ? lp->node_number : 0);
	}
	p->node_number = node_number;
	p->np = NULL;
	p->lp = lp;
	return p;
}

layer *layers = NULL;
layer *layer_o = NULL;

int print_result(int print);
int prepare_train();

int init()
{
	int node_num[] = {2, 16, 4, 0};
	layer *p = layers = create_layer(2, NULL);
	for (int i = 1; node_num[i] != 0; i++) {
		p->np = create_layer(node_num[i], p);
		p = p->np;
	}
	layer_o = p;
	return 0;
}

/* 计算节点结果 */
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

/* 更改节点预期 */
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

/* 应用节点预期 */
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

#define Get() for (layer *p = layers->np; p != NULL ; p = p->np) for (int i = 0; i < p->node_number; i++)
int save()
{
	FILE *fp = fopen("neuro_output.txt", "w");
	if (!fp) return -1;

	Get() {
		fprintf(fp, "%lf ", p->p[i]->bias);
		for (int j = 0; j < p->p[i]->weight_len; j++)
			fprintf(fp, "%lf ", p->p[i]->weight[j]);
		fprintf(fp, "\n");
	}
	fclose(fp);
	return 0;
}

int load()
{
	FILE *fp = fopen("neuro_output.txt", "r");
	if (!fp) return -1;

	Get() {
		fscanf(fp, "%lf ", &p->p[i]->bias);
		for (int j = 0; j < p->p[i]->weight_len; j++)
			fscanf(fp, "%lf ", &p->p[i]->weight[j]);
	}
	fclose(fp);
	return 0;
}

/* 调用模型计算 */
int run()
{
	Get() counting(p->lp->p, p->p[i]);
	return 0;
}

/* 训练 */
int train(int count)
{
	for (int i = 1; i <= count; i++) {
		prepare_train();
		run();
		for (layer *p = layer_o; p->lp != NULL ; p = p->lp)
			for (int i = 0; i < p->node_number; i++) getcost(p->lp->p, p->p[i]);
		if (i % 50 == 0)
			Get() applycost(p->p[i]);
		if (i % 10000 == 0) print_result(1);
	}
	return 0;
}
#undef Get

/* 计算错误数量并打印运行结果 */
int print_result(int print)
{
	double i1 = layers->p[0]->output,
	       i2 = layers->p[1]->output,
	       o1 = layer_o->p[0]->output,
	       o2 = layer_o->p[1]->output,
	       o3 = layer_o->p[2]->output,
	       o4 = layer_o->p[3]->output;
	int error = 0;
	int f1 = o1 > 0.5,
	    f2 = o2 > 0.5,
	    f3 = o3 > o4;
	if ((i1 > 0) ^ f1) error++;
	if ((i2 > 0) ^ f2) error++;
	if ((i1 > i2) ^ f3) error++;

	if (print == 1) {
		printf("input:%7.4lf,\t%7.4lf;\tresult:%lf,%lf,%lf,%lf;\t\n", i1, i2, o1, o2, o3, o4);
	} else if(print == 2) {
		printf("%7.4lf %s零, %7.4lf %s零,且 %7.4lf %s %7.4lf, ERROR=%d\n",
		       i1, f1 ? "大于" : "小于",
		       i2, f2 ? "大于" : "小于",
		       i1, f3 ? "大于" : "小于", i2, error);
	}
	return error;
}

/* 随机生成输入并设置预期输出结果 */
int prepare_train()
{
	node **p = layers->p;
	node **op = layer_o->p;

	p[0]->output = (rand() % 200000 - 100000) / 1000.0;
	p[1]->output = (rand() % 200000 - 100000) / 1000.0;
	op[0]->o_want = p[0]->output >= 0 ? 1 : 0;
	op[1]->o_want = p[1]->output >= 0 ? 1 : 0;
	op[2]->o_want = p[0]->output >= p[1]->output ? 1 : 0;
	op[3]->o_want = p[0]->output >= p[1]->output ? 0 : 1;
	run();
	op[0]->o_want = op[0]->o_want - op[0]->output;
	op[1]->o_want = op[1]->o_want - op[1]->output;
	op[2]->o_want = op[2]->o_want - op[2]->output;
	op[3]->o_want = op[3]->o_want - op[3]->output;
	return 0;
}


/* 获取用户输入 */
void input()
{
	double inp1 = 0, inp2 = 0;
	printf("Your input(double * 2):\n");
	scanf("%lf %lf", &inp1, &inp2);
	layers->p[0]->output = inp1;
	layers->p[1]->output = inp2;
}

int opt(int argc, char *argv[])
{
	int ch = 0, flag = 0;
	srand(time(NULL));

	while ((ch = getopt(argc, argv, "hlsti")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: neuro [OPTIONS]\n"
			       "Options:\n"
			       "    -l    load data\n"
			       "    -s    save data\n"
			       "    -t    train\n"
			       "    -i    test by input\n");
			exit(0);
			break;
		case 'l':
			flag ^= 0b0100;
			break;
		case 's':
			flag ^= 0b0010;
			break;
		case 't':
			flag ^= 0b0001;
			break;
		case 'i':
			flag ^= 0b1000;
			break;
		default:
			break;
		}
	}
	return flag;
}

int main(int argc, char *argv[])
{
	int flag = opt(argc, argv);

	srand(time(NULL));
	init();
	if (flag & 0b100) load();
	if (flag & 0b001) {
		printf("Train:\n");
		train(2000000);
	}
	if (flag & 0b010) save();
	printf("Test:\n");
	int error = 0, errors = 0, max = 200000;
	for (int i = 1; i <= max; i++) {
		prepare_train();
		run();
		error += print_result(0);
		if (print_result(0)) errors++;
		if (i % 1000 == 0) print_result(2);
	}
	printf("Test result: Count:%d, Error:%d, Error present:%lf%%, Right present:%lf%%\n",
	       max, error, (double)errors / max * 100, (double)(max - errors) / max * 100);
	if (flag & 0b1000) {
		printf("Test by user:\n");
		input();
		run();
		print_result(1);
		print_result(2);
	}
	return 0;
}

