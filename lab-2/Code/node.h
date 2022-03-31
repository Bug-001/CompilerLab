#ifndef NODE_H
#define NODE_H

#include "config.h"

enum node_type {
  LEXICAL,
  SYNTAX
};

struct node {
  const char* name;// all name strings are literal
  enum node_type type;
  unsigned lineno;
  char* info;// all info strings are allocated in heap
  union {
    unsigned i_val;
    float f_val;
  };

  struct node* child;
  struct node* sibling;
};

extern struct node* tree;

struct node* gen_tree(const char* name, unsigned lineno, int num, ...);

struct node* new_node(const char* name, enum node_type type, unsigned lineno, char* info);

#define new_lexical_node(name, info) \
  new_node(name, LEXICAL, 0, info)

#define new_syntax_node(name, lineno) \
  new_node(name, SYNTAX, lineno, "unused")

void print_node(struct node* p);

void print_tree(struct node* root);

#endif /* NODE_H */