/**
 * @file        parse_org.c
 * @author      Chglish
 * @date        2026-08-04
 * @brief       尝试解析org文件
 */

#include "../../include/path.h"
#include <ctype.h>

#define NODE_LIST	\
	X(meta)		\
	X(headline)	\
	X(block)	\
	X(comment)	\
	X(list)		\
	X(table)	\
	X(footnote)	\
	X(text)		\
	X(root)

#define X(name) NODE_##name,
enum node_type { NODE_LIST };
#undef X

struct node {
	struct document *doc;
	struct node *parents;
	struct node *children;
	struct node *next;
	SV_t content;
	void *private_data;
	enum node_type type;
};

static struct node_rule {
	const char *name;
	const bool *non_printable;
	bool (*const match)(SV_t line);
	bool (*const end_condition)(SV_t line);
	struct node *(*const create)(SV_t line);
	struct node *(*const end)(SV_t line);
	struct node *(*const free)(SV_t line);
} NODE_RULES[] = {
#define X(tag) [NODE_##tag] = {\
	.name = #tag,	\
},
NODE_LIST
#undef X
};

#define X(name) bool node_match_##name(SV_t line);
NODE_LIST
#undef X

bool node_match_headline(SV_t line)
{
	if (!line.p || line.len <= 1) return false;
	if (line.p[0] != '*') return false;
	size_t i = 0;
	while (i < line.len && line.p[i] == '*') i++;
	if (i == line.len) return false;
	return isspace(line.p[i]);
}

struct document {
	struct node *root;
};


struct node *node_create(enum node_type type, SV_t content, struct node *parent)
{
	struct node *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (struct node){
		.type = type,
		.content = content,
		.parents = parent,
	};
	return p;
}

struct node *node_add_child(struct node *node, struct node *child_node)
{
	if (!node || !child_node) return NULL;
	struct node *p = node->children;
	while (p && p->next) p = p->next;
	if (!p) {
		node->children = child_node;
	}
	return node;
}

/* 将本元素从同级中弹出 */
struct node *node_pop(struct node *node)
{
	if (!node || !node->parents) return NULL;
	struct node *p = node->parents;
	while (p && p->next != node) p = p->next;
	if (!p) return NULL;
	p->next = node->next;
	return node;
}

void node_free(struct node *node)
{
	if (!node) return;
	if (node->private_data) free(node->private_data);
	struct node *p = node->children, *next;
	while (p) {
		next = p->next;
		node_free(p);
		p = next;
	}
	node_pop(node);
	free(node);
}

struct document *document_build(SV_t content)
{
	struct document *doc = malloc(sizeof(*doc));
	if (!doc) return NULL;
	*doc = (struct document){
		// .root = node_create(NODE_root, content, NULL),
	};
	if (!content.p) return doc;
	SV_t line = {}, left = content;
	int num = 0;
	while ((line = sv_chop_by_delim(&left, '\n')).p && (line.len||left.len)) {
		if (line.len && line.p[line.len-1] == '\r')
			sv_chop_right(&line, 1);
		num++;
		if (node_match_headline(line))
			printf("HEADLINE %d >> '%.*s'\n", num, (int)line.len, line.p);
		// printf("LINE %d >> '%.*s'\n", num, (int)line.len, line.p);
	}
	return doc;
}

void document_free(struct document *doc)
{
	if (!doc) return;
	node_free(doc->root);
	free(doc);
}

int main(int argc, char *argv[])
{
	int ch = 0;
	SVA_t content = {};
	SV_t path = {};
	while ((ch = getopt(argc, argv, "hi:")) != -1) {
		switch (ch) {
		case '?':
		case 'h':
			printf("[INFO] 帮助信息(暂时没有)\n");
			return 0;
			break;
		case 'i':
			path = sv_from_cstr(optarg);
			break;
		}
	}
	path_readfile(path, &content, -1);
	printf("[INFO] 文件长度：%zu (占内存：%zu)\n", content.len, content.capacity);
	// printf(">> '%s'\n", content.p);
	struct document *doc = document_build(sv_from_sva(&content));
	document_free(doc);
	sva_free(&content);
	return EXIT_SUCCESS;
}

