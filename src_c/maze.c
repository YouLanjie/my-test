/*
 *   Copyright (C) 2024 u0_a221
 *   
 *   文件名称：maze.c
 *   创 建 者：u0_a221
 *   创建日期：2024年02月02日
 *   描    述：可视化显示迷宫生成
 *             生成迷宫逻辑部分的代码原作者id:jjwwwww
 *             生成迷宫逻辑部分的代码参考链接:
 *             https://blog.csdn.net/jjwwwww/article/details/82872922
 *
 */

#include "include/tools.h"

#define SECOND 1000000
#define TPS    (SECOND / 20)

//地图长度L，包括迷宫主体20，外侧的包围的墙体2，
//最外侧包围路径2（之后会解释）
#define L (20 + 4)

//墙和路径的标识
#define WALL  0
#define ROUTE 1
#define MARK  2
#define WALL_C  "国"
#define ROUTE_C "  "
#define MARK_C  ":;"

int **Maze = NULL;
static int Rank = 0; // 控制迷宫的复杂度，数值越大复杂度越低，最小值为0
int level = 0;

void init();
void print_map();
void CreateMaze(int **maze, int x, int y); //生成迷宫

int main(void)
{				/*{{{ */
	printf("\033[?25l");
	init();
	print_map();

	//创造迷宫，（2，2）为起点
	CreateMaze(Maze, 2, 2);

	//画迷宫的入口和出口
	Maze[2][1] = ROUTE;

	//由于算法随机性，出口有一定概率不在（L-3,L-2）处，
	//此时需要寻找出口
	for (int i = L - 3; i >= 0; i--) {
		if (Maze[i][L - 3] == ROUTE) {
			Maze[i][L - 2] = ROUTE;
			break;
		}
	}

	print_map();
	printf("\033[%dB", L);

	FILE *fp = fopen("maze.txt", "w");
	if (!fp) goto LEAVE;
	for (int i = 1; i < L - 1; i++) {
		for (int j = 1; j < L - 1; j++) {
			fprintf(fp, "%d ", Maze[i][j]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);

LEAVE:
	printf("\033[?25h");
	for (int i = 0; i < L; i++)
		free(Maze[i]);
	free(Maze);
	return 0;
}				/*}}} */

void init()
{				/*{{{ */
	srand((unsigned)time(NULL));

	Maze = (int **)malloc(L * sizeof(int *));
	for (int i = 0; i < L; i++) {
		Maze[i] = (int *)calloc(L, sizeof(int));
	}

	// 最外围层设为路径的原因，为了防止挖路时挖出边界，
	// 同时为了保护迷宫主体外的一圈墙体被挖穿
	for (int i = 0; i < L; i++) {
		Maze[i][0] = ROUTE;
		Maze[0][i] = ROUTE;
		Maze[i][L - 1] = ROUTE;
		Maze[L - 1][i] = ROUTE;
	}
	return;
}				/*}}} */

void print_map()
{				/*{{{ */
	int l = L - 2;
	int deep = L * L / 2 - Rank;    /* 并无实际意义，经验公式 */

	//画迷宫
	for (int i = 1; i < L - 1; i++) {
		for (int j = 1; j < L - 1; j++) {
			printf("%s",
			       Maze[i][j] != WALL ? (Maze[i][j] == MARK ? MARK_C : ROUTE_C ) : WALL_C);
		}
		printf("\n");
	}
	printf("\033[%dA", l);
	printf("\033[%dC  | 符号解释: '%s'为遍历节点\n", l*2, MARK_C);
	printf("\033[%dC  |           '%s'为走道\n", l*2, ROUTE_C);
	printf("\033[%dC  |           '%s'为边墙\n", l*2, WALL_C);
	printf("\033[%dC  | 全图边长: %d\n", l*2, L);
	printf("\033[%dC  | 实体边长: %d\n", l*2, l);
	printf("\033[%dC  | 本体边长: %d\n", l*2, l - 2);
	printf("\033[%dC  | 复杂程度: %d (值越小越复杂) \n", l*2, Rank);
	printf("\033[%dC  | 延迟时间: %.4fs\n", l*2, (double)(int)TPS/SECOND);
	printf("\033[%dC  | 遍历深度: %-3d [%4.1f%%]\n", l*2, level, (double)level / deep * 100);
	printf("\033[1A\033[%dC [", l*2 + 25);
	int lim = get_winsize_col() - (l*2 + 28);
	for (int i = 0; i < lim; i++) {
		printf((double)i / lim < (double)level / deep ? "#" : " ");
	}
	printf("]\n");
	printf("\033[%dA", 9);
	return;
}				/*}}} */

void CreateMaze(int **maze, int x, int y)
{				/*{{{ */
	level++;
	maze[x][y] = maze[x][y] == WALL ? ROUTE : maze[x][y];

	//确保四个方向随机
	int direction[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	for (int i = 0; i < 4; i++) {
		/* Swap data */
		int r = rand() % 4;
		int temp = direction[0][0];
		direction[0][0] = direction[r][0];
		direction[r][0] = temp;

		temp = direction[0][1];
		direction[0][1] = direction[r][1];
		direction[r][1] = temp;
	}

	//向四个方向开挖
	for (int i = 0; i < 4; i++) {
		int dx = x;
		int dy = y;

		//控制挖的距离，由Rank来调整大小
		int range = 1 + (Rank == 0 ? 0 : rand() % Rank);
		while (range > 0) {
			dx += direction[i][0];
			dy += direction[i][1];

			//排除掉回头路
			if (maze[dx][dy] != WALL) {
				break;
			}
			//判断是否挖穿路径
			int count = 0;
			for (int j = dx - 1; j < dx + 2; j++) {
				for (int k = dy - 1; k < dy + 2; k++) {
					// abs(j - dx) + abs(k - dy) == 1 
					// 确保只判断九宫格的四个特定位置
					if (abs(j - dx) + abs(k - dy) == 1
					    && maze[j][k] != WALL) {
						count++;
					}
				}
			}

			if (count > 1) {
				break;
			}
			//确保不会挖穿时，前进
			--range;
			maze[dx][dy] = ROUTE;
			print_map();
			usleep(TPS);
		}

		//没有挖穿危险，以此为节点递归
		if (range <= 0) {
			maze[dx][dy] = MARK;
			CreateMaze(maze, dx, dy);
			maze[dx][dy] = ROUTE;
		}
	}
	level--;
	return;
}				/*}}} */

