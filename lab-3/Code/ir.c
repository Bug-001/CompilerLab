#include "ir.h"
#include "exp.h"
#include "node.h"
#include "prototype.h"
#include "syntax.tab.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ir ir_list = { .type = IR_DUMMY, .prev = NULL, .next = NULL };
struct ir *ir_tail = &ir_list;

int label_cnt = 0;
int temp_cnt = 0;

struct value zero = { .i_val = 0 };
struct value one = { .i_val = 1 };

struct ir_seg declaration_translator(struct node *dec, struct symbol *sym);
struct ir_seg statement_translator(struct node *stmt);
struct ir_seg comp_st_translator(struct node *comp_st);
struct ir_seg expression_translator(struct node *exp, struct operand *place);
struct ir_seg expression_translator_1(struct node *exp, struct operand *place);
struct ir_seg expression_translator_2(struct node *exp, struct operand *place);
struct ir_seg expression_translator_3(struct node *exp, struct operand *place);
struct ir_seg expression_translator_4(struct node *exp, struct operand *place);
struct ir_seg condition_translator(struct node *exp, struct operand *label_true, struct operand *label_false);
struct ir_seg function_call_translator(struct node *exp, struct operand *place);

static struct operand *new_temp(struct type *type)
{
	struct operand *tmp = malloc(sizeof(struct operand));
	tmp->type = OPERAND_TEMP;
	tmp->no = temp_cnt++;
	tmp->value_type = type;
	tmp->temp.dec1 = NULL;
	tmp->temp.dec2 = NULL;
	tmp->temp.used = NULL;

	return tmp;
}

static struct operand *new_var(struct type *sym_type, int sym_no)
{
	struct operand *var = malloc(sizeof(struct operand));
	var->type = OPERAND_VAR;
	var->no = sym_no;
	var->value_type = sym_type;

	return var;
}

static struct operand *new_addr(struct type *sym_type)
{
	struct operand *addr = malloc(sizeof(struct operand));
	addr->type = OPERAND_ADDR;
	addr->value_type = sym_type;

	return addr;
}

static struct operand *new_label()
{
	struct operand *new_lbl = malloc(sizeof(struct operand));
	new_lbl->type = OPERAND_LABEL;
	new_lbl->no = label_cnt++;
	new_lbl->label.ref_count = 0;
	new_lbl->label.dec = NULL;

	return new_lbl;
}

static struct operand *new_const(struct type *type, struct value val)
{
	struct operand *const_val = malloc(sizeof(struct operand));
	const_val->type = OPERAND_CONST;
	const_val->value_type = type;
	const_val->val = val;

	return const_val;
}

static struct operand *new_func(struct func *func)
{
	struct operand *op_func = malloc(sizeof(struct operand));
	op_func->type = OPERAND_FUNC;
	op_func->func = func;

	return op_func;
}

#define zero_const new_const(get_int_type(), zero)
#define one_const new_const(get_int_type(), one)

enum ir_type node_to_irtype(struct node *node)
{
	switch (node->ntype) {
	case ASSIGNOP:
		return IR_ASSIGN;
	case PLUS:
		return IR_PLUS;
	case MINUS:
		return IR_MINUS;
	case STAR:
		return IR_MULTIPLY;
	case DIV:
		return IR_DIVIDE;
	case RELOP:
		switch (node->lattr.relop_type) {
		case LT:
			return IR_IF_LT_GOTO;
		case GT:
			return IR_IF_GT_GOTO;
		case LE:
			return IR_IF_LE_GOTO;
		case GE:
			return IR_IF_GE_GOTO;
		case EQ:
			return IR_IF_EQ_GOTO;
		case NE:
			return IR_IF_NE_GOTO;
		default:
			assert(0);
		}
	case AND:
		return IR_AND;
	case OR:
		return IR_OR;
	case NOT:
		return IR_NOT;
	default:
		return IR_DUMMY;
	}
}

void ir_seg_append_seg(struct ir_seg *seg, struct ir_seg new_seg)
{
	if (seg->head == NULL) {
		seg->head = new_seg.head;
		seg->tail = new_seg.tail;
		return;
	}
	seg->tail->next = new_seg.head;
	seg->tail = new_seg.tail;
}

