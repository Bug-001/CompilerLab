#include "config.h"
#include "exp.h"
#include "node.h"
#include "prototype.h"
#include "semantic.h"
#include "syntax.tab.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int has_error;

static struct func* cur_func = NULL;

static void pr_err(int err_type, int line, const char* msg_format, ...) {
    printf("Error type %d at Line %d: ", err_type, line);
    va_list args;
    va_start(args, msg_format);
    vprintf(msg_format, args);
    printf(".\n");
    ++has_error;
}

/* Analyse the Program tree. */
void semantic_analysis(struct node* tree) {
    assert(tree);
    assert(tree->ntype == PROGRAM);
    assert(tree->nr_children == 1);
    struct node* ext_def_list = tree->children[0];
    while (ext_def_list->nr_children) {
        assert(ext_def_list->ntype == EXT_DEF_LIST);
        /* Analyse External Definitions. */
        struct node* ext_def = ext_def_list->children[0];
        assert(ext_def->ntype == EXT_DEF);
        switch (ext_def->children[1]->ntype) {
        case EXT_DEC_LIST:
            /* ExtDef -> Specifier ExtDecList SEMI */
            global_variable(ext_def);
            break;
        case SEMI:
            /* ExtDef -> Specifier SEMI */
            specifier_analyser(ext_def->children[0]);
            break;
        case FUN_DEC:
            /* ExtDef -> Specifier FunDec CompSt */
            /* ExtDef -> Specifier FunDec SEMI */
            function(ext_def);
            break;
        default:
            assert(0);
        }
        ext_def_list = ext_def_list->children[1];
    }
    struct rb_node* node;
    for (node = rb_first(get_func_table()); node; node = rb_next(node)) {
        struct func* func = container_of(node, struct func, obj.node);
        struct func_dec_node* func_dec_list = func->dec_node_list;
        while (func_dec_list) {
            if (!func->is_defined)
                pr_err(18, func_dec_list->dec->lineno, "Function '%s' declared but not defined", func->obj.id);
            struct func_dec_node* del = func_dec_list;
            func_dec_list = func_dec_list->next;
            free(del);
        }
    }
}

bool check_variable(struct node* id) {
    void* ptr;
    char* name = id->lattr.info;
    if ((ptr = search_symbol_within_compst(name))) {
        pr_err(3, id->lineno, "Redefinition of variable '%s'", name);
        return false;
    }
    if ((ptr = search_type(name))) {
        pr_err(3, id->lineno, "Variable name '%s' conflicts with defined struct type", name);
        return false;
    }
    if ((ptr = search_field(name))) {
        char* struct_name = ((struct var_list* )ptr)->parent.type->obj.id;
        if ('0' <= struct_name[0] && struct_name[0] <= '9') {
            /* anonymous type */
            pr_err(3, id->lineno, "Variable name '%s' conflicts with a field of an unnamed struct", name);
        } else {
            pr_err(3, id->lineno, "Variable name '%s' conflicts with a field of struct '%s'", name, struct_name);
        }
        return false;
    }
    return true;
}

bool check_function(struct node* id, bool func_def, struct func** func_proto) {
    *func_proto = NULL;
    char* name = id->lattr.info;
    struct func* ptr = search_func(name);
    if (!ptr)
        return true;
    if (ptr->is_declared)
        *func_proto = ptr;
    if (func_def && ptr->is_defined) {
        pr_err(4, id->lineno, "Redefinition of function '%s'", name);
        return false;
    }
    return true;
}

bool check_struct(struct node* id) {
    void* ptr;
    char* name = id->lattr.info;
    if ((ptr = search_symbol(name))) {
        pr_err(16, id->lineno, "Struct type name '%s' conflicts with defined variable", name);
        return false;
    }
    if ((ptr = search_type(name))) {
        pr_err(16, id->lineno, "Redefinition of struct type '%s'", name);
        return false;
    }
    if ((ptr = search_field(name))) {
        char* struct_name = ((struct var_list* )ptr)->parent.type->obj.id;
        if ('0' <= struct_name[0] && struct_name[0] <= '9') {
            /* anonymous type */
            pr_err(16, id->lineno, "Struct type name '%s' conflicts with a field of an unnamed struct", name);
        } else {
            pr_err(16, id->lineno, "Struct type name '%s' conflicts with a field of struct '%s'", name, struct_name);
        }
        return false;
    }
    return true;
}

