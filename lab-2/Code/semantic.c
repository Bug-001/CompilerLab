#include "exp.h"
#include "node.h"
#include "prototype.h"
#include "semantic.h"
#include "syntax.tab.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

extern int has_error;

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
}

bool check_variable(struct node* id) {
    void* ptr;
    char* name = id->lattr.info;
    if ((ptr = search_symbol_within_compst(name))) {
        pr_err(3, id->lineno, "Redefinition of variable '%s'", name);
        return ptr;
    }
    if ((ptr = search_type(name))) {
        pr_err(3, id->lineno, "Variable name '%s' conflicts with defined struct type", name);
        return ptr;
    }
    if ((ptr = search_field(name))) {
        char* struct_name = ((struct var_list* )ptr)->parent.type->obj.id;
        if ('0' <= struct_name[0] && struct_name[0] <= '9') {
            /* anonymous type */
            pr_err(3, id->lineno, "Variable name '%s' conflicts with a field of an unnamed struct", name);
        } else {
            pr_err(3, id->lineno, "Variable name '%s' conflicts with a field of struct '%s'", name, struct_name);
        }
        return ptr;
    }
    return NULL;
}

bool check_function(struct node* id) {
    char* name = id->lattr.info;
    void* ptr = search_func(name);
    if (ptr) {
        pr_err(4, id->lineno, "Redefinition of function '%s'", name);
        return ptr;
    }
    return NULL;
}

bool check_struct(struct node* id) {
    void* ptr;
    char* name = id->lattr.info;
    if ((ptr = search_symbol(name))) {
        pr_err(16, id->lineno, "Struct type name '%s' conflicts with defined variable", name);
        return ptr;
    }
    if ((ptr = search_type(name))) {
        pr_err(16, id->lineno, "Redefinition of struct type '%s'", name);
        return ptr;
    }
    if ((ptr = search_field(name))) {
        char* struct_name = ((struct var_list* )ptr)->parent.type->obj.id;
        if ('0' <= struct_name[0] && struct_name[0] <= '9') {
            /* anonymous type */
            pr_err(16, id->lineno, "Struct type name '%s' conflicts with a field of an unnamed struct", name);
        } else {
            pr_err(16, id->lineno, "Struct type name '%s' conflicts with a field of struct '%s'", name, struct_name);
        }
        return ptr;
    }
    return NULL;
}

bool check_field(struct node* id) {
    void* ptr;
    char* name = id->lattr.info;
    if ((ptr = search_symbol(name))) {
        pr_err(15, id->lineno, "Field name '%s' conflicts with defined variable", name);
        return ptr;
    }
    if ((ptr = search_type(name))) {
        pr_err(16, id->lineno, "Field name '%s' conflicts with struct type", name);
        return ptr;
    }
    if ((ptr = search_field(name))) {
        char* struct_name = ((struct var_list* )ptr)->parent.type->obj.id;
        if ('0' <= struct_name[0] && struct_name[0] <= '9') {
            /* anonymous type */
            pr_err(16, id->lineno, "Field name '%s' conflicts with a field of an unnamed struct", name);
        } else {
            pr_err(16, id->lineno, "Field name '%s' conflicts with a field of struct '%s'", name, struct_name);
        }
        return ptr;
    }
    return NULL;
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
 * Return an existing type or a new type.
 * Return the defined type for error 16.
 */
struct type* specifier_analyser(struct node* specifier) {
    assert(specifier->ntype == SPECIFIER);
    struct node* node = specifier->children[0];
    if (node->ntype == TYPE) {
        return search_type(node->lattr.info);
    }
    /* for StructSpecifier */
    assert(node->ntype == STRUCT_SPECIFIER);
    struct node* tag_id = get_tag_id(node->children[1]);
    char* id = tag_id->lattr.info;
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
        return specifier_creator(specifier->children[3], id);
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
    while (def_list->nr_children) {
        /* DefList -> Def DefList */
        struct node* def = def_list->children[0];
        def_list = def_list->children[1];
        assert(def->ntype == DEF);
        /* Def -> Specifier DecList SEMI */
        struct type* type = specifier_analyser(def->children[0]);
        struct node* dec_list = def->children[1];
        while (dec_list->nr_children == 3) {
            /* DecList -> Dec COMMA DecList */
            struct node* dec = dec_list->children[0];
            dec_list = dec_list->children[2];
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
        }
        /* DecList -> Dec */
        /* Dec -> VarDec | VarDec ASSIGNOP Exp */
        struct node* dec = dec_list->children[0];
        if (dec->nr_children == 3) {
            pr_err(15, dec->children[1]->lineno, "Expected ';' at end of declaration list");
        }
        field_declaration(dec->children[0], type, ret);
    }
    insert_type(ret);
    return ret;
}

void function(struct node* ext_def) {
    struct type* ret_type = specifier_analyser(ext_def->children[0]);
    struct node* fun_dec = ext_def->children[1];
    assert(fun_dec->ntype == FUN_DEC);
    // TODO: 
}

/* 
 * variable_declaration:
 * Derive the actual type of variable, and insert it into symbol table.
 * If `type` is NULL, it's a bad type and errors have been reported;
 * the `actual_type` is no need to be analysed.
 */
void variable_declaration(struct node* var_dec, struct type* type) {
    struct type* actual_type;
    char* id;
    // TODO:
    insert_new_symbol(id, actual_type);
}

void field_declaration(struct node* var_dec, struct type* type, struct type* struct_parent) {
    // insert_struct_field(struct var_list *field, struct type *type)
}