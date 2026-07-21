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
#include <errno.h>
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

#define SELF_SESSION_NAME sv_from_lstr("USERINPUT")
#define SELF_USER_NAME sv_from_lstr("Administor")

double getnowtime()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec + ts.tv_nsec/1e9;
}

char *timestamp2str(double timestamp, char *buf, size_t bufsize, const char *fmt)
{
	if (!buf || !bufsize) return NULL;
	time_t p = timestamp;
	struct tm tm_info;
	localtime_r(&p, &tm_info);
	strftime(buf, bufsize, fmt?fmt:"%Y-%m-%d %H:%M:%S", &tm_info);
	return buf;
}

SVA_t *get_ip_addr(SVA_t *dest, struct sockaddr_storage *remoteaddr, bool add_port)
{
	if (!dest || !remoteaddr) return NULL;
	char addr_str[INET6_ADDRSTRLEN] = {};
	void *addr;
	uint16_t port;
	if (remoteaddr->ss_family == AF_INET6) {
		struct sockaddr_in6 *ipv6_addr = (void*)remoteaddr;
		addr = &ipv6_addr->sin6_addr;
		port = ipv6_addr->sin6_port;
	} else {
		struct sockaddr_in *ipv4_addr = (void*)remoteaddr;
		addr = &ipv4_addr->sin_addr;
		port = ipv4_addr->sin_port;
	}
	inet_ntop(remoteaddr->ss_family, addr, addr_str, INET6_ADDRSTRLEN);
	port = ntohs(port);
	if (remoteaddr->ss_family == AF_INET6)
		sva_sprintf(dest, "[%s]", addr_str);
	else
		sva_sprintf(dest, "%s", addr_str);
	if (add_port) sva_sprintfcat(dest, ":%u", port);
	return dest;
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
		.ai_flags = (mode == 1 ? AI_PASSIVE : AI_PASSIVE),
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
	uint64_t pid;
	SVA_t name;
} Platform_t;

typedef struct {
	double timestamp;
	uint64_t uid;
	uint64_t pid;
} Loginhist_t;

typedef struct {
	uint64_t uid;
	uint64_t fd_id;
	SVA_t ipaddr;
	SVA_t ua;
	bool is_web_session;    /* 如果为true，则仅使用ip地址和ua判断 */
	bool is_logined;
} Session_t;

typedef struct Runtimedata_t {
	DA_t messages;
	DA_t users;
	DA_t platforms;
	DA_t login_hist;
	DA_t sessions;
	DA_t fds;

	SVA_t logs;
	SVA_t input;
	SVA_t share_buffer;

	enum {DM_NORM, DM_SELECT, DM_FOCUS} dispaly_mode;
	void (*msg_handle_callback)(struct Runtimedata_t*);

	Session_t *main_session;
	const char *hint_text;
	size_t msg_select;
	str_window_t win;

	int fd;
	bool flag_refresh_msgs;
	bool flag_exited;
	bool flag_esc_confirm;
} Runtimedata_t;

User_t *user_get_by_uid(Runtimedata_t *rt, size_t uid)
{
	if (!rt) return NULL;
	User_t *user = NULL;
	for (size_t i = 0; i < rt->users.len; i++) {
		user = da_get(&rt->users, i);
		if (!user) continue;
		if (user->uid == uid) break;
		user = NULL;
	}
	return user;
}

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
		  .type = UT_BAN,
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

Session_t *session_get_by_fd_id(Runtimedata_t *rt, size_t fd_id)
{
	if (!rt || fd_id == (size_t)-1) return NULL;
	Session_t *session = NULL;
	for (size_t i = 0; i < rt->sessions.len; i++) {
		session = da_get(&rt->sessions, i);
		if (!session) continue;
		if (session->fd_id == fd_id) break;
		session = NULL;
	}
	return session;
}

