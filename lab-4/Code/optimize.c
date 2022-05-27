#include "config.h"
#include "ir.h"
#include "prototype.h"
#include <assert.h>
#include <stdlib.h>

extern struct ir ir_list;

struct ir *delete_ir(struct ir *del)
{
        del->prev->next = del->next;
        if (del->next)
                del->next->prev = del->prev;
        struct ir *ret = del->next;
        free(del);
        return ret;
}

bool unfold_inplace(struct ir *ir)
{
        bool changed = false;

        assert(ir->op1);
        /* e.g., x := &temp1 */
        struct operand *temp1 = ir->op1;
        if (!(temp1->type == OPERAND_TEMP || temp1->type == OPERAND_ADDR))
                goto op2_unfold;
        if (!temp1->can_fold)
                goto op2_unfold;
        struct ir *temp1_dec = temp1->dec;
        if (temp1_dec->type == IR_ASSIGN) {
                /* temp1 := y */
                struct operand *y = temp1_dec->op1;
                ir->op1 = y;
                temp1->ref_count--;
                y->ref_count++;
                unfold_inplace(ir);
                changed = true;
        }

op2_unfold:
        if (!ir->op2)
                return changed;
        struct operand *temp2 = ir->op2;
        if (!(temp2->type == OPERAND_TEMP || temp2->type == OPERAND_ADDR))
                return false;
        if (!temp2->can_fold)
                return false;
        struct ir *temp2_dec = temp2->dec;
        if (temp2_dec->type == IR_ASSIGN) {
                /* temp2 := y */
                struct operand *y = temp2_dec->op1;
                ir->op2 = y;
                temp2->ref_count--;
                y->ref_count++;
                unfold_inplace(ir);
                changed = true;
        }

        return changed;
}

bool unfold_assign(struct ir *assign)
{
        /* assign IR: x := temp */
        /* assign->op1 == temp, assign->res == x */
        struct operand *temp = assign->op1;
        if (!(temp->type == OPERAND_TEMP || temp->type == OPERAND_ADDR))
                return false;
        if (!temp->can_fold)
                return false;
        struct ir *temp_dec = temp->dec;
        switch (temp_dec->type) {
        case IR_ASSIGN:
        {
                /* temp := y */
                /* temp_dec->op1 == y, temp_dec->res == temp */
                struct operand *y = temp_dec->op1;
                assign->op1 = y;
                temp->ref_count--;
                y->ref_count++;
                unfold_assign(assign);
                break;
        }
        case IR_PLUS:
        case IR_MINUS:
        case IR_MULTIPLY:
        case IR_DIVIDE:
        {
                /* temp := y + z */
                struct operand *y = temp_dec->op1;
                struct operand *z = temp_dec->op2;
                assign->type = temp_dec->type;
                assign->op1 = y;
                assign->op2 = z;
                temp->ref_count--;
                y->ref_count++, z->ref_count++;
                unfold_inplace(assign);
                break;
        }
        case IR_REF:
        case IR_LOAD:
        case IR_STORE:
        {
                /* temp = &y */
                struct operand *y = temp_dec->op1;
                assign->type = temp_dec->type;
                assign->op1 = y;
                temp->ref_count--;
                y->ref_count++;
                unfold_inplace(assign);
                break;
        }
        case IR_READ:
        {
                /* READ temp */
                if (temp_dec->res->ref_count > 1)
                        // READ temp
                        // x := temp
                        // y := temp
                        // temp should not be optimized out
                        return false;
                // READ temp -> temp := 0
                // x := temp -> READ x
                assign->type = IR_READ;
                assign->op1 = NULL;
                temp->ref_count--;
                temp_dec->type = IR_ASSIGN;
                temp_dec->op1 = new_int_const(0);
                break;
        }
        case IR_CALL:
        {
                /* temp := CALL f */
                if (temp_dec->res->ref_count > 1)
                        return false;
                struct operand *f = temp_dec->op1;
                // temp := CALL f -> temp := 0
                // x := temp -> x := CALL f
                assign->type = IR_CALL;
                assign->op1 = f;
                temp->ref_count--;
                temp_dec->type = IR_ASSIGN;
                temp_dec->op1 = new_int_const(0);
                break;
        }
        default:
                assert(0);
        }
        return true;
}

