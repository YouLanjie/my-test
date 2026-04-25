#include "include.h"
#ifdef __linux

// 定义宏
#define LineH "─"
#define LineV "│"
#define LineLU "┌"
#define LineLD "└"
#define LineRU "┐"
#define LineRD "┘"
#define LineLC "├"
#define LineRC "┤"
#define LineCC "┼"
#define LineUC "┬"
#define LineDC "┴"
#define LineCLU "╭"
#define LineCLD "╰"
#define LineCRU "╮"
#define LineCRD "╯"
#define ArrowUp "↑"
#define ArrowDn "↓"
#define ArrowLf "←"
#define ArrowRi "→"

/* 开启颜色或效果 */
#define color_on(n) attron(COLOR_PAIR(n))
/* 关闭n1并开启n2颜色或效果 */
#define color_onf(n1, n2) (attroff(COLOR_PAIR(n1)), attron(COLOR_PAIR(n2)))
/* 关闭颜色或效果 */
#define color_off(n) attroff(COLOR_PAIR(n))

typedef struct {
	int *var;	/* 调整的变量值 */
	int  type;	/* 类型：1数值，2开关 */
	int  foot;	/* 设置的步长 */
	int  max;	/* 设置的最大值 */
	int  min;	/* 设置的最小值 */
} Value;

struct Node;
typedef struct Node Node;
struct Node {
	char  *text;		/* 条例内容 */
	char  *describe;	/* 描述/帮助信息 */
	void (*function)();	/* 调用的函数 */
	int    id;		/* 编号 */
	int    reload;		/* is_reload */
	Value  var;		/* 设置 */
	Node  *next;		/* 下一条例（链表） */
};				/* 条例结构体 */

typedef struct {
	char *title;	/* 标题 */
	Node *text;	/* 条例链表头 */
	Node *focus;	/* 选中的条例 */
	int   type;	/* 菜单类型: 0.默认 1.仅显示主界面 2.显示帮助 3.显示设置 4.仅显示帮助，无输入处理 */
} Menu;			/* 菜单类/结构体 */

enum TYPE {t_normal = 0, t_main_only, t_help, t_setting, t_help_only};

/* 移动焦点选项 */
static int set_focus(Menu *menu, int id)
{
	if (menu->text == NULL)
		return -1;
	if (menu->focus == NULL)
		menu->focus = menu->text;

	if (id <= 0) {
		while (menu->focus->next != NULL)
			menu->focus = menu->focus->next;
		return 0;
	}

	if (menu->focus->id > id)
		menu->focus = menu->text;
	while (menu->focus->next != NULL && menu->focus->id < id)
		menu->focus = menu->focus->next;
	return 0;
}

/* ==================================================
 * Desplay
 * ================================================== */

/* 填充颜色 */
static int dsp_fill(int y1, int x1, int y2, int x2)
{
	for (int i = y1; i < y2; i++) {
		for (int i2 = x1; i2 < x2; i2++) {
			mvaddch(i, i2, ' ');
		}
	}
	return 0;
}

/* 绘制屏幕背景 */
static int dsp_background(Menu * menu)
{
	if (menu == NULL)
		return -1;
	int cond = menu->type == t_normal || menu->type == t_setting;
	/* 铺上底色 */
	color_on(C_WHITE_BLUE);
	dsp_fill(0, 0, LINES, COLS);
	/* 绘制边框 */
	box(stdscr, 0, 0);
	for (int i = 1; i < COLS; i++) {
		mvaddstr(4, i, LineH);		/* 第二横线 */
		mvaddstr(LINES - 1, i, LineH);	/* 第三横线 */
		if (cond) mvaddstr(6, i, LineH);/* 第四横线 */
	}
	if (cond) {
		mvaddstr(5, COLS / 4 - 3, "选项");
		mvaddstr(5, COLS / 4 * 3 - 3, "描述");
	}
	for (int i = 1; i < LINES; i++) {
		mvaddstr(i, 0, LineV);		/* 左垂线 */
		mvaddstr(i, COLS - 1, LineV);	/* 右垂线 */
		if (i >= 5 && cond)
			mvaddstr(i, COLS / 2 - 1, LineV);	/* 中垂线 */
	}
	mvaddstr(LINES - 1, 0       , LineLD);    /*   左下角   */
	mvaddstr(LINES - 1, COLS - 1, LineRD);    /*   右下角   */
	mvaddstr(0        , COLS - 1, LineRU);    /*   右上角   */
	mvaddstr(4        , 0       , LineLC);    /* 左第一连接 */
	mvaddstr(4        , COLS - 1, LineRC);    /* 右第一连接 */
	if (cond) {
		mvaddstr(4        , COLS / 2 - 1, LineUC);    /* 中第一连接!1&2 */
		mvaddstr(LINES - 1, COLS / 2 - 1, LineDC);    /* 中第二连接!1&2 */
		mvaddstr(6        , 0           , LineLC);    /* 左第二连接!1&2 */
		mvaddstr(6        , COLS - 1    , LineRC);    /* 右第二连接!1&2 */
		mvaddstr(6        , COLS / 2 - 1, LineCC);    /*  中线交界!1&2  */
	}
	if (menu->title != NULL) {
		attron(A_BOLD);
		mvaddstr(2, COLS / 2 - (int)strlen(menu->title) / 2, menu->title);
		attroff(A_BOLD);
	}
	color_off(C_WHITE_BLUE);
	return 0;
}

