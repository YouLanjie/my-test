/**
 * @file        new_socket.c
 * @author      Chglish
 * @date        2026-07-17
 * @brief       重写的socket.c
 */

#include "../../include/string_view.h"
#include "../../include/tools.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdcountof.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct {
	void  *ptr;
	size_t cap;
	size_t len;
	const size_t size;
} DA_t;

#define _DAP(da, offset) (da->ptr+(da->size*(offset)))
/* 需要提前初始化好.size字段
 * 申请内存，至少可用长度cap(不会释放多出的空间) */
DA_t *da_create(DA_t *da, size_t cap)
{
	if (!da || da->size == 0) return NULL;
	if (cap == 0) cap = 10;
	if (da->ptr && da->cap >= cap) return da;
	if (da->len > da->cap) da->len = 0;
	void *p = da->ptr;
	da->cap = cap;
	if (da->ptr) da->ptr = realloc(da->ptr, da->size*da->cap);
	else da->ptr = malloc(da->size*da->cap);

	if (!da->ptr) {
		if (p) free(p);
		da->cap = 0;
		da->len = 0;
		return NULL;
	}
	memset(_DAP(da, da->len), 0, da->size*(da->cap-da->len));
	return da;
}

/* 在idx处插入含有指向ptr指针的元素(让da->ptr[idx] = ptr) */
DA_t *da_insert(DA_t *da, size_t idx, void *ptr)
{
	if (!da || da->size == 0) return NULL;
	if (idx >= da->cap) {
		if (!da_create(da, idx)) return NULL;
	}
	if (da->len+1 > da->cap) {
		if (!da_create(da, da->cap*2)) return NULL;
	}
	if (idx < da->len) {
		memmove(_DAP(da, idx+1), _DAP(da, idx), da->size*(da->len-idx));
	}
	memcpy(_DAP(da, idx), ptr, da->size);
	da->len++;
	return da;
}

/* 将ptr里的内容（长度da->size）追加到末尾数组 */
DA_t *da_append(DA_t *da, void *ptr)
{
	if (!da || da->size == 0) return NULL;
	da_insert(da, da->len, ptr);
	return da;
}

DA_t *da_free(DA_t *da, void (*free_function)(void *ptr))
{
	if (!da || da->size == 0) return NULL;
	if (da->ptr) {
		for (size_t i = 0; i < da->len && free_function; i++) {
			if (_DAP(da, i)) free_function(_DAP(da, i));
		}
		free(da->ptr);
	}
	da->len = 0;
	da->cap = 0;
	da->ptr = NULL;
	return da;
}

DA_t *da_pop(DA_t *da, size_t idx, void (*free_function)(void *ptr))
{
	if (!da || da->size == 0) return NULL;
	if (idx >= da->len) return da;
	if (free_function && _DAP(da, idx)) free_function(_DAP(da, idx));
	if (idx < da->len-1) memmove(_DAP(da, idx), _DAP(da, idx+1), da->size*(da->len-idx-1));
	memset(_DAP(da, da->len), 0, da->size);
	da->len--;
	return da;
}

void *da_get(DA_t *da, size_t idx)
{
	if (!da || da->size == 0 || idx >= da->len) return NULL;
	return _DAP(da, idx);
}

#undef _DAP


/**
 * @brief 尝试获得socket fd
 *
 * @param node 传给getaddrinfo的
 * @param port 端口
 * @param mode 自动模式(0手动,1绑定并监听,2连接)
 * @return fd，负数为出错
 */
