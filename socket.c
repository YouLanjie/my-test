#include "include/tools.h"
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <termios.h>

#define MAXSIZE 8192
/* #define PORT 8002 */


int    listen_fd  = -1,         /* <fd>     监听套接字 */
       connect_fd = -1;         /* <fd>     连接套接字 */
struct sockaddr_in servaddr;    /* <strcut> 套接字地址 */

char   sendbuf[MAXSIZE],    /* <ch>  接收缓冲区 */
       recbuf[MAXSIZE];     /* <ch>  发送缓冲区 */

int flag_run = 0;
int flag_file = 0;
int flag_enter = '\r';
char filename[MAXSIZE];

struct termios flag_oldt;
int flag_termux;

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
	while (ch != 0x1B && ch != flag_enter) {
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
		if (ch == 0x7F && count > 0 && flag_enter == '\r') {
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
		}
		if (ch != 0x1B && ch != 0x7F && ch != flag_enter) {
			count++;
			printf("\033[0;37m%c\033[0m", ch);
			sprintf(str2, "%s%c", str, ch);
			if (ch == '\r') {
				ch = '\n';
				printf("\033[0;37m%c\033[0m", ch);
				sprintf(str2, "%s\r%c", str, ch);
			}
			strcpy(str, str2);
		}
		fflush(stdout);
		fflush(stdin);
	}
	free(str2);
	return;
}


void *get_msg()
{
	char recbuf[MAXSIZE];     /* <ch>  发送缓冲区 */
	struct winsize size;

	fflush(stdout);
	flag_run = 1;
	// 读取客户端发来的信息，会阻塞
	time_t timep1;
	struct tm *timep2;
	while (1) {
		ssize_t len = read(connect_fd, recbuf, sizeof(recbuf));
		if (len <= 0){
			if(len == 0 || errno == EINTR) {
				flag_run = 0;
				pthread_exit(NULL);
			}
			tcsetattr(STDIN_FILENO, TCSANOW, &flag_oldt);
			fcntl(STDIN_FILENO, F_SETFL, flag_termux);
			fprintf(stderr, "\n\033[1;31mERROR > msg get error: %s(error: %d)\033[0m\n", strerror(errno), errno);
			perror("消息接收错误");
			exit(-6);
		}
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		if (flag_enter == 0x1B) printf("\n");
		printf("\033[0;47m\r");
		for (int i = 0; i < size.ws_col; i++) {
			printf(" ");
		}
		printf("\033[0m\r");
		fflush(stdout);
		time(&timep1);
		timep2 = gmtime(&timep1);
		printf("\033[0;1;33;47m%04d-%02d-%02d %02d-%02d-%02d OUTPUT > \033[0;30;47m%s\033[0m\r\n",
		       1900 + timep2->tm_year,
		       1 + timep2->tm_mon,
		       timep2->tm_mday,
		       8 + timep2->tm_hour,
		       timep2->tm_min,
		       timep2->tm_sec,
		       recbuf
		);
		printf("\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d INPUT  > \033[0;37m%s\033[0m",
		       1900 + timep2->tm_year,
		       1 + timep2->tm_mon,
		       timep2->tm_mday,
		       8 + timep2->tm_hour,
		       timep2->tm_min,
		       timep2->tm_sec,
		       sendbuf
		);
		if (flag_file == -1) {
			FILE *fp = fopen("socket_get_output.txt", "w");
			if (!fp) pthread_exit(NULL);
			fprintf(fp, "%s", recbuf);
			fclose(fp);
		}
		fflush(stdout);
		usleep(30000);
	}
	flag_run = 0;
	pthread_exit(NULL);
	return NULL;
}
 
