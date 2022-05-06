#include "prototype.h"
#include "exp.h"
#include "object.h"
#include "rbtree.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct rb_root type_table = RB_ROOT;

static struct type __int_type = { .obj.id = "int",
				  .kind = TYPE_BASIC,
				  .base = TYPE_INT };

static struct type __float_type = { .obj.id = "float",
				    .kind = TYPE_BASIC,
				    .base = TYPE_FLOAT };

/* If id is NULL, a hidden id will be allocated. */
struct type *alloc_type(const char *id)
{
	static unsigned hidden_id = 0;
	char buf[32] = { 0 };
	if (!id) {
		sprintf(buf, "%u_anonymous_type", hidden_id++);
		id = buf;
	}
	INIT_OBJECT(struct type, type, id);
	return type;
}

bool insert_type(struct type *type)
{
	return insert_object(&type_table, type);
}

struct type *search_type(const char *id)
{
	if (strcmp(id, "int") == 0)
		return get_int_type();
	if (strcmp(id, "float") == 0)
		return get_float_type();
	return search_object(&type_table, id);
}

inline struct type *get_int_type()
{
	return &__int_type;
}

inline struct type *get_float_type()
{
	return &__float_type;
}

static struct rb_root func_table = RB_ROOT;

struct func *alloc_func(const char *id)
{
	INIT_OBJECT(struct func, func, id);
	return func;
}

struct func *search_func(const char *id)
{
	return search_object(&func_table, id);
}

bool insert_func(struct func *func)
{
	return insert_object(&func_table, func);
}

bool add_func_dec(struct func *existing, struct node *node)
{
	if (!existing)
		return false;
	if (existing->is_defined)
		return true;
	struct func_dec_node *new = malloc(sizeof(struct func_dec_node));
	new->next = existing->dec_node_list;
	new->dec = node;
	existing->dec_node_list = new;
	return false;
}

struct rb_root *get_func_table()
{
	return &func_table;
}

/* If id is NULL, a hidden id will be allocated. */
struct var_list *alloc_var(const char *id)
{
	static unsigned hidden_id = 0;
	char buf[32] = { 0 };
	if (!id) {
		sprintf(buf, "%u_anonymous_var", hidden_id++);
		id = buf;
	}
	INIT_OBJECT(struct var_list, var, id);
	return var;
}

struct var_list *search_field(const char *id, struct type *type)
{
	if (!type || type->kind != TYPE_STRUCT)
		return NULL;
	struct var_list *cur = type->field;
	while (cur) {
		if (strcmp(cur->obj.id, id) == 0)
			return cur;
		cur = cur->pred;
	}
	return NULL;
}

/* Insert ONE field into type->field and field_table.
 * Some of field's attributes will be modified.
 */
bool insert_struct_field(struct var_list *field, struct type *type)
{
	if (!type)
		return false;
	if (type->kind != TYPE_STRUCT)
		return false;
	if (search_field(field->obj.id, type))
		return false;
	field->kind = STRUCT_FIELD_LIST;
	field->parent.type = type;
	field->pred = type->field;
	field->succ = NULL;
	if (field->pred)
		field->pred->succ = field;
	type->field = field;
	return true;
}

/* Insert ONE field into func->args.
 * arg->type must be defined.
 */
bool insert_func_args(struct var_list *arg, struct func *func)
{
	arg->kind = FUNC_PARAM_LIST;
	arg->parent.func = func;
	arg->pred = func->args;
	arg->succ = NULL;
	if (arg->pred)
		arg->pred->succ = arg;
	func->args = arg;
	++func->nr_args;
	return true;
}

bool func_eq(struct func *func1, struct func *func2)
{
	assert(strcmp(func1->obj.id, func2->obj.id) == 0);
	struct var_list *args1 = func1->args;
	struct var_list *args2 = func2->args;
	while (args1 && args2) {
		assert(args1->kind == FUNC_PARAM_LIST);
		assert(args2->kind == FUNC_PARAM_LIST);
		if (!type_eq(args1->type, args2->type))
			return false;
		args1 = args1->pred;
		args2 = args2->pred;
	}
	if (args1 || args2)
		return false;
	if (!type_eq(func1->ret_type, func2->ret_type))
		return false;
	return true;
}

bool type_eq(struct type *type1, struct type *type2)
{
	/* Assumption: Error type equals to any type. */
	if (!type1 || !type2)
		return true;
	if (type1->kind != type2->kind)
		return false;
	if (type1->kind == TYPE_BASIC)
		return type1->base == type2->base;
	if (type1->kind == TYPE_ARRAY)
		return type_eq(type1->elem, type2->elem);
	if (type1->kind == TYPE_STRUCT) {
		if (strcmp(type1->obj.id, type2->obj.id) == 0)
			return true;
#ifdef LAB_2_3
		struct var_list *field1 = type1->field;
		struct var_list *field2 = type2->field;
		while (field1 && field2) {
			assert(field1->kind == STRUCT_FIELD_LIST);
			assert(field2->kind == STRUCT_FIELD_LIST);
			if (!type_eq(field1->type, field2->type))
				return false;
			field1 = field1->pred;
			field2 = field2->pred;
		}
		if (field1 || field2)
			return false;
		return true;
#else
		return false;
#endif
	}

	return false;
}

bool type_eq_arithmetic(struct type *type1, struct type *type2)
{
	if (!type1 || !type2)
		return true;
	if (type1->kind != TYPE_BASIC || type2->kind != TYPE_BASIC)
		return false;
	return type1->base == type2->base;
}
