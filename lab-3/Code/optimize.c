#include "ir.h"
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

// if cond is true, then return delete_ir(del);
// else return del->next
struct ir *delete_ir_if(struct ir *del, bool cond)
{
        if (cond) return delete_ir(del);
        return del->next;
}

void unfold_op1(struct ir *ir);
void addr_1_folding(struct ir *cur);
void addr_2_folding(struct ir *cur);
void addr_3_folding(struct ir *cur);

void unfold_op1(struct ir *ir)
{
        /*
         * e.g.:
         * temp := x
         * RETURN temp
         */
        if (ir->op1->type == OPERAND_CONST)
                // Could not unfold more.
                return;
        // temp := x
        struct ir *temp_dec = ir->op1->dec;
        if (!temp_dec->res->can_fold)
                return;
        if (temp_dec->type == IR_ASSIGN) {
                // RETURN temp -> RETURN x
                ir->op1 = temp_dec->op1;
                // temp ref_count--
                temp_dec->res->ref_count--;
                // x ref_count++
                ir->op1->ref_count++;
                unfold_op1(ir);
        }
}



void addr_1_folding(struct ir *cur)
{
        switch (cur->type) {
        case IR_RETURN:
        case IR_ARG:
        case IR_WRITE:
                unfold_op1(cur);
                break;
        default:
                break;
        }
}

void addr_2_folding(struct ir *cur)
{
        struct ir *temp_dec;
        switch (cur->type) {
        case IR_ASSIGN:
                // x := temp
                if (cur->op1->type == OPERAND_CONST)
                        break;
                temp_dec = cur->op1->dec;
                if (!temp_dec->res->can_fold)
                        break;
                if (temp_dec->type == IR_ASSIGN || temp_dec->type == IR_REF || temp_dec->type == IR_LOAD) {
                        // eliminate unit assign
                        // temp := y
                        // x := temp -> x := y
                        cur->op1 = temp_dec->op1;
                        cur->type = temp_dec->type;
                        // temp ref_count--
                        temp_dec->res->ref_count--;
                        // x ref_count++
                        cur->op1->ref_count++;
                        addr_2_folding(cur);
                } else if (temp_dec->type == IR_READ) {
                        // READ temp -> temp := 0
                        // x := temp -> READ x
                        if (temp_dec->res->ref_count > 1)
                                // READ temp1
                                // temp2 := temp1
                                // x := temp1
                                // temp1 should not be optimized
                                break;
                        temp_dec->type = IR_ASSIGN;
                        temp_dec->op1 = new_int_const(0);
                        cur->type = IR_READ;
                        cur->op1->ref_count--;
                        cur->op1 = NULL;
                }
                break;
        case IR_REF:
        case IR_LOAD:
        case IR_STORE:
                break;
        }
}

void addr_3_folding(struct ir *cur)
{

}

void (*addr_folding[4])(struct ir *) = {
        NULL, addr_1_folding, addr_2_folding, addr_3_folding
};

void peephole_optimization(struct ir *cur)
{

}

bool unfold_inplace(struct ir *ir)
{
        return false;
}

bool unfold_assign(struct ir *assign)
{
        /* assign IR: x := temp */
        /* assign->op1 == temp, assign->res == x */
        struct operand *x = assign->res;
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
        return false;
}

bool unfold_dec(struct ir *dec)
{
        return false;
}

bool inverse_label(struct ir *jc)
{
        return false;
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
                case IR_READ:
                case IR_CALL:
                        changed |= unfold_dec(cur);
                        break;
	        case IR_IF_LT_GOTO:
	        case IR_IF_GT_GOTO:
	        case IR_IF_LE_GOTO:
	        case IR_IF_GE_GOTO:
	        case IR_IF_EQ_GOTO:
	        case IR_IF_NE_GOTO:
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
                        if (cur->op1->val.i_val == 4) {
                                cur = delete_ir(cur);
                                changed = true;
                        } else
                                cur = cur->next;
                        break;
                case IR_GOTO:
                        if (!cur->next) {
                                cur = cur->next;
                                break;
                        }
                        if (cur->op1->no == cur->next->res->no) {
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