/* 比对逻辑 fd_id || (ipaddr&&ua) */
Session_t *session_get(Runtimedata_t *rt, SV_t ipaddr, SV_t ua)
{
	if (!rt) return NULL;
	Session_t *session = NULL;
	for (size_t i = 0; i < rt->sessions.len; i++) {
		session = da_get(&rt->sessions, i);
		if (!session) continue;
		if (sv_cmp(sv_from_sva(&session->ipaddr), ipaddr) &&
		    sv_cmp(sv_from_sva(&session->ua), ua)) break;
		session = NULL;
	}
	return session;
}

Session_t *session_attach(Runtimedata_t *rt, size_t fd_id, SV_t ipaddr, SV_t ua)
{
	if (!rt || fd_id == (size_t)-1) return NULL;
	Session_t *session = session_get(rt, ipaddr, ua);
	if (!session) session = session_get_by_fd_id(rt, fd_id);
	if (session) return session;

	size_t len = rt->sessions.len;
	da_append(&rt->sessions, &(Session_t){
		  .uid = -1,
		  .fd_id = fd_id,
		  });
	session = da_get(&rt->sessions, len);
	if (!session) return NULL;
	sva_from_sv(&session->ipaddr, ipaddr);
	sva_from_sv(&session->ua, ua);
	return session;
}

Session_t *session_disattach(Runtimedata_t *rt, size_t fd_id)
{
	if (!rt || fd_id == (size_t)-1) return NULL;
	Session_t *session = session_get_by_fd_id(rt, fd_id);
	if (!session) return NULL;
	struct pollfd* fds = da_get(&rt->fds, fd_id);
	if (!fds) return NULL;
	int fd = fds->fd;
	if (fd >= 0 && session->fd_id == fd_id) {
		close(fd);
		fds->fd = -1;
	}
	session->fd_id = -1;
	return session;
}

void session_free(void *ptr)
{
	if (!ptr) return;
	Session_t *session = ptr;
	sva_free(&session->ipaddr);
	sva_free(&session->ua);
}

Session_t *session_login(Runtimedata_t *rt, Session_t *session, SV_t username, SV_t passwd)
{
	if (!rt || !session) return NULL;
	size_t session_id = (session-(Session_t*)rt->sessions.ptr);
	int code = 0;
	do {
		if (session->is_logined) break;
		code++;
		session->is_logined = false;
		session->uid = -1;
		User_t *u = user_get(rt, username);
		if (!u) break;
		code++;
		if (!sv_cmp(sv_from_sva(&u->passwd), passwd)) break;
		code++;
		if (u->type == UT_BAN) break;
		session->is_logined = true;
		session->uid = u->uid;
		sva_sprintfcat(&rt->logs, "[会话%zu]登录到:\n'%.*s'\n",
			       session_id, (int)username.len, username.p);
		return session;
	}while (0);
	sva_sprintfcat(&rt->logs, "[会话%zu]登录失败(%d)\n", session_id, code);
	return NULL;
}

