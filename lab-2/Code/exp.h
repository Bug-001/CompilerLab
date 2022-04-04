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

enum relop_type {
  GT,
  LT,
  GE,
  LE,
  EQ,
  NE
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

struct exp_attr {
  enum exp_type type;
  struct node* parent;
  union {
    struct symbol* sym;
    struct field_access* field;
    struct array_access* array;
    struct expression* exp;
  };
};

struct symbol* search_symbol(const char* id);

struct symbol* search_symbol_within_compst(const char* id);

struct symbol* insert_new_symbol(const char* id, struct type* type);

void push_symbol_table();

void pop_symbol_table();

#endif