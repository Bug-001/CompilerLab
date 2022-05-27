#ifndef IR_H
#define IR_H

#include "asm.h"
#include "exp.h"
#include "prototype.h"
#include <stdio.h>

struct ir;

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
		// For const, temp, var, addr
		struct {
			struct type *value_type;
			struct value val;
			enum alloc_type alloc_type;
			int mem_loc;
			enum reg_type reg_no;
		};
	};
	struct ir *dec;
	int ref_count;
	bool can_fold;
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

struct operand *new_int_const(int i_val);

void function_translator(struct node *ext_def, struct func *func);

void optimize();

void print_one_ir(FILE *fp, struct ir *ir);
void print_ir(FILE *fp);

#endif