void session_logout(Runtimedata_t *rt, Session_t *session)
{
	if (!session) return;
	const size_t session_id = (session-(Session_t*)rt->sessions.ptr);
	if (session->is_logined)
		sva_sprintfcat(&rt->logs, "[会话%zu]登出\n", session_id);
	session->uid = -1;
	session->is_logined = false;
	return;
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

Messages_t *message_create(Runtimedata_t *rt, Session_t *session, SV_t content)
{
	if (!rt || !session || !content.p || !content.len) return NULL;
	if (!session->is_logined) return NULL;
	User_t *user = user_get_by_uid(rt, session->uid);
	if (!user) return NULL;
	if (user->type == UT_BAN || user->type == UT_VISIT) {
		sva_sprintfcat(&rt->logs, "uid%zu无权限发消息\n", user->uid);
		return NULL;
	}

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

	// TODO: 实现多端的消息协议同步
	// send_to_all(rt.fd, rt.input.p, rt.input.len,
	// 	    rt.fds.ptr, rt.fds.len);
	return new_msg;
}

void message_free(void *ptr)
{
	if (!ptr) return;
	Messages_t *p = ptr;
	sva_free(&p->content);
}

size_t get_fd_id(Runtimedata_t *rt, int fd)
{
	if (!rt) return -1;
	struct pollfd *p = NULL;
	size_t i = 0;
	for (; i < rt->fds.len; i++) {
		p = da_get(&rt->fds, i);
		if (!p) continue;
		if (p->fd == fd) break;
		p = NULL;
	}
	if (p) return i;
	return -1;
}

void close_fds(void *ptr)
{
	if (!ptr) return;
	struct pollfd *p = ptr;
	if (p->fd >= 0) close(p->fd);
}

// 将 buf 发送给所有连接的客户端
// TODO: REMOVE IT
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
		"\e[0;30;42m", "\e[0;30;43m", "\e[0;30;44m", "\e[0;30;46m", "\e[0;30;47m", "\e[0;30;41m", "\e[0;30;45m",
		"\e[0;37;42m", "\e[0;37;43m", "\e[0;37;44m", "\e[0;37;45m", "\e[0;37;46m", "\e[0;37;41m",
	};
	// [2026-07-18 04:03:00] -> len() == 21
	const int timewidth = 23;   /* 消息栏左侧消息长度 */
	const int userwidth = 9;    /* 显示用户名宽度 */
	const int dotwidth = 3;     /* 分隔空白宽度 */
	const int logwidth = 18;    /* 日志区宽度 */
	const int msgheigh = 3;     /* 消息框高度 */
	const int userheigh = 4;    /* 日志区信息量上方用户信息高度 */
	int col = 0, row = 0;
	col = get_winsize_col();
	row = get_winsize_row();

	if (rt->flag_refresh_msgs) {
		/* 刷新消息区背景 */
		print_in_box((str_window_t){
			     .x = 1,
			     .y = 1,
			     .width = col-logwidth,
			     .heigh = col-msgheigh,
			     .color_code = NULL,
			     .no_auto_fflush = true,
			     }, "");
	}

	str_window_t win_msg = {
		.x = timewidth+userwidth,
		.width = col-timewidth-userwidth-logwidth-dotwidth,
		.heigh = 1,
		.no_auto_fflush = true,
	}, win_user = {
		.x = timewidth,
		.width = userwidth-1,
		.heigh = 1,
		.no_auto_fflush = true,
	}, win_focus = {
		.x = (col-logwidth)/5,
		.y = (row-msgheigh)/5,
		.width = (col-logwidth)*3/5,
		.heigh = (row-msgheigh)*3/5,
		.follow_end = false,
		.color_code = "\e[0;37;44m",
		.no_auto_fflush = true,
	};
	if (rt->messages.len == 0) {
		win_focus.focus = 1;
		print_in_box(win_focus, "局域网聊天室（暂无消息）");
	}
	SVA_t buf = {};
	sva_adjust_minimun(&buf, 124);
	size_t idx = (int)rt->messages.len>=(row-msgheigh)?rt->messages.len-(row-msgheigh):0;
	if (rt->dispaly_mode == DM_SELECT) {
		idx = rt->msg_select > (size_t)row/2 ? rt->msg_select - row/2 : 0;
	}
	size_t j = rt->flag_refresh_msgs ? 0 : row;
	for (; (int)j < row-msgheigh && idx < rt->messages.len; idx++, j++) {
		Messages_t *msg = da_get(&rt->messages, idx);
		if (!msg) continue;
		User_t *user = da_get(&rt->users, msg->owner_uid);
		if (!user) continue;

		win_msg.focus = ((size_t)rt->msg_select == idx+1);
		win_user.y = win_msg.y = j+1;
		win_user.color_code = win_msg.color_code = colors[(user->uid)%countof(colors)];

		printf("\e[0m\e[%zu;0H[%s]", j+1, timestamp2str(msg->timestamp, buf.p, buf.capacity,NULL));

		print_in_box(win_user, user->name.p);
		if (print_in_box(win_msg, msg->content.p)) printf("\e[0m...");
	}
	if (rt->dispaly_mode == DM_FOCUS) {
		/* 居中大消息 */
		win_focus.hide = rt->win.hide;
		const char *p = NULL;
		if (rt->share_buffer.p && rt->share_buffer.len)
			p = rt->share_buffer.p;
		else if (rt->msg_select) {
			Messages_t *msg = da_get(&rt->messages, rt->msg_select-1);
			p = msg ? msg->content.p : "（未找到对应消息）";
		} else p = "（这是什么？）";
		print_in_box(win_focus, p);
	}
	rt->flag_refresh_msgs = false;
	/* 日志 */
	print_in_box((str_window_t){
		     .x = col - logwidth+1,
		     .y = 1,
		     .width = logwidth,
		     .heigh = row-userheigh-msgheigh,
		     .color_code = "\e[0;30;44m",
		     .follow_end = true,
		     .no_auto_fflush = true,
		     }, rt->logs.p);
	/* 用户状态栏 */
	User_t *u = NULL;
	if (rt->main_session->is_logined && (u = user_get_by_uid(rt, rt->main_session->uid))) {
		char buf_strtime[124] = "";
		static const char *status[] = {
			"BAN", "VISIT", "NORM", "ADMIN",
		};
		sva_sprintf(&buf, "|[%.*s]\n|(%s)\n|ST:(%s)",
			    (int)u->name.len, u->name.p,
			    timestamp2str(u->reg_time, buf_strtime, sizeof(buf_strtime), "%Y-%m-%d"),
			    status[u->type%countof(status)]);
	} else {
		sva_sprintf(&buf, "|{尚未登录}");
	}
	print_in_box((str_window_t){
		     .x = col - logwidth+1,
		     .y = row-userheigh-msgheigh+1,
		     .width = logwidth,
		     .heigh = userheigh,
		     .focus = 1,
		     .color_code = "\e[0;30;44m",
		     .follow_end = true,
		     .no_auto_fflush = true,
		     }, buf.p);
	/* 状态栏 */
	sva_sprintf(&buf, "|在线:(%zu/%zu)\n|ESC发消息:%s\n|M:%d, SEL:%zu",
		    rt->fds.len, rt->sessions.len, rt->flag_esc_confirm?"true":"false",
		    rt->dispaly_mode, rt->msg_select);
	print_in_box((str_window_t){
		     .x = col-logwidth+1,
		     .y = row-msgheigh+1,
		     .width = logwidth,
		     .heigh = msgheigh,
		     .color_code = "\e[0;37;44m",
		     .no_auto_fflush = true,
		     }, buf.p);
	sva_free(&buf);
	/* 输入框 */
	static const char *hint_texts[] = {
		[DM_NORM] = "请输入消息：",
		[DM_SELECT] = "（选择模式,`/`以输入命令,`q`退出）",
		[DM_FOCUS] = "（具体信息查看,`q`退出）",
	};
	print_in_box((str_window_t){
		     .x = 1,
		     .y = row - msgheigh+1,
		     .width = col - logwidth,
		     .heigh = msgheigh,
		     .focus = -1,
		     .color_code = "\e[0;30;47m",
		     .follow_end = true,
		     .no_auto_fflush = true,
		     }, rt->input.p&&rt->input.len?rt->input.p:\
		     (rt->hint_text?rt->hint_text:\
		      hint_texts[rt->dispaly_mode%countof(hint_texts)]));
	fflush(stdout);
	running = false;
	return false;
}

