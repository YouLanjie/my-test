/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：history.c
 *   创 建 者：youlanjie
 *   创建日期：2023年06月17日
 *   描    述：历史笔记
 *
 */


#include "ctools.h"

#define INPUT_FILE_NAME "/usr/local/share/history_note/input.txt"
#define OUTPUT_FILE_NAME "/usr/local/share/history_note/output.txt"

extern ctools_config CT_CONF;

/* 不同条目的内容 */
struct Note {
	struct ctools_CONFIG_NODE *data;
	struct Note *next;
};

const static char *tag_table[2][10] = {
	{"title", "background", "obj", "time1", "time2", "local", "who", "describe", "end", "meaning"},
	{"标题    ", "背景    ", "目的    ", "开始时间", "结束时间", "位置    ", "人物    ", "描述    ", "结果    ", "意义    "},
};

/*
 * 换算标签
 */
const static char *get_tag(const char *ch)
{
	if (ch == NULL) return NULL;
	for (int i = 0; i < 10; ++i) {
		if (strcmp(ch, tag_table[0][i]) == 0) {
			return tag_table[1][i];
		}
	}
	return ch;
}

/*
 * 打印
 */
static int print(struct Note *pNew)
{
	ctools_config *p = &CT_CONF;
	struct ctools_CONFIG_NODE *pTemp = NULL;
	printf("\033[0;32m==> ===========================\033[0m\n");
	while (pNew != NULL) {
		pTemp = pNew->data;
		while (pTemp != NULL) {
			if (p->get_str(pTemp) != NULL) printf("\033[0;1;32m==> \033[1;33m%s \033[0;31m: \033[1;34m%s\n", get_tag(p->get_name(pTemp)), p->get_str(pTemp));
			pTemp = p->get_next(pTemp);
		}
		printf("\033[0;32m==> ===========================\033[0m\n");
		pNew = pNew->next;
	}
	return 0;
}


/*
 * 保存
 */
static int save(char *filename, struct Note *pNew)
{
	FILE *fp = fopen(filename, "w");
	ctools_config *p = &CT_CONF;
	struct ctools_CONFIG_NODE *pTemp = NULL;

	if (!fp) return -1;
	while (pNew != NULL) {
		pTemp = pNew->data;
		while (pTemp != NULL) {
			if (p->get_str(pTemp) != NULL) fprintf(fp, "%s=\"%s\";\n", p->get_name(pTemp), p->get_str(pTemp));
			pTemp = p->get_next(pTemp);
		}
		pNew = pNew->next;
		if (pNew != NULL) fprintf(fp, "-\n");
	}
	fclose(fp);
	return 0;
}

/*
 * 事件排序
 */
static struct Note *sort_time(struct Note *pNew)
{
	ctools_config *p = &CT_CONF;
	struct Note
		*pNHead = NULL,    /* 新表头 */
		*pTmp   = NULL,    /* 新表临时指针 */
		*pOHead = pNew,    /* 旧表头 */
		*pMark  = pNew;    /* 旧表临时指针 */
	struct ctools_CONFIG_NODE
		*data  = NULL,    /* 旧表用 */
		*data2 = NULL;    /* 新表用 */

	if (pNew == NULL) return NULL;
	while (pOHead != NULL) {
		pMark = pNew;
		while (pNew != NULL) {    /* 标记要替换的内容 */
			data = pNew->data;
			while (data != NULL && strcmp("time1", p->get_name(data)) != 0) data = p->get_next(data);
			data2 = pMark->data;
			while (data2 != NULL && strcmp("time1", p->get_name(data2)) != 0) data2 = p->get_next(data2);
			if (data == NULL) return NULL;
			if (strcmp(p->get_str(data2), p->get_str(data)) > 0) pMark = pNew;
			pNew = pNew->next;
		}
		/* 建立链接 */
		if (pNHead == NULL) pTmp = pNHead = pMark;    /* 为空 */
		else {    /* 非空 */
			pTmp->next = pMark;
			pTmp = pTmp->next;
		}
		/* 切断链接 */
		pNew = pOHead;
		while (pNew != NULL && pNew != pMark && pNew->next != pMark) pNew = pNew->next;
		if (pMark != NULL) {
			if (pNew != NULL && pNew != pMark) pNew->next = pMark->next;
			if (pNew == pMark) pOHead = pOHead->next;
			pMark->next = NULL;
		}
	}
	return pNHead;
}

/*
 * 读取笔记
 */
struct Note *read_note(char *filename)
{
	const ctools_config *p = &CT_CONF;
	struct Note *list = NULL, *pNew = NULL, *pLast = NULL;
	char *base = NULL, *last = NULL;

