#include "include/tools.h"
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <termios.h>
#include <libintl.h>

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
int flag_exit = 1;
char filename[MAXSIZE] = "(null)";

struct termios flag_oldt;
int flag_termux;

/*
 * 获取输入
 */
void input(char *str)
{
	struct ctools ctools = ctools_init();
	int ch    = 0,
	    count = 0;
	if (str == NULL) return;
	char *str2 = malloc(sizeof(char)*MAXSIZE);
	memset(str,  0, sizeof(char)*MAXSIZE);
	memset(str2, 0, sizeof(char)*MAXSIZE);
	while (ch != 0x1B && ch != flag_enter) {
		ch = ctools.getcha();
		if (ch == 0x1B && ctools.kbhit() != 0)
			ch = '\0';
		if (ch & 0x80) {    /* If is Chinese */
			printf("\033[0;37m%c", ch);
			sprintf(str2, "%s%c", str, ch);
			strcpy(str, str2);
			ch = ctools.getcha();
			printf("%c", ch);
			sprintf(str2, "%s%c", str, ch);
			strcpy(str, str2);
			ch = ctools.getcha();
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
	while (flag_exit) {
		ssize_t len = read(connect_fd, recbuf, sizeof(recbuf));
		if (len <= 0){
			if(len == 0 || errno == EINTR) {
				flag_run = 0;
				pthread_exit(NULL);
			}
			tcsetattr(STDIN_FILENO, TCSANOW, &flag_oldt);
			fcntl(STDIN_FILENO, F_SETFL, flag_termux);
			fprintf(stderr, "\n\033[1;31mERROR > %s: %s(error: %d)\033[0m\n", gettext("获取信息错误"), strerror(errno), errno);
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
		printf("\033[0;1;33;47m"
		       "%04d-%02d-%02d %02d-%02d-%02d %s > \033[0;30;47m%s\033[0m\r\n", 1900 + timep2->tm_year, 1 + timep2->tm_mon, timep2->tm_mday, 8 + timep2->tm_hour, timep2->tm_min, timep2->tm_sec, gettext(" 输出 "), recbuf);
		printf("\033[0;1;33m"
		       "%04d-%02d-%02d %02d-%02d-%02d %s  > \033[0;37m%s\033[0m", 1900 + timep2->tm_year, 1 + timep2->tm_mon, timep2->tm_mday, 8 + timep2->tm_hour, timep2->tm_min, timep2->tm_sec, gettext(" 输入"), sendbuf);
		if (access("socket_get_output.txt", F_OK) != 0 && flag_file == -1) {
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

/*
 * UI交互
 */
int ui(void)
{
	pthread_t pid;
	pthread_create(&pid, NULL, get_msg, NULL);

	usleep(30000);

	time_t timep1;
	struct tm *timep2;
	
	printf("\033[0;1;33mINFO > %s\033[0m\n", gettext("连接成功，输入`/help`然后按ESC或回车获取帮助"));
	while (strcmp(sendbuf, "/exit") != 0) {
		if (flag_file != 1) {
			time(&timep1);
			timep2 = gmtime(&timep1);
			printf("\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d %s  > \033[0m", 1900 + timep2->tm_year, 1 + timep2->tm_mon, timep2->tm_mday, 8 + timep2->tm_hour, timep2->tm_min, timep2->tm_sec, gettext(" 输入"));
			input(sendbuf);
			time(&timep1);
			timep2 = gmtime(&timep1);
			printf("\r\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d %s  > \033[0;37m%s\033[0m\n", 1900 + timep2->tm_year, 1 + timep2->tm_mon, timep2->tm_mday, 8 + timep2->tm_hour, timep2->tm_min, timep2->tm_sec, gettext(" 输入"), sendbuf);
		} else {
			FILE *fp = fopen(filename, "r");
			if (!fp) return -1;
			char ch[2] = "0\0";
			memset(sendbuf, 0, sizeof(sendbuf));
			while (ch[0] != (char)EOF) {
				ch[0] = fgetc(fp);
				strcat(sendbuf, ch);
			}
			time(&timep1);
			timep2 = gmtime(&timep1);
			printf("\r\033[0;1;33m%04d-%02d-%02d %02d-%02d-%02d %s  > \033[0;37m%s\033[0m\n", 1900 + timep2->tm_year, 1 + timep2->tm_mon, timep2->tm_mday, 8 + timep2->tm_hour, timep2->tm_min, timep2->tm_sec, gettext(" 输入"), sendbuf);
			fclose(fp);
			flag_file = -1;
		}
		if (flag_run == 0) {
			printf("\033[0;1;33mINFO > %s\033[0m\n", gettext("对方已退出，重新开始等待连接"));
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
		} else if (strcmp(sendbuf, "/help") == 0) {
			printf("\033[0;1;33m"
			       "INFO > Help message:\n"
			       "\033[0;1;32m"
			       "HELP > /help        %s\n"
			       "HELP > /exit        %s\n"
			       "HELP > /flag_enter  %s\n"
			       "HELP > /lang_query  %s\n"
			       "HELP > /lang_en     %s\n"
			       "HELP > /lang_zh     %s\n"
			       "HELP > <message>    %s\n"
			       "HELP > <ESC>        %s\n"
			       "HELP > <Enter>      %s\n"
			       "HELP > <Backspace>  %s"
			       "\033[0m\n",
			       gettext("显示这条帮助"),
			       gettext("退出程序"),
			       gettext("切换按下回车结束消息的功能"),
			       gettext("查询当前的语言设置"),
			       gettext("设置界面语言为英文"),
			       gettext("设置界面语言为中文"),
			       gettext("输入消息"),
			       gettext("发送消息"),
			       gettext("结束消息(需要FLAG_ENTER显示为True)"),
			       gettext("删除字符(需开启回车结束消息，否则它不会工作)")
			       );
			continue;
		} else if (strcmp(sendbuf, "/lang_zh") == 0) {
			setlocale(LC_ALL, "zh_CN.UTF-8");
			bindtextdomain("socket", "Lang");
			textdomain("socket");
			printf("\033[0;1;33mINFO > LANG: zh_CN\033[0m\n");
			continue;
		} else if (strcmp(sendbuf, "/lang_en") == 0) {
			setlocale(LC_ALL, "en_US.UTF-8");
			bindtextdomain("socket", "Lang");
			textdomain("socket");
			printf("\033[0;1;33mINFO > LANG: en_US\033[0m\n");
			continue;
		} else if (strcmp(sendbuf, "/lang_query") == 0) {
			printf("\033[0;1;33mINFO > LANG: %s\033[0m\n", setlocale(LC_ALL, NULL));
			continue;
		}
	
		//向客户端发送信息
		write(connect_fd, sendbuf, sizeof(sendbuf));
	}
	flag_exit = 0;
	/*pthread_cancel(pid);*/
	usleep(50000);
	return 0;
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
		fprintf(stderr, "\033[1;31m%s: %s(error: %d)\033[0m\n", gettext("创建套接字错误"), strerror(errno), errno);
		exit(-1);
	}

	//绑定套接字和本地IP地址和端口
	if(bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		//绑定出现错误
		fprintf(stderr, "%s: %s(error: %d)\n", gettext("绑定套接字错误"), strerror(errno), errno);
		printf("%s\n", gettext("小提示：一般情况下用`sudo lsof -i:$port`检查$port是否有进程占用$port，如果没有那么等一会可能就好了，或者更换端口"));
		exit(-2);
	} 

	//使得listen_fd变成监听描述符
	if(listen(listen_fd, 10) == -1){
		fprintf(stderr, "%s: %s(error: %d)\n", gettext("监听端口错误"), strerror(errno), errno);
		exit(-3);
	}
 
	while (strcmp(sendbuf, "/exit") != 0) {
		//accept阻塞等待客户端请求
		printf("\033[0;1;33mINFO > %s\033[0m\n", gettext("等待客户端发起连接"));
		if((connect_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL)) == -1){
			printf("accept socket error: %s(error: %d)\n", strerror(errno), errno);
			exit(-4);
		}
		if (ui() == -1) strcpy(sendbuf, "/exit");
		//关闭连接套接字
		close(connect_fd);
	}
	printf("\033[0;1;33mINFO > %s\033[0m\n", gettext("检测到退出关键词，退出程序"));
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
		printf("%s: %s(error: %d)\n", gettext("创建套接字错误"), strerror(errno), errno);
		exit(-1);
	} 
	//向服务器发送连接请求
	if(connect(connect_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		//连接失败
		printf("%s: %s(error: %d)\n", gettext("监听套接字错误"), strerror(errno), errno);
		exit(-2);
	}

	ui();
	//关闭套接字
	close(connect_fd);
	return;
}

/*
 * 帮助函数
 */
void help(int flag)
{
	printf("%s:\n"
	       "    socket [-f <filename>] [-s | -c] [-a <addrs>] [-p <port>] [-e]\n"
	       "    socket -h\n"
	       "%s:\n"
	       "    -f <filename> %s\n"
	       "                  %s\n"
	       "    -s            %s\n"
	       "    -c            %s\n"
	       "    -a <addrs>    %s\n"
	       "    -p <port>     %s\n"
	       "    -e            %s\n"
	       "    -h            %s\n",
	       gettext("用法"),
	       gettext("选项"),
	       gettext("使用指定的文件作为程序的输入（仅一次）"),
	       gettext("(早期功能，已废弃)"),
	       gettext("使用服务器模式"),
	       gettext("使用客户端模式"),
	       gettext("设置目标IP地址 [默认: 127.0.0.1]"),
	       gettext("设置目标端口号 [默认: 8080]"),
	       gettext("取消使用回车发送消息"),
	       gettext("显示这条帮助")
	       );
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

	setlocale(LC_ALL, "");
	bindtextdomain("socket", "Lang");
	textdomain("socket");
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
	       "INFO > Mode: %d    \t[1:%s | 2:%s]\n"
	       "INFO > Addr: %s\n"
	       "INFO > Port: %04d\n"
	       "INFO > Entr: 0x%02X\t[0x0A:True | 0x1B:False]\n"
	       "INFO > File: %s\n"
	       "\033[0m\n",
	       flag_mode, gettext("服务端"), gettext("客户端"), flag_addr, flag_port, flag_enter, filename);
	if (flag_mode == 1) server(flag_port);
	else if (flag_mode == 2) client(flag_addr, flag_port);
	else help(-3);
	return 0;
}