void ir_seg_append_ir(struct ir_seg *seg, struct ir *new_ir)
{
	if (seg->head == NULL) {
		seg->head = new_ir;
		seg->tail = new_ir;
		return;
	}
	seg->tail->next = new_ir;
	seg->tail = new_ir;
}

void ir_seg_create_ir(struct ir_seg *seg, enum ir_type type, struct operand *op1,
		     struct operand *op2, struct operand *res)
{
	struct ir *new_ir = get_ir(type, op1, op2, res);
	ir_seg_append_ir(seg, new_ir);
}

void ir_commit(struct ir_seg *seg)
{
	ir_tail->next = seg->head;
	ir_tail = seg->tail;
}

struct ir *get_ir(enum ir_type type, struct operand *op1,
		     struct operand *op2, struct operand *res)
{
	struct ir *new_ir = malloc(sizeof(struct ir));
	new_ir->type = type;
	new_ir->op1 = op1;
	new_ir->op2 = op2;
	new_ir->res = res;

	// struct ir **append_tail = ir_stack_top ? &ir_stack_top->tail : &ir_tail;

	// (*append_tail)->next = new_ir;
	// new_ir->prev = *append_tail;
	// new_ir->next = NULL;
	// *append_tail = new_ir;

	// print_one_ir(stdout, new_ir);
	// printf("\n");

	// if (op1 && op1->type == OPERAND_TEMP)
	// TODO: set used and dec to enable optimization

	return new_ir;
}

struct ir *get_ir_label(struct operand *label)
{
	struct ir *new_ir = get_ir(IR_LABEL, NULL, NULL, label);
	label->label.dec = new_ir;
	return new_ir;
}

struct ir *get_ir_goto(struct operand *label)
{
	struct ir *new_ir = get_ir(IR_GOTO, label, NULL, NULL);
	label->label.ref_count++;
	return new_ir;
}

struct ir *get_ir_def_func(struct func *func)
{
	struct operand *op_func = malloc(sizeof(struct operand));
	op_func->type = OPERAND_FUNC;
	op_func->func = func;

	return get_ir(IR_FUNC_DEF, NULL, NULL, op_func);
}

struct ir *get_ir_param(struct type *sym_type, int sym_no)
{
	struct operand *op_param = new_var(sym_type, sym_no);
	return get_ir(IR_PARAM, NULL, NULL, op_param);
}

struct ir *get_ir_dec_var(struct type *sym_type, int sym_no)
{
	struct operand *op_local_var = new_var(sym_type, sym_no);
	struct value var_size = {.i_val = sym_type->size};
	return get_ir(IR_DEC, new_const(get_int_type(), var_size), NULL, op_local_var);
}

void function_translator(struct node *ext_def, struct func *func)
{
	struct ir_seg seg = {0};
	/* ExtDef -> Specifier FunDec CompSt */
	struct node *comp_st = ext_def->children[2];

	ir_seg_append_ir(&seg, get_ir_def_func(func));

	struct var_list *param_list = func->args;
	while (param_list && param_list->pred) {
		param_list = param_list->pred;
	}
	while (param_list) {
		ir_seg_append_ir(&seg, get_ir_param(param_list->type, param_list->var_no));
		param_list = param_list->succ;
	}

	ir_seg_append_seg(&seg, comp_st_translator(comp_st));

	/* End the story. */
	ir_commit(&seg);
}

struct ir_seg declaration_translator(struct node *dec, struct symbol *sym)
{
	/* Dec -> VarDec */
	/* Dec -> VarDec ASSIGNOP Exp */
	struct type *sym_type = sym->type;
	int sym_no = sym->var_no;
	struct operand *op_local_var = new_var(sym_type, sym_no);
	struct ir_seg seg = {0};
	if (sym->type->kind != TYPE_BASIC) {
		/* DEC x [size] */
		struct value var_size = {.i_val = sym_type->size};
		ir_seg_create_ir(&seg, IR_DEC, new_const(get_int_type(), var_size), NULL, op_local_var);
	}
	if (dec->nr_children == 3) {
		struct operand *tmp = new_temp(sym->type);
		ir_seg_append_seg(&seg, expression_translator(dec->children[2], tmp));
		ir_seg_create_ir(&seg, IR_ASSIGN, tmp, NULL, op_local_var);
	}

