/**
 * @file        parse_org.c
 * @author      Chglish
 * @date        2026-08-04
 * @brief       尝试解析org文件
 */

#include "../../include/path.h"
#include "../../include/dynamic_array.h"
#include <ctype.h>
#include <stdcountof.h>

#define container_of(ptr, typ, member) ((typ *)((char*)(ptr) - offsetof(typ, member)))

enum node_flags {
	NODE_FLG_CHILDABLE = 1<<1,
	NODE_FLG_UNBREAKABLE = 1<<2,
	NODE_FLG_UNPRINTABLE = 1<<3,
};

/* #define NODE_LIST	\
 	X(meta)		\
 	X(headline)	\
 	X(block)	\
 	X(comment)	\
 	X(list)		\
 	X(table)	\
 	X(footnote)	\
 	X(text)		\
 	X(root) */

#define NODE_LIST	\
	X(meta, .flags = NODE_FLG_UNPRINTABLE)		\
	X(headline, .flags = NODE_FLG_CHILDABLE)	\
	X(comment, .flags = NODE_FLG_UNPRINTABLE)	\
	X(root, .flags = NODE_FLG_CHILDABLE)

#define X(name, ...) NODE_##name,
enum node_type { NODE_LIST };
#undef X

struct node_ops;
struct node {
	struct document *doc;
	struct node *parent;
	struct node *child;
	struct node *next;
	const struct node_ops *ops;
	SV_t content;
};

#define NODE_INIT(tag, ...)	\
	(struct node){		\
		.doc = doc,	\
		.ops = NODE_RULES+NODE_##tag,	\
		.parent = parent,	\
		.content = line,	\
		__VA_ARGS__	\
	}
#define NODE_CREATE_DEF(name) struct node *node_create_##name(struct document *doc, struct node *parent, SV_t line)
#define NODE_PRINT_DEF(name) void node_print_##name(struct node *self)
#define X(name, ...) \
	NODE_CREATE_DEF(name); \
	NODE_PRINT_DEF(name);
NODE_LIST
#undef X

static const struct node_ops {
	const char *name;
	struct node *(*const create)(struct document *doc, struct node *parent, SV_t line);
	struct node *(*const end)(struct node *self, SV_t line);
	struct node *(*const free)(struct node *self);
	void (*const print)(struct node *self);
	const uint64_t flags;
	enum node_type type;
} NODE_RULES[] = {
#define X(tag, ...) [NODE_##tag] = {	\
	.type = NODE_##tag,\
	.name = #tag,			\
	.create = node_create_##tag,	\
	.print = node_print_##tag,	\
	__VA_ARGS__\
},
NODE_LIST
#undef X
};

struct node *node_add_child(struct node *node, struct node *child_node)
{
	if (!node || !child_node) return NULL;
	struct node *p = node->child;
	while (p && p->next) p = p->next;
	if (!p) {
		node->child = child_node;
	} else p->next = child_node;
	return node;
}

/* 将本元素从同级中弹出 */
struct node *node_pop(struct node *node)
{
	if (!node || !node->parent) return NULL;
	struct node *p = node->parent;
	while (p && p->next != node) p = p->next;
	if (!p) return NULL;
	p->next = node->next;
	return node;
}

void node_free(struct node *node)
{
	if (!node) return;
	if (node->ops && node->ops->free) node->ops->free(node);
	struct node *p = node->child, *next;
	while (p) {
		next = p->next;
		node_free(p);
		p = next;
	}
	node_pop(node);
	free(node);
}

void node_print(struct node *node)
{
	if (!node) return;
	if (!node->ops) {
		fprintf(stderr, "[ERR] 空操作表\n");
		return;
	}
	if (!node->ops->print) {
		fprintf(stderr, "[ERR] 不支持print函数\n");
		return;
	}
	node->ops->print(node);
}


NODE_CREATE_DEF(root)
{
	if (parent) return NULL;
	struct node *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = NODE_INIT(root);
	return p;
}
NODE_PRINT_DEF(root)
{
	if (!self) {
		printf("ROOT but got nullptr\n");
		return;
	}
	printf("ROOT >> '%.*s'\n",
	       (int)self->content.len, self->content.p);
	return;
}