	base = p->read(filename);
	if (base == NULL) return NULL;
	for (char *i = base; *i != '\0'; ++i) {    /* 分段读取 */
		last = i;
		for (; *i != '\0' && *i != '-'; ++i);
		*i = '\0';
		pNew = malloc(sizeof(struct Note));
		pNew->data = p->runner(last);
		pNew->next = NULL;
		if (list == NULL) list = pNew;
		if (pLast != NULL) pLast->next = pNew;
		pLast = pNew;
	}
	pLast = pNew = NULL;
	list = sort_time(list);
	return list;
}

#define Swich(ch) { node = list->data; while (node != NULL && strcmp(p->get_name(node), ch) != 0) {node = p->get_next(node);}}
/*
 * 在NCURSES显示笔记（子内容）
 */
static int show_subnote(struct Note *list, int number)
{
	const ctools_menu *m = &CT_MENU;
	const ctools_config *p = &CT_CONF;
	struct ctools_menu_t *menu = NULL;
	struct ctools_CONFIG_NODE *node = NULL;

	for (int i = 0; i < number && list != NULL; ++i) list = list->next;
	if (list == NULL) return -1;

	m->data_init(&menu);
	Swich("title");
	m->set_title(menu, p->get_str(node));

	for (int i = 1; i < 10; ++i) {
		Swich(tag_table[0][i]);
		if (node != NULL) {
			m->add_text(menu, (char*)tag_table[1][i]);
			m->add_text_data(menu, "describe", p->get_str(node));
		}
	}
	int input = -1;
	while (input != 'q' && input != 0) {
		input = m->show(menu);
		if (input == 0) continue;
	}
	free(menu);
	return 0;
}

/*
 * 在NCURSES显示笔记
 */
static int show_note(struct Note *list)
{
	const ctools_menu *m = &CT_MENU;
	const ctools_config *p = &CT_CONF;
	struct ctools_menu_t *menu = NULL;
	struct ctools_CONFIG_NODE *node = NULL;
	int input = -1;
	struct Note *list2 = list;
	char *str1 = NULL, *str2 = NULL, *str3 = NULL;

	m->data_init(&menu);
	m->set_title(menu, "笔记显示");
	while (list != NULL) {
		Swich("title");
		m->add_text(menu, p->get_str(node));
		Swich("time1");
		if (node != NULL) str1 = p->get_str(node);
		Swich("describe");
		if (node != NULL) str2 = p->get_str(node);
		if (str1 == NULL) return -2;
		if (str2 == NULL) str2 = " ";
		str3 = malloc(sizeof(char)*(strlen(str1) + strlen(str2) + 2));
		sprintf(str3, "%s\n%s", str1, str2);
		m->add_text_data(menu, "describe", str3);
		free(str3);
		str1 = str2 = str3 = NULL;
		list = list->next;
	}
	while (input != 'q' && input != 0) {
		input = m->show(menu);
		if (input == 0) continue;
		show_subnote(list2, input - 1);
	}
	free(menu);
	return 0;
}
#undef Swich

int main()
{
	const ctools_menu *m = &CT_MENU;
	struct ctools_menu_t *menu = NULL;
	struct Note *list = NULL;
	int input = -1;

	list = read_note(INPUT_FILE_NAME);
	if (list == NULL) {
		printf("\033[0;32m==> \033[1;31m错误！！！文件无法打开\n\033[0;32m==> \033[1;34mFile Name:\033[33m%s\033[0m\n", INPUT_FILE_NAME);
		return -1;
	}

	m->ncurses_init();
	def_prog_mode();
	m->data_init(&menu);
	m->set_title(menu, "太酷辣");
	m->set_text(menu, "查看笔记", "查看笔记（终端）", "保存文件", "退出程序", NULL);
	m->set_text_data(menu, "describe", "%s%s%s", "在ncurses内部通过套用菜单库实现显示笔记的效果", "退出Ncurses在正常的终端界面显示历史事件（按照时间顺序排序）", "将排序后的文件输出保存", "选择此项即可退出程序");
	while (input != 'q' && input != 0 && input != 4) {
		input = m->show(menu);
		switch (input) {
		case 1: {
			show_note(list);
			break;
		}
		case 2: {
			endwin();
			print(list);
			printf("\033[0;32m==> \033[1;33m按下任意按键返回\033[0m\n");
			ctools_getch();
			reset_prog_mode();
			break;
		}
		case 3: {
			endwin();
			save(OUTPUT_FILE_NAME, list);
			printf("%s\n%s\033[0m\n",
			       "\033[0;32m==> \033[1;33m保存成功！",
			       "\033[0;32m==> \033[1;33m按下任意按键返回");
			ctools_getch();
			reset_prog_mode();
			break;
		}
		default:
			break;
		}
	}
	endwin();
	free(list);
	return 0;
}