void server(int port)
{
	// 初始化套接字地址结构体
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;                  // IPv4协议
	servaddr.sin_port = htons(port);                // 设置监听端口
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
		printf("小提示：一般情况下用`sudo lsof -i:$port`检查$port是否有进程占用$port，如果没有那么等一会可能就好了，或者更换端口\n");
		exit(-2);
	} 

	//使得listen_fd变成监听描述符
	if(listen(listen_fd, 10) == -1){
		fprintf(stderr, "listen socket error: %s(error: %d)\n", strerror(errno), errno);
		exit(-3);
	}
 
	while (strcmp(sendbuf, "/exit") != 0) {
		//accept阻塞等待客户端请求
		printf("\033[0;1;33mINFO > 等待客户端发起连接\033[0m\n");
		if((connect_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL)) == -1){
			printf("accept socket error: %s(error: %d)\n", strerror(errno), errno);
			exit(-4);
		}

		pthread_t pid;
		pthread_create(&pid, NULL, get_msg, NULL);

		usleep(30000);

		time_t timep1;
		struct tm *timep2;
	
		while (strcmp(sendbuf, "/exit") != 0) {
			if (flag_file != 1) {
				time(&timep1);
				timep2 = gmtime(&timep1);
				printf("\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d INPUT  > \033[0m",
				       1900 + timep2->tm_year,
				       1 + timep2->tm_mon,
				       timep2->tm_mday,
				       8 + timep2->tm_hour,
				       timep2->tm_min,
				       timep2->tm_sec
				       );
				input(sendbuf);
				time(&timep1);
				timep2 = gmtime(&timep1);
				printf("\r\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d INPUT  > \033[0;37m%s\033[0m\n",
				       1900 + timep2->tm_year,
				       1 + timep2->tm_mon,
				       timep2->tm_mday,
				       8 + timep2->tm_hour,
				       timep2->tm_min,
				       timep2->tm_sec,
				       sendbuf
				       );
			} else {
				FILE *fp = fopen(filename, "r");
				if (!fp) return;
				char ch[2] = "0\0";
				memset(sendbuf, 0, sizeof(sendbuf));
				while (ch[0] != EOF) {
					ch[0] = fgetc(fp);
					strcat(sendbuf, ch);
				}
				time(&timep1);
				timep2 = gmtime(&timep1);
				printf("\r\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d INPUT  > \033[0;37m%s\033[0m\n",
				       1900 + timep2->tm_year,
				       1 + timep2->tm_mon,
				       timep2->tm_mday,
				       8 + timep2->tm_hour,
				       timep2->tm_min,
				       timep2->tm_sec,
				       sendbuf
				       );
				fclose(fp);
				flag_file = -1;
			}
			if (flag_run == 0) {
				printf("\033[0;1;33mINFO > 对方已退出，重新开始等待连接\033[0m\n");
				break;
			}
			if (strcmp(sendbuf, "/flag_enter") == 0) {
				if (flag_enter == '\r') {
					flag_enter = 0x1B;
					printf("\033[0;1;33mINFO > FLAG_ENTER: False\033[0m\n");
				} else {
					flag_enter = '\r';
					printf("\033[0;1;33mINFO > FLAG_ENTER: True\033[0m\n");
				}
				continue;
			}
	
			//向客户端发送信息
			write(connect_fd, sendbuf, sizeof(sendbuf));
		}
		pthread_cancel(pid);
		usleep(50000);
		/* usleep(10000); */
		//关闭连接套接字
		close(connect_fd);
	}
	printf("\033[0;1;33mINFO > 检测到退出关键词，退出程序\033[0m\n");
	//关闭监听套接字
	close(listen_fd);
	return;
}
 
