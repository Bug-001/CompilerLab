#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "exp.h"
#include "prototype.h"
#include "rbtree.h"
#include <stdbool.h>

void semantic_analysis(struct node* tree);

/* Return true if check passed. */
bool check_variable(struct node* id);
bool check_function(struct node* id, bool func_def, struct func** func_proto);
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

bool func_eq(struct func* func1, struct func* func2);
bool type_eq(struct type* type1, struct type* type2);

struct type* var_dec_analyser(struct node* var_dec, struct type* type, struct node** id);
struct type* variable_declaration(struct node* var_dec, struct type* type);
void field_declaration(struct node* var_dec, struct type* type, struct type* struct_parent);
void parameter_declaration(struct node* var_dec, struct type* type, struct func* func_parent, bool func_def);

void comp_st_analyser(struct node* comp_st);
void __comp_st_analyser(struct node* comp_st);

void statement_analyser(struct node* stmt);
void if_statement_analyser(struct node* stmt);
void while_statement_analyser(struct node* stmt);

/*
 * Young and simple type checkers. 
 * Will re-write in Lab3.
 */
struct type* expression_analyser(struct node* exp);
struct type* expression_analyser_1(struct node* exp);
struct type* expression_analyser_2(struct node* exp);
struct type* expression_analyser_3(struct node* exp);
struct type* expression_analyser_4(struct node* exp);

#endif /* SEMANTIC_H */