#include "asm.h"
#include "ir.h"
#include "prototype.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern struct ir ir_list;

const char *reg_name[MAX_NR_REG] = {
	"$zero",
	"$at",
	"$v0", "$v1",
	"$a0", "$a1", "$a2", "$a3",
	"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
	"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
	"$t8", "$t9",
	"$k0", "$k1",
	"$gp", "$sp", "$fp", "$ra"
};

static FILE *_fp = NULL;

// Global context
static int cur_mem_loc = 0;

// Function context
static int saved_reg_no = 0;
static int temp_reg_no = 0;
static struct func *cur_func = NULL;
static int nr_param = 0;
static int cur_param = 0;
static int temp_reg_occupied[8] = {0};

// Function call context within caller
static int arg_count = 0;
static int nr_additional_args = 0;

#define ival(op) op->val.i_val

void print_asm(const char *format, ...)
{
	fprintf(_fp, "    ");
	va_list args;
	va_start(args, format);
	vfprintf(_fp, format, args);
	fprintf(_fp, "\n");
}

void leave_funtion()
{
	// End the old function
	fprintf(_fp, "leave_%s:\n", cur_func->obj.id);
	if (strcmp(cur_func->obj.id, "main") != 0) {
		for (int i = 0; i < 8; ++i) {
			print_asm("lw %s, %d($sp)", reg_name[REG_S0 + i], (2*i) * 4);
			print_asm("lw %s, %d($sp)", reg_name[REG_T0 + i], (2*i+1) * 4);
		}
		print_asm("move $sp, $fp");
		print_asm("lw $fp, 0($sp)");
		print_asm("add $sp, $sp, 4");
	}
	print_asm("jr $ra");
	fprintf(_fp, "\n");
}

void enter_function()
{
	saved_reg_no = 0;
	temp_reg_no = 0;
	nr_param = cur_func->nr_args;
	cur_param = 0;
	for (int i = 0; i < 8; ++i) {
		temp_reg_occupied[i] = 0;
	}
	
	if (strcmp(cur_func->obj.id, "main") == 0) {
		fprintf(_fp, "main:\n");
		print_asm("la $v1, all");
	} else {
		fprintf(_fp, "func_%s:\n", cur_func->obj.id);
		print_asm("sub $sp, $sp, 4");
		print_asm("sw $fp, 0($sp)");
		print_asm("move $fp, $sp");
		print_asm("sub $sp, $sp, 64");
		for (int i = 0; i < 8; ++i) {
			print_asm("sw %s, %d($sp)", reg_name[REG_S0 + i], (2*i) * 4);
			print_asm("sw %s, %d($sp)", reg_name[REG_T0 + i], (2*i+1) * 4);
		}
	}
}

void store_arg_regs()
{
	int nr_store_arg_reg = nr_param > 4 ? 4 : nr_param;
	print_asm("sub $sp, $sp, %d", nr_store_arg_reg * 4);
	for (int i = 0; i < nr_store_arg_reg; ++i) {
		print_asm("sw %s, %d($sp)", reg_name[REG_A0 + i], i * 4);
	}
}

