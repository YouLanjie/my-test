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

#define NODE_LIST					\
	X(meta, .flags = NODE_FLG_UNPRINTABLE)		\
	X(headline, .flags = NODE_FLG_CHILDABLE)	\
	X(block, .flags = NODE_FLG_CHILDABLE|NODE_FLG_UNBREAKABLE)	\
	X(comment, .flags = NODE_FLG_UNPRINTABLE)	\
	X(text)						\
	X(root, .flags = NODE_FLG_CHILDABLE)

#define X(name, ...) NODE_##name,
enum node_type { NODE_LIST };
#undef X

struct node_ops;
struct node {
	struct document *doc;
	struct node *parent;
	struct node *child;
	struct node *prev;
	struct node *next;
	const struct node_ops *ops;
	/* 需要约定，content应当指向该节点第一行的文本 */
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
#define NODE_ENDCOND_DEF(name) bool node_end_condition_##name(struct node *self, SV_t line)
#define NODE_PRINT_DEF(name) void node_print_##name(struct node *self)
#define X(name, ...) \
	NODE_CREATE_DEF(name); \
	NODE_ENDCOND_DEF(name); \
	NODE_PRINT_DEF(name);
NODE_LIST
#undef X

static const struct node_ops {
	const char *name;
	struct node *(*create)(struct document *doc, struct node *parent, SV_t line);
	struct node *(*free)(struct node *self);
	bool (*end_condition)(struct node *self, SV_t line);
	void (*print)(struct node *self);
	const uint64_t flags;
	enum node_type type;
} NODE_RULES[] = {
#define X(tag, ...) [NODE_##tag] = {	\
	.type = NODE_##tag,		\
	.name = #tag,			\
	.create = node_create_##tag,	\
	.end_condition = node_end_condition_##tag,	\
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
	child_node->next = NULL;
	child_node->prev = NULL;
	if (p) {
		p->next = child_node;
		child_node->prev = p;
	} else node->child = child_node;
	return node;
}

/* 将本元素从同级中弹出 */
struct node *node_pop(struct node *node)
{
	if (!node || !node->parent) return NULL;
	if (node->prev) {
		node->prev->next = node->next;
	} else if (node->parent && node == node->parent->child) {
		node->parent->child = node->next;
	}
	if (node->next) node->next->prev = node->prev;
	node->prev = NULL;
	node->next = NULL;
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
	// node_pop(node);
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

bool node_checkend(struct node *node, SV_t line)
{
	if (!node || !node->ops)
		return true;
	if (!node->ops->end_condition)
		return node->ops->flags&NODE_FLG_CHILDABLE ? true : false;
	return node->ops->end_condition(node, line);
}


struct document {
	struct node *root;
	SV_t content;
};


NODE_CREATE_DEF(root)
{
	if (parent) return NULL;
	struct node *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = NODE_INIT(root);
	return p;
}
NODE_ENDCOND_DEF(root) { (void)self; (void)line; return false; }
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
NODE_ENDCOND_DEF(headline)
{
	if (!self) return true;
	bool ret = false;
	int level = 0;
	do {
		/* from headline */
		if (!line.p || line.len <= 1) break;
		if (line.p[0] != '*') break;
		size_t i = 0;
		while (i < line.len && line.p[i] == '*') i++;
		if (i == line.len) break;
		if (!isspace(line.p[i])) break;
		ret = true;
		level = i;
	}while (false);
	if (!ret) return false;
	struct node_t_headline *p = (struct node_t_headline*)self;
	if (ret) ret = (p->level >= level);
	return ret;
}
NODE_PRINT_DEF(headline)
{
	if (!self) {
		printf("HEADLINE but got nullptr\n");
		return;
	}
	struct node_t_headline *p =
		container_of(self, typeof(struct node_t_headline), base);
	printf("HEADLINE %d >> ", p->level);
	for (int i = 1; i < p->level; i++) {
		printf("  ");
	}
	printf("\e[32m'%.*s'\e[0m", (int)self->content.len, self->content.p);
	if (p->tags.len) {
 		printf("\t\t[\e[31m%.*s\e[0m]", (int)p->tags.len, p->tags.p);
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
	if (line.len > 1 && !isspace(line.p[1])) return NULL;
	struct node *p = malloc(sizeof(*p));
	if (!p) return NULL;
	sv_chop_left(&line, 2);
	sv_trim_left_by_type(&line, isspace);
	*p = NODE_INIT(comment);
	return p;
}
NODE_ENDCOND_DEF(comment) { (void)self; (void)line; return true; }
NODE_PRINT_DEF(comment)
{
	if (!self) {
		printf("COMMENT but got nullptr\n");
		return;
	}
	printf("\e[2mCOMMENT '%.*s'\e[0m\n",
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
	line.len = i;
	sv_chop_left(&line, 2);
	sv_chop_left(&value, i+1);
	sv_trim_left_by_type(&value, isspace);
	*p = (struct node_t_meta){
		.base = NODE_INIT(meta),
		.value = value,
	};
	return &p->base;
}
NODE_ENDCOND_DEF(meta) { (void)self; (void)line; return true; }
NODE_PRINT_DEF(meta)
{
	if (!self) {
		printf("META but got nullptr\n");
		return;
	}
	struct node_t_meta *p =
		container_of(self, typeof(struct node_t_meta), base);
	printf("META >> \e[33m'%.*s'\e[0m => \e[34m'%.*s'\e[0m\n",
	       (int)self->content.len, self->content.p,
	       (int)p->value.len, p->value.p);
	return;
}

enum node_block_type {
	NODE_BLK_UNKNOW,
	NODE_BLK_SRC,
	NODE_BLK_QUOTE,
};
struct node_t_block {
	struct node base;
	/* 在base.content存储begin_xxx类型 */
	SV_t opt;    /* begin_xxx opt跟随的选项 */
	SV_t origin;    /* 块内原始内容 */
	enum node_block_type type;
};
NODE_CREATE_DEF(block)
{
	if (!line.p) return NULL;
	sv_trim_left_by_type(&line, isspace);
	if (line.len <= 3) return NULL;
	static const SV_t pat = sv_from_lstr("#+begin_");
	if (!sv_case_begin_with(line, pat)) return NULL;
	sv_chop_left(&line, pat.len);
	size_t i = 0;
	while (i < line.len && !isspace(line.p[i])) i++;
	if (i == 0) return NULL;

	SV_t opt = isspace(line.p[i]) ? line : (SV_t){};
	sv_chop_left(&opt, i);
	sv_trim_left_by_type(&opt, isspace);
	line.len = i;

	enum node_block_type type = NODE_BLK_UNKNOW;
	static const char *type_name[] = {
		[NODE_BLK_SRC] = "src",
		[NODE_BLK_QUOTE] = "quote",
	};
	for (; type < countof(type_name); type++) {
		if (!type_name[type]) continue;
		if (sv_case_cmp(line, sv_from_cstr(type_name[type])) == 0)
			break;
	}
	if (type >= countof(type_name)) type = NODE_BLK_UNKNOW;

	struct node_t_block *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (struct node_t_block){
		.base = NODE_INIT(block),
		.opt = opt,
		.type = type,
	};
	return &p->base;
}
NODE_ENDCOND_DEF(block) {
	if (!self || !line.p) return true;
	sv_trim_left_by_type(&line, isspace);
	static const SV_t pat = sv_from_lstr("#+end_");
	if (!sv_case_begin_with(line, pat)) return false;
	sv_chop_left(&line, pat.len);
	size_t i = 0;
	while (i < line.len && !isspace(line.p[i])) i++;
	if (i == 0) return false;

	SV_t opt = isspace(line.p[i]) ? line : (SV_t){};
	sv_chop_left(&opt, i);
	sv_trim_left_by_type(&opt, isspace);
	line.len = i;

	line = sv_seekline(self->doc->content, line, 0);
	if (sv_case_cmp(opt, ((struct node_t_block*)self)->opt) != 0) return false;

	SV_t *origin = &((struct node_t_block*)self)->origin;
	SV_t first = sv_seekline(self->doc->content, self->content, 0);
	SV_t last = sv_seekline(self->doc->content, line, 0);
	*origin = sv_merge(self->doc->content, first, last);
	sv_forline(&first, origin);
	sv_forline_reverse(&last, origin);
	// printf("\e[2mBLOCK ORIGIN(%zu): \e[0;31m'%.*s'\e[0m\n",
	//        origin->len, (int)origin->len, origin->p);
	return true;
}
NODE_PRINT_DEF(block)
{
	if (!self) {
		printf("BLOCK but got nullptr\n");
		return;
	}
	struct node_t_block *p = (struct node_t_block*)self;
	printf("BLOCK >> \e[34m'%.*s'\e[0m (\e[33m'%.*s'\e[0m)\n",
	       (int)self->content.len, self->content.p,
	       (int)p->opt.len, p->opt.p);
	return;
}

NODE_CREATE_DEF(text)
{
	if (!line.p || !line.len) return NULL;
	sv_trim_left_by_type(&line, isspace);
	struct node *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = NODE_INIT(text);
	return p;
}
NODE_ENDCOND_DEF(text) { (void)self; (void)line; return true; }
NODE_PRINT_DEF(text)
{
	if (!self) {
		printf("TEXT but got nullptr\n");
		return;
	}
	printf("TEXT \e[36m'%.*s'\e[0m\n",
	       (int)self->content.len, self->content.p);
	return;
}


inline int int_log10(int x)
{
	if (x <= 0) return 0;
	int i = 0;
	while (x > 0) {
		x /= 10;
		i++;
	}
	return i;
}

void document_printstack(DA_t *stack, int linenum)
{
	if (!stack) return;
	printf("\e[2J\e[H\e[31;1m***Stack*** L.%d\e[0m\n", linenum);
	void **p;
	for (size_t i = 0; i < stack->len; i++) {
		p = da_get(stack, i);
		if (p) node_print(*p);
	}
	usleep(1e3);
}

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
	struct node **parent_ptr = NULL;
	int num = 0, num_width = int_log10(sv_countlines(content));
	enum node_type init_typ = 0;
	while (sv_forline(&line, &left)) {
		num++;
		p = NULL;
		// document_printstack(&stack, num);
		bool flag_is_headline = node_checkend(&(struct node){.ops=NODE_RULES+NODE_headline}, line);
		for (parent_ptr = NULL; !p && (parent_ptr = da_get(&stack, stack.len-1)) && *parent_ptr;) {
			const enum node_type typ = (*parent_ptr)->ops->type;
			if ((!flag_is_headline || typ == NODE_headline || typ == NODE_root) &&
			    !node_checkend(*parent_ptr, line))
				break;
			if (flag_is_headline && (*parent_ptr)->ops->flags&NODE_FLG_UNBREAKABLE) {
				p = *parent_ptr;
				init_typ = typ;
				init_typ++;
				if (init_typ >= countof(NODE_RULES))
					init_typ = 0;
			}
			da_pop(&stack, stack.len-1, NULL);
		}
		if (p) {
			left = sv_merge(doc->content, sv_seekline(doc->content, p->content, 0), left);
			printf("[\e[31mINFO\e[0m] RESET to line(%zu) start at '%s' caused at line(%zu)\n",
			       sv_getlinenum(doc->content, left),
			       init_typ<countof(NODE_RULES)?NODE_RULES[init_typ].name:"Unknow",
			       sv_getlinenum(doc->content, line));
			node_pop(p);
			node_free(p);
			continue;
		}
		parent_ptr = da_get(&stack, stack.len-1);
		if (!parent_ptr || !(parent = *parent_ptr)) continue;

		for (size_t i = init_typ; i < countof(NODE_RULES); i++) {
			p = NODE_RULES[i].create(doc, parent, line);
			if (p) break;
		}
		init_typ = 0;
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
		printf("%*dL #%zu, ", num_width, num, stack.len-1);
		node_print(p);
		// printf("(%zu) META %d >> '%.*s'\n", stack.len, num, (int)line.len, line.p);
		// printf("L.%d >> '%.*s'\n", num, (int)line.len, line.p);
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