/* 服务器响应可读fd处理 */
int handle_fd(Runtimedata_t *rt, size_t i)
{
	struct pollfd* fds = rt->fds.ptr;
	if (!fds || !(fds[i].revents & POLLIN)) return i;
	if (fds[i].fd == rt->fd) {
		struct sockaddr_storage remoteaddr;
		socklen_t addrlen = sizeof(remoteaddr);
		int newfd = accept(rt->fd, (void*)&remoteaddr, &addrlen);
		if (newfd == -1) {
			sva_sprintfcat(&rt->logs, "ERR/accept:%s\n", strerror(errno));
			// perror("accept");
			return i;
		}
		da_append(&rt->fds, &(struct pollfd){.fd = newfd, .events = POLLIN});
		SVA_t addr_str = {};
		get_ip_addr(&addr_str, &remoteaddr, false);
		session_attach(rt, rt->fds.len-1, sv_from_sva(&addr_str), (SV_t){});
		get_ip_addr(&addr_str, &remoteaddr, true);
		sva_sprintfcat(&rt->logs, "新连接: (%d) FROM\n'%s'\n",
			       newfd, addr_str.p);
		sva_free(&addr_str);
		redraw(rt);
		return i;
	}
	// 如果不是 listener，那就是客户端
	SVA_t content = {};
	sva_adjust_minimun(&content, 24);
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
		message_create(rt, session_get_by_fd_id(rt, i), sv_from_sva(&content));
		sva_free(&content);
		redraw(rt);
		return i;
	}
	sva_free(&content);
	// 连接关闭或出错
	if (ret == 0) {
		sva_sprintfcat(&rt->logs, "断连: (%d)\n", fds[i].fd);
	} else {
		sva_sprintfcat(&rt->logs, "ERR/recv:%s\n", strerror(errno));
		// perror("recv");
	}
	// close(fds[i].fd);
	session_disattach(rt, i);
	da_pop(&rt->fds, i, NULL);
	i--;
	redraw(rt);
	return i;
}

