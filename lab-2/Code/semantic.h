#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "exp.h"
#include "prototype.h"
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

enum name_kind {
  NO_NAME,
  VAR_NAME,
  FUNC_NAME,
  FIELD_NAME,
  STRUCT_NAME
};

void semantic_analysis(struct node* tree);

bool check_variable(struct node* id);
bool check_function(struct node* id);
bool check_struct(struct node* id);
bool check_field(struct node* id);

struct symbol* require_variable(struct node* id);
struct func* require_function(struct node* id);
struct type* require_struct(struct node* id);
struct var_list* require_field(struct node* id);


void global_variable(struct node* ext_def);

struct type* specifier_analyser(struct node* specifier);

struct type* specifier_creator(struct node* def_list, const char* tag);

void function(struct node* ext_def);

void variable_declaration(struct node* var_dec, struct type* type);

void field_declaration(struct node* var_dec, struct type* type, struct type* struct_parent);

#endif /* SEMANTIC_H */