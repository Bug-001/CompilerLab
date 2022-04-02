#include "prototype.h"
#include "exp.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct rb_root type_table = RB_ROOT;

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

static void init_type_table() {
    struct type* __int_type = NULL;
    __int_type = alloc_type("int");
    __int_type->kind = TYPE_BASIC;
    __int_type->base = TYPE_INT;
    insert_type(__int_type);

    struct type* __float_type = NULL;
    __float_type = alloc_type("float");
    __float_type->kind = TYPE_BASIC;
    __float_type->base = TYPE_FLOAT;
    insert_type(__float_type);
}

struct type* search_type(const char* id) {
    static bool __inited = false;
    if (!__inited) {
        /* static initialize */
        init_type_table();
        __inited = true;
    }
    return search_object(&type_table, id);
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

bool search_field(const char* id) {
    return search_object(&func_table, id);
}

/* Insert ONE field into type->field and field_table.
 * Some of field's attributes will be modified.
 */
bool insert_struct_field(struct var_list* field, struct type* type) {
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