bool unfold_arithmetic(struct ir *arith)
{
        /* x := y + z */
        struct operand *y = arith->op1;
        struct operand *z = arith->op2;
        if (y->type == OPERAND_CONST && z->type == OPERAND_CONST) {
                // XXX: only consider integer value.
                int y_val = y->val.i_val;
                int z_val = z->val.i_val;
                if (arith->type == IR_PLUS) {
                        arith->op1 = new_int_const(y_val + z_val);
                } else if (arith->type == IR_MINUS) {
                        arith->op1 = new_int_const(y_val - z_val);
                } else if (arith->type == IR_MULTIPLY) {
                        arith->op1 = new_int_const(y_val * z_val);
                } else if (arith->type == IR_DIVIDE) {
                        if (z_val == 0)
                                // Leave this bullshit to irsim!
                                return false;
                        arith->op1 = new_int_const(y_val / z_val);
                } else  {
                        assert(0);
                }
                free(y);
                free(z);
                arith->op2 = NULL;
                arith->type = IR_ASSIGN;
                return true;
        }
        if (y->type != OPERAND_CONST && z->type != OPERAND_CONST)
                // Really no way to optimize.
                return false;
        /* x := y + c     x := c + z */
        // Do some math tricks...
        struct operand *c = y->type == OPERAND_CONST ? y : z;
        struct operand *nc = y->type != OPERAND_CONST ? y : z;
        if (arith->type == IR_PLUS) {
                if (c->val.i_val == 0) {
                        /* x := nc + #0 -> x := nc */
                        arith->type = IR_ASSIGN;
                        arith->op1 = nc;
                        arith->op2 = NULL;
                        free(c);
                        return true;
                }
        } else if (arith->type == IR_MULTIPLY) {
                if (c->val.i_val == 0) {
                        /* x := nc * #0 -> x := 0 */
                        arith->type = IR_ASSIGN;
                        arith->op1 = new_int_const(0);
                        arith->op2 = NULL;
                        nc->ref_count--;
                        free(c);
                        return true;
                }
                if (c->val.i_val == 1) {
                        /* x := nc * #1 -> x := nc */
                        arith->type = IR_ASSIGN;
                        arith->op1 = nc;
                        arith->op2 = NULL;
                        free(c);
                        return true;
                }
        } else if (arith->type == IR_DIVIDE) {
                if (z == c && z->val.i_val == 1) {
                        /* x := y / #1 -> x := y */
                        arith->type = IR_ASSIGN;
                        arith->op1 = nc;
                        arith->op2 = NULL;
                        free(z);
                        return true;
                }
        } else if (arith->type == IR_MINUS) {
                if (z == c && z->val.i_val == 0) {
                        /* x := y - #0 -> x := y */
                        arith->type = IR_ASSIGN;
                        arith->op1 = nc;
                        arith->op2 = NULL;
                        free(z);
                        return true;
                }
        } else {
                assert(0);
        }
        // Math tricks failed? Try more aggressive ways...
        /* x := nc + c1, nc := w + c2 -> x := w + val(c1+c2) */
        if (!nc->can_fold)
                return false;
        struct ir *nc_dec = nc->dec;
        if (nc_dec->type != arith->type)
                return false;
        if (nc_dec->type != IR_PLUS && nc_dec->type != IR_MULTIPLY)
                return false;
        struct operand *c1 = c;
        struct operand *c2 = NULL;
        struct operand *w = NULL;
        if (nc_dec->op1->type == OPERAND_CONST) {
                c2 = nc_dec->op1;
                w = nc_dec->op2;
        } else if (nc_dec->op2->type == OPERAND_CONST) {
                w = nc_dec->op1;
                c2 = nc_dec->op2;
        }
        if (!c2)
                return false;
        struct operand *new_c;
        if (nc_dec->type == IR_PLUS)
                new_c = new_int_const(c1->val.i_val + c2->val.i_val);
        else
                new_c = new_int_const(c1->val.i_val * c2->val.i_val);
        arith->op1 = w;
        arith->op2 = new_c;
        nc->ref_count--;
        w->ref_count++;
        return true;
}

