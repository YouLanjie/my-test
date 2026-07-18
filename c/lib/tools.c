#include "../include/tools.h"
#include <fcntl.h>

#ifdef __linux__
#include <termios.h>
#include <poll.h>
#include <stdcountof.h>
#endif

static bool inp_lock = false;


#ifdef __linux__
/*
 * 判断有没有输入
 */
extern int kbhit()
{
	if (inp_lock) return -1;
	inp_lock = true;
	int stat = -1;
	int is_tty = isatty(STDIN_FILENO);
	static struct termios new_attr = {}, old_attr = {};
	do {
		/* 保存现在的终端设置 */
		if (is_tty && tcgetattr(STDIN_FILENO, &old_attr) < 0) break;
		new_attr = old_attr;
		/* 设置无缓冲输入 */
		new_attr.c_lflag &= ~(ICANON | ECHO);
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &new_attr) < 0) break;
		/* 设置无阻塞 */
		int old_fl = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, old_fl | O_NONBLOCK);
		int ch = getchar();
		fcntl(STDIN_FILENO, F_SETFL, old_fl);
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &old_attr) < 0) break;
		/* 将输入内容“塞”回到输入流中 */
		if (ch != EOF) {
			ungetc(ch, stdin);
			stat = ch;
			break;
		}
		stat = 0;
	} while (0);
	inp_lock = false;
	return stat;
}
#endif


/*
 * 不阻塞输入
 */
extern int kbhitGetchar()
{
#ifdef __linux__
	if (inp_lock) return -1;
	inp_lock = true;
	int stat = -1;
	int is_tty = isatty(STDIN_FILENO);
	static struct termios new_attr = {}, old_attr = {};
	do {
		if (is_tty && tcgetattr(STDIN_FILENO, &old_attr) < 0) break;
		new_attr = old_attr;
		new_attr.c_lflag &= ~(ICANON | ECHO);
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &new_attr) < 0) break;
		int old_fl = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, old_fl | O_NONBLOCK);
		int ch = getchar();
		fcntl(STDIN_FILENO, F_SETFL, old_fl);
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &old_attr) < 0) break;
		if (ch != EOF) {
			stat = ch;
			break;
		}
		stat = ch;
	} while (0);
	inp_lock = false;
	return stat;
#endif

#ifdef _WIN32
	if (kbhit() != 0) return getch();
	return 0;
#endif
}


#ifdef __linux__
static struct termios old_attr;
static void (*old_handler[256])(int) = {NULL};
static int old_fl = 0;

static void signal_handler(int sig)
{
	int is_tty = isatty(STDIN_FILENO);
	if (is_tty) tcsetattr(STDIN_FILENO, TCSANOW, &old_attr);
	fcntl(STDIN_FILENO, F_SETFL, old_fl);
	if (old_handler[sig]) old_handler[sig](sig);
	signal(sig, old_handler[sig] ? old_handler[sig] : SIG_DFL);
	kill(getpid(), sig);
	return;
}

#endif

extern int _getch(void)
{
#ifdef __linux__
	if (inp_lock) return -1;
	int is_tty = isatty(STDIN_FILENO);
	int stat = -2;

	inp_lock = true;
	do {
		if (is_tty && tcgetattr(STDIN_FILENO, &old_attr) < 0)
			break;
		stat--;
		static struct termios new_attr;
		new_attr = old_attr;
		new_attr.c_lflag &= ~(ICANON | ECHO);

		old_handler[SIGINT] = signal(SIGINT, signal_handler);
		old_handler[SIGSEGV] = signal(SIGSEGV, signal_handler);
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &new_attr) < 0)
			break;
		stat--;
		old_fl = fcntl(STDIN_FILENO, F_GETFL, 0);
		int ch = getchar();
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &old_attr) < 0)
			break;
		stat = ch;
	} while (0);
	if (stat < -2) {
		signal(SIGINT, old_handler[SIGINT] ? old_handler[SIGINT] : SIG_DFL);
		signal(SIGSEGV, old_handler[SIGSEGV] ? old_handler[SIGSEGV] : SIG_DFL);
	}
	inp_lock = false;
	return stat;
#endif

#ifdef _WIN32
	return getch();
#endif
}

