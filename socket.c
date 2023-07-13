#include "include/tools.h"
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAXLINE 4096
#define PORT 8001
 
void server();
void client();
void stop();

int a = 0;

int main() {
	Clear_SYS;
	signal(SIGINT, stop);
	printf("No.1 server\nor\nNo.2 client\n?");
	if (ctools_getch() == '1') {
		Clear_SYS;
		server();
	}
	else {
		Clear_SYS;
		client();
	}
	Clear_SYS;
	return 0;
}

void server(){
	//  监听套接字      连接套接字
	int listen_fd = -1, connect_fd = -1;

	// 套接字地址
	struct sockaddr_in servaddr;

	//   接收缓冲区        发送缓冲区
	char sendbuf[MAXLINE], recbuf[MAXLINE];

	//    定义进程
	pid_t pid;

	// 初始化套接字地址结构体
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;                  //IPv4
	servaddr.sin_port = htons(PORT);                //设置监听端口
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);   //表示接收任意IP的连接请求

	//创建套接字
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		//如果创建套接字失败，返回错误信息
		//strerror(int errnum)获取错误的描述字符串
		printf("\033[1;31mcreate socket error: %s(error: %d)\033[0m\n", strerror(errno), errno);
		perror("套接字创建失败");
		exit(-1);
	}

	//绑定套接字和本地IP地址和端口
	if(bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		//绑定出现错误
		printf("bind socket error: %s(error: %d)\n", strerror(errno), errno);
		perror("套接字绑定失败");
		exit(-2);
	} 

	//使得listen_fd变成监听描述符
	if(listen(listen_fd, 10) == -1){
		printf("listen socket error: %s(error: %d)\n", strerror(errno), errno);
		exit(-3);
	}
 
	//accept阻塞等待客户端请求
	printf("等待客户端发起连接\n");
	if((connect_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL)) == -1){
		printf("accept socket error: %s(error: %d)\n", strerror(errno), errno);
		exit(-4);
	}

	pid = fork();
	if (pid == EOF) {
		perror("进程创建错误!");
		exit(-5);
	}
	Clear_SYS;
	while(!pid){
		printf("\033[s\033[13;1H\033[1;31m读（子）%d\033[0m\n\033[u",pid);
		ctools_kbhitGetchar();
		// 读取客户端发来的信息，会阻塞
		ssize_t len = read(connect_fd, recbuf, sizeof(recbuf));
		if(len < 0){
			if(errno == EINTR){
				continue;
			}
			exit(-6);
		}
		printf("\033[s\033[14;1H接收：\n%s\n\033[u", recbuf);
		ctools_kbhitGetchar();
		if(a) {
			exit(0);
		}
	} 
	while (pid) {
		printf("\033[1;1H\033[1;32m发（父）%d\033[0m",pid);
		//向客户端发送信息
		printf("\033[2;1H发送：\n");
		fgets(sendbuf, sizeof(sendbuf), stdin);
		write(connect_fd, sendbuf, sizeof(sendbuf));
		if (a) {
			break;
		}
	}
	kill(pid,1);
	//关闭连接套接字
	close(connect_fd);
	//关闭监听套接字
	close(listen_fd);
	return;
}
 
void client(){
	//定义客户端套接字
	int sockfd = -1;
	//定义想连接的服务器的套接字地址
	struct sockaddr_in servaddr;
	//发送和接收数据的缓冲区
	char sendbuf[MAXLINE], recbuf[MAXLINE];

	pid_t pid;

	//初始化服务器套接字地址
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;//IPv4
	servaddr.sin_port = htons(PORT);//想连接的服务器的端口
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");//服务器的IP地址
 
	//创建套接字
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		//如果创建套接字失败，返回错误信息
		//strerror(int errnum)获取错误的描述字符串
		printf("create socket error: %s(error: %d)\n", strerror(errno), errno);
		perror("套接字创建失败");
		exit(-1);
	} 
	//向服务器发送连接请求
	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		//连接失败
		printf("connect socket error: %s(error: %d)\n", strerror(errno), errno);
		perror("套接字连接失败");
		exit(-2);
	}
 
	pid = fork();
	system("clear");
	while(!pid){
		printf("\033[s\033[13;1H\033[1;31m读（子）%d\033[0m\n\033[u",pid);
		ctools_kbhitGetchar();
		//从服务器接收信息
		ssize_t len = read(sockfd, recbuf, sizeof(recbuf));
		if(len < 0){
			if(errno == EINTR){
				continue;
			}
			exit(0);
		}
		printf("\033[s\033[14;40H接收：\n%s\n\033[u", recbuf);
		ctools_kbhitGetchar();
		if (a) {
			exit(0);
		}
	}
	while(pid) {
		printf("\033[1;1H\033[1;32m发（父）%d\033[0m",pid);
		//向服务器发送信息
		printf("\033[2;1H发送(pid=%-4d)：\n",pid);
		fgets(sendbuf, sizeof(sendbuf), stdin);
		write(sockfd, sendbuf, sizeof(sendbuf));
		if (a) {
			kill(pid,1);
			break;
		}

	} 
	//关闭套接字
	close(sockfd);
	exit(0);
	return;
}

void stop() {
	a = 1;
}

