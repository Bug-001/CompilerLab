#include "config.h"
#include "ir.h"
#include "node.h"
#include "semantic.h"
#include "syntax.tab.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

extern int yyparse(void);
extern void yyrestart(FILE *input_file);

#define fatal_error(info) printf("%s: fatal error: %s\n", argv[0], (info))

int has_error = 0;
int translate_fail = 0;

int main(int argc, char **argv)
{
	if (argc == 1) {
		fatal_error("no input file");
		goto terminated;
	} else if (argc > 3) {
		fatal_error("usage: parser in.cmm [out.ir]");
		goto terminated;
	}
	FILE *f = fopen(argv[1], "r");
	if (!f) {
		fatal_error(strerror(errno));
		goto terminated;
	}

	yyrestart(f);
	yyparse();
	if (has_error)
		goto out;
#ifdef LAB_1
	print_tree(tree);
	goto out;
#else
	has_error = 0;
	semantic_analysis(tree);
	if (has_error || translate_fail)
		goto out;
	optimize();
#endif

#ifdef LAB_3
	FILE *ir_out;
	if (argc == 2)
		ir_out = stdout;
	else
		ir_out = fopen(argv[2], "wt");
	print_ir(ir_out);
	// FILE *fp_out = fopen("out.ir", "wt");
	// print_ir(fp_out);
#endif

	return 0;

terminated:
	printf("compilation terminated.\n");

out:
	return 0;
}