void process_one_ir(FILE *fp, struct ir *ir)
{
	struct operand *op1 = ir->op1;
	struct operand *op2 = ir->op2;
	struct operand *res = ir->res;

	const char *reg1, *reg2, *regres;
	
op1_set:
	if (!op1)
		goto op2_set;
	if (op1->alloc_type == REG_UNALLOC) {
		if (ir->type == IR_ASSIGN || ir->type == IR_PLUS ||
		    ir->type == IR_RETURN || ir->type == IR_DEC ||
		    ir->type == IR_CALL || ir->type == IR_GOTO)
			goto op2_set;
		assert(op1->type == OPERAND_CONST);
		int val = ival(op1);
		if (val == 0) {
			reg1 = reg_name[REG_ZERO];
		} else {
			reg1 = reg_name[REG_T8];
			print_asm("li %s, %d", reg1, ival(op1));
		}
	} else if (op1->alloc_type == REG_ALLOC_MEM) {
		if (ir->type != IR_REF) {
			reg1 = reg_name[REG_T8];
			print_asm("lw %s, %d(%s)", reg1, op1->mem_loc, reg_name[op1->reg_no]);
		}
	} else {
		reg1 = reg_name[op1->reg_no];
	}

op2_set:
	if (!op2)
		goto res_set;
	if (op2->alloc_type == REG_UNALLOC) {
		assert(op2->type == OPERAND_CONST);
		if (!(ir->type == IR_ASSIGN || ir->type == IR_PLUS ||
		      ir->type == IR_MINUS || ir->type == IR_DEC)) {
			int val = ival(op2);
			if (val == 0) {
				reg2 = reg_name[REG_ZERO];
			} else {
				reg2 = reg_name[REG_T9];
				print_asm("li %s, %d", reg2, val);
			}
		}
	} else if (op2->alloc_type == REG_ALLOC_MEM) {
		reg2 = reg_name[REG_T9];
		print_asm("lw %s, %d(%s)", reg2, op2->mem_loc, reg_name[op2->reg_no]);
	} else {
		reg2 = reg_name[op2->reg_no];
	}

res_set:
	if (!res || ir->type == IR_LABEL || ir->type == IR_FUNC_DEF || ir->type >= IR_IF_LT_GOTO || ir->type == IR_PARAM)
		goto gen_asm;
	if (res->alloc_type == REG_UNALLOC) {
		if (res->type == OPERAND_VAR) {
		// if (1) {
			if ((ir->type == IR_DEC && res->value_type->kind != TYPE_BASIC)
			|| saved_reg_no > 7) {
				// Must allocate memory space.
				res->alloc_type = REG_ALLOC_MEM;
				res->mem_loc = cur_mem_loc;
				res->reg_no = REG_V1;
				cur_mem_loc += res->value_type->size;
			} else {
				res->alloc_type = REG_ALLOC_REG;
				res->reg_no = (saved_reg_no++) + REG_S0;
			}
		} else {
			// allocate temp reg
			res->alloc_type = REG_ALLOC_REG;
			int i = 0;
			for (i = 0; i < 8; ++i) {
				if (temp_reg_occupied[(temp_reg_no + i) % 8] == 0)
					break;
			}
			if (i == 8) {
				// we have no way but to allocate a temp var in memory...
				res->alloc_type = REG_ALLOC_MEM;
				res->mem_loc = cur_mem_loc;
				res->reg_no = REG_V1;
				cur_mem_loc += res->value_type->size;
			} else {
				temp_reg_no = (temp_reg_no + i) % 8;
				res->reg_no = temp_reg_no + REG_T0;
				if (res->ref_count > 0)
					temp_reg_occupied[temp_reg_no] = res->no;
				temp_reg_no = (temp_reg_no + 1) % 8;
			}
		}
	}
	if (res->alloc_type == REG_ALLOC_MEM) {
		regres = reg_name[REG_T8];
	} else {
		regres = reg_name[res->reg_no];
	}

gen_asm:
	switch (ir->type) {
	case IR_LABEL:
		fprintf(_fp, "label%d:\n", res->no);
		break;
	case IR_FUNC_DEF:
		if (cur_func != NULL) {
			leave_funtion();
		}
		cur_func = res->func;
		enter_function();
		break;
	case IR_ASSIGN:
		if (op1->type == OPERAND_CONST) {
			print_asm("li %s, %d", regres, ival(op1));
		} else {
			print_asm("move %s, %s", regres, reg1);
		}
		break;
	case IR_PLUS:
		if (op1->type == OPERAND_CONST) {
			print_asm("add %s, %s, %d", regres, reg2, ival(op1));
		} else if (op2->type == OPERAND_CONST) {
			print_asm("add %s, %s, %d", regres, reg1, ival(op2));
		} else {
			print_asm("add %s, %s, %s", regres, reg1, reg2);
		}
		break;
	case IR_MINUS:
		if (op2->type == OPERAND_CONST) {
			print_asm("sub %s, %s, %d", regres, reg1, ival(op2));
		} else {
			print_asm("sub %s, %s, %s", regres, reg1, reg2);
		}
		break;
	case IR_MULTIPLY:
		print_asm("mul %s, %s, %s", regres, reg1, reg2);
		break;
	case IR_DIVIDE:
		print_asm("div %s, %s", reg1, reg2);
		print_asm("mflo %s", regres);
		break;
	case IR_REF:
		print_asm("la %s, %d(%s)", regres, op1->mem_loc, reg_name[op1->reg_no]);
		break;
	case IR_LOAD:
		print_asm("lw %s, 0(%s)", regres, reg1);
		break;
	case IR_STORE:
		print_asm("sw %s, 0(%s)", reg1, regres);
		if (res->type == OPERAND_ADDR && res->alloc_type == REG_ALLOC_REG) {
			temp_reg_occupied[res->reg_no - REG_T0] = 0;
		}
		break;
	case IR_GOTO:
		print_asm("j label%d", op1->no);
		break;
	case IR_RETURN:
		if (op1->alloc_type == REG_UNALLOC) {
			assert(op1->type == OPERAND_CONST);
			print_asm("li $v0, %d", ival(op1));
		} else if (op1->alloc_type == REG_ALLOC_REG) {
			// Return value is not stored in $v0 now
			print_asm("move $v0, %s", reg1);
		} else {
			print_asm("lw $v0, %d(%s)", op1->mem_loc, reg_name[op1->reg_no]);
		}
		print_asm("j leave_%s", cur_func->obj.id);
		break;
	case IR_ARG:
		if (arg_count == 0 && nr_param > 0) {
			store_arg_regs();
		}
		if (arg_count <= 3) {
			if (op1->reg_no >= REG_A0 && op1->reg_no <= REG_A3) {
				print_asm("lw %s, %d($sp)", reg_name[REG_A0 + arg_count], (op1->reg_no - REG_A0) * 4);
			} else {
				print_asm("move %s, %s", reg_name[REG_A0 + arg_count], reg1);
			}
		} else {
			print_asm("sub $sp, $sp, 4");
			++nr_additional_args;
			if (op1->reg_no >= REG_A0 && op1->reg_no <= REG_A3) {
				print_asm("lw $t9, %d($sp)", (op1->reg_no - REG_A0 + nr_additional_args) * 4);
				print_asm("sw $t9, 0($sp)", reg1);
			} else {
				print_asm("sw %s, 0($sp)", reg1);
			}
		}
		++arg_count;
		break;
	case IR_CALL:
		// Function store_arg_regs() may not be called. Check again.
		if (op1->func->nr_args == 0) {
			store_arg_regs();
		}
		// reset arg count
		print_asm("sub $sp, $sp, 4");
		print_asm("sw $ra, 0($sp)");
		if (strcmp(op1->func->obj.id, "main") == 0) {
			print_asm("jal main", op1->func->obj.id);
		} else {
			print_asm("jal func_%s", op1->func->obj.id);
		}
		print_asm("lw $ra, 0($sp)");
		int nr_store_arg_reg = nr_param > 4 ? 4 : nr_param;
		if (nr_store_arg_reg > 0 && res->reg_no == REG_A0) {
			print_asm("sw $v0, %d($sp)", (1 + nr_additional_args) * 4);
		} else {
			print_asm("move %s, $v0", regres);
		}
		for (int i = 0; i < nr_store_arg_reg; ++i) {
			print_asm("lw %s, %d($sp)", reg_name[REG_A0 + i], (1 + nr_additional_args + i) * 4);
		}
		print_asm("add $sp, $sp, %d", (1 + nr_additional_args + nr_store_arg_reg) * 4);
		nr_additional_args = 0;
		arg_count = 0;
		break;
	case IR_PARAM:
		if (cur_param + 4 >= nr_param) {
			// fetch from arg reg
			res->alloc_type = REG_ALLOC_REG;
			res->reg_no = REG_A0 + nr_param - 1 - cur_param;
		} else {
			res->alloc_type = REG_ALLOC_MEM;
			res->reg_no = REG_FP;
			// 68 == (1 + 8 + 8) * 4
			res->mem_loc = 8 + cur_param * 4;
		}
		++cur_param;
		return;
	case IR_DEC:
		return;
	case IR_READ:
		print_asm("li $v0, 5");
		print_asm("syscall");
		print_asm("move %s, %s", regres, reg_name[REG_V0]);
		break;
	case IR_WRITE:
		if (nr_param > 0) {
			print_asm("sub $sp, $sp, 4");
			print_asm("sw $a0, 0($sp)");
		}
		print_asm("move $a0, %s", reg1);
		print_asm("li $v0, 1");
		print_asm("syscall");
		print_asm("li $a0, %d", '\n');
		print_asm("li $v0, 11");
		print_asm("syscall");
		if (nr_param > 0) {
			print_asm("lw $a0, 0($sp)");
			print_asm("add $sp, $sp, 4");
		}
		break;
	case IR_IF_LT_GOTO:
		print_asm("blt %s, %s, label%d", reg1, reg2, res->no);
		break;
	case IR_IF_GT_GOTO:
		print_asm("bgt %s, %s, label%d", reg1, reg2, res->no);
		break;
	case IR_IF_LE_GOTO:
		print_asm("ble %s, %s, label%d", reg1, reg2, res->no);
		break;
	case IR_IF_GE_GOTO:
		print_asm("bge %s, %s, label%d", reg1, reg2, res->no);
		break;
	case IR_IF_EQ_GOTO:
		print_asm("beq %s, %s, label%d", reg1, reg2, res->no);
		break;
	case IR_IF_NE_GOTO:
		print_asm("bne %s, %s, label%d", reg1, reg2, res->no);
		break;
	default:
		break;
	}
	
	if (op1 && op1->alloc_type == REG_ALLOC_REG &&
	    (op1->type == OPERAND_ADDR || op1->type == OPERAND_TEMP)) {
		temp_reg_occupied[op1->reg_no - REG_T0] = 0;
	}
	if (op2 && op2->alloc_type == REG_ALLOC_REG &&
	    (op2->type == OPERAND_ADDR || op2->type == OPERAND_TEMP)) {
		temp_reg_occupied[op2->reg_no - REG_T0] = 0;
	}
	if (res && res->alloc_type == REG_ALLOC_MEM) {
		print_asm("sw %s, %d(%s)", regres, res->mem_loc, reg_name[res->reg_no]);
	}
}

void generate(FILE *fp)
{
	_fp = fp;
	fprintf(fp, ".globl main\n");
	fprintf(fp, ".text\n");
	struct ir *cur = ir_list.next;
	while (cur) {
		process_one_ir(fp, cur);
		cur = cur->next;
	}
	if (cur_func != NULL) {
		leave_funtion();
	}
	fprintf(fp, ".data\n");
	fprintf(fp, "all: .space %d\n", cur_mem_loc);
}