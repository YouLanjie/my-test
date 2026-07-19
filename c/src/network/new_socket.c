/**
 * @file        new_socket.c
 * @author      Chglish
 * @date        2026-07-17
 * @brief       重写的socket.c
 */

#include "../../include/string_view.h"
#include "../../include/dynamic_array.h"
#include "../../include/tools.h"
#include <arpa/inet.h>
#include <locale.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdcountof.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

double getnowtime()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec + ts.tv_nsec/1e9;
}

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
	double reg_time;
	enum { UT_BAN, UT_VISIT, UT_NORM, UT_ADMIN } type;
} User_t;

typedef struct {
	uint64_t mid;          /* 消息id */
	uint64_t owner_uid;    /* 发送用户id */
	double timestamp;
	SVA_t content;
	enum { MT_TEXT, MT_MD, MT_ORG, MT_FILE } type;
} Messages_t;

typedef struct {
	DA_t messages;
	DA_t users;
	DA_t platforms;
	DA_t login_hist;
	DA_t fds;

	SVA_t logs;
	SVA_t input;

	const char *hint_text;
	enum {DM_NORM, DM_SELECT, DM_FOCUS} dispaly_mode;
	size_t msg_select;
	str_window_t win;

	int fd;
	bool flag_refresh_msgs;
	bool flag_exited;
	bool flag_esc_confirm;
} Runtimedata_t;

User_t *user_get(Runtimedata_t *rt, SV_t name)
{
	if (!rt) return NULL;
	User_t *user = NULL;
	for (size_t i = 0; i < rt->users.len; i++) {
		user = da_get(&rt->users, i);
		if (!user) continue;
		if (sv_cmp(sv_from_sva(&user->name), name)) break;
		user = NULL;
	}
	return user;
}

User_t *user_create(Runtimedata_t *rt, SV_t name)
{
	if (!rt) return NULL;
	if (user_get(rt, name)) return NULL;
	User_t *user = NULL;
	size_t len = rt->users.len;
	da_append(&rt->users, &(User_t){
		  .uid = len,
		  .reg_time = getnowtime(),
		  .type = UT_NORM,
		  });
	user = da_get(&rt->users, len);
	if (!user) return NULL;
	sva_from_sv(&user->name, name);
	return user;
}

void user_free(void *ptr)
{
	if (!ptr) return;
	User_t *user = ptr;
	sva_free(&user->name);
	sva_free(&user->passwd);
	sva_free(&user->note);
}

Messages_t *message_get(Runtimedata_t *rt, size_t mid)
{
	if (!rt) return NULL;
	Messages_t *msg = NULL;
	for (size_t i = 0; i < rt->messages.len; i++) {
		/* 优先从后向前搜索 */
		msg = da_get(&rt->messages, rt->messages.len-i-1);
		if (!msg) continue;
		if (msg->mid == mid) break;
		msg = NULL;
	}
	return msg;
}

Messages_t *message_create(Runtimedata_t *rt, User_t *user, SV_t content)
{
	if (!rt || !user) return NULL;
	size_t mid = rt->messages.len;
	while (message_get(rt, mid)) mid++;
	da_append(&rt->messages, &(Messages_t){
		  .mid = mid,
		  .owner_uid = user->uid,
		  .timestamp = getnowtime(),
		  });
	Messages_t *new_msg = message_get(rt, mid);
	if (!new_msg) return NULL;
	sva_from_sv(&new_msg->content, content);
	rt->flag_refresh_msgs = true;
	return new_msg;
}

void message_free(void *ptr)
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

