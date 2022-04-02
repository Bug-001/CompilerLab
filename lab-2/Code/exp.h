#ifndef EXP_H
#define EXP_H

#include "object.h"
#include "rbtree.h"

enum exp_type {
  EXP_NON_EXP = 0,
  EXP_SYMBOL,
  EXP_FIELD_ACCESS,
  EXP_ARRAY_ACCESS,
  EXP_NORMAL
};

struct value {
  union {
    int i_val;
    float f_val;
  };
};

struct field_access {
  struct type* parent_type;
  struct type* type;
  struct var_list* field;
};

struct array_access {
  struct type* parent_type;
  struct type* type;
  int index;
};

/* Inherited from struct object */
struct symbol {
  struct object obj;
  struct type* type;
  bool value_is_avail;
  struct value val;
};

struct expression {
  struct type* type;
  bool value_is_avail;
  struct value val;
};

#endif