	return seg;
}

struct ir_seg comp_st_translator(struct node *comp_st)
{
	struct ir_seg seg = {0};

	/* CompSt -> LC DefList StmtList RC */
	/* Analyse DefList */
	struct node *def_list = comp_st->children[1];
	while (def_list->nr_children) {
		/* DefList -> Def DefList */
		struct node *def = def_list->children[0];
		def_list = def_list->children[1];
		/* Def -> Specifier DecList SEMI */
		struct node *dec_list = def->children[1];
		while (true) {
			/* DecList -> Dec COMMA DecList */
			/* DecList -> Dec */
			struct node *dec = dec_list->children[0];
			/* Dec -> VarDec */
			/* Dec -> VarDec ASSIGNOP Exp */
			struct node *var_dec = dec->children[0];
			while (var_dec->nr_children == 4) {
				/* VarDec -> VarDec LB INT RB */
				var_dec = var_dec->children[0];
			}
			/* VarDec -> ID */
			struct symbol *sym = search_symbol(var_dec->children[0]->lattr.info);
			assert(sym);
			ir_seg_append_seg(&seg, declaration_translator(dec, sym));
			if (dec_list->nr_children == 3)
				dec_list = dec_list->children[2];
			else
				break;
		}
	}

	/* Analyse StmtList */
	struct node *stmt_list = comp_st->children[2];
	while (stmt_list->nr_children) {
		/* StmtList -> Stmt StmtList */
		struct node *stmt = stmt_list->children[0];
		stmt_list = stmt_list->children[1];
		ir_seg_append_seg(&seg, statement_translator(stmt));
	}

	return seg;
}

struct ir_seg statement_translator(struct node *stmt)
{
	struct ir_seg seg = {0};
	assert(stmt->ntype == STMT);
	if (stmt->nr_children == 2) {
		/* Stmt -> Exp SEMI */
		struct operand *tmp = new_temp(stmt->children[0]->p_sattr->type);
		ir_seg_append_seg(&seg, expression_translator(stmt->children[0], tmp));
	} else if (stmt->nr_children == 1) {
		/* Stmt -> CompSt */
		ir_seg_append_seg(&seg, comp_st_translator(stmt->children[0]));
	} else if (stmt->nr_children == 3) {
		/* Stmt -> RETURN Exp SEMI */
		struct node *exp = stmt->children[1];
		assert(exp->p_sattr);
		struct operand *tmp = new_temp(exp->p_sattr->type);
		ir_seg_append_seg(&seg, expression_translator(exp, tmp));
		ir_seg_create_ir(&seg, IR_RETURN, tmp, NULL, NULL);
	} else if (stmt->children[0]->ntype == IF && stmt->nr_children == 5) {
		/* Stmt -> IF LP Exp RP Stmt */
		struct node *exp = stmt->children[2];
		struct operand *label1 = new_label();
		struct operand *label2 = new_label();
		ir_seg_append_seg(&seg, condition_translator(exp, label1, label2));
		ir_seg_append_ir(&seg, get_ir_label(label1));
		ir_seg_append_seg(&seg, statement_translator(stmt->children[4]));
		ir_seg_append_ir(&seg, get_ir_label(label2));
	} else if (stmt->children[0]->ntype == IF && stmt->nr_children == 7) {
		/* Stmt -> IF LP Exp RP Stmt ELSE Stmt */
		struct node *exp = stmt->children[2];
		struct operand *label1 = new_label();
		struct operand *label2 = new_label();
		struct operand *label3 = new_label();
		ir_seg_append_seg(&seg, condition_translator(exp, label1, label2));
		ir_seg_append_ir(&seg, get_ir_label(label1));
		ir_seg_append_seg(&seg, statement_translator(stmt->children[4]));
		ir_seg_append_ir(&seg, get_ir_goto(label3));
		ir_seg_append_ir(&seg, get_ir_label(label2));
		ir_seg_append_seg(&seg, statement_translator(stmt->children[6]));
		ir_seg_append_ir(&seg, get_ir_label(label3));
	} else if (stmt->children[0]->ntype == WHILE) {
		/* Stmt -> WHILE LP Exp RP Stmt */
		struct node *exp = stmt->children[2];
		struct operand *label1 = new_label();
		struct operand *label2 = new_label();
		struct operand *label3 = new_label();
		ir_seg_append_ir(&seg, get_ir_label(label1));
		ir_seg_append_seg(&seg, condition_translator(exp, label2, label3));
		ir_seg_append_ir(&seg, get_ir_label(label2));
		ir_seg_append_seg(&seg, statement_translator(stmt->children[4]));
		ir_seg_append_ir(&seg, get_ir_goto(label1));
		ir_seg_append_ir(&seg, get_ir_label(label3));
	} else {
		assert(0);
	}
	return seg;
}

