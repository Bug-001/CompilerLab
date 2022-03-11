#include "node.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node* tree = NULL;

struct node* gen_tree(const char* name, unsigned lineno, int num, ...) {
  struct node* root = new_syntax_node(name, lineno);
  if (num == 0)
    return root;
  va_list valist;
  va_start(valist, num);
  root->child = va_arg(valist, struct node*);
  struct node* p_children = root->child;
  for (int i = 1; i < num; ++i) {
    p_children->sibling = va_arg(valist, struct node*);
    p_children = p_children->sibling;
  }
  va_end(valist);
  return root;
}

struct node* new_node(const char* name, enum node_type type, unsigned lineno, char* info) {
  struct node* root = (struct node*) malloc(sizeof(struct node));
  root->name = name;
  root->type = type;
  root->lineno = lineno;
  root->info = info;
  root->child = NULL;
  root->sibling = NULL;
  return root;
}

static int indents = -2;
void print_tree(struct node* root) {
  indents += 2;
  struct node* child = root->child;
#ifndef L1_HARD
  if (root->type == SYNTAX && child == NULL) {
    indents -= 2;
    return;
  }
#endif
  for (int i = 0; i < indents; ++i)
    printf(" ");
  print_node(root);
  while (child) {
    print_tree(child);
    child = child->sibling;
  }
  indents -= 2;
}

void print_node(struct node* p) {
  if (p->type == SYNTAX) {
    printf("%s (%d)\n", p->name, p->lineno);
  } else if (p->type == LEXICAL) {
    printf("%s", p->name);
    if (strcmp(p->name, "ID") == 0 || strcmp(p->name, "TYPE") == 0) {
      printf(": %s\n", p->info);
    } else if (strcmp(p->name, "INT") == 0) {
      printf(": %u\n", p->i_val);
    } else if (strcmp(p->name, "FLOAT") == 0) {
#ifdef L1_HARD
      printf(": %s\n", p->info);
#else
      printf(": %f\n", p->f_val);
#endif
#ifdef L1_HARD
    } else if (strcmp(p->name, "RELOP") == 0) {
      if (strcmp(p->info, ">=") == 0) {
        printf(": %s\n", "GE");
      } else if (strcmp(p->info, "<=") == 0) {
        printf(": %s\n", "LE");
      } else if (strcmp(p->info, "!=") == 0) {
        printf(": %s\n", "NE");
      } else if (strcmp(p->info, ">") == 0) {
        printf(": %s\n", "GT");
      } else if (strcmp(p->info, "<") == 0) {
        printf(": %s\n", "LT");
      } else if (strcmp(p->info, "==") == 0) {
        printf(": %s\n", "EQ");
      }
#endif
    } else {
      printf("\n");
    }
  } else {
    printf("%s: Bad Type\n", p->name);
    assert(0);
  }
}