int get_socket_fd(const char *node, int port, int mode)
{
	const struct addrinfo hint = (struct addrinfo){
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = (mode == 1 ? AI_PASSIVE : 0),
	};
	struct addrinfo *res;

	int tried = 0;
	int status = 0;
	char buf_port[20];
	sprintf(buf_port, "%d", port);
	while (((status = getaddrinfo(node, buf_port, &hint, &res)) || !res) && tried < 20) {
		sprintf(buf_port, "%d", port);
		tried++;
	}
	if (status) {
		fprintf(stderr, "getaddrinfo错误: [%d] %s\n", status, gai_strerror(status));
		return -2;
	}

	int fd = -1;
	struct addrinfo *p;
	for (p = res; p; p=p->ai_next) {
		if (fd >= 0) close(fd);
		fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (fd == -1) {
			perror("sockopt");
			continue;
		}

		/* 允许复用端口(socketserver.TCPServer.allow_reuse_address) */
		int reuse = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
			perror("setsockopt");
			continue;
		}

		/* 服务端： bind + listen + accept */
		//绑定套接字和本地IP地址和端口
		if (mode == 1 && bind(fd, res->ai_addr, res->ai_addrlen)) {
			perror("bind");
			continue;
		} else if (mode == 2 && connect(fd, res->ai_addr, res->ai_addrlen)) {
			perror("connect");
			continue;
		}

		if (mode == 1 && listen(fd, 10)) {
			perror("listen");
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	if (!p) {
		fprintf(stderr, "尝试建立套接字绑定失败");
		return -3;
	}
	return fd;
}

typedef struct {
	uint64_t pid;
	SVA_t name;
} Platform_t;

typedef struct {
	double timestamp;
	uint64_t uid;
	uint64_t pid;
} UserLogin_t;

typedef struct Users_t {
	uint64_t uid;
	SVA_t name;
	SVA_t passwd;
	SVA_t note;
	enum { UT_BAN, UT_VISIT, UT_NORM, UT_ADMIN } type;
} User_t;

typedef struct {
	uint64_t mid;          /* 消息id */
	uint64_t owner_uid;    /* 发送用户id */
	double timestamp;
	SVA_t content;
	enum { MT_TEXT, MT_MD, MT_ORG, MT_FILE } type;
} Messages_t;

void free_message(void *ptr)
{
	if (!ptr) return;
	Messages_t *p = ptr;
	sva_free(&p->content);
}

void close_fds(void *ptr)
{
	if (!ptr) return;
	struct pollfd *p = ptr;
	if (p->fd >= 0) close(p->fd);
}

typedef struct {
	DA_t platforms;
	DA_t logins;
	DA_t users;
	DA_t messages;
	DA_t fds;
	SVA_t logs;
	SVA_t input;

	int fd;
	int flag_lock;    /* getch的输入锁变量 */
	bool flag_exited;
} Runtimedata_t;

// 获取 sockaddr，IPv4 或 IPv6：
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr);
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// 将 buf 发送给所有连接的客户端
void send_to_all(int sender_fd, char *buf, int nbytes,
		 struct pollfd *pfds, int fd_count)
{
	for (int j = 0; j < fd_count; j++) {
		int dest_fd = pfds[j].fd;
		if (dest_fd != sender_fd && dest_fd != pfds[0].fd) {
			if (send(dest_fd, buf, nbytes, 0) == -1) {
				perror("send");
			}
		}
	}
}

double getnowtime()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec + ts.tv_nsec/1e9;
}

bool redraw(Runtimedata_t *rt)
{
	if (!rt) return true;
	const char *colors[] = {
		"\e[0;30;42m",
		"\e[0;30;43m",
	};
	str_window_t win = {};
	// [2026-07-18 04:03:00] -> len() == 21
	const int left = 23;
	const int logwidth = 15;
	const int msgheigh = 2;
	int col = 0, row = 0;
	if (!rt->flag_lock) {
		col = get_winsize_col();
		row = get_winsize_row();
		win = (str_window_t){
			.x = col - logwidth - 3,
			.y = 1,
			.width = 3,
			.heigh = col-msgheigh,
			.focus = -1,
			.color_code = NULL,
		};
		print_in_box(win, "");
		win.x = left;
		win.width = col - left - logwidth - 3;
		win.heigh = 1;
		win.focus = -1;
	}
	for (size_t i = (int)rt->messages.len>=(row-msgheigh)?rt->messages.len-(row-msgheigh):0, j = 0;
	     !rt->flag_lock && i < rt->messages.len && (int)j < row-msgheigh; i++, j++) {
		win.y = j+1;
		win.color_code = colors[i%countof(colors)];
		Messages_t *msg = da_get(&rt->messages, i);
		if (!msg) continue;
		time_t p = msg->timestamp;
		struct tm *tm_info = localtime(&p);
		char buf[30];
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
		printf("\e[0m\e[%zu;0H[%s]", i+1, buf);
		if (print_in_box(win, msg->content.p)) printf("...");
	}
	win = (str_window_t){
		.x = col - logwidth+1,
		.y = 1,
		.width = logwidth,
		.heigh = col-msgheigh,
		.focus = -1,
		.color_code = "\e[0;34m",
	};
	print_in_box(win, rt->logs.p);
	win = (str_window_t){
		.x = 1,
		.y = row - msgheigh+1,
		.width = -1,
		.heigh = msgheigh,
		.focus = -1,
		.color_code = "\e[0;2m",
	};
	print_in_box(win, rt->input.p?rt->input.p:"请输入文本");
return false;
}