bool check_field(struct node* id) {
    void* ptr;
    char* name = id->lattr.info;
    if ((ptr = search_symbol(name))) {
        pr_err(15, id->lineno, "Field name '%s' conflicts with defined variable", name);
        return false;
    }
    if ((ptr = search_type(name))) {
        pr_err(16, id->lineno, "Field name '%s' conflicts with struct type", name);
        return false;
    }
    if ((ptr = search_field(name))) {
        char* struct_name = ((struct var_list* )ptr)->parent.type->obj.id;
        if ('0' <= struct_name[0] && struct_name[0] <= '9') {
            /* anonymous type */
            pr_err(16, id->lineno, "Field name '%s' conflicts with a field of an unnamed struct", name);
        } else {
            pr_err(16, id->lineno, "Field name '%s' conflicts with a field of struct '%s'", name, struct_name);
        }
        return false;
    }
    return true;
}

struct symbol* require_variable(struct node* id) {
    char* name = id->lattr.info;
    struct symbol* ptr = search_symbol(name);
    if (!ptr) {
        pr_err(1, id->lineno, "Undefined variable '%s'", name);
    }
    return ptr;
}

struct func* require_function(struct node* id) {
    char* name = id->lattr.info;
    struct func* ptr = search_func(name);
    if (!ptr) {
        pr_err(2, id->lineno, "Undefined function '%s'", name);
    }
    return ptr;
}

struct type* require_struct(struct node* id) {
    char* name = id->lattr.info;
    struct type* ptr = search_type(name);
    assert(!ptr || ptr->kind == TYPE_STRUCT);
    if (!ptr) {
        pr_err(17, id->lineno, "Undefined struct type '%s'", name);
    }
    return ptr;
}

struct var_list* require_field(struct node* id) {
    char* name = id->lattr.info;
    struct var_list* ptr = search_field(name);
    assert(!ptr || ptr->kind == STRUCT_FIELD_LIST);
    if (!ptr) {
        pr_err(14, id->lineno, "Undefined field '%s'", name);
    }
    return ptr;
}

/* 
 * global_variable:
 * Define and global variables.
 */
void global_variable(struct node* ext_def) {
    struct type* type = specifier_analyser(ext_def->children[0]);
    /* Analyse External Declarations. */
    struct node* ext_dec_list = ext_def->children[1];
    while (ext_dec_list->nr_children == 3) {
        /* ExtDecList -> VarDec COMMA ExtDecList */
        variable_declaration(ext_dec_list->children[0], type);
        ext_dec_list = ext_dec_list->children[2];
    }
    /* ExtDecList -> VarDec */
    variable_declaration(ext_dec_list->children[0], type);
}

static inline struct node* get_tag_id(struct node* tag) {
    if (tag->nr_children)
        return tag->children[0];
    return NULL;
}

/* 
 * specifier_analyser:
 * Return an existing type or a new type, or NULL as a bad type.
 */
struct type* specifier_analyser(struct node* specifier) {
    assert(specifier->ntype == SPECIFIER);
    struct node* node = specifier->children[0];
    if (node->ntype == TYPE) {
        return search_type(node->lattr.info);
    }
    /* node must be StructSpecifier */
    assert(node->ntype == STRUCT_SPECIFIER);
    struct node* tag_id = get_tag_id(node->children[1]);
    char* id = tag_id ? tag_id->lattr.info : NULL;
    if (node->nr_children == 5) {
        /*
         * StructSpecifier -> STRUCT OptTag LC DefList RC
         * To define a new struct type.
         */
        bool no_conflict = tag_id ? check_struct(tag_id) : true;
        if (!no_conflict) {
            /* Wanna define a struct, but it's been defined. */
            /* But we can define an anonymous struct. */
            id = NULL;
        }
        return specifier_creator(node->children[3], id);
    }
    /* 
     * StructSpecifier -> STRUCT Tag 
     * The struct should have been defined.
     * If not, `require_struct` will return NULL for us.
     */
    return require_struct(tag_id);
}

/* 
 * specifier_creator:
 * Define a new type and insert it into type table.
 * tag is NULL for an anonymous struct.
 */