/* 服务区住循环 */
void *server(void *data)
{
	if (!data) return NULL;
	Runtimedata_t *rt = data;

	da_create(&rt->fds, 10);
	if (!rt->fds.cap) {
		rt->flag_exited = 1;
		return NULL;
	}

	sva_sprintfcat(&rt->logs, "等待连接中...\n");
	while (!rt->flag_exited) {
		if (poll(rt->fds.ptr, rt->fds.len, 1e3) == -1) {
			sva_sprintfcat(&rt->logs, "ERR/poll:%s\n", strerror(errno));
			// perror("poll");
			rt->flag_exited = 1;
			return NULL;
		}
		/* 遍历处理所有fd */
		for (size_t i = 0; i < rt->fds.len; i++)
			i = handle_fd(rt, i);
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

/* 主系统注册 */
void handle_register(Runtimedata_t *rt)
{
	if (!rt) return;
	if (rt->main_session && rt->main_session->is_logined) {
		rt->hint_text = "（不能在登录状态注册账号）";
		rt->msg_handle_callback = NULL;
		return;
	}

	static enum { ST_NAME, ST_PASSWD } state;
	if (rt->msg_handle_callback != handle_register) {
		rt->hint_text = "（请输入要注册的账号名）";
		rt->msg_handle_callback = handle_register;
		state = ST_NAME;
		return;
	}
	static User_t *u = NULL;
	if (state == ST_NAME) {
		if (!user_get(rt, sv_from_sva(&rt->input))) {
			u = user_create(rt, sv_from_sva(&rt->input));
			state = ST_PASSWD;
			rt->hint_text = "（请输入要注册的账号密码）";
			return;
		}
		sva_sprintfcat(&rt->logs, "注册失败，用户已存在\n");
	}
	// if (state == ST_PASSWD)
	if (u) {
		u->type = UT_NORM;
		sva_from_sv(&u->passwd, sv_from_sva(&rt->input));
		rt->main_session = session_get(rt, SELF_SESSION_NAME, (SV_t){});
		session_login(rt, rt->main_session, sv_from_sva(&u->name),
			      sv_from_sva(&rt->input));
	}
	rt->hint_text = NULL;
	rt->msg_handle_callback = NULL;
	u = NULL;
	state = ST_PASSWD;
}

/* 主系统登录 */
void handle_login(Runtimedata_t *rt)
{
	if (!rt) return;
	if (!rt->main_session) return;
	if (rt->main_session->is_logined) return;

	static enum { ST_NAME, ST_PASSWD } state;
	if (rt->msg_handle_callback != handle_login) {
		rt->hint_text = "（请输入要登录的账号名）";
		rt->msg_handle_callback = handle_login;
		state = ST_NAME;
		return;
	}
	static User_t *u = NULL;
	if (state == ST_NAME) {
		u = user_get(rt, sv_from_sva(&rt->input));
		state = ST_PASSWD;
		rt->hint_text = "（请输入要登录的账号密码）";
		return;
	}
	// if (state == ST_PASSWD)
	if (u) {
		session_login(rt, rt->main_session, sv_from_sva(&u->name),
			      sv_from_sva(&rt->input));
	}
	rt->hint_text = NULL;
	rt->msg_handle_callback = NULL;
	state = ST_NAME;
	u = NULL;
}

/* 返回值大于0应continue,小于0应break,等于0发消息 */
int input_command(Runtimedata_t *rt)
{
	if (!rt || !rt->input.p || rt->input.len < 2 || rt->input.p[0] != '/')
		return -1;
	int ret = 0;

#define CMD_LIST                                  \
	CMD(exit,     "退出")                     \
	CMD(quit,     "退出")                     \
	CMD(q,        "退出")                     \
	CMD(leave,    "退出")                     \
	CMD(enter,    "切换是否使用回车确认消息") \
	CMD(select,   "切换到选择模式")           \
	CMD(show,     "(选择模式下)显示选中消息") \
	CMD(help,     "显示此帮助")               \
	CMD(clear,    "清屏")                     \
	CMD(ls,       "列出用户列表")             \
	CMD(reg,      "注册")                     \
	CMD(login,    "登录")                     \
	CMD(logout,   "登出")
#define CMD(cmd, desc) #cmd,
	static const char *cmd_str[] = {CMD_LIST};
#undef CMD
#define CMD(cmd, desc) "/"#cmd"\t - "desc"\n"
	static const char *cmd_help =
		"#########################\n"
		"#       命令列表        #\n"
		"#########################\n\n"
		CMD_LIST;
#undef CMD
#define CMD(cmd, desc) CMD_##cmd,
	enum {CMD_UNSET = -1, CMD_LIST} cmd_choice = CMD_UNSET;
#undef CMD

	for (size_t i = 0; i < countof(cmd_str); i++) {
		if (strcmp(rt->input.p+1, cmd_str[i]) != 0) continue;
		cmd_choice = i;
		break;
	}

	switch (cmd_choice) {
	case CMD_exit: case CMD_quit: case CMD_q: case CMD_leave:
		ret = -1;
		break;
	case CMD_enter:
		rt->flag_esc_confirm = !rt->flag_esc_confirm;
		break;
	case CMD_clear:
		rt->flag_refresh_msgs = true;
		printf("\e[2J\e[H");
		break;
	case CMD_ls:
		sva_sprintf(&rt->share_buffer, "#### 用户列表 ####\n\n共计%zu位用户\n\n", rt->users.len);
		User_t *users = rt->users.ptr;
		char buf[124] = "";
		static const char *status[] = { "BAN", "VISIT", "NORM", "ADMIN", };
		for (size_t i = 0; users && i < rt->users.len; i++) {
			sva_sprintfcat(&rt->share_buffer, "- [%.*s] (%s)\n  -> UID:%-3zu   TYPE:%s\n  -> \"%s\"\n\n",
				       (int)users[i].name.len,
				       users[i].name.p,
				       timestamp2str(users[i].reg_time, buf, sizeof(buf), NULL),
				       users[i].uid, status[users[i].type%countof(status)],
				       users[i].note.p&&users[i].note.len ? users[i].note.p : "「暂无备注」"
				       );
		}
		rt->dispaly_mode = DM_FOCUS;
		break;
	case CMD_reg:
		handle_register(rt);
		break;
	case CMD_login:
		handle_login(rt);
		break;
	case CMD_logout:
		session_logout(rt, rt->main_session);
		break;
	case CMD_UNSET:
		rt->hint_text = "（非法的命令）请输入消息：";
		break;
	case CMD_help:
		sva_from_cstr(&rt->share_buffer, cmd_help);
		rt->dispaly_mode = DM_FOCUS;
		break;
	case CMD_select:
		bool flag = rt->dispaly_mode == DM_SELECT;
		rt->dispaly_mode = flag ? DM_NORM : DM_SELECT;
		rt->msg_select = flag ? 0 : 1;
		rt->flag_refresh_msgs = true;
		break;
	case CMD_show:
		if (rt->dispaly_mode == DM_SELECT && rt->msg_select > 0) {
			rt->win.hide = 0;
			rt->dispaly_mode = DM_FOCUS;
		}
		rt->flag_refresh_msgs = true;
		break;
	}
	sva_clear(&rt->input);
	return ret;
#undef CMD_LIST
}

/* 输入处理（兼界面处理） */
int input_handle(Runtimedata_t *rt, int ret)
{
	if (!rt) return -1;

	/* 处理流程：
	 * 1. 控制流？
	 *   a. C-pnfb/方向键(进入选择、切换选择、移动位置)
	 *   b. wasd/hjkl 类同方向键（选择模式且无/命令、聚焦模式）
	 *   c. q 退出选择、聚焦模式
	 *   d. \n (选择模式且为/命令)||普通消息 -> 发信息
	 * */
	enum {K_NONE, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_ENTER, K_EXIT, K_TYPE} key_event = K_NONE;

	/* 转义处理WASD */
	if (!rt->input.len && !rt->flag_esc_confirm && ret == '\e' && kbhit() > 0) {
		_getch();
		ret = _getch();
		static const char table[UINT8_MAX] = {
			['A'] = K_UP,
			['B'] = K_DOWN,
			['D'] = K_LEFT,
			['C'] = K_RIGHT,
		};
		if (ret > 0) key_event = table[ret%countof(table)];
	}

	if (rt->dispaly_mode == DM_NORM) {
		if (key_event) {
			rt->dispaly_mode = DM_SELECT;
			rt->flag_refresh_msgs = true;
		}
	} else if (rt->input.len == 0 && ret != '/') {
		static const char table[UINT8_MAX] = {
			['W'] = K_UP,    ['w'] = K_UP,    ['K'] = K_UP,    ['k'] = K_UP,    [0x10] = K_UP,
			['S'] = K_DOWN,  ['s'] = K_DOWN,  ['J'] = K_DOWN,  ['j'] = K_DOWN,  [0x0e] = K_DOWN,
			['A'] = K_LEFT,  ['a'] = K_LEFT,  ['H'] = K_LEFT,  ['h'] = K_LEFT,  [0x02] = K_LEFT,
			['D'] = K_RIGHT, ['d'] = K_RIGHT, ['L'] = K_RIGHT, ['l'] = K_RIGHT, [0x06] = K_RIGHT,
			['Q'] = K_EXIT,  ['q'] = K_EXIT,
			['\n'] = K_ENTER,
		};
		if (table[ret%sizeof(table)]) key_event = table[ret%sizeof(table)];
	} else {
		key_event = K_TYPE;
	}

	if (rt->dispaly_mode == DM_FOCUS) {
		switch (key_event) {
		case K_UP:   case K_LEFT:  rt->win.hide--; break;
		case K_DOWN: case K_RIGHT: rt->win.hide++; break;
		case K_EXIT:
			if (rt->msg_select) rt->dispaly_mode = DM_SELECT;
			else rt->dispaly_mode = DM_NORM;
			rt->flag_refresh_msgs = true;
			sva_clear(&rt->share_buffer);
			printf("\e[2J\e[H");
			break;
		default: break;
		}
		return 0;
	} else if (rt->dispaly_mode == DM_SELECT) {
		switch (key_event) {
		case K_UP: case K_LEFT:
			if (rt->msg_select > 1) rt->msg_select--;
			else rt->msg_select = rt->messages.len;
			rt->flag_refresh_msgs = true;
			return 0;
			break;
		case K_DOWN: case K_RIGHT:
			if (rt->msg_select && rt->msg_select < rt->messages.len)
				rt->msg_select++;
			else rt->msg_select = 1;
			rt->flag_refresh_msgs = true;
			return 0;
			break;
		case K_EXIT:
			rt->dispaly_mode = DM_NORM;
			rt->msg_select = 0;
			rt->flag_refresh_msgs = true;
			return 0;
			break;
		case K_ENTER:
			sva_sprintf(&rt->input, "/show");
			break;
		case K_TYPE:
			break;
		default:
			return 0;
			break;
		}
	}

	if (rt->flag_esc_confirm?ret=='\e':ret == '\n') {
		if (rt->input.p && rt->input.len && rt->input.p[0] == '/') {
			return input_command(rt);
		}
		return 1;
	}
	if (ret == 127) {
		if (!rt->input.p || !rt->input.len) return 0;
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
		return 0;
	}
	if (ret >= 0) {
		sva_sprintfcat(&rt->input, "%c", ret);
		while (kbhit()) sva_sprintfcat(&rt->input, "%c", _getch());
		return 0;
	}
	return 0;
}

int main(int argc, const char *argv[])
{
	if (argc < 1) return 3;

	int port = 9999;
	int fd = -1;
	int mode = 0;
	(void)argv;
	if (argc > 1 && strcmp(argv[1], "-c") == 0) {
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
		.messages.size = sizeof(Messages_t),
		.users.size = sizeof(User_t),
		.platforms.size = sizeof(Platform_t),
		.login_hist.size = sizeof(Loginhist_t),
		.sessions.size = sizeof(Session_t),
		.fds.size = sizeof(struct pollfd),
		.fd = fd
	};
	sva_sprintfcat(&rt.logs, "这里是日志区\n");
	do {
		User_t *u = user_create(&rt, SELF_USER_NAME);
		if (u) u->type = UT_ADMIN;
		da_append(&rt.fds, &(struct pollfd){.fd = rt.fd, .events = POLLIN});
		rt.main_session = session_attach(&rt, get_fd_id(&rt, fd), SELF_SESSION_NAME, (SV_t){});
		u = user_create(&rt, sv_from_lstr("Visitor"));
		if (u) u->type = UT_VISIT;
	} while(0);

	pthread_t pid = 0;
	if (mode == 1) pthread_create(&pid, NULL, server, &rt);
	else client(&rt);

	setlocale(LC_ALL, "");
	printf(/* "\033[?25l" */ "\e[2J");
	while (!rt.flag_exited) {
		if (rt.input.len && rt.input.len >= rt.input.capacity)
			sva_double(&rt.input);
		redraw(&rt);
		int ret = _getch();
		if ((ret = input_handle(&rt, ret)) < 0) break;
		else if (ret == 0) continue;

		if (rt.hint_text) rt.hint_text = NULL;
		if (rt.msg_handle_callback)
			rt.msg_handle_callback(&rt);
		else message_create(&rt, rt.main_session, sv_from_sva(&rt.input));
		sva_free(&rt.input);
	}
	rt.flag_exited = 1;
	printf("\e[%d;0H\n正在退出。。。\n", get_winsize_row());
	if (pid) pthread_join(pid, NULL);

	if (rt.fd >= 0) close(rt.fd);
	sva_free(&rt.input);
	sva_free(&rt.logs);
	sva_free(&rt.share_buffer);
	da_free(&rt.fds, close_fds);
	da_free(&rt.platforms, NULL);
	da_free(&rt.login_hist, NULL);
	da_free(&rt.sessions, session_free);
	da_free(&rt.users, user_free);
	da_free(&rt.messages, message_free);
	return 0;
}

