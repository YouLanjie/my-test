#include <stdio.h>

int main() {
	int a = 1,b = 1;
	int s = 1,y = 0,h = 10,n = 1;

	printf("\033[2J\033[1;1H\033[?25l循环破解\n");
	printf("请输入商：");
	scanf("%d",&s);
	printf("\033[2J\033[1;1H\033[?25l循环破解\n");
	printf("请输入余数：");
	scanf("%d",&y);
	printf("\033[2J\033[1;1H\033[?25l循环破解\n");
	printf("请输入和：");
	scanf("%d",&h);
	printf("\033[2J\033[1;1H\033[?25l循环破解\n");
	printf("是否显示破解进度？（1 = Yes）：");
	scanf("%d",&n);
	printf("\033[2J\033[1;1H\033[?25l循环破解\n");
	if(h <= h && h <= y) {
		printf("\033[2J\033[1;1Herror!!!\033[?25h\n");
		return 0;
	}
	printf("\033[2;1H破解中...\033[3;1H被除数：\033[4;1H除数：");
	while(1) {
		b++;
		a = b;
		while(1) {
			if(a + b > h) {
				printf("\033[2J\033[1;1H对不起，没有找到您想要的数\n");
			}
			if(b > a) {
				printf("\033[2J\033[1;1H对不起，没有找到您想要的数\n");
				return 0;
				break;
			}
			if(a + b > y > h || (a + b + s + y == h && a % b == y && (a - y) / b == s) || a > h) break;
			if(a < b) continue;
			if(n == 1) printf("\033[3;9H%3d\033[4;7H%3d\n",a,b);
			a++;
		}
		if(a + b > h || (a + b + s + y == h && a % b == y && (a - y) / b == s) || b > a) break;
	}
	printf("\033[2J\033[1;1H被除数：%d\n除数：%d\033[?25h\n",a,b);
	return 0;
}
