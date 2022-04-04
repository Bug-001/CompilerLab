#ifndef NODE_H
#define NODE_H

#include "exp.h"

/* syntax types */
enum syntax_type {
  PROGRAM = 1,
  EXT_DEF_LIST,
  EXT_DEF,
  EXT_DEC_LIST,
  SPECIFIER,
  STRUCT_SPECIFIER,
  OPT_TAG,
  TAG,
  VAR_DEC,
  FUN_DEC,
  VAR_LIST,
  PARAM_DEC,
  COMP_ST,
  STMT_LIST,
  STMT,
  DEF_LIST,
  DEF,
  DEC_LIST,
  DEC,
  EXP,
  ARGS,

  NR_SYNTAX_TYPES
};

#define is_lex(type) ((type) > NR_SYNTAX_TYPES ? 1 : 0)
#define is_syn(type) (!is_lex(type))

struct lexical_attr {
  char* info;
  union {
    struct value value;
    enum relop_type relop_type;
  };
};

struct node {
  const char* name;
  int ntype;
  unsigned lineno;
  /* lexical attribute or syntax attribute */
  union {
    struct lexical_attr lattr;
    struct syntax_attr* p_sattr;
  };

  struct node* parent;
  int nr_children;
  struct node** children;
};

extern struct node* tree;

struct node* alloc_node(const char* const name, int ntype, unsigned lineno, int nr_children);

void free_node(struct node* root);

struct node* gen_tree(const char* const name, int ntype, unsigned lineno, int nr_children, ...);

struct node* new_lexical_node(const char* const name, int ntype, unsigned lineno, const char* info);

void print_node(struct node* p);

void print_tree(struct node* root);

#endif /* NODE_H */