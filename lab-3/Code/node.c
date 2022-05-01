#include "node.h"
#include "config.h"
#include "syntax.tab.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node *tree = NULL;

struct node *alloc_node(const char *const name, int ntype, unsigned lineno,
			int nr_children)
{
	struct node *node = (struct node *)malloc(sizeof(struct node));
	/* set basic node fields */
	memset(node, 0, sizeof(struct node));
	node->ntype = ntype;
	node->lineno = lineno;
	node->name = name;
	/* set children */
	if (nr_children) {
		node->nr_children = nr_children;
		node->children = (struct node **)malloc(sizeof(struct node *) *
							nr_children);
	}
	return node;
}

void free_node(struct node *root)
{
	if (!root)
		return;
	/* free lattr or sattr */
	if (is_lex(root->ntype)) {
		free(root->lattr.info);
	} else {
		/* TODO: free_sattr() */;
	}
	/* free children */
	for (int i = 0; i < root->nr_children; ++i) {
		free_node(root->children[i]);
	}
	free(root->children);
	/* free itself */
	free(root);
}

struct node *gen_tree(const char *const name, int ntype, unsigned lineno,
		      int nr_children, ...)
{
	struct node *root = alloc_node(name, ntype, lineno, nr_children);
	if (nr_children == 0)
		return root;
	/* set children */
	va_list valist;
	va_start(valist, nr_children);
	for (int i = 0; i < nr_children; ++i) {
		root->children[i] = va_arg(valist, struct node *);
		root->children[i]->parent = root;
	}
	va_end(valist);
	return root;
}

struct node *new_lexical_node(const char *const name, int ntype,
			      unsigned lineno, const char *info)
{
	struct node *node = alloc_node(name, ntype, lineno, 0);

	/* set node->lattr */
	switch (ntype) {
	case INT:
		if (info[0] == '0' && (info[1] == 'x' || info[1] == 'X'))
			sscanf(info, "%x", &node->lattr.value.i_val);
		else if (info[0] == '0')
			sscanf(info, "%o", &node->lattr.value.i_val);
		else
			sscanf(info, "%d", &node->lattr.value.i_val);
		break;
	case FLOAT:
		node->lattr.value.f_val = atof(info);
		node->lattr.info = (char *)malloc(strlen(info) + 1);
		strcpy(node->lattr.info, info);
		break;
	case RELOP:
		if (info[0] == '>') {
			if (info[1] == '=')
				node->lattr.relop_type = GE;
			else
				node->lattr.relop_type = GT;
		} else if (info[0] == '<') {
			if (info[1] == '=')
				node->lattr.relop_type = LE;
			else
				node->lattr.relop_type = LT;
		} else if (info[0] == '=') {
			node->lattr.relop_type = EQ;
		} else {
			node->lattr.relop_type = NE;
		}
		break;
	case ID:
	case TYPE:
		node->lattr.info = (char *)malloc(strlen(info) + 1);
		strcpy(node->lattr.info, info);
		break;
	}

	return node;
}

static int indents = -2;
void print_tree(struct node *root)
{
	indents += 2;
#ifndef L1_HARD
	/* In L1 normal tests, do not print empty syntax node. */
	if (is_syn(root->ntype) && root->nr_children == 0) {
		indents -= 2;
		return;
	}
#endif
	for (int i = 0; i < indents; ++i)
		printf(" ");
	print_node(root);
	for (int i = 0; i < root->nr_children; ++i)
		print_tree(root->children[i]);
	indents -= 2;
}

void print_node(struct node *p)
{
	if (is_syn(p->ntype)) {
		/* print syntax node */
		printf("%s (%d)\n", p->name, p->lineno);
		return;
	}
	/* print lexical node */
	printf("%s", p->name);
	switch (p->ntype) {
	case ID:
	case TYPE:
		printf(": %s\n", p->lattr.info);
		break;
	case INT:
		printf(": %u\n", p->lattr.value.i_val);
		break;
	case FLOAT:
#ifdef L1_HARD
		printf(": %s\n", p->lattr.info);
#else
		printf(": %f\n", p->lattr.value.f_val);
#endif
		break;
#ifdef L1_HARD
	case RELOP:
		switch (p->lattr.relop_type) {
		case GT:
			printf(": GT\n");
			break;
		case LT:
			printf(": LT\n");
			break;
		case GE:
			printf(": GE\n");
			break;
		case LE:
			printf(": LE\n");
			break;
		case EQ:
			printf(": EQ\n");
			break;
		case NE:
			printf(": NE\n");
			break;
		default:
			printf(": BAD\n");
			break;
		}
		break;
#endif
	default:
		printf("\n");
	}
}