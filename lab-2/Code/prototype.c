#include "prototype.h"
#include "exp.h"
#include "object.h"
#include "rbtree.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct rb_root type_table = RB_ROOT;

static struct type __int_type = {
    .obj.id = "int",
    .kind = TYPE_BASIC,
    .base = TYPE_INT
};

static struct type __float_type = {
    .obj.id = "float",
    .kind = TYPE_BASIC,
    .base = TYPE_FLOAT
};

/* If id is NULL, a hidden id will be allocated. */
struct type* alloc_type(const char* id) {
    static unsigned hidden_id = 0;
    char buf[32] = {0};
    if (!id) {
        sprintf(buf, "%u_anonymous_type", hidden_id++);
        id = buf;
    }
    INIT_OBJECT(struct type, type, id);
    return type;
}

bool insert_type(struct type* type) {
    return insert_object(&type_table, type);
}

struct type* search_type(const char* id) {
    if (strcmp(id, "int") == 0)
        return get_int_type();
    if (strcmp(id, "float") == 0)
        return get_float_type();
    return search_object(&type_table, id);
}

inline struct type* get_int_type() {
    return &__int_type;
}

inline struct type* get_float_type() {
    return &__float_type;
}

static struct rb_root func_table = RB_ROOT;

struct func* alloc_func(const char* id) {
    INIT_OBJECT(struct func, func, id);
    return func;
}

struct func* search_func(const char* id) {
    return search_object(&func_table, id);
}

bool insert_func(struct func* func) {
    return insert_object(&func_table, func);
}

bool add_func_dec(struct func* existing, struct node* node) {
    if (!existing)
        return false;
    if (existing->is_defined)
        return true;
    struct func_dec_node* new = malloc(sizeof(struct func_dec_node));
    new->next = existing->dec_node_list;
    new->dec = node;
    existing->dec_node_list = new;
    return false;
}

struct rb_root* get_func_table() {
    return &func_table;
}

static struct rb_root field_table = RB_ROOT;

/* If id is NULL, a hidden id will be allocated. */
struct var_list* alloc_var(const char* id) {
    static unsigned hidden_id = 0;
    char buf[32] = {0};
    if (!id) {
        sprintf(buf, "%u_anonymous_var", hidden_id++);
        id = buf;
    }
    INIT_OBJECT(struct var_list, var, id);
    return var;
}

struct var_list* search_field(const char* id) {
    return search_object(&field_table, id);
}

/* Insert ONE field into type->field and field_table.
 * Some of field's attributes will be modified.
 */
bool insert_struct_field(struct var_list* field, struct type* type) {
    if (!type)
        return false;
    if (type->kind != TYPE_STRUCT)
        return false;
    bool res = insert_object(&field_table, field);
    if (!res)
        return false;
    field->kind = STRUCT_FIELD_LIST;
    field->parent.type = type;
    field->thread = type->field;
    type->field = field;
    return true;
}

/* Insert ONE field into func->args.
 * arg->type must be defined.
 */
bool insert_func_args(struct var_list* arg, struct func* func) {
    arg->kind = FUNC_PARAM_LIST;
    arg->parent.func = func;
    arg->thread = func->args;
    func->args = arg;
    return true;
}