/* 显示选项 */
static int dsp_text(Menu *menu, int focus, int hide_len, int len)
{
	if (menu == NULL || menu->text == NULL)
		return -1;
	int limt = len <= LINES - 10 ? len : LINES - 10;
	for (int i = 1; menu->focus != NULL && i - hide_len <= limt; i++) {
		if (i <= hide_len)
			continue;
		set_focus(menu, i);
		if (i != focus) {
			color_onf(C_WHITE_YELLOW, C_WHITE_BLUE);
			attroff(A_BOLD);
		} else {
			color_onf(C_WHITE_BLUE, C_WHITE_YELLOW);
			attron(A_BOLD);
		}
		move(i + 7 - hide_len, 3);
		for (int i = 0; i <= COLS / 2 - 6; i++)
			printw(" ");
		if (menu->focus->text != NULL)
			mvaddstr(i + 7 - hide_len, 3, menu->focus->text);
		if (menu->type != t_setting || menu->focus->var.var == NULL)
			continue;
		if (menu->focus->var.type == 1) {
			mvaddch(i + 7 - hide_len, COLS / 2 - 11, '[');
			attron(A_UNDERLINE);
			printw("%7d", *(menu->focus->var.var));
			attroff(A_UNDERLINE);
			printw("]");
		} else if (menu->focus->var.type == 2) {
			move(i + 7 - hide_len, COLS / 2 - 5);
			printw("(%c)", *(menu->focus->var.var) == 0 ? ' ' : '*');
		}
	}
	color_off(C_BLUE_WHITE);
	color_off(C_WHITE_YELLOW);
	return 0;
}

static int dsp_range_print(char *ch, int x_start, int y_start, int width, int heigh, int hide, int focus)
{
	int count = 0,
	    line_num = 0;

	color_on(C_BLACK_WHITE);
	dsp_fill(y_start, x_start, y_start + heigh + 2, x_start + width + 1);
	move(y_start + line_num - (hide > line_num ? 0 : hide) + 1, x_start + 1);
	while (ch && *ch != '\0' && width >= 8) {
		char buf[20] = {*ch, '\0', '\0', '\0'};
		if (*ch & 0x80) {
			buf[1] = ch[1];
			buf[2] = ch[2];
			count++;
			if (strcmp("…", buf) == 0) count--;
		} else if (*ch == '\t') {
			count += 7;
			strcpy(buf, "        ");
		}
		count++;

		int cond_out = (count > width - 1 && ch && *ch != '\0');
		int cond_print = (line_num - hide >= 0 && line_num - hide < heigh);

		if (cond_out || *ch == '\n' || *ch == '\r') {
			/* 行数增加 */
			line_num++;
			/* 字符清零 */
			count = 0;
			/* 移动光标 */
			if (cond_print) move(y_start + line_num - (hide > line_num ? 0 : hide) + 1, x_start + 1);
			/* 字符指针下移 */
			if (*ch == '\n' || *ch == '\r') {
				ch++;
			}
			continue;
		}

		if (line_num == focus - 1)
			color_onf(C_BLACK_WHITE, C_WHITE_BLACK);
		if (cond_print)
			printw("%s", buf);
		if (line_num == focus - 1)
			color_onf(C_WHITE_BLACK, C_BLACK_WHITE);
		/* 字符指针下移 */
		ch += *ch & 0x80 ? 3 : 1;
	}
	color_off(C_BLACK_WHITE);
	return line_num + 1;
}