struct ir_seg expression_translator(struct node *exp, struct operand *place)
{
	switch (exp->nr_children) {
	case 1:
		return expression_translator_1(exp, place);
	case 2:
		return expression_translator_2(exp, place);
	case 3:
		return expression_translator_3(exp, place);
	case 4:
		return expression_translator_4(exp, place);
	default:
		assert(0);
	}
}

inline struct ir_seg expression_translator_1(struct node *exp, struct operand *place)
{
	struct ir_seg seg = {0};
	struct exp_attr *attr = exp->p_sattr;
	if (exp->children[0]->ntype == ID) {
		/* EXP -> ID */
		struct operand *var = new_var(attr->type, attr->sym_no);
		if (attr->type->kind == TYPE_BASIC)
			ir_seg_create_ir(&seg, IR_ASSIGN, var, NULL, place);
		else
		 	ir_seg_create_ir(&seg, IR_REF, var, NULL, place);
		return seg;
	}
	/* EXP -> INT | FLOAT */
	struct operand *const_val =
		new_const(attr->type, exp->children[0]->lattr.value);
	ir_seg_create_ir(&seg, IR_ASSIGN, const_val, NULL, place);
	return seg;
}

inline struct ir_seg expression_translator_2(struct node *exp, struct operand *place)
{
	struct ir_seg seg = {0};
	struct exp_attr *attr = exp->p_sattr;
	/* EXP -> MINUS EXP | NOT EXP */
	if (exp->children[0]->ntype == MINUS) {
		struct operand *tmp = new_temp(attr->type);
		ir_seg_append_seg(&seg, expression_translator_1(exp->children[1], tmp));
		ir_seg_create_ir(&seg, IR_MINUS, zero_const, tmp, place);
	} else if (exp->children[0]->ntype == NOT) {
		struct operand *label1 = new_label();
		struct operand *label2 = new_label();
		ir_seg_create_ir(&seg, IR_ASSIGN, zero_const, NULL, place);
		ir_seg_append_seg(&seg, condition_translator(exp, label1, label2));
		ir_seg_append_ir(&seg, get_ir_label(label1));
		ir_seg_create_ir(&seg, IR_ASSIGN, one_const, NULL, place);
		ir_seg_append_ir(&seg, get_ir_label(label2));
	} else {
		assert(0);
	}
	return seg;
}

