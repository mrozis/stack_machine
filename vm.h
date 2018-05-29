#include <stdint.h>
#ifndef __VM_H__
#define __VM_H__


#define NUM_OF_INSTRUCTIONS 16
#define INITIAL_SIZE 1024       //Initial Size of Stack


/* Very strange GOTO. What this goto does is calculate the jump for the next program counter
This is done by the following statement written in bit format
If LSB=0 then jump to labels[0][x] = &&load
else jump to labels[1][opcode] */
#define NEXT_INSTRUCTION goto *labels[pc[0] & 0x01][(pc[0] & 0x1E) >> 1]
#define HALT goto out       //goto halt label


typedef enum {
    op_add,
    op_minus,
    op_mult,
    op_div,
    op_less,
    op_equal,
    op_and,
    op_not,
    op_dup,
    op_pop,
    op_swap,
    op_swap2,
    op_cjump,
    op_jump,
    op_get,
    op_put
} enum_opcode ;

typedef struct {
    uint32_t *s_stack;          //This is the Stack implemented as an array of INITIAL_SIZE size
    int top;
    int size;                   //Variable for holding the size of stack - Used for possible reallocation
} stackStr;

extern int stack_init();
extern int stack_realloc();
extern int stack_push(uint32_t data);
extern uint32_t stack_pop();
extern int stack_destroy();


#endif
