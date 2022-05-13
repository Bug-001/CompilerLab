#include "config.h"
#include "exp.h"
#include "object.h"
#include "prototype.h"
#include "rbtree.h"
#include <stdlib.h>
#include <string.h>

static struct symbol_table {
	struct rb_root table;
	struct symbol_table *prev;
} symbol_table_stack = { .prev = NULL };

static struct symbol_table *cur_table = &symbol_table_stack;

static int sym_no = 1;

struct symbol *search_symbol(const char *id)
{
	struct symbol_table *table = cur_table;
	while (table) {
		struct symbol *ret = search_object(&table->table, id);
		if (ret)
			return ret;
		table = table->prev;
	}
	return NULL;
}

struct symbol *search_symbol_within_compst(const char *id)
{
	return search_object(&cur_table->table, id);
}

/* Caller guarantees that id is not in cur_table. */
struct symbol *insert_new_symbol(const char *id, struct type *type)
{
	INIT_OBJECT(struct symbol, sym, id);
	if (!sym)
		return NULL;
	sym->type = type;
	sym->var_no = sym_no++;
	insert_object(&cur_table->table, sym);
	return sym;
}

void push_symbol_table()
{
#ifdef LAB_2_2
	struct symbol_table *new_table =
		(struct symbol_table *)malloc(sizeof(struct symbol_table));
	memset(new_table, 0, sizeof(struct symbol_table));
	new_table->prev = cur_table;
	cur_table = new_table;
#endif
}

void pop_symbol_table()
{
#ifdef LAB_2_2
	struct rb_root *root = &cur_table->table;
	struct rb_node *node, *tmp;
	struct symbol *del;
	for (node = rb_first(root); node;) {
		tmp = rb_next(node);
		del = container_of(node, struct symbol, obj.node);
		erase_object(del, root);
		free_object(del);
		node = tmp;
	}
	struct symbol_table *del_table = cur_table;
	cur_table = cur_table->prev;
	free(del_table);
#endif
}
