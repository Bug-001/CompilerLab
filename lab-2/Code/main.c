#include "node.h"
#include "semantic.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

extern int yyparse(void);
extern void yyrestart(FILE* input_file);

#define fatal_error(info) \
  printf("%s: fatal error: %s\n", argv[0], (info))

int has_error = 0;

int main(int argc, char** argv) {
  if (argc == 1) {
    fatal_error("no input file");
    goto terminated;
  } else if (argc > 2) {
    fatal_error("only support one input file");
    goto terminated;
  }
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    fatal_error(strerror(errno));
    goto terminated;
  }

  yyrestart(f);
  yyparse();
  if (has_error)
    goto error_out;
  print_tree(tree);

  has_error = 0;
  semantic_analysis(tree);
  if (has_error)
    goto error_out;

  free_node(tree);
  return 0;

terminated:
  printf("compilation terminated.\n");

error_out:
  return -1;
}