inline struct ir_seg expression_translator_3(struct node *exp, struct operand *place)
{
	struct ir_seg seg = {0};
	/* Assignment and Arithmetic operations */
	if (exp->children[0]->ntype == EXP && exp->children[2]->ntype == EXP) {
		struct node *exp1 = exp->children[0];
		struct node *op = exp->children[1];
		struct node *exp2 = exp->children[2];
		assert(type_eq_arithmetic(exp1->p_sattr->type, exp2->p_sattr->type));
		if (op->ntype == ASSIGNOP) {
			struct operand *tmp = new_temp(exp->p_sattr->type);
			struct operand *var = new_var(exp1->p_sattr->type, exp1->p_sattr->sym_no);
			ir_seg_append_seg(&seg, expression_translator(exp2, tmp));
			// TODO: if var is field or array index, do dereference here.
			ir_seg_create_ir(&seg, IR_ASSIGN, tmp, NULL, var);
			ir_seg_create_ir(&seg, IR_ASSIGN, var, NULL, place);
			return seg;
		}
		if (op->ntype == RELOP || op->ntype == AND || op->ntype == OR) {
			/* Condition operations */
			struct operand *label1 = new_label();
			struct operand *label2 = new_label();
			ir_seg_create_ir(&seg, IR_ASSIGN, zero_const, NULL, place);
			ir_seg_append_seg(&seg, condition_translator(exp, label1, label2));
			ir_seg_append_ir(&seg, get_ir_label(label1));
			ir_seg_create_ir(&seg, IR_ASSIGN, one_const, NULL, place);
			ir_seg_append_ir(&seg, get_ir_label(label2));
			return seg;
		}
		/* Arithmetic operations */
		enum ir_type ir_type = node_to_irtype(op);
		struct operand *tmp1 = new_temp(exp1->p_sattr->type);
		struct operand *tmp2 = new_temp(exp2->p_sattr->type);
		ir_seg_append_seg(&seg, expression_translator(exp1, tmp1));
		ir_seg_append_seg(&seg, expression_translator(exp2, tmp2));
		ir_seg_create_ir(&seg, ir_type, tmp1, tmp2, place);
		return seg;
	}

	/* Exp -> LP Exp RP */
	if (exp->children[0]->ntype == LP) {
		ir_seg_append_seg(&seg, expression_translator(exp->children[1], place));
		return seg;
	}

	/* Exp -> Exp DOT ID */
	if (exp->children[1]->ntype == DOT) {
		struct node *struc = exp->children[0];
		struct node *field_id = exp->children[2];
		struct var_list *field = search_field(field_id->lattr.info, exp->p_sattr->type);
		struct operand *base_addr = new_addr(struc->p_sattr->type);
		struct value offset_val = {.i_val = field->offset};
		struct operand *offset = new_const(get_int_type(), offset_val);
		ir_seg_append_seg(&seg, expression_translator(struc, base_addr));
		if (field->type->kind == TYPE_BASIC) {
			/* Dereference can be exceuted. */
			struct operand *var_addr = new_addr(field->type);
			ir_seg_create_ir(&seg, IR_PLUS, base_addr, offset, var_addr);
			ir_seg_create_ir(&seg, IR_LOAD, var_addr, NULL, place);
		} else {
			ir_seg_create_ir(&seg, IR_PLUS, base_addr, offset, place);
		}
		
		return seg;
	}

	/* Exp -> ID LP RP */
	if (exp->children[0]->ntype == ID) {
		ir_seg_append_seg(&seg, function_call_translator(exp, place));
		return seg;
	}

	assert(0);
}

inline struct ir_seg expression_translator_4(struct node *exp, struct operand *place)
{
	struct ir_seg seg = {0};
	/* Exp -> ID LP Args RP */
	if (exp->children[0]->ntype == ID) {
		ir_seg_append_seg(&seg, function_call_translator(exp, place));
		return seg;
	}

	/* Exp -> Exp LB Exp RB */
	if (exp->children[1]->ntype == LB) {
		struct node *arr = exp->children[0];
		struct node *idx = exp->children[2];

		struct operand *base_addr = new_addr(arr->p_sattr->type);
		struct operand *tmp_idx = new_temp(get_int_type());
		struct value elem_size_val = {.i_val = exp->p_sattr->type->size};
		struct operand *elem_size = new_const(get_int_type(), elem_size_val);
		struct operand *offset = new_temp(get_int_type());

		ir_seg_append_seg(&seg, expression_translator(arr, base_addr));
		ir_seg_append_seg(&seg, expression_translator(idx, tmp_idx));
		ir_seg_create_ir(&seg, IR_MULTIPLY, tmp_idx, elem_size, offset);
		
		if (exp->p_sattr->type == TYPE_BASIC) {
			/* Dereference can be exceuted. */
			struct operand *var_addr = new_addr(exp->p_sattr->type);
			ir_seg_create_ir(&seg, IR_PLUS, base_addr, offset, var_addr);
			ir_seg_create_ir(&seg, IR_LOAD, var_addr, NULL, place);
		} else {
			ir_seg_create_ir(&seg, IR_PLUS, base_addr, offset, place);
		}

		return seg;
	}

	assert(0);
}