struct type* specifier_creator(struct node* def_list, const char* tag) {
    assert(def_list->ntype == DEF_LIST);
    struct type* ret = alloc_type(tag);
    ret->kind = TYPE_STRUCT;
    while (def_list->nr_children) {
        /* DefList -> Def DefList */
        struct node* def = def_list->children[0];
        def_list = def_list->children[1];
        assert(def->ntype == DEF);
        /* Def -> Specifier DecList SEMI */
        struct type* type = specifier_analyser(def->children[0]);
        struct node* dec_list = def->children[1];
        while (true) {
            /* DecList -> Dec COMMA DecList */
            /* DecList -> Dec */
            struct node* dec = dec_list->children[0];
            assert(dec->ntype == DEC);
            if (dec->nr_children == 3) {
                /* 
                 * Dec -> VarDec ASSIGNOP Exp
                 * Assign can not be done in struct definition. 
                 * Report an error, then discard the assignment.
                 */
                pr_err(15, dec->children[1]->lineno, "Expected ';' at end of declaration list");
            }
            field_declaration(dec->children[0], type, ret);
            if (dec_list->nr_children == 3)
                dec_list = dec_list->children[2];
            else
                break;
        }
    }
    assert(insert_type(ret));
    return ret;
}

void function(struct node* ext_def) {
    struct type* ret_type = specifier_analyser(ext_def->children[0]);
    struct node* fun_dec = ext_def->children[1];
    bool func_def = ext_def->children[2]->ntype == COMP_ST ? true : false;

    struct func* func_proto;
    struct node* id_node = fun_dec->children[0];
    if (!check_function(id_node, func_def, &func_proto))
        return;

    push_symbol_table();

    struct func* func = alloc_func(id_node->lattr.info);
    func->ret_type = ret_type;
    assert(fun_dec->ntype == FUN_DEC);
    /* FunDec -> ID LP [Varist] RP */
    if (fun_dec->nr_children == 4) {
        /* Analyse VarList */
        struct node* var_list = fun_dec->children[2];
        assert(var_list->ntype == VAR_LIST);
        while (true) {
            /* VarList -> ParamDec COMMA VarList */
            /* VarList -> ParamDec */
            struct node* param_dec = var_list->children[0];
            assert(param_dec->ntype == PARAM_DEC);
            /* ParamDec -> Specifier VarDec */
            struct type* param_type = specifier_analyser(param_dec->children[0]);
            parameter_declaration(param_dec->children[1], param_type, func, func_def);
            if (var_list->nr_children == 3)
                var_list = var_list->children[2];
            else
                break;
        }
    }

    if (func_proto && !func_eq(func, func_proto)) {
        if (!func_proto->is_defined) {
            if (func_def) {
                pr_err(19, fun_dec->lineno, "Definition of function '%s' doesn't match prototype declaration", id_node->lattr.info);
            } else {
                pr_err(19, fun_dec->lineno, "Inconsistent declaration of function '%s'", id_node->lattr.info);
            }
        } else {
            assert(!func_def);
            pr_err(19, fun_dec->lineno, "Declaration of function '%s' doesn't match previous definition", id_node->lattr.info);
        }
    } else /* Declaration or definition is OK, update table */ {
        if (!func_def) {
            if (func_proto) {
                add_func_dec(func_proto, fun_dec);
            } else {
                func->is_declared = true;
                insert_func(func);
                add_func_dec(func, fun_dec);
            }
        } else {
            if (func_proto) {
                /* Simply set defined. */
                func_proto->is_defined = true;
            } else {
                func->is_declared = true;
                func->is_defined = true;
                insert_func(func);
            }
        }
    }
    
    if (func_def) {
        cur_func = func;
        __comp_st_analyser(ext_def->children[2]);
        cur_func = NULL;
    }

    pop_symbol_table();
    return;
}

bool func_eq(struct func* func1, struct func* func2) {
    assert(strcmp(func1->obj.id, func2->obj.id) == 0);
    struct var_list* args1 = func1->args;
    struct var_list* args2 = func2->args;
    while (args1 && args2) {
        assert(args1->kind == FUNC_PARAM_LIST);
        assert(args2->kind == FUNC_PARAM_LIST);
        if (!type_eq(args1->type, args2->type))
            return false;
        args1 = args1->thread;
        args2 = args2->thread;
    }
    if (args1 || args2)
        return false;
    if (!type_eq(func1->ret_type, func2->ret_type))
        return false;
    return true;
}

bool type_eq(struct type* type1, struct type* type2) {
    /* Assumption: Error type equals to any type. */
    if (!type1 || !type2)
        return true;
    if (type1->kind != type2->kind)
        return false;
    if (type1->kind == TYPE_BASIC)
        return type1->base == type2->base;
    if (type1->kind == TYPE_ARRAY)
        return type_eq(type1->elem, type2->elem);
#ifdef LAB_2_3
    if (type1->kind == TYPE_STRUCT) {
        struct var_list* field1 = type1->field;
        struct var_list* field2 = type2->field;
        while (field1 && field2) {
            assert(field1->kind == STRUCT_FIELD_LIST);
            assert(field2->kind == STRUCT_FIELD_LIST);
            if (!type_eq(field1->type, field2->type))
                return false;
            field1 = field1->thread;
            field2 = field2->thread;
        }
        if (field1 || field2)
            return false;
        return true;
    }
#else
    if (type1->kind == TYPE_STRUCT)
        return strcmp(type1->obj.id, type2->obj.id) == 0;
#endif
    return false;
}

