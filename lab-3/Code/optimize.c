#include "ir.h"
#include <stdlib.h>

extern struct ir ir_list;

void sweep_unused(struct ir *list)
{
        struct ir *cur = ir_list.next;
        while (cur) {
                struct ir *del = NULL;
                if (cur->type == IR_ASSIGN && cur->res->ref_count == 0) {
                        cur->prev->next = cur->next;
                        cur->next->prev = cur->prev;
                        del = cur;
                }
                cur = cur->next;
                free(del);
        }  
}

void addr_1_folding(struct ir *cur)
{
        /*
         * temp := x
         * RETURN temp
         */
        struct ir *temp_dec;
        switch (cur->type) {
        case IR_RETURN:
        case IR_ARG:
        case IR_WRITE:
                if (cur->op1->type == OPERAND_CONST)
                        break;
                // temp := x
                temp_dec = cur->op1->dec;
                if (!temp_dec->res->can_fold)
                        break;
                if (temp_dec->type == IR_ASSIGN) {
                        // RETURN temp -> RETURN x
                        cur->op1 = temp_dec->op1;
                        // temp ref_count--
                        temp_dec->res->ref_count--;
                        // x ref_count++
                        cur->op1->ref_count++;
                        addr_1_folding(cur);
                }
                break;
        case IR_READ:
                break;
        }
        // if ()
}

void addr_2_folding(struct ir *cur)
{
        switch (cur->type) {
        case IR_ASSIGN:
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

void optimize()
{
        /* Main function of optimizor. */
        struct ir *cur = ir_list.next;
        while (cur) {
                addr_folding[cur->addr_cnt](cur);
                peephole_optimization(cur);
                cur = cur->next;
        }
        sweep_unused(ir_list.next);
}