struct node_t_headline {
	struct node base;
	SV_t todo_name;
	SV_t tags;
	int level;
	bool is_comment;
	// SVA_t idxes;
};
NODE_CREATE_DEF(headline)
{
	if (!line.p || line.len <= 1) return NULL;
	if (line.p[0] != '*') return NULL;
	size_t i = 0;
	while (i < line.len && line.p[i] == '*') i++;
	if (i == line.len) return NULL;
	if (!isspace(line.p[i])) return NULL;

	struct node_t_headline *p = malloc(sizeof(*p));
	if (!p) return NULL;
	sv_chop_left(&line, i);
	sv_trim_left_by_type(&line, isspace);
	int level = i;
	SV_t tags = line;
	enum stat {s_none, s_colon1, s_tags, s_colon2, s_space} st = s_none;
	for (i = 0; i < tags.len; i++) {
		switch (st) {
		case s_none:
			if (tags.p[i] != ':') break;
			st = s_colon1;
			sv_chop_left(&tags, i);
			i = -1;
			break;
		case s_colon1:
			if (isspace(tags.p[i])) st = s_none;
			if (tags.p[i] != ':') st = s_tags;
			break;
		case s_tags:
			if (isspace(tags.p[i]) || tags.p[i] == '-') st = s_none;
			if (tags.p[i] == ':') st = s_colon2;
			break;
		case s_colon2:
			if (isspace(tags.p[i])) st = s_space;
			else if (tags.p[i] == '-') st = s_none;
			else st = s_tags;
			break;
		case s_space:
			if (!isspace(tags.p[i])) st = s_none;
			break;
		}
	}
	if (st == s_space || st == s_colon2) {
		sv_chop_right(&line, tags.len);
		sv_trim_right_by_type(&tags, isspace);
	} else tags = (SV_t){};
	sv_trim_right_by_type(&line, isspace);
	*p = (struct node_t_headline){
		.base = NODE_INIT(headline),
		.level = level,
		.tags = tags,
	};
	return &p->base;
}
NODE_PRINT_DEF(headline)
{
	if (!self) {
		printf("HEADLINE but got nullptr\n");
		return;
	}
	struct node_t_headline *p =
		container_of(self, typeof(struct node_t_headline), base);
	printf("HEADLINE %d >> '%.*s'",
	       p->level,
	       (int)self->content.len, self->content.p);
	if (p->tags.len) {
 		printf("\t\t[\e[32m%.*s\e[0m]", (int)p->tags.len, p->tags.p);
	}
	printf("\n");
	return;
}

NODE_CREATE_DEF(comment)
{
	if (!line.p) return NULL;
	sv_trim_left_by_type(&line, isspace);
	if (!line.len) return NULL;
	if (line.p[0] != '#') return NULL; 
	if (line.len != 1 || !isspace(line.p[1])) return NULL;
	struct node *p = malloc(sizeof(*p));
	if (!p) return NULL;
	sv_chop_left(&line, 2);
	sv_trim_left_by_type(&line, isspace);
	*p = NODE_INIT(comment);
	return p;
}
NODE_PRINT_DEF(comment)
{
	if (!self) {
		printf("COMMENT but got nullptr\n");
		return;
	}
	printf("COMMENT '%.*s'\n",
	       (int)self->content.len, self->content.p);
	return;
}

struct node_t_meta {
	struct node base;
	// 使用content作为key
	SV_t value;
};
NODE_CREATE_DEF(meta)
{
	if (!line.p) return NULL;
	sv_trim_left_by_type(&line, isspace);
	if (line.len <= 3) return NULL;
	if (line.p[0] != '#' || line.p[1] != '+')
		return NULL; 
	size_t i = 2;
	while (i < line.len && line.p[i]!=':' && !isspace(line.p[i]))
		i++;
	if (i == 2 || line.p[i] != ':') return NULL;
	struct node_t_meta *p = malloc(sizeof(*p));
	if (!p) return NULL;
	SV_t value = line;
	sv_chop_left(&line, 2);
	line.len = i-1;
	sv_chop_left(&value, i+1);
	sv_trim_left_by_type(&value, isspace);
	*p = (struct node_t_meta){
		.base = NODE_INIT(meta),
		.value = value,
	};
	return &p->base;
}
NODE_PRINT_DEF(meta)
{
	if (!self) {
		printf("META but got nullptr\n");
		return;
	}
	struct node_t_meta *p =
		container_of(self, typeof(struct node_t_meta), base);
	printf("META >> '%.*s' =  '%.*s'\n",
	       (int)self->content.len, self->content.p,
	       (int)p->value.len, p->value.p);
	return;
}


struct document {
	struct node *root;
	SV_t content;
};

struct document *document_build(SV_t content)
{
	struct document *doc = malloc(sizeof(*doc));
	if (!doc) return NULL;
	*doc = (struct document){
		.root = node_create_root(doc, NULL, sv_from_lstr("TESTING ROOT NODE")),
		.content = content,
	};
	if (!content.p) return doc;
	DA_t stack = { .size = sizeof(struct node*) };
	da_create(&stack, 20);
	da_append(&stack, &doc->root);
	SV_t line = {}, left = content;
	struct node *p = NULL, *parent = NULL;
	void **parent_ptr = NULL;
	int num = 0;
	while ((line = sv_chop_by_delim(&left, '\n')).p && (line.len||left.len)) {
		if (line.len && line.p[line.len-1] == '\r')
			sv_chop_right(&line, 1);
		num++;
		parent_ptr = da_get(&stack, stack.len-1);
		if (!parent_ptr || !(parent = *parent_ptr)) continue;

		for (size_t i = 0; i < countof(NODE_RULES); i++) {
			p = NODE_RULES[i].create(doc, parent, line);
			if (p) break;
		}
		if (!p) continue;
		if (!p->ops) {
			node_free(p);
			continue;
		}
		node_add_child(parent, p);
		if (p->ops && p->ops->flags&NODE_FLG_CHILDABLE) {
			da_append(&stack, &p);
			da_get(&stack, stack.len-1);
		}
		node_print(p);
		// printf("(%zu) META %d >> '%.*s'\n", stack.len, num, (int)line.len, line.p);
		// printf("LINE %d >> '%.*s'\n", num, (int)line.len, line.p);
	}
	da_free(&stack, NULL);
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

