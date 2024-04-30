#include "include/tools.h"

struct Input {
	int data;
	struct Input *pNext;
} *p = NULL;

void stop()
{
	printf("退出...  \n");
	free(p);
	exit(0);
}

int sum(struct Input *ph)
{
	int count = 0;
	int size = sizeof(*ph);
	while (ph != NULL) {
		ph = ph->pNext;
		count++;
	}
	return (size*count)/(1024*1024);
}

int main()
{
	struct Input *pNew = NULL, *pEnd = NULL;
	int size = sizeof(struct Input);
	int input = 0;

	signal(SIGINT, stop);
	printf("内存占用测试程序\n"
	       "每停顿一次大概能增加50M内存\n"
	       "按下任意键开始\n按C-c或者q键退出\n");
	input = _getch();
	p = pEnd = pNew = (struct Input *)malloc(sizeof(struct Input));

 BEGIN:
	for (int count = 0; count < 50 * 1024 * 1024 / size; count++) {
		pNew = (struct Input *)malloc(sizeof(struct Input));
		pNew->data = count;
		pEnd->pNext = pNew;
		pEnd = pNew;
	}
	printf("sum(p) = %dM\n", sum(p));
	if (input != 'f')
		input = _getch();
	if (input != 'q')
		goto BEGIN;
	stop();
	return 0;
}
