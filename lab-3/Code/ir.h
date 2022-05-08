#ifndef IR_H
#define IR_H

#include "exp.h"
#include "prototype.h"
#include <stdio.h>

struct ir;

struct temp_var {
	struct ir *dec1, *dec2, *used;
};

struct label {
	struct ir *dec;
	int ref_count;
};

enum operand_type {
	OPERAND_DUMMY = 0,
	OPERAND_VAR,
	OPERAND_ADDR,
	OPERAND_CONST,
	OPERAND_TEMP,
	OPERAND_FUNC,
	OPERAND_LABEL
};

struct operand {
	enum operand_type type;
	int no;
	union {
		// For func
		struct func *func;
		// For label
		struct label label;
		// For const, temp, var, addr
		struct {
			struct type *value_type;
			union {
				// For const
				struct value val;
				// For temp
				struct temp_var temp;
			};
		};
	};
};

enum ir_type {
	IR_DUMMY = 0,
	IR_LABEL, 	// LABEL x :
	IR_FUNC_DEF, 	// FUNCTION f :
	IR_ASSIGN, 	// x := y
	IR_PLUS, 	// x := y + z
	IR_MINUS, 	// x := y - z
	IR_MULTIPLY, 	// x := y * z
	IR_DIVIDE, 	// x := y / z
	IR_REF, 	// x := &y
	IR_LOAD, 	// x := *y
	IR_STORE, 	// *x := y
	IR_GOTO, 	// GOTO x
	IR_RETURN, 	// RETURN x
	IR_DEC, 	// DEC x [size]
	IR_ARG, 	// ARG x
	IR_CALL, 	// x := CALL f
	IR_PARAM, 	// PARAM x
	IR_READ, 	// READ x
	IR_WRITE, 	// WRITE x

	// do not append more types after this comment
	IR_IF_LT_GOTO,
	IR_IF_GT_GOTO,
	IR_IF_LE_GOTO,
	IR_IF_GE_GOTO,
	IR_IF_EQ_GOTO,
	IR_IF_NE_GOTO,
	IR_NOT,
	IR_AND,
	IR_OR
};

struct ir {
	enum ir_type type;
	struct operand *op1;
	struct operand *op2;
	struct operand *res;
	struct ir *prev, *next;
};

struct ir_seg {
	struct ir *head;
	struct ir *tail;
};

struct ir *get_ir(enum ir_type type, struct operand *op1,
		     struct operand *op2, struct operand *res);

// struct ir *get_ir_param(struct type *sym_type, int sym_no);
// struct ir *get_ir_def_func(struct func *func);

void function_translator(struct node *ext_def, struct func *func);

void print_one_ir(FILE *fp, struct ir *ir);
void print_ir(FILE *fp);

#endif