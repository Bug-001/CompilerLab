#ifndef PROTOTYPE_H
#define PROTOTYPE_H

#include "object.h"
#include "rbtree.h"

enum type_kind {
  TYPE_BASIC,
  TYPE_ARRAY,
  TYPE_STRUCT
};

enum basic_kind {
  TYPE_INT,
  TYPE_FLOAT
};

enum var_list_kind {
  FUNC_PARAM_LIST,
  STRUCT_FIELD_LIST
};

/* Inherited from struct object */
struct var_list {
  struct object obj;
  enum var_list_kind kind;
  struct type* type;
  union {
    struct type* type;
    struct func* func;
  } parent;
  /* When traversing var_list, we have to follow the thread
   * rather than rbtree, to keep the declared order of vars.
   */
  struct var_list* thread;
};

/* Inherited from struct object */
struct type {
  struct object obj;
  enum type_kind kind;
  union {
    /* for basic type */
    enum basic_kind base;
    /* for array */
    struct {
      struct type* elem;
      int size;
    };
    /* for struct */
    struct var_list* field;
  };
};

struct func_dec_node {
  struct node* dec;
  struct func_dec_node* next;
};

/* Inherited from struct object */
struct func {
  struct object obj;
  bool is_declared;
  bool is_defined;
  struct type* ret_type;
  /* args is reversed */
  struct var_list* args;
  /* `dec_node_list` is not managed by GC */
  struct func_dec_node* dec_node_list;
};

struct type* alloc_type(const char* id);
bool insert_type(struct type* type);
struct type* search_type(const char* id);
struct type* get_int_type();
struct type* get_float_type();

struct func* alloc_func(const char* id);
struct func* search_func(const char* id);
bool insert_func(struct func* func);
bool add_func_dec(struct func* existing, struct node* node);
struct rb_root* get_func_table();

struct var_list* alloc_var(const char* id);
struct var_list* search_field(const char* id);
bool insert_struct_field(struct var_list* field, struct type* type);
bool insert_func_args(struct var_list* arg, struct func* func);

#endif