/* 显示描述 */
static void dsp_describe(Node *node, int focus, int hide_len, int *len)
{
	if (node == NULL)    /* 若数据为空 */
		return;
	if (node->describe == NULL)    /* 若数据为空 */
		return;

	*len = dsp_range_print(node->describe, COLS / 2 + 1, 7, COLS / 2 - 3, LINES - 10, hide_len, focus);
	return;
}

static void dsp_help(Menu * data, int focus, int hide_len, int *len)
{
	char *tmp = "";
	char *ch_1 = NULL,
	     *ch_2 = NULL,
	     *ch_3 = tmp;
	int i = 1;

	if (data == NULL || data->text == NULL)
		return;

	do {    /* 遍历多项 */
		set_focus(data, i);
		if (data->focus == NULL)
			break;
		ch_1 = data->focus->text;
		ch_2 = malloc(sizeof(char) * (strlen(ch_1) + strlen(ch_3) + 2));
		if (ch_3 != tmp) {
			sprintf(ch_2, "%s\n%s", ch_3, ch_1);
			free(ch_3);
		} else {
			sprintf(ch_2, "%s", ch_1);
		}
		ch_3 = ch_2;
		i++;
	} while (data->focus->next != NULL);

	*len = dsp_range_print(ch_3, 2, 5, COLS - 5, LINES - 8, hide_len, focus);
	if (ch_3 != tmp)
		free(ch_3);
	return;
}


/* ==================================================
 * Show
 * ================================================== */

/* 处理输入(菜单选项上下移动) */
static int Input(int input, int *focus, int *hide_len, int allChose, int y_start)
{
	input = (input > 'a' && input < 'z' && input != 'g') ? input - 32 : input;
	switch (input) {
	case 0x1B:
		if (kbhit() != 0) {
			getchar();
			input = getchar();
			switch (input) {
			case 'A':
			case 'D':
				goto LABEL_LAST;
				break;
			case 'B':
			case 'C':
				goto LABEL_NEXT;
				break;
			}
		} else {
			erase();
			/*clear();*/
			return '0';
		}
		break;
	case 'D':
	case 'L':
	case 'S':
	case 'J':
	LABEL_NEXT:
		if (*focus < allChose) {
			(*focus)++;
		} else {
			*focus = 1;
			*hide_len = 0;
			break;
		}
		while (*focus - *hide_len > LINES - y_start)
			(*hide_len)++;
		break;
	case 'g':
		*focus = 1;
		while (*focus - *hide_len < 1)
			(*hide_len)--;
		break;
	case 'G':
		while (*focus < allChose)
			(*focus)++;
		while (*focus - *hide_len > LINES - 10)
			(*hide_len)++;
		break;
	case 'A':
	case 'H':
	case 'W':
	case 'K':
	LABEL_LAST:
		if (*focus > 1) {
			(*focus)--;
		} else {
			while (*focus < allChose)
				(*focus)++;
			while (*focus - *hide_len > LINES - 10)
				(*hide_len)++;
		}
		while (*focus - *hide_len < 1)
			(*hide_len)--;
		break;
	case 'Q':
	case '0':
		/*clear();*/
		erase();
		return '0';
		break;
	case ' ':
	case '\n':
	case '\r':
		return '\n';
		break;
	case '=':
		return '+';
		break;
	case '\t':
		return '\t';
		break;
	default:
		return input;
		break;
	}
	/*clear();*/
	erase();
	return 0;
}