void *server(void *data)
{
	if (!data) return NULL;
	Runtimedata_t *rt = data;

	da_create(&rt->fds, 10);
	if (!rt->fds.cap) {
		rt->flag_exited = 1;
		return NULL;
	}

	da_append(&rt->fds, &(struct pollfd){.fd = rt->fd, .events = POLLIN});

	sva_sprintfcat(&rt->logs, "等待连接中...\n");
	while (!rt->flag_exited) {
		if (poll(rt->fds.ptr, rt->fds.len, 2500) == -1) {
			perror("poll");
			rt->flag_exited = 1;
			return NULL;
		}
		struct pollfd* fds = rt->fds.ptr;
		for (size_t i = 0; i < rt->fds.len; i++) {
			if (!(fds[i].revents & POLLIN)) continue;
			if (fds[i].fd == rt->fd) {
				struct sockaddr_storage remoteaddr;
				socklen_t addrlen = sizeof(remoteaddr);
				int newfd = accept(rt->fd, (void*)&remoteaddr, &addrlen);
				if (newfd == -1) {
					perror("accept");
					continue;
				}
				da_append(&rt->fds, &(struct pollfd){
					  .fd = newfd, .events = POLLIN});
				fds = rt->fds.ptr;
				char remoteIP[INET6_ADDRSTRLEN] = {};
				const void *tmp = get_in_addr((struct sockaddr *)&remoteaddr);
				const char *p = inet_ntop(remoteaddr.ss_family, tmp, remoteIP, INET6_ADDRSTRLEN);
				sva_sprintfcat(&rt->logs, "新连接: (%d) '%s'\n", newfd, p);
				rt->flag_lock = false;
				redraw(rt);
				continue;
			}
			// 如果不是 listener，那就是客户端
			da_append(&rt->messages, &(Messages_t){.mid = rt->messages.len});
			Messages_t *new_msg = da_get(&rt->messages, rt->messages.len-1);
			if (!new_msg) continue;
			new_msg->timestamp = getnowtime();
			sva_adjust_minimun(&new_msg->content, 10);
			ssize_t ret;
			do {
				if (new_msg->content.len >= new_msg->content.capacity-1)
					sva_double(&new_msg->content);
				ret = recv(fds[i].fd, new_msg->content.p+new_msg->content.len,
					   new_msg->content.capacity-new_msg->content.len-1, 0);
				if (ret > 0) new_msg->content.len+=ret;
			} while (ret > 0 && new_msg->content.len == new_msg->content.capacity-1);
			new_msg->content.p[new_msg->content.len] = '\0';

			rt->flag_lock = false;
			if (ret > 0) {
				send_to_all(fds[i].fd, new_msg->content.p,
					    new_msg->content.len,
					    fds, rt->fds.len);
				continue;
			}
			// 连接关闭或出错
			if (ret == 0) sva_sprintfcat(&rt->logs, "断连: (%d)\n", fds[i].fd);
			else perror("recv");
			close(fds[i].fd);
			da_pop(&rt->fds, i, NULL);
			i--;
			redraw(rt);
		}
	}

	da_free(&rt->fds, close_fds);
	rt->flag_exited = 1;
	return NULL;
}

void *client(void *data)
{
	/* 客户端： connect */
	if (!data) return NULL;
	// Runtimedata_t *rt = data;
	return NULL;
}

int main(int argc, const char *argv[])
{
	if (argc < 1) return 3;

	int port = 9999;
	int fd = -1;
	int mode = 0;
	if (argv[0][0] == 'c' || argv[0][0] == 'C') {
		mode = 2;
	} else {
		mode = 1;
	}
	fd = get_socket_fd(NULL, port, mode);
	if (fd < 0) {
		printf("[ERROR] 获取套接字错误\n");
		return 1;
	}

	Runtimedata_t rt = {
		.platforms.size = sizeof(Platform_t),
		.logins.size = sizeof(UserLogin_t),
		.users.size = sizeof(User_t),
		.messages.size = sizeof(Messages_t),
		.fds.size = sizeof(struct pollfd),
		.fd = fd
	};
	pthread_t pid = 0;
	if (mode == 1) pthread_create(&pid, NULL, server, &rt);
	else client(&rt);

	printf(/* "\033[?25l" */ "\e[2J");
	while (!rt.flag_exited) {
		redraw(&rt);
		rt.flag_lock = true;
		int ret = _getch_cond(&rt.flag_lock);
		if (ret == '\n' && rt.input.p) {
			if (strcmp(rt.input.p, "/exit") == 0) {
				rt.flag_exited = 1;
				printf("\e[%d;0H\n正在退出。。。\n", get_winsize_row());
				break;
			}
			da_append(&rt.messages, &(Messages_t){.mid = rt.messages.len});
			Messages_t *new_msg = da_get(&rt.messages, rt.messages.len-1);
			if (!new_msg) {
				memset(rt.input.p, 0, rt.input.len);
				rt.input.len = 0;
				continue;
			}
			new_msg->timestamp = getnowtime();
			sva_strcpy(&new_msg->content, &rt.input);
			send_to_all(rt.fd, rt.input.p, rt.input.len,
				    rt.fds.ptr, rt.fds.len);
			sva_free(&rt.input);
			rt.flag_lock = false;
		} else if (ret >= 0) {
			sva_sprintfcat(&rt.input, "%c", ret);
			while (kbhit()) sva_sprintfcat(&rt.input, "%c", _getch());
		}
		if (rt.input.len >= rt.input.capacity) sva_double(&rt.input);
	}
	pthread_join(pid, NULL);

	if (rt.fd >= 0) close(rt.fd);
	sva_free(&rt.input);
	sva_free(&rt.logs);
	da_free(&rt.fds, close_fds);
	da_free(&rt.platforms, NULL);
	da_free(&rt.logins, NULL);
	da_free(&rt.users, NULL);
	da_free(&rt.messages, free_message);
	return 0;
}