void client(char *addr, int port)
{
	//初始化服务器套接字地址
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;//IPv4
	servaddr.sin_port = htons(port);//想连接的服务器的端口
	servaddr.sin_addr.s_addr = inet_addr(addr);//服务器的IP地址
 
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

	time_t timep1;
	time(&timep1);
	struct tm *timep2;

	while(strcmp(sendbuf, "/exit") != 0) {
		if (flag_file != 1) {
			time(&timep1);
			timep2 = gmtime(&timep1);
			printf("\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d INPUT  > \033[0m",
			       1900 + timep2->tm_year,
			       1 + timep2->tm_mon,
			       timep2->tm_mday,
			       8 + timep2->tm_hour,
			       timep2->tm_min,
			       timep2->tm_sec
			       );
			input(sendbuf);
			time(&timep1);
			timep2 = gmtime(&timep1);
			printf("\r\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d INPUT  > \033[0;37m%s\033[0m\n",
			       1900 + timep2->tm_year,
			       1 + timep2->tm_mon,
			       timep2->tm_mday,
			       8 + timep2->tm_hour,
			       timep2->tm_min,
			       timep2->tm_sec,
			       sendbuf
			       );
		} else {
			FILE *fp = fopen(filename, "r");
			if (!fp) return;
			char ch[2] = "0\0";
			memset(sendbuf, 0, sizeof(sendbuf));
			while (ch[0] != EOF) {
				ch[0] = fgetc(fp);
				strcat(sendbuf, ch);
			}
			time(&timep1);
			timep2 = gmtime(&timep1);
			printf("\r\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d INPUT  > \033[0;37m%s\033[0m\n",
			       1900 + timep2->tm_year,
			       1 + timep2->tm_mon,
			       timep2->tm_mday,
			       8 + timep2->tm_hour,
			       timep2->tm_min,
			       timep2->tm_sec,
			       sendbuf
			       );
			fclose(fp);
			flag_file = -1;
		}
		if (flag_run == 0) {
			printf("\033[0;1;33mINFO > 对方已退出，退出程序\033[0m\n");
			break;
		}
		if (strcmp(sendbuf, "/flag_enter") == 0) {
			if (flag_enter == '\r') {
				flag_enter = 0x1B;
				printf("\033[0;1;33mINFO > FLAG_ENTER: False\033[0m\n");
			} else {
				flag_enter = '\r';
				printf("\033[0;1;33mINFO > FLAG_ENTER: True\033[0m\n");
			}
			continue;
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

/*
 * 帮助函数
 */
void help(int flag)
{
	printf("usage:\n"
	       "    socket [-f <filename>] [-s | -c] [-a <addrs>] [-p <port>] [-e]\n"
	       "    socket -h\n"
	       "option:\n"
	       "    -f <filename> Use the file as the program\n"
	       "                  input(allow use `\\r` or `\\n`)\n"
	       "    -s            Usage server mode\n"
	       "    -c            Usage client mode\n"
	       "    -a <addrs>    Set client addr [default: 127.0.0.1]\n"
	       "    -p <port>     Set port [default: 8080]\n"
	       "    -e            Disable use enter to send msg\n"
	       "    -h            show this help\n");
	if (flag > 0) flag--;
	exit(flag);
	return;
}


int main(int argc, char *argv[])
{
	int ch = 0;
	int flag_help = 0,
	    flag_mode = 0,
	    flag_port = 8080;
	char *flag_addr = "127.0.0.1";
	while ((ch = getopt(argc, argv, "hf:scp:a:e")) != -1) {    /* 获取参数 */
		switch (ch) {
		case '?':
			flag_help = -1;
			break;
		case 'h':
			flag_help = 1;
			break;
		case 'f':
			if (optopt == '?') flag_help = -2;
			strcpy(filename, optarg);
			flag_file = 1;
			break;
		case 's':
			if (flag_mode) flag_help = -3;
			flag_mode = 1;
			break;
		case 'c':
			if (flag_mode) flag_help = -4;
			flag_mode = 2;
			break;
		case 'p':
			if (optopt == '?') flag_help = -5;
			flag_port = strtod(optarg, NULL);
			break;
		case 'a':
			if (optopt == '?') flag_help = -6;
			flag_addr = optarg;
			break;
		case 'e':
			flag_enter = 0x1B;
			break;
		default:
			break;
		}
	}

	if (flag_help) help(flag_help);
	tcgetattr(STDIN_FILENO, &flag_oldt);
	flag_termux = fcntl(STDIN_FILENO, F_GETFL, 0);

	printf("\033[0;1;33m"
	       "INFO > Mode: %d(1:Server, 2:Client)\n"
	       "INFO > Addr: %s\n"
	       "INFO > Port: %04d\n"
	       "\033[0m\n",
	       flag_mode, flag_addr, flag_port);
	if (flag_mode == 1) server(flag_port);
	else if (flag_mode == 2) client(flag_addr, flag_port);
	else help(-3);
	return 0;
}