/* 显示 */
extern int cmenu_show(cmenu menu)
{
	Menu *p = menu;
	int input = 1,			/* 保存输入 */
	    focus_id = 1,		/* 保存焦点选项的数字 */
	    focus_id2 = 0,		/* 描述的焦点 */
	    hide_len = 0,		/* 显示的内容与读取的偏差值（相当于屏幕上方隐藏的条目），用作实现界面滑动 */
	    hide_len2 = 0,		/* 偏差值的备份 */
	    side = 0,			/* 焦点位置(Left side or right side) */
	    y_start = 0,		/* 显示区域高度（偏差值） */
	    line_node = 0,		/* 保存所有的选择总数（打印进度用） */
	    line_desc = 0;		/* 保存所有的描述字符总行数（打印进度用） */

	/* 倘若焦点指针不为空，
	 * 则获得焦点指针指向的文本数字编号
	 */
	if (p->focus != NULL)
		focus_id = p->focus->id;

	/* 移动焦点指针到最后一条文本 */
	set_focus(p, 0);
	line_node = p->focus->id;

	/* 配置选项判断 */
	if (p->type == t_main_only || p->type == t_help_only) {
		dsp_background(p);
		if (p->text == NULL)
			return 0;
		focus_id = 0;
		/* 打印选项 */
		if (p->type == t_main_only) {
			dsp_text(p, focus_id, hide_len, line_node); /* 仅显示正常屏幕框架 */
			dsp_describe(p->focus, focus_id2, hide_len2, &line_desc);    /* 显示焦点选项的描述 */
		} else
			dsp_help(p, focus_id, hide_len, &line_node); /* 仅显示帮助屏幕框架 */
		return 0;
	}

	y_start = p->type == t_help ? 8 : 10;

	clear();
	while (input != 0x30 && input != 0x1B) {
		/* 显示屏幕框架 */
		dsp_background(p);

		/* 移动焦点指针到焦点文本 */
		set_focus(p, focus_id);

		/* 打印选项 */
		if (p->type != t_help && p->type != t_help_only)    /* 非帮助 */
			dsp_describe(p->focus, focus_id2, hide_len2, &line_desc);    /* 显示焦点选项的描述 */
		if (p->type == t_normal || p->type == t_setting)
			dsp_text(p, focus_id, hide_len, line_node);
		else if (p->type == t_help || p->type == t_help_only)    /* 帮助 */
			dsp_help(p, focus_id, hide_len, &line_node);

		/* 移动焦点指针到焦点文本 */
		set_focus(p, focus_id);

		if (p->type != t_help && p->type != t_help_only) {    /* 非帮助 */
			if (side) {    /* 若焦点在描述内容上（side != 0） */
				/* 打印描述的按键提示 */
				attron(A_DIM);
				color_on(C_BLACK_WHITE);
				move(7,         COLS / 4 * 3 - 4);printw("%sw k%s", ArrowUp, ArrowUp);
				move(LINES - 2, COLS / 4 * 3 - 4);printw("%ss j%s", ArrowDn, ArrowDn);
				move(LINES - 2, COLS - 7);printw("%02d/%02d", focus_id2, line_desc);
				color_off(C_BLACK_WHITE);
				attroff(A_DIM);

				attron(A_BOLD);
				color_on(C_WHITE_BLUE);
				move(5, COLS / 2);printw("%sTAB", ArrowLf);
				attroff(A_BOLD);
				color_off(C_WHITE_BLUE);
			} else {    /* 焦点在选项上 */
				/* 打印选项的按键提示 */
				attron(A_BOLD);
				color_on(C_WHITE_BLUE);
				move(7,         COLS / 4 - 4);printw("%sw k%s", ArrowUp, ArrowUp);
				move(LINES - 2, COLS / 4 - 4);printw("%ss j%s", ArrowDn, ArrowDn);
				move(LINES - 2, COLS / 2 - 6);printw("%02d/%02d", focus_id, line_node);
				if (p->focus->describe != NULL) {    /* 如果有描述 */
					move(5, COLS / 2 - 5);printw("TAB%s", ArrowRi);
				}
				attroff(A_BOLD);
				color_off(C_WHITE_BLUE);
			}
		} else {    /* 帮助类型限定 */
			/* 打印描述的按键提示 */
			color_on(C_BLACK_WHITE);
			move(5,         COLS / 2 - 4);printw("%sw k%s", ArrowUp, ArrowUp);
			move(LINES - 2, COLS / 2 - 4);printw("%ss j%s", ArrowDn, ArrowDn);
			move(LINES - 2, COLS - 7);printw("%02d/%02d", focus_id, line_node);
			color_off(C_BLACK_WHITE);
		}
		refresh();
		input = getch();
		/* 输入判断 */
		if (!side)
			input = Input(input, &focus_id, &hide_len, line_node, y_start);
		else {
			input = Input(input, &focus_id2, &hide_len2, line_desc, y_start);
			if (input == '0') {
				input = '\t';
			}
		}
		switch (input) {
		case '\n':	/* 返回字符 */
			if (p->type == t_setting && p->focus->var.type == 2
			    && p->focus->var.var != NULL) {
				if (!*(p->focus->var.var))
					*(p->focus->var.var) = 1;
				else
					*(p->focus->var.var) = 0;
			} else {
				/* clear(); */
				/* char output[10];	/\* 仅用作字符输出 *\/ */
				/* sprintf(output, "%d", focus_text); */
				if (p->focus->function != NULL)
					p->focus->function();
				return focus_id;    /* 输出所选数字 */
			}
			break;
		case '+':
			if (p->type == t_setting && p->focus->var.type == 1
			    && p->focus->var.var != NULL) {
				if ((*p->focus->var.var) + p->focus->var.foot > p->focus->var.max)
					(*p->focus->var.var) = p->focus->var.min;
				else
					(*p->focus->var.var) += p->focus->var.foot;
			}
			break;
		case '-':
			if (p->type == t_setting && p->focus->var.type == 1
			    && p->focus->var.var != NULL) {
				if ((*p->focus->var.var) - p->focus->var.foot < p->focus->var.min)
					(*p->focus->var.var) = p->focus->var.max;
				else
					(*p->focus->var.var) -= p->focus->var.foot;
			}
			break;
		case 0:	/* 什么都不做 */
			break;
		case '\t':	/* 切换介绍与选项 */
			if (p->focus->describe != NULL) {
				if (!side) {    /* 在选项 */
					side = 1;
					focus_id2 = 1;
				} else {    /* 在描述 */
					side = 0;
					focus_id2 = 0;
					hide_len2 = 0;
				}
			}
			break;
		default:	/* 返回输入的字符 */
			if (p->type == 0) {
				if (input >= '0' && input <= '9') {
					input-=48;
				}
				return input;
			}
			break;
		}
	}
	return 0;
}