#ifdef __linux__
extern int ct_getch_timeout(int millisecond)
{
	if (inp_lock) return -2;
	inp_lock = true;
	int stat = -3;
	int is_tty = isatty(STDIN_FILENO);
	static struct termios new_attr = {}, old_attr = {};
	do {
		if (is_tty && tcgetattr(STDIN_FILENO, &old_attr) < 0) break;
		stat--;
		new_attr = old_attr;
		new_attr.c_lflag &= ~(ICANON | ECHO);

		old_handler[SIGINT] = signal(SIGINT, signal_handler);
		old_handler[SIGSEGV] = signal(SIGSEGV, signal_handler);
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &new_attr) < 0) break;
		stat--;

		struct pollfd fds[] = { (struct pollfd){.fd = STDIN_FILENO, .events = POLLIN}, };
		if (poll(fds, countof(fds), millisecond) != 0) break;
		stat--;

		int ch = getchar();
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &old_attr) < 0) break;
		stat--;
		stat = ch;
	} while (0);
	if (stat < -3) {
		signal(SIGINT, old_handler[SIGINT] ? old_handler[SIGINT] : SIG_DFL);
		signal(SIGSEGV, old_handler[SIGSEGV] ? old_handler[SIGSEGV] : SIG_DFL);
	}
	inp_lock = false;
	return stat;
}
#endif

extern int ct_getch_cond(int *cond)
{
	if (!cond) return -5;
	if (!*cond) return -6;
#ifdef __linux__
	if (inp_lock) return -2;
	inp_lock = true;
	int stat = -3;
	int is_tty = isatty(STDIN_FILENO);
	static struct termios new_attr = {}, old_attr = {};
	do {
		if (is_tty && tcgetattr(STDIN_FILENO, &old_attr) < 0) break;
		stat--;
		new_attr = old_attr;
		new_attr.c_lflag &= ~(ICANON | ECHO);

		old_handler[SIGINT] = signal(SIGINT, signal_handler);
		old_handler[SIGSEGV] = signal(SIGSEGV, signal_handler);
		old_handler[SIGTERM] = signal(SIGTERM, signal_handler);
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &new_attr) < 0) break;
		stat--;

		struct pollfd fds[] = { (struct pollfd){.fd = STDIN_FILENO, .events = POLLIN}, };
		int ret;
		for (int i = 0; *cond && (ret = poll(fds, countof(fds), 1e3/20)) == 0; i++);
		if (ret == -1) break;
		stat--;

		int ch = getchar();
		if (is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &old_attr) < 0) break;
		stat--;
		stat = ch;
	} while (0);
	if (stat < -3) {
		signal(SIGINT, old_handler[SIGINT] ? old_handler[SIGINT] : SIG_DFL);
		signal(SIGSEGV, old_handler[SIGSEGV] ? old_handler[SIGSEGV] : SIG_DFL);
		signal(SIGTERM, old_handler[SIGTERM] ? old_handler[SIGTERM] : SIG_DFL);
	}
	inp_lock = false;
	return stat;
#endif

#ifdef _WIN32
	while (cond && *cond && !kbhit()) {
		Sleep(10);
	}
	if (kbhit() != 0) return getch();
	return 0;
#endif
}

/* Get the size(x) of the window */
extern int get_winsize_col()
{
#ifdef __linux__
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	return size.ws_col;
#endif

#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#endif
}

/* Get the size(y) of the window */
extern int get_winsize_row()
{
#ifdef __linux__
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	return size.ws_row;
#endif

#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#endif
}

/* 读取文件 */
extern char *_fread(FILE *fp)
{
	long size = 0;
	char *str = NULL;

	if (!fp)
		return NULL;

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	if (sizeof(char) * (size + 1) == 0) return NULL;
	str = malloc(sizeof(char) * (size + 1));
	if (str == NULL) {
		perror("The file is too big");
		return NULL;
	}
	fseek(fp, 0L, SEEK_SET);
	fread(str, 1, size, fp);

	str[size] = '\0';

	return str;
}

#ifdef __linux__
struct timespec timespec_add(struct timespec t, struct timespec t2)
{
	t.tv_nsec += t2.tv_nsec;
	if (t.tv_nsec >= 1e9) {
		t.tv_sec++;
		t.tv_nsec -= 1e9;
	} else if (t.tv_nsec < 0) {
		t.tv_sec--;
		t.tv_nsec += 1e9;
	}
	t.tv_sec += t2.tv_sec;
	return t;
}

struct timespec timespec_from_sec(double t)
{
	return (struct timespec){(int)t, (t-(int)t)*1e9};
}

/* 由于日益严重的延迟因而使用特定的时钟避免usleep的额外等待
 * 返回本次需要等待的时间 
 * */
double sleep_fixed_step(double sec)
{
	static double wait_time = 0;
	static struct timespec t = {0}, t2 = {0};
	if (t.tv_sec == 0 && t.tv_nsec == 0) clock_gettime(CLOCK_MONOTONIC, &t);

	t = timespec_add(t, timespec_from_sec(sec));

	clock_gettime(CLOCK_MONOTONIC, &t2);
	t2 = timespec_add(t, (struct timespec){-t2.tv_sec, -t2.tv_nsec});
	wait_time = t2.tv_sec + t2.tv_nsec/1e9;
	if (wait_time < 0) {
		clock_gettime(CLOCK_MONOTONIC, &t);
		return wait_time;    /* 跳帧 */
	}

	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
	return wait_time;
}
#endif