bool redraw(Runtimedata_t *rt)
{
	if (!rt) return true;

	static bool running = false;
	if (running) return false;
	running = true;

	const char *colors[] = {
		"\e[0;30;42m",
		"\e[0;30;43m",
	};
	// [2026-07-18 04:03:00] -> len() == 21
	const int timewidth = 23;
	const int userwidth = 9;
	const int logwidth = 18;
	const int dotwidth = 3;
	const int msgheigh = 3;
	int col = 0, row = 0;
	col = get_winsize_col();
	row = get_winsize_row();
	print_in_box((str_window_t){
		     .x = col - logwidth - 3,
		     .y = 1,
		     .width = 3,
		     .heigh = col-msgheigh,
		     .focus = -1,
		     .color_code = NULL,
		     }, "");
	str_window_t win = {
		.x = timewidth+userwidth,
		.width = col-timewidth-userwidth-logwidth-dotwidth,
		.heigh = 1,
		.focus = -1,
	}, winuser = {
		.x = timewidth,
		.width = userwidth-1,
		.heigh = 1,
		.focus = -1,
	};
	if (rt->messages.len == 0) {
		win.y = 1;
		win.color_code = colors[1];
		print_in_box(win, "局域网聊天室（暂无消息）");
	}
	char buf[124];
	size_t idx = (int)rt->messages.len>=(row-msgheigh)?rt->messages.len-(row-msgheigh):0;
	if (rt->dispaly_mode == DM_SELECT) {
		idx = rt->msg_select > (size_t)row/2 ? rt->msg_select - row/2 : 0;
	}
	for (size_t j = 0; rt->flag_refresh_msgs && (int)j < row-msgheigh; idx++, j++) {
		if (idx >= rt->messages.len) {
			win.x = 1;
			win.y = j+1;
			win.width = col-logwidth;
			win.color_code = NULL;
			print_in_box(win, "");
			continue;
		}

		if ((size_t)rt->msg_select == idx+1) win.focus = 0;
		else win.focus = -1;

		Messages_t *msg = da_get(&rt->messages, idx);
		if (!msg) continue;
		User_t *user = da_get(&rt->users, msg->owner_uid);
		if (!user) continue;
		winuser.y = win.y = j+1;
		winuser.color_code = win.color_code = colors[(user->uid)%countof(colors)];
		time_t p = msg->timestamp;
		struct tm tm_info;
		localtime_r(&p, &tm_info);
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
		printf("\e[0m\e[%zu;0H[%s]", idx+1, buf);
		print_in_box(winuser, user->name.p);
		if (print_in_box(win, msg->content.p)) printf("...");
	}
	if (rt->dispaly_mode == DM_FOCUS) {
		rt->win.x = (col-msgheigh)/5;
		rt->win.y = timewidth/2;
		rt->win.width = col-timewidth-userwidth-logwidth-dotwidth;
		rt->win.heigh = (row-msgheigh)*3/5;
		rt->win.focus = -1;
		rt->win.follow_end = false;
		rt->win.color_code = "\e[0;37;44m";
		Messages_t *msg = da_get(&rt->messages, rt->msg_select-1);
		print_in_box(rt->win, msg ? msg->content.p : "（未找到对应消息）");
	}
	rt->flag_refresh_msgs = false;
	print_in_box((str_window_t){
		     .x = col - logwidth+1,
		     .y = 1,
		     .width = logwidth,
		     .heigh = row-msgheigh,
		     .focus = -1,
		     .color_code = "\e[0;30;44m",
		     .follow_end = true,
		     }, rt->logs.p);
	sprintf(buf, "|现在线数:%zu\n|ESC发消息:%s\n|M:%d, SEL:%zu",
		rt->fds.len, rt->flag_esc_confirm?"true":"false",
		rt->dispaly_mode, rt->msg_select);
	print_in_box((str_window_t){
		     .x = col-logwidth+1,
		     .y = row-msgheigh+1,
		     .width = logwidth,
		     .heigh = msgheigh,
		     .focus = -1,
		     .color_code = "\e[0;37;44m",
		     }, buf);
	print_in_box((str_window_t){
		     .x = 1,
		     .y = row - msgheigh+1,
		     .width = col - logwidth,
		     .heigh = msgheigh,
		     .focus = -1,
		     .color_code = "\e[0;30;47m",
		     .follow_end = true,
		     }, rt->hint_text?rt->hint_text:\
		     (rt->input.p&&rt->input.len?rt->input.p:"请输入消息："));
	running = false;
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
		if (poll(rt->fds.ptr, rt->fds.len, 1e3) == -1) {
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
				sva_sprintfcat(&rt->logs, "新连接: (%d) FROM\n'%s'\n", newfd, p);
				redraw(rt);
				continue;
			}
			// 如果不是 listener，那就是客户端
			SVA_t content = {};
			sva_adjust_minimun(&content, 10);
			ssize_t ret;
			do {
				if (content.len >= content.capacity-1)
					sva_double(&content);
				ret = recv(fds[i].fd, content.p+content.len,
					   content.capacity-content.len-1, 0);
				if (ret > 0) content.len+=ret;
			} while (ret > 0 && content.len == content.capacity-1);
			content.p[content.len] = '\0';

			if (ret > 0) {
				message_create(rt, user_get(rt, sv_from_lstr("Visitor")), sv_from_sva(&content));
				send_to_all(fds[i].fd, content.p, content.len, fds, rt->fds.len);
				sva_free(&content);
				redraw(rt);
				continue;
			}
			sva_free(&content);
			// 连接关闭或出错
			if (ret == 0) {
				sva_sprintfcat(&rt->logs, "断连: (%d)\n", fds[i].fd);
			} else perror("recv");
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

int input_handle(Runtimedata_t *rt, int ret);

/* 返回值大于0应continue,小于0应break,等于0发消息 */
int input_command(Runtimedata_t *rt)
{
	if (!rt || !rt->input.p || !rt->input.len) return -1;
	int ret = 1;
	if (strcmp(rt->input.p, "/exit") == 0 ||
	    strcmp(rt->input.p, "/quit") == 0 ||
	    strcmp(rt->input.p, "/q") == 0 ||
	    strcmp(rt->input.p, "/leave") == 0) {
		ret = -1;
	} else if (strcmp(rt->input.p, "/enter") == 0) {
		rt->flag_esc_confirm = !rt->flag_esc_confirm;
	} else if (strcmp(rt->input.p, "/select") == 0) {
		bool flag = rt->dispaly_mode == DM_SELECT;
		rt->dispaly_mode = flag ? DM_NORM : DM_SELECT;
		rt->msg_select = flag ? 0 : 1;
		rt->flag_refresh_msgs = true;
		if (!flag) rt->hint_text = "（选择模式：使用C-pnfb或命令移动选中项）";
	} else if (strcmp(rt->input.p, "/show") == 0) {
		if (rt->dispaly_mode == DM_SELECT && rt->msg_select > 0)
			rt->dispaly_mode = DM_FOCUS;
		rt->flag_refresh_msgs = true;
	} else if (strcmp(rt->input.p, "/clear") == 0) {
		rt->flag_refresh_msgs = true;
		printf("\e[2J\e[H");
	} else if (rt->dispaly_mode == DM_SELECT &&
		   (strcmp(rt->input.p, "/s-h") == 0 || strcmp(rt->input.p, "/s-k") == 0)) {
		input_handle(rt, 0x10);
	} else if (rt->dispaly_mode == DM_SELECT &&
		   (strcmp(rt->input.p, "/s-l") == 0 || strcmp(rt->input.p, "/s-j") == 0)) {
		input_handle(rt, 0x0e);
	} else {
		rt->hint_text = "（非法的命令）请输入消息：";
	}
	if (ret > 0) sva_clear(&rt->input);
	return ret;
}

int input_handle(Runtimedata_t *rt, int ret)
{
	if (!rt) return -1;
	if (rt->dispaly_mode == DM_FOCUS) {
		if (rt->msg_select <= 0 || rt->msg_select > rt->messages.len) ret = 'q';
		switch (ret) {
		case 'k': case 'h': rt->win.hide--; break;
		case 'j': case 'l': rt->win.hide++; break;
		case 'q':
			rt->dispaly_mode = DM_SELECT;
			rt->flag_refresh_msgs = true;
			rt->hint_text = NULL;
			printf("\e[2J\e[H");
			break;
		default: rt->hint_text = "（使用HJKL移动）"; break;
		}
		return 1;
	}

	if (!rt->input.len && !rt->flag_esc_confirm && ret == '\e' && kbhit() > 0) {
		_getch();
		ret = _getch();
		char table[UINT8_MAX] = {
			['A'] = 0x10,
			['D'] = 0x02,
			['B'] = 0x0e,
			['C'] = 0x06,
		};
		if (ret > 0 && table[ret%countof(table)]) {
			ret = table[ret];
			if (rt->dispaly_mode == DM_NORM) {
				rt->dispaly_mode = DM_SELECT;
				rt->flag_refresh_msgs = true;
			}
		}
	}
	if (rt->dispaly_mode == DM_SELECT) {
		switch (ret) {
		case 0x10:    /* ^P */
		case 0x02:    /* ^B */
			if (rt->msg_select > 1) rt->msg_select--;
			else rt->msg_select = rt->messages.len;
			rt->flag_refresh_msgs = true;
			return 1;
			break;
		case 0x0e:    /* ^N */
		case 0x06:    /* ^F */
			if (rt->msg_select && rt->msg_select < rt->messages.len)
				rt->msg_select++;
			else rt->msg_select = 1;
			rt->flag_refresh_msgs = true;
			return 1;
			break;
		case '/':
			break;
		default:
			if (rt->input.p && rt->input.len && rt->input.p[0] == '/')
				break;
			if (ret == '\n') {
				sva_sprintf(&rt->input, "/show");
				break;
			} else if (ret == 'q') {
				rt->dispaly_mode = DM_NORM;
				rt->msg_select = 0;
				rt->flag_refresh_msgs = true;
				break;
			}
			return 1;
			break;
		}
	}

	if (rt->flag_esc_confirm?ret=='\e':ret == '\n') {
		if (!rt->input.p || !rt->input.len) return 1;
		ret = 0;
		if (rt->input.p[0] == '/' && (ret = input_command(rt)) < 0) {
			return ret;
		} else if (ret > 0) return ret;
		return 0;
	}
	if (ret == 127) {
		if (!rt->input.p || !rt->input.len) return 1;
		/* 计算最后一个宽字符字节数 */
		mbstate_t state = (mbstate_t){};
		wchar_t wc = L'\0';
		size_t len = 0;
		for (size_t i = rt->input.len>10?rt->input.len-10:0; i < rt->input.len; i+=len) {
			len = mbrtowc(&wc, rt->input.p+i, rt->input.len-i, &state);
			if (len != (size_t)-1 && len != (size_t)-2 && len != 0) continue;
			state = (mbstate_t){};
			len = 1;
		}
		sva_chop_right(&rt->input, len);
		return 1;
	}
	if (ret >= 0) {
		if (rt->hint_text) rt->hint_text = NULL;
		sva_sprintfcat(&rt->input, "%c", ret);
		while (kbhit()) sva_sprintfcat(&rt->input, "%c", _getch());
		return 1;
	}
	return 2;
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
		.login_hist.size = sizeof(UserLogin_t),
		.users.size = sizeof(User_t),
		.messages.size = sizeof(Messages_t),
		.fds.size = sizeof(struct pollfd),
		.fd = fd
	};
	sva_sprintfcat(&rt.logs, "这里是日志区\n");
	do {
		User_t *u = user_create(&rt, sv_from_lstr("Administor"));
		if (u) u->type = UT_ADMIN;
		u = user_create(&rt, sv_from_lstr("Visitor"));
		if (u) u->type = UT_VISIT;
	} while(0);

	pthread_t pid = 0;
	if (mode == 1) pthread_create(&pid, NULL, server, &rt);
	else client(&rt);

	setlocale(LC_ALL, "");
	printf(/* "\033[?25l" */ "\e[2J");
	while (!rt.flag_exited) {
		redraw(&rt);
		int ret = _getch();
		if ((ret = input_handle(&rt, ret)) > 0) {
			if (rt.input.len >= rt.input.capacity) sva_double(&rt.input);
		} else if (ret < 0) {
			break;
		} else {
			message_create(&rt, user_get(&rt, sv_from_lstr("Administor")), sv_from_sva(&rt.input));
			send_to_all(rt.fd, rt.input.p, rt.input.len,
				    rt.fds.ptr, rt.fds.len);
			sva_free(&rt.input);
		}
	}
	rt.flag_exited = 1;
	printf("\e[%d;0H\n正在退出。。。\n", get_winsize_row());
	pthread_join(pid, NULL);

	if (rt.fd >= 0) close(rt.fd);
	sva_free(&rt.input);
	sva_free(&rt.logs);
	da_free(&rt.fds, close_fds);
	da_free(&rt.platforms, NULL);
	da_free(&rt.login_hist, NULL);
	da_free(&rt.users, user_free);
	da_free(&rt.messages, message_free);
	return 0;
}