/* ==================================================
 * Data
 * ================================================== */

/* 初始化变量 */
extern cmenu cmenu_create()
{
	Menu *p;
	p = malloc(sizeof(Menu));
	p->title = NULL;
	p->text  = NULL;
	p->focus = NULL;
	p->type  = 0;
	return p;
}

extern void cmenu_set_title(cmenu menu, char *title)
{
	Menu *p = menu;
	char *t = NULL;
	if (p == NULL || title == NULL || *title == '\0') return;
	t = (char*)malloc(strlen(title) + 1);
	strcpy(t, title);
	p->title = t;
	return;
}

extern void cmenu_set_type(cmenu menu, char *type_str)
{
	char *key[] = {"normal", "main_only", "help", "setting", "help_only"};
	Menu *p = menu;
	if (p == NULL || type_str == NULL || *type_str == '\0') return;
	for (int i = 0; i < 5; i++) {
		if (!strcmp(key[i], type_str))
			p->type = i;
	}
	return;
}

/* Add a node */
static Node *add_node(Menu *menu, int id)
{
	Node *pNew = NULL,
	     *pLast = NULL,
	     *pNext = NULL;
	int id_last = 0;

	pNew = menu->text;
	while (pNew != NULL && pNew->next != NULL && (id == 0 || pNew->id < id)) {
		pLast = pNew;
		pNew = pNew->next;
	}
	if (pNew != NULL && (id == 0 || pNew->id < id)) {
		/* Add */
		id_last = pNew->id;
		pLast = pNew;
		pNext = NULL;
		pNew->next = malloc(sizeof(Node));
		pNew = pNew->next;
	} else if (pNew != NULL) {
		/* Insert */
		id_last = pLast->id;
		pNext = pNew;
		pLast->next = malloc(sizeof(Node));
		pNew = pLast->next;
	} else {
		/* New Head */
		pNew = malloc(sizeof(Node));
		menu->text = pNew;
	}
	if (pLast != NULL)
		pLast->next = pNew;

	pNew->id = id_last + 1;
	pNew->text = NULL;
	pNew->describe = NULL;
	pNew->function = NULL;
	pNew->reload = 0;
	pNew->var.var  = NULL;
	pNew->var.type = 0;
	pNew->var.foot = 1;
	// pNew -> max      = 2147483647;    /* 整型的最大值 */
	// pNew -> min      = -2147483648;    /* 整型的最小值 */
	pNew->var.max = 10000000;
	pNew->var.min = -10000000;
	pNew->next = pNext;

	pLast = pNew;
	pNew = pNew->next;
	while (pNew != NULL) pNew->id++, pNew = pNew->next;
	return pLast;
}

static int type_chose(char *type)
{
	if (type == NULL) return 0;
#define S(t) (strcmp(t, type) == 0)
	if (S("number")) return 1;
	else if (S("toggle") || S("button")) return 2;
#undef S
	return 0;
}

