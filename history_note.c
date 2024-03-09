/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：history.c
 *   创 建 者：youlanjie
 *   创建日期：2023年06月17日
 *   描    述：历史笔记
 *
 */


#include "include/tools.h"

#define INPUT_FILE_NAME "./res/history_note_input.txt"
#define OUTPUT_FILE_NAME "./res/history_note_output.txt"

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
	struct ctools ctools = ctools_init();
	struct ctools_CONFIG_NODE *pTemp = NULL;
	printf("\033[0;32m==> ===========================\033[0m\n");
	while (pNew != NULL) {
		pTemp = pNew->data;
		while (pTemp != NULL) {
			if (ctools.config.get_str(pTemp) != NULL) printf("\033[0;1;32m==> \033[1;33m%s \033[0;31m: \033[1;34m%s\n", get_tag(ctools.config.get_name(pTemp)), ctools.config.get_str(pTemp));
			pTemp = ctools.config.get_next_node(pTemp);
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
	struct ctools ctools = ctools_init();
	FILE *fp = fopen(filename, "w");
	struct ctools_CONFIG_NODE *pTemp = NULL;

	if (!fp) return -1;
	while (pNew != NULL) {
		pTemp = pNew->data;
		while (pTemp != NULL) {
			if (ctools.config.get_str(pTemp) != NULL) fprintf(fp, "%s=\"%s\";\n", ctools.config.get_name(pTemp), ctools.config.get_str(pTemp));
			pTemp = ctools.config.get_next_node(pTemp);
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
	struct ctools ctools = ctools_init();
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
		pMark = pNew = pOHead;
		while (pNew != NULL) {    /* 标记要替换的内容 */
			data = pNew->data;
			while (data != NULL && strcmp("time1", ctools.config.get_name(data)) != 0) data = ctools.config.get_next_node(data);
			data2 = pMark->data;
			while (data2 != NULL && strcmp("time1", ctools.config.get_name(data2)) != 0) data2 = ctools.config.get_next_node(data2);
			if (data == NULL) {
				printf("\033[0;32m==> \033[1;34m[ERROR] sort_time:\033[1;31m不存在`time1`标记！\033[0m\n");
				/* printf("\033[0;32m==> \033[1;34m[INFO] sort_time:title:\033[33m%s\033[0m\n"); */
				return NULL;
			}
			if (ctools.config.get_str(data2) != NULL
			    && ctools.config.get_str(data) != NULL
			    && strcmp(ctools.config.get_str(data2), ctools.config.get_str(data)) > 0)
				pMark = pNew;
			pNew = pNew->next;
		}
		/* 建立链接 */
		if (pNHead == NULL) pTmp = pNHead = pMark;    /* 为空 */
		else if (pTmp != NULL){    /* 非空 */
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
	struct ctools ctools = ctools_init();
	FILE *fp;
	struct Note *list = NULL, *pNew = NULL, *pLast = NULL;
	char *base = NULL, *base2 = NULL;

	fp = fopen(filename, "r");
	if (!fp) return NULL;
	for (int i = '1'; i != '\0' && i != EOF; i = fgetc(fp)) {    /* 分段读取 */
		base = malloc(sizeof(char)*2);
		int i2 = '\n';
		for (i = fgetc(fp); i != '\0' && i != EOF && (i != '-' || i2 != '\n'); i2 = i, i = fgetc(fp)) {
			base2 = malloc(sizeof(char)*(strlen(base) + 2));
			/* memset(base2, 0, sizeof(base2)); */
			sprintf(base2, "%s%c", base, i);
			free(base);
			base = base2;
		}
		pNew = malloc(sizeof(struct Note));
		pNew->data = ctools.config.run_char(base);
		pNew->next = NULL;
		if (list == NULL) list = pNew;
		if (pLast != NULL) pLast->next = pNew;
		pLast = pNew;
	}
	pLast = pNew = NULL;
	list = sort_time(list);
	free(base);
	fclose(fp);
	return list;
}

#define Swich(ch) { node = list->data; while (node != NULL && strcmp(ctools.config.get_name(node), ch) != 0) {node = ctools.config.get_next_node(node);}}
/*
 * 在NCURSES显示笔记（子内容）
 */
static int show_subnote(struct Note *list, int number)
{
	struct ctools ctools = ctools_init();
	struct ctools_CONFIG_NODE *node = NULL;
	cmenu menu = cmenu_create();

	for (int i = 0; i < number && list != NULL; ++i) list = list->next;
	if (list == NULL) return -1;

	Swich("title");
	cmenu_set_title(menu, ctools.config.get_str(node));

	for (int i = 1; i < 10; ++i) {
		Swich(tag_table[0][i]);
		if (node != NULL) {
			cmenu_add_text(menu, 0, (char*)tag_table[1][i], ctools.config.get_str(node), NULL, NULL, NULL, 0, 0, 0);
		}
	}
	int input = -1;
	while (input != 'q' && input != 0) {
		input = cmenu_show(menu);
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
	struct ctools ctools = ctools_init();
	struct ctools_CONFIG_NODE *node = NULL;
	cmenu menu = cmenu_create();
	int input = -1;
	struct Note *list2 = list;
	char *str1 = NULL, *str2 = NULL, *str3 = NULL;
	int count = 0;

	cmenu_set_title(menu, "笔记显示");
	while (list != NULL) {
		Swich("title");
		char *title = ctools.config.get_str(node);
		cmenu_add_text(menu, 0, title, "", NULL, NULL, NULL, 0, 0, 0);
		count++;
		Swich("time1");
		if (node != NULL) str1 = ctools.config.get_str(node);
		Swich("describe");
		if (node != NULL) str2 = ctools.config.get_str(node);
		if (str1 == NULL) return -2;
		if (str2 == NULL) str2 = " ";
		str3 = malloc(sizeof(char)*(strlen(str1) + strlen(str2) + 2));
		sprintf(str3, "%s\n%s", str1, str2);
		cmenu_set_text(menu, count, "describe", str3);
		free(str3);
		str1 = str2 = str3 = NULL;
		list = list->next;
	}
	while (input != 'q' && input != 0) {
		input = cmenu_show(menu);
		if (input == 0) continue;
		show_subnote(list2, input - 1);
	}
	free(menu);
	return 0;
}
#undef Swich

int main()
{
	cmenu menu = cmenu_create();
	struct Note *list = NULL;
	int input = -1;

	list = read_note(INPUT_FILE_NAME);
	if (list == NULL) {
		printf("\033[0;32m==> \033[1;34m[ERROR] main:\033[1;31m错误！！！文件无法打开或者格式错误！\n\033[0;32m==> \033[1;34m[INFO] main:File Name:\033[33m%s\033[0m\n", INPUT_FILE_NAME);
		return -1;
	}

	ctools_ncurses_init();
	def_prog_mode();
	cmenu_set_title(menu, "太酷辣");
	cmenu_add_text(menu, 0, "查看笔记", "在ncurses内部通过套用菜单库实现显示笔记的效果", NULL, NULL, NULL, 0, 0, 0);
	cmenu_add_text(menu, 0, "查看笔记（终端）", "退出Ncurses在正常的终端界面显示历史事件（按照时间顺序排序）", NULL, NULL, NULL, 0, 0, 0);
	cmenu_add_text(menu, 0, "保存文件", "将排序后的文件输出保存", NULL, NULL, NULL, 0, 0, 0);
	cmenu_add_text(menu, 0, "退出程序", "选择此项即可退出程序", NULL, NULL, NULL, 0, 0, 0);
	while (input != 'q' && input != 0 && input != 4) {
		input = cmenu_show(menu);
		switch (input) {
		case 1: {
			show_note(list);
			break;
		}
		case 2: {
			endwin();
			print(list);
			printf("\033[0;32m==> \033[1;33m按下任意按键返回\033[0m\n");
			_getch();
			reset_prog_mode();
			break;
		}
		case 3: {
			endwin();
			save(OUTPUT_FILE_NAME, list);
			printf("%s\n%s\033[0m\n",
			       "\033[0;32m==> \033[1;33m保存成功！",
			       "\033[0;32m==> \033[1;33m按下任意按键返回");
			_getch();
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