/* 
 * var_dec_analyser:
 * Derive the actual type of variable, which could be the original
 *   type or array type.
 * If `type` is NULL, we only need to find ID of it.
 */
struct type* var_dec_analyser(struct node* var_dec, struct type* type, struct node** id) {
    assert(var_dec->ntype == VAR_DEC);
    if (!type) {
        /* Bad type. */
        while (var_dec->nr_children == 4) {
            var_dec = var_dec->children[0];
        }
        *id = var_dec->children[0];
        return NULL;
    }
    struct type* actual_type = type;
    while (var_dec->nr_children == 4) {
        /* VarDec -> VarDec LB INT RB */
        struct type* container = alloc_type(NULL);
        container->kind = TYPE_ARRAY;
        container->elem = actual_type;
        container->size = var_dec->children[2]->lattr.value.i_val;
        actual_type = container;
        var_dec = var_dec->children[0];
    }
    /* VarDec -> ID */
    *id = var_dec->children[0];
    return type;
}

/* 
 * variable_declaration:
 * Derive the actual type of variable, and insert it into symtab.
 * `type` is nullable.
 */
struct type* variable_declaration(struct node* var_dec, struct type* type) {
    struct type* actual_type;
    struct node* id;
    actual_type = var_dec_analyser(var_dec, type, &id);
    if (check_variable(id))
        insert_new_symbol(id->lattr.info, actual_type);
    return actual_type;
}

/* 
 * field_declaration:
 * Derive the actual type of variable, and insert it into
 *   struct type field list and field_table.
 * If `type` is NULL, it's a bad type and errors have been reported;
 *   but `struct_parent` must not be NULL.
 */
void field_declaration(struct node* var_dec, struct type* type, struct type* struct_parent) {
    assert(struct_parent);
    struct type* actual_type;
    struct node* id;
    actual_type = var_dec_analyser(var_dec, type, &id);
    if (check_field(id)) {
        struct var_list* field = alloc_var(id->lattr.info);
        field->type = actual_type;
        insert_struct_field(field, struct_parent);
    }
}

/* 
 * parameter_declaration:
 * Derive the actual type of parameter, and insert it into
 *   function args table.
 * If `type` is NULL, it's a bad type and errors have been reported;
 *   but `func_parent` must not be NULL.
 * If the function is being defined, we should insert the 
 *   parameter into symtab.
 */
void parameter_declaration(struct node* var_dec, struct type* type, struct func* func_parent, bool func_def) {
    assert(func_parent);
    struct type* actual_type;
    struct node* id;
    actual_type = var_dec_analyser(var_dec, type, &id);
    if (check_variable(id)) {
        struct var_list* arg = alloc_var(id->lattr.info);
        arg->type = actual_type;
        insert_func_args(arg, func_parent);
        if (func_def)
            insert_new_symbol(id->lattr.info, actual_type);
    }
}

inline void comp_st_analyser(struct node* comp_st) {
    push_symbol_table();
    __comp_st_analyser(comp_st);
    pop_symbol_table();
}

void __comp_st_analyser(struct node* comp_st) {
    assert(comp_st->ntype == COMP_ST);
    /* CompSt -> LC DefList StmtList RC */ 

    /* Analyse DefList */
    struct node* def_list = comp_st->children[1];
    assert(def_list->ntype == DEF_LIST);
    while (def_list->nr_children) {
        /* DefList -> Def DefList */
        struct node* def = def_list->children[0];
        def_list = def_list->children[1];
        assert(def->ntype == DEF);
        /* Def -> Specifier DecList SEMI */
        struct type* base_type = specifier_analyser(def->children[0]);
        struct node* dec_list = def->children[1];
        while (true) {
            /* DecList -> Dec COMMA DecList */
            /* DecList -> Dec */
            struct node* dec = dec_list->children[0];
            assert(dec->ntype == DEC);
            /* Dec -> VarDec */
            /* Dec -> VarDec ASSIGNOP Exp */
            struct type* actual_type = variable_declaration(dec->children[0], base_type);
            if (dec->nr_children == 3) {
                /* Process assign */
                struct type* exp_type = expression_analyser(dec->children[2]);
                if (!type_eq(actual_type, exp_type))
                    pr_err(5, dec->children[1]->lineno, "Initializing '%s' with an expression of incompatible type '%s'", actual_type->obj.id, exp_type->obj.id);
                else {
                    /* TODO: Assignment code */
                }
            }
            if (dec_list->nr_children == 3)
                dec_list = dec_list->children[2];
            else
                break;
        }
    }

    /* Analyse StmtList */
    struct node* stmt_list = comp_st->children[2];
    assert(stmt_list->ntype == STMT_LIST);
    while (stmt_list->nr_children) {
        /* StmtList -> Stmt StmtList */
        struct node* stmt = stmt_list->children[0];
        stmt_list = stmt_list->children[1];
        statement_analyser(stmt);
    }
}