static int set_node(Node *node, void *arg[][2])
{
	if (!node)
		return -1;
	int i = 0,
	    i2 = 0;
	char *key[] = {"text", "describe", "func", "var", "type", "foot", "max", "min"};
	for (i = 0; arg[i][0] != NULL; i++) {
		for (i2 = 0; i2 < 8; i2++) {
			if (!strcmp(key[i2], arg[i][0]))
				break;
		}
		if (i2 >= 8)
			continue;
		char *str = NULL;
		if (i2 <= 1 && arg[i][1] != NULL) {
			str = malloc(sizeof(char) * (strlen(arg[i][1]) + 1));
			strcpy(str, arg[i][1]);
		}
		if (i2 == 0) (node->text = str);
		else if (i2 == 1) (node->describe = str);
		else if (i2 == 2) (node->function = arg[i][1]);
		else if (i2 == 3) (node->var.var = arg[i][1]);
		else if (i2 == 4) (node->var.type = type_chose(arg[i][1]));
		else if (i2 == 5) (node->var.foot = (int)(long)arg[i][1]);
		else if (i2 == 6) (node->var.max = (int)(long)arg[i][1]);
		else if (i2 == 7) (node->var.min = (int)(long)arg[i][1]);
	}
	return 0;
}

/* Key: "text", "describe", "func", "var", "type", "foot", "max", "min" */
extern void cmenu_add_text(cmenu menu, int id, char *text, char *describe, void (*func)(), int *var, char *type, int foot, int max, int min)
{
	foot = foot ? foot: 1;
	max <= min && (max = 10000000, min = -10000000);

	Menu *p = menu;
	if (!p)
		return;
	Node *node = NULL;
	int last_id = p->focus != NULL ? p->focus->id : 0;
	void *arg[][2] = {
		{"text",     text},
		{"describe", describe},
		{"func",     func},
		{"var",      var},
		{"type",     type},
		{"foot",     (void*)(long)foot},
		{"max",      (void*)(long)max},
		{"min",      (void*)(long)min},
		{NULL,       NULL}
	};

	node = add_node(menu, id);
	set_node(node, arg);
	set_focus(p, last_id);
	return;
}

/* Key: "text", "describe", "func", "var", "type", "foot", "max", "min" */
extern void cmenu_set_text(cmenu menu, int id, char *tag, void *value)
{
	Menu *p = menu;
	if (!p)
		return;
	int last_id = p->focus ? p->focus->id : 0;
	void *arg[][2] = {
		{tag, value},
		{NULL,       NULL}
	};

	set_focus(p, id);
	set_node(p->focus, arg);
	set_focus(p, last_id);
}

extern void cmenu_del_text(cmenu menu, int id)
{
	Menu *p = menu;
	Node *pNew = NULL,
	     *pLast = NULL,
	     *pNext = NULL;
	pNew = p->text;
	if (!pNew)
		return;
	while (pNew != NULL && pNew->next != NULL && (id == 0 || pNew->id < id)) {
		pLast = pNew;
		pNew = pNew->next;
	}
	pNext = pNew->next;
	if (pLast != NULL) pLast->next = pNext;
	if (p->text == pNew) p->text = NULL;
	if (p->focus == pNew) p->focus = pNext ? pNext : pLast;
	free(pNew);

	pNew = pNext;
	while (pNew != NULL) pNew->id--, pNew = pNew->next;
	return;
}

#endif

extern void ctools_ncurses_init()
{
#ifdef __linux__
	setlocale(LC_ALL, "zh_CN.UTF8");
	initscr();
	cbreak();		/* 取消行缓冲 */
	noecho();		/* 不回显 */
	curs_set(0);		/* 隐藏光标 */
	if (has_colors() == FALSE) {
		endwin();
		exit(-1);
	}
	start_color();
	/* 初始化颜色对 */
	/*       颜色对           字色（表）   底色（背景） */
	init_pair(C_WHITE_BLUE, COLOR_WHITE, COLOR_BLUE);	/* 蓝底白字 */
	init_pair(C_BLUE_WHITE, COLOR_BLUE, COLOR_WHITE);	/* 白底蓝字 */
	init_pair(C_WHITE_YELLOW, COLOR_WHITE, COLOR_YELLOW);	/* 黄底白字 */
	init_pair(C_BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);	/* 白底黑字 */
	init_pair(C_WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);	/* 黑底白字 */
#endif
	return;
}