bool inverse_label(struct ir *jc)
{
        /* IF x op y GOTO label1 - jc */
        /* GOTO label2 - jmp */
        /* LABEL label1 : - another_label1 */
        /* Try to get another label1 */
        if (!jc->next || !jc->next->next)
                return false;
        struct ir *jmp = jc->next;
        struct ir *another_label1 = jmp->next;
        if (jmp->type != IR_GOTO || another_label1->type != IR_LABEL)
                return false;
        struct operand *label1 = jc->res;
        if (label1->no != another_label1->res->no)
                return false;
        // inverse jcc
        struct operand *label2 = jmp->op1;
        jc->res = label2;
        jmp->op1 = label1;
        switch (jc->type) {
        case IR_IF_LT_GOTO:
                jc->type = IR_IF_GE_GOTO;
                break;
        case IR_IF_GT_GOTO:
                jc->type = IR_IF_LE_GOTO;
                break;
        case IR_IF_LE_GOTO:
                jc->type = IR_IF_GT_GOTO;
                break;
        case IR_IF_GE_GOTO:
                jc->type = IR_IF_LT_GOTO;
                break;
        case IR_IF_EQ_GOTO:
                jc->type = IR_IF_NE_GOTO;
                break;
        case IR_IF_NE_GOTO:
                jc->type = IR_IF_EQ_GOTO;
                break;
        default:
                assert(0);
        }
        return true;
}

bool __optimize()
{
        /* Main function of optimizor. */

        struct ir *cur = ir_list.next;
        bool changed = false;
        while (cur) {
                switch (cur->type) {
                case IR_ASSIGN:
                        changed |= unfold_assign(cur);
                        break;
                case IR_PLUS:
                case IR_MINUS:
                case IR_MULTIPLY:
                case IR_DIVIDE:
                        changed |= unfold_inplace(cur);
                        changed |= unfold_arithmetic(cur);
                        break;
                case IR_REF:
                case IR_LOAD:
                case IR_STORE:
                case IR_ARG:
                case IR_RETURN:
                case IR_WRITE:
                        changed |= unfold_inplace(cur);
                        break;
	        case IR_IF_LT_GOTO:
	        case IR_IF_GT_GOTO:
	        case IR_IF_LE_GOTO:
	        case IR_IF_GE_GOTO:
	        case IR_IF_EQ_GOTO:
	        case IR_IF_NE_GOTO:
                        changed |= unfold_inplace(cur);
                        changed |= inverse_label(cur);
                        break;
                default:
                        break;
                }
                cur = cur->next;
        }

        cur = ir_list.next;
        while (cur) {
                switch (cur->type) {
                case IR_ASSIGN:
                case IR_REF:
                case IR_LOAD:
                case IR_STORE:
                        if (cur->res->ref_count == 0) {
                                cur->op1->ref_count--;
                                cur = delete_ir(cur);
                                changed = true;
                        } else
                                cur = cur->next;
                        break;
                case IR_PLUS:
                case IR_MINUS:
                case IR_MULTIPLY:
                case IR_DIVIDE:
                        if (cur->res->ref_count == 0) {
                                cur->op1->ref_count--;
                                cur->op2->ref_count--;
                                cur = delete_ir(cur);
                                changed = true;
                        } else
                                cur = cur->next;
                        break;
                case IR_DEC:
#ifndef LAB_4
                        if (cur->res->value_type->kind == TYPE_BASIC) {
                                cur = delete_ir(cur);
                                changed = true;
                        } else
                                cur = cur->next;
#else
                        cur = cur->next;
#endif
                        break;
                case IR_GOTO:
                        if (!cur->next) {
                                cur = cur->next;
                                break;
                        }
                        if (cur->next->type == IR_LABEL && cur->op1->no == cur->next->res->no) {
                                cur->next->res->ref_count--;
                                cur = delete_ir(cur);
                                changed = true;
                        } else if (cur->prev->type == IR_RETURN) {
                                cur->op1->ref_count--;
                                cur = delete_ir(cur);
                                changed = true;
                        } else
                                cur = cur->next;
                        break;
                case IR_LABEL:
                        if (cur->res->ref_count == 0) {
                                cur = delete_ir(cur);
                                changed = true;
                        } else
                                cur = cur->next;
                        break;
                default:
                        cur = cur->next;
                }
        }
        return changed;
}

void optimize()
{
        while (__optimize());
}