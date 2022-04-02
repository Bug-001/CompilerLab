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
  struct rb_node rb_node;
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

/* Inherited from struct object */
struct func {
  struct object obj;
  bool is_declared;
  bool is_defined;
  struct type* ret_type;
  struct var_list* args;
};

#endif