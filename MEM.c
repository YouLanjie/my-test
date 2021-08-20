#include "include/include.h"

struct Input {
        struct Input * pNext;
};

struct Input * pHead = NULL;
void stop();

int main() {
	struct Input * pNew,* pEnd;
	unsigned long count = 1;

	pHead = pNew = pEnd = NULL;

	signal(SIGINT,stop);
	Clear
	printf("\033[?25l欢迎使用“内存毁灭者”程序\n本程序能够内填满内存，每停顿一次大概能增加50M内存\n按下回车开始\n按C-c退出");
	getchar();
	pEnd = pNew = (struct Input *)malloc(sizeof(struct Input));
	while (1) {
		if (count % (1000 * 1646) == 0) {
			printf("\033[5;1H休息时间到！\n");
			getchar();
			printf("\033[5;1H            \n");
		}
		pNew = (struct Input *)malloc(sizeof(struct Input));
		pEnd -> pNext = pNew;
		count++;
	}
	return 0;
}

void stop() {
	printf("\033[?25h\033[5;1H             \033[5;1H退出...  \n");
	free(pHead);
	exit(0);
}

