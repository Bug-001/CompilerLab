#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "exp.h"
#include "rbtree.h"
#include <stdbool.h>

enum relop_type {
  GT,
  LT,
  GE,
  LE,
  EQ,
  NE
};

struct lexical_attr {
  char* info;
  union {
    struct value value;
    enum relop_type relop_type;
  };
};

struct syntax_attr {
  enum exp_type type;
  struct node* parent;
  union {
    struct symbol* sym;
    struct field_access* field;
    struct array_access* array;
    struct expression* exp;
  };
};

void semantic_analysis(struct node* tree);

#endif /* SEMANTIC_H */