struct ir_seg function_call_translator(struct node *exp, struct operand *place)
{
	struct ir_seg seg = {0};
	struct arg_temp_list {
		struct operand *arg;
		struct arg_temp_list *prev;
	};

	assert(exp->children[0]->ntype == ID);
	struct func *func = search_func(exp->children[0]->lattr.info);
	assert(func);

	/* Exp -> ID LP RP */
	if (exp->nr_children == 3) {
		if (strcmp(func->obj.id, "read") == 0) {
			ir_seg_create_ir(&seg, IR_READ, NULL, NULL, place);
		} else {
			ir_seg_create_ir(&seg, IR_CALL, new_func(func), NULL, place);
		}
		return seg;
	}

	/* Exp -> ID LP Args RP */
	struct arg_temp_list *cur = NULL;
	struct node *args = exp->children[2];
	assert(args->ntype == ARGS);
	while (true) {
		/* Args-> Exp COMMA Args */
		struct node *arg = args->children[0];
		struct operand *tmp = new_temp(arg->p_sattr->type);
		struct arg_temp_list *list_node = malloc(sizeof(struct arg_temp_list));
		ir_seg_append_seg(&seg, expression_translator(arg, tmp));
		list_node->arg = tmp;
		list_node->prev = cur;
		cur = list_node;

		if (args->nr_children == 3)
			args = args->children[2];
		else
			break;
	}

	if (strcmp(func->obj.id, "write") == 0) {
		ir_seg_create_ir(&seg, IR_WRITE, cur->arg, NULL, NULL);
		ir_seg_create_ir(&seg, IR_ASSIGN, zero_const, NULL, place);

		struct arg_temp_list *del = cur;
		cur = cur->prev;
		assert(cur == NULL);
		free(del);

		return seg;
	}

	while (cur != NULL) {
		ir_seg_create_ir(&seg, IR_ARG, cur->arg, NULL, NULL);

		struct arg_temp_list *del = cur;
		cur = cur->prev;
		free(del);
	}
	ir_seg_create_ir(&seg, IR_CALL, new_func(func), NULL, place);

	return seg;
}

struct ir_seg condition_translator(struct node *exp, struct operand *label_true,
			  struct operand *label_false)
{
	struct ir_seg seg = {0};
	struct exp_attr *attr = exp->p_sattr;
	if (exp->children[0]->ntype == NOT) {
		/* Exp -> NOT Exp */
		condition_translator(exp->children[1], label_false, label_true);
	} else if (exp->children[1]->ntype == RELOP) {
		/* Exp -> Exp RELOP Exp */
		struct node *exp1 = exp->children[0];
		struct node *op = exp->children[1];
		struct node *exp2 = exp->children[2];
		struct operand *t1 = new_temp(attr->type);
		struct operand *t2 = new_temp(attr->type);
		enum ir_type rel_type = node_to_irtype(op);
		ir_seg_append_seg(&seg, expression_translator(exp1, t1));
		ir_seg_append_seg(&seg, expression_translator(exp2, t2));
		ir_seg_create_ir(&seg, rel_type, t1, t2, label_true);
		ir_seg_append_ir(&seg, get_ir_goto(label_false));
	} else if (exp->children[1]->ntype == AND) {
		struct operand *label = new_label();
		ir_seg_append_seg(&seg, condition_translator(exp->children[0], label, label_false));
		ir_seg_append_ir(&seg, get_ir_label(label));
		ir_seg_append_seg(&seg, condition_translator(exp->children[2], label_true, label_false));
	} else if (exp->children[1]->ntype == OR) {
		struct operand *label = new_label();
		ir_seg_append_seg(&seg, condition_translator(exp->children[0], label_true, label));
		ir_seg_append_ir(&seg, get_ir_label(label));
		ir_seg_append_seg(&seg, condition_translator(exp->children[2], label_true, label_false));
	} else {
		struct operand *tmp = new_temp(get_int_type());
		ir_seg_append_seg(&seg, expression_translator(exp, tmp));
		ir_seg_create_ir(&seg, IR_IF_NE_GOTO, tmp, zero_const, label_true);
		ir_seg_append_ir(&seg, get_ir_goto(label_false));
	}
	return seg;
}