void statement_analyser(struct node* stmt) {
    assert(stmt->ntype == STMT);
    if (stmt->nr_children == 2) {
        /* Stmt -> Exp SEMI */
        expression_analyser(stmt->children[0]);
    } else if (stmt->nr_children == 1) {
        /* Stmt -> CompSt */
        comp_st_analyser(stmt->children[0]);
    } else if (stmt->nr_children == 3) {
        /* Stmt -> RETURN Exp SEMI */
        struct type* ret_type = expression_analyser(stmt->children[1]);
        if (!type_eq(ret_type, cur_func->ret_type))
            pr_err(8, stmt->children[1]->lineno, "Type mismatched for return");
    } else if (stmt->children[0]->ntype == IF) {
        /* Stmt -> IF LP Exp RP Stmt */
        /* Stmt -> IF LP Exp RP Stmt ELSE Stmt */
        if_statement_analyser(stmt);
    } else if (stmt->children[0]->ntype == WHILE) {
        /* Stmt -> WHILE LP Exp RP Stmt */
        while_statement_analyser(stmt);
    } else {
        assert(0);
    }
}

void if_statement_analyser(struct node* stmt) {
    assert(stmt->ntype == STMT);
    /* Stmt -> IF LP Exp RP Stmt */
    /* Stmt -> IF LP Exp RP Stmt ELSE Stmt */
    struct node* exp = stmt->children[2];
    struct type* cond_type = expression_analyser(exp);
    if (!type_eq(cond_type, search_type("int")))
        /* TODO: */
        pr_err(114514, exp->lineno, "Bad type in condition expression");
    statement_analyser(stmt->children[4]);
    if (stmt->nr_children == 7) {
        statement_analyser(stmt->children[6]);
    }
}

void while_statement_analyser(struct node* stmt) {
    assert(stmt->ntype == STMT);
    /* Stmt -> WHILE LP Exp RP Stmt */
    struct node* exp = stmt->children[2];
    struct type* cond_type = expression_analyser(exp);
    if (!type_eq(cond_type, search_type("int")))
        /* TODO: */
        pr_err(114514, exp->lineno, "Bad type in condition expression");
    statement_analyser(stmt->children[4]);
}

struct type* expression_analyser(struct node* exp) {
    assert(exp->ntype == EXP);
    struct type* ret;
    switch (exp->nr_children) {
    case 1:
        ret = expression_analyser_1(exp);
        break;
    case 2:
        ret = expression_analyser_2(exp);
        break;
    case 3:
        ret = expression_analyser_3(exp);
        break;
    case 4:
        ret = expression_analyser_4(exp);
        break;
    default:
        assert(0);
    }
    return ret;
}

inline struct type* expression_analyser_1(struct node* exp) {
    if (exp->children[0]->ntype == ID) {
        /* EXP -> ID */
        struct symbol* sym = require_variable(exp->children[0]);
        if (!sym)
            return NULL;
        return sym->type;
    }
    /* EXP -> INT | FLOAT */
    if (exp->children[0]->ntype == INT)
        return search_type("int");
    if (exp->children[0]->ntype == FLOAT)
        return search_type("float");
}

inline struct type* expression_analyser_2(struct node* exp) {
    /* EXP -> MINUS EXP | NOT EXP */
    return expression_analyser(exp->children[1]);
}

inline struct type* expression_analyser_3(struct node* exp) {
    if (exp->children[0]->ntype == EXP) {
        /* Arithmetic operations */
        if (exp->children[1]->ntype == ASSIGNOP) {

        }
    }
}

inline struct type* expression_analyser_4(struct node* exp) {

}