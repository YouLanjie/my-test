#include "include/tools.h"
#include <math.h>
#include <time.h>

int eular(int n);
int mgcd(int a,int b);

void stop();

int main() {
	FILE * fp;
	unsigned int e = 0, n = 0, k = 0, d = 0, m = 0, c = 0, i = 0;

	signal(SIGINT, stop);
	printf("这是一个使用RSA不对称加密算法的测试程序\n请选择模式\n纯手动---1\n半自动---2\n");
	i = _getch();
	if (i == '1') {
		do {
			printf("这是一个使用RSA不对称加密算法的测试程序\n请输入三个整数值(e n k)，e要与φ(n)互质，以空格分开\n");
			scanf("%d%d%d",&e, &n, &k);
			getchar();
			if (!mgcd(e,eular(n))) {
				printf("\033[1;31me与φ(n)不互质！\033[0mn=%d\nφ(n)=%d\n",n,eular(n));
				_getch();
			}
			if ((k * eular(n) + 1) % e != 0) {
				printf("\033[1;31me与k乘φ(n)的积加1的和不能整除！\033[0mn=%d\nk * φ(n) + 1=%d\n",n,k * eular(n) + 1);
				_getch();
			}
		}while (!mgcd(e,eular(n)) || (k * eular(n) + 1) % e != 0);
	}
	else {
		printf("这是一个使用RSA不对称加密算法的测试程序\n");
		fp = fopen("/dev/random", "rb");
		printf("稍等一会儿...\n");
		if (!fp) {
			perror("文件无法打开");
			do {
				srand(time(NULL));
				n = rand() % 1500 + 500;
				e = rand() % 200 + 200;
				k = rand() % 100 + 100;
			}while (!mgcd(e,eular(n)) || (k * eular(n) + 1) % e != 0);
		} else {
			n = fgetc(fp);
			srand(n);
			e = rand() % 200 + 200;
			n = fgetc(fp);
			srand(n);
			k = rand() % 100 + 100;
			n = fgetc(fp);
			srand(n);
			n = rand() % 1500 + 500;
			fclose(fp);
			do {
				k++;
			}while (!mgcd(e,eular(n)) || (k * eular(n) + 1) % e != 0);
		}
	}
	d = ( k * eular(n) + 1 ) / e;
	printf("最终参数：\ne=%d\nd=%d\nn=%d\n注意！加密的数字不能够超过n", e, d, n);
	_getch();
	i = 0;
	while (i != 0x1B) {
		m = c = i = 0;
		printf("加密或解密？\n加密---1\n解密---2\n");
		i = _getch();
		if (i == '1') {
			printf("请输入任意整数值(m)用于加密\n");
			scanf("%d",&m);
			getchar();
			c = pow(m, e);
			c = c % n;
			printf("源信息：%d\n加密信息：%d\n", m, c);
		}
		else if (i == '2') {
			printf("请输入任意整数值(c)用于解密\n");
			scanf("%d",&c);
			getchar();
			m = pow(c, d);
			m = m % n;
			printf("加密信息：%d\n解密信息：%d\n", c, m);
		}
		else if (i == 0x1B) {
			return 0;
		}
		i = _getch();
	}
	return 0;
}

//欧拉函数
int eular(int n) {
	int res = n;

	for(int i = 2; i <= sqrt(n); i++) {
		if(n % i == 0) {
			res = res / i * (i - 1);
		}
		while(n % i == 0) {
			n /= i;
		}
	}
	if(n > 1) {
		res = res / n * (n - 1);
	}
	return res;
}

//判断数值是否互质，返回值为1则互质
int mgcd(int a,int b) {
	int t;

	if(a < b) {
		t = a;
		a = b;
		b = t;
	}
	while(a % b) {
		t = b;
		b = a % b;
		a = t;
	}
	return b;
}

void stop() {
	printf("Exiting......\n");
	exit(0);
}

