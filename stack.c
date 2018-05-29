#include <stdlib.h>    
#include <stdio.h>     
#include "vm.h"

stackStr stack;                //Global variable stack. Here is everything needed for stack implementation

/* Stack initialization routine */
int stack_init() {
    if ((stack.s_stack = (uint32_t *)malloc(INITIAL_SIZE*sizeof(uint32_t)))<0) return -1;
    stack.size = INITIAL_SIZE;
    stack.top = 0;
    return 0;
}

/*Stack reallocation routine. When top of the stack reaches the size of the stack
reallocation of the stack is needed. Exponential reallocation is used for avoiding
possible waste of time on continuous reallocations */
int stack_realloc() {
    stack.size *= 2;
    stack.s_stack = (uint32_t *) realloc(stack.s_stack,stack.size*sizeof(uint32_t));
    if (stack.s_stack == NULL) {
        perror("reallocation of stack");
        exit(-1);
    }
    return 0;
}

int stack_push(uint32_t data) {
    stack.s_stack[stack.top] = data;
    (stack.top)++;
    if (stack.top == stack.size) stack_realloc();
    return 0;
}

uint32_t stack_pop() {
    stack.top--;
    return  stack.s_stack[stack.top];
}



/* Routine for freeing all the stack-consumed space.
This routine should never fail  */
int stack_destroy() {
    free(stack.s_stack);
    return 0;
}