void operand_set_buf(char *buf, struct operand *op)
{
	if (!op) {
		// But NULL should not be printed out.
		strcpy(buf, "NULL");
		return;
	}
	switch (op->type) {
	case OPERAND_FUNC:
		strcpy(buf, op->func->obj.id);
		return;
	case OPERAND_LABEL:
		sprintf(buf, "label%d", op->no);
		return;
	case OPERAND_CONST:
		if (op->value_type == get_int_type())
			sprintf(buf, "#%d", op->val.i_val);
		else
		 	sprintf(buf, "#%f", op->val.f_val);
		return;
	case OPERAND_TEMP:
		sprintf(buf, "temp%d", op->no);
		return;
	case OPERAND_VAR:
		sprintf(buf, "var%d", op->no);
		return;
	case OPERAND_ADDR:
		sprintf(buf, "addr%d", op->no);
		return;
	case OPERAND_DUMMY:
		strcpy(buf, "BAD");
	}
}

void print_one_ir(FILE *fp, struct ir *ir)
{
	char buf1[32], buf2[32], bufr[32];
	operand_set_buf(buf1, ir->op1);
	operand_set_buf(buf2, ir->op2);
	operand_set_buf(bufr, ir->res);
	switch (ir->type) {
	case IR_LABEL:
		fprintf(fp, "LABEL %s :", bufr);
		break;
	case IR_FUNC_DEF:
		fprintf(fp, "FUNCTION %s :", bufr);
		break;
	case IR_ASSIGN:
		fprintf(fp, "%s := %s", bufr, buf1);
		break;
	case IR_PLUS:
		fprintf(fp, "%s := %s + %s", bufr, buf1, buf2);
		break;
	case IR_MINUS:
		fprintf(fp, "%s := %s - %s", bufr, buf1, buf2);
		break;
	case IR_MULTIPLY:
		fprintf(fp, "%s := %s * %s", bufr, buf1, buf2);
		break;
	case IR_DIVIDE:
		fprintf(fp, "%s := %s / %s", bufr, buf1, buf2);
		break;
	case IR_REF:
		fprintf(fp, "%s := &%s", bufr, buf1);
		break;
	case IR_LOAD:
		fprintf(fp, "%s := *%s", bufr, buf1);
		break;
	case IR_STORE:
		fprintf(fp, "*%s := %s", bufr, buf1);
		break;
	case IR_GOTO:
		fprintf(fp, "GOTO %s", buf1);
		break;
	case IR_RETURN:
		fprintf(fp, "RETURN %s", buf1);
		break;
	case IR_DEC:
		fprintf(fp, "DEC %s %s", bufr, buf1 + 1);
		break;
	case IR_ARG:
		fprintf(fp, "ARG %s", buf1);
		break;
	case IR_CALL:
		fprintf(fp, "%s := CALL %s", bufr, buf1);
		break;
	case IR_PARAM:
		fprintf(fp, "PARAM %s", bufr);
		break;
	case IR_READ:
		fprintf(fp, "READ %s", bufr);
		break;
	case IR_WRITE:
		fprintf(fp, "WRITE %s", buf1);
		break;
	case IR_IF_LT_GOTO:
		fprintf(fp, "IF %s < %s GOTO %s", buf1, buf2, bufr);
		break;
	case IR_IF_GT_GOTO:
		fprintf(fp, "IF %s > %s GOTO %s", buf1, buf2, bufr);
		break;
	case IR_IF_LE_GOTO:
		fprintf(fp, "IF %s <= %s GOTO %s", buf1, buf2, bufr);
		break;
	case IR_IF_GE_GOTO:
		fprintf(fp, "IF %s >= %s GOTO %s", buf1, buf2, bufr);
		break;
	case IR_IF_EQ_GOTO:
		fprintf(fp, "IF %s == %s GOTO %s", buf1, buf2, bufr);
		break;
	case IR_IF_NE_GOTO:
		fprintf(fp, "IF %s != %s GOTO %s", buf1, buf2, bufr);
		break;
	default:
		assert(0);
	}
}

void print_ir(FILE *fp)
{
	struct ir *cur = ir_list.next;
	while (cur) {
		print_one_ir(fp, cur);
		fprintf(fp, "\n");
		cur = cur->next;
	}
}