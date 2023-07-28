#include "include/tools.h"
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>

#define MAXSIZE 4096
#define PORT 8002


int    listen_fd  = -1,         /* <fd>     监听套接字 */
       connect_fd = -1;         /* <fd>     连接套接字 */
struct sockaddr_in servaddr;    /* <strcut> 套接字地址 */

char   sendbuf[MAXSIZE],    /* <ch>  接收缓冲区 */
       recbuf[MAXSIZE];     /* <ch>  发送缓冲区 */

int flag_run = 0;

/*
 * 获取输入
 */
void input(char *str)
{
	int ch    = 0,
	    count = 0;
	if (str == NULL) return;
	char *str2 = malloc(sizeof(char)*MAXSIZE);
	memset(str,  0, sizeof(char)*MAXSIZE);
	memset(str2, 0, sizeof(char)*MAXSIZE);
	while (ch != 0x1B && ch != '\r') {
		ch = ctools_getch();
		if (ch == 0x1B && ctools_kbhit() != 0)
			ch = '\0';
		if ( ch & 0x80) {    /* If is Chinese */
			printf("\033[0;37m%c", ch);
			sprintf(str2, "%s%c", str, ch);
			strcpy(str, str2);
			ch = ctools_getch();
			printf("%c", ch);
			sprintf(str2, "%s%c", str, ch);
			strcpy(str, str2);
			ch = ctools_getch();
			printf("%c\033[0m", ch);
			sprintf(str2, "%s%c", str, ch);
			strcpy(str, str2);
			count += 3;
			fflush(stdout);
			fflush(stdin);
			continue;
		}
		if (ch == 0x7F && count > 0) {
			if (str[strlen(str) - 1] & 0x80) {
				printf("\b\b  \b\b");
				str[strlen(str) - 1] = '\0';
				str[strlen(str) - 2] = '\0';
				str[strlen(str2) - 1] = '\0';
				str[strlen(str2) - 2] = '\0';
				count -= 2;
			} else {
				printf("\b \b");
				str[strlen(str) - 1] = '\0';
				str[strlen(str2) - 1] = '\0';
			}
			count--;
			ch = 0;
		} else if (ch != 0x1B && ch != 0x7F && ch != '\r') {
			count++;
			printf("\033[0;37m%c\033[0m", ch);
			sprintf(str2, "%s%c", str, ch);
			strcpy(str, str2);
		}
		fflush(stdout);
		fflush(stdin);
	}
	return;
}


void *get_msg()
{
	char recbuf[MAXSIZE];     /* <ch>  发送缓冲区 */
	struct winsize size;

	fflush(stdout);
	flag_run = 1;
	// 读取客户端发来的信息，会阻塞
	while (1) {
		ssize_t len = read(connect_fd, recbuf, sizeof(recbuf));
		if (len <= 0){
			if(len == 0 || errno == EINTR) {
				flag_run = 0;
				pthread_exit(NULL);
			}
			exit(-6);
		}
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		printf("\033[0;47m\r");
		for (int i = 0; i < size.ws_col; i++) {
			printf(" ");
		}
		printf("\033[0m\r");
		fflush(stdout);
		printf("\033[0;1;33;47mOUTPUT > \033[0;30;47m%s\033[0m\r\n", recbuf);
		printf("\033[0;1;33mINPUT  > \033[0;37m%s\033[0m", sendbuf);
		fflush(stdout);
		usleep(30000);
	}
	flag_run = 0;
	pthread_exit(NULL);
	return NULL;
}
 
void server()
{
	// 初始化套接字地址结构体
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;                  // IPv4协议
	servaddr.sin_port = htons(PORT);                // 设置监听端口
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);   // 表示接收任意IP的连接请求

	//创建套接字
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		//如果创建套接字失败，返回错误信息
		//strerror(int errnum)获取错误的描述字符串
		fprintf(stderr, "\033[1;31mcreate socket error: %s(error: %d)\033[0m\n", strerror(errno), errno);
		perror("套接字创建失败");
		exit(-1);
	}

	//绑定套接字和本地IP地址和端口
	if(bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		//绑定出现错误
		fprintf(stderr, "bind socket error: %s(error: %d)\n", strerror(errno), errno);
		perror("套接字绑定失败");
		printf("小提示：一般情况下用`sudo lsof -i:$port`检查$port是否有进程占用$port哦，如果没有那么等一会可能就好了呢\n");
		exit(-2);
	} 

	//使得listen_fd变成监听描述符
	if(listen(listen_fd, 10) == -1){
		fprintf(stderr, "listen socket error: %s(error: %d)\n", strerror(errno), errno);
		exit(-3);
	}
 
	//accept阻塞等待客户端请求
	printf("\033[0;1;33mINFO > 等待客户端发起连接\033[0m\n");
	if((connect_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL)) == -1){
		printf("accept socket error: %s(error: %d)\n", strerror(errno), errno);
		exit(-4);
	}

	pthread_t pid;
	pthread_create(&pid, NULL, get_msg, NULL);

	usleep(30000);
	while (strcmp(sendbuf, "/exit") != 0) {
		printf("\033[0;1;33mINPUT  > \033[0m");
		input(sendbuf);
		printf("\n");
		if (flag_run == 0) {
			printf("\033[0;1;33mINFO > 对方已退出，退出程序\033[0m\n");
			break;
		}
		
		//向客户端发送信息
		write(connect_fd, sendbuf, sizeof(sendbuf));
	}
	pthread_cancel(pid);
	usleep(50000);
	/* usleep(10000); */
	//关闭连接套接字
	close(connect_fd);
	//关闭监听套接字
	close(listen_fd);
	return;
}
 
void client()
{
	//初始化服务器套接字地址
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;//IPv4
	servaddr.sin_port = htons(PORT);//想连接的服务器的端口
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");//服务器的IP地址
 
	//创建套接字
	if((connect_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		//如果创建套接字失败，返回错误信息
		//strerror(int errnum)获取错误的描述字符串
		printf("create socket error: %s(error: %d)\n", strerror(errno), errno);
		perror("套接字创建失败");
		exit(-1);
	} 
	//向服务器发送连接请求
	if(connect(connect_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		//连接失败
		printf("connect socket error: %s(error: %d)\n", strerror(errno), errno);
		perror("套接字连接失败");
		exit(-2);
	}

	pthread_t pid;
	pthread_create(&pid, NULL, get_msg, NULL);
 
	usleep(30000);
	while(strcmp(sendbuf, "/exit") != 0) {
		printf("\033[0;1;33mINPUT  > \033[0m");
		input(sendbuf);
		printf("\n");
		if (flag_run == 0) {
			printf("\033[0;1;33mINFO > 对方已退出，退出程序\033[0m\n");
			break;
		}
		//向客户端发送信息
		write(connect_fd, sendbuf, sizeof(sendbuf));
	} 
	pthread_cancel(pid);
	usleep(50000);
	//关闭套接字
	close(connect_fd);
	return;
}

int main()
{
	Clear_SYS;
	printf("No.1 server\nor\nNo.2 client\n?");
	if (ctools_getch() == '1') {
		Clear_SYS;
		server();
	}
	else {
		Clear_SYS;
		client();
	}
	return 0;
}


