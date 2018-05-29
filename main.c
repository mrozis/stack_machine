#include <stdio.h>                      /****************************************************/
#include <sys/stat.h>                   /* Virtual Machine for stack-based language         */
#include <fcntl.h>                      /* May-June 2014                                    */
#include <unistd.h>                     /*                                                  */
#include <stdlib.h>                     /*                                                  */
#include <stdint.h>                     /*                                                  */
#include <endian.h>                     /*                                                  */
#include <sys/types.h>                  /****************************************************/
#include "vm.h"

extern stackStr stack;                //Global variable stack. Here is everything needed for stack implementation

uint32_t get_4_bytes(uint8_t *pc) {
    uint32_t res;
    res = pc[3];
    res = (res << 8) | pc[2];
    res = (res << 8) | pc[1];
    res = (res << 8) | pc[0];
    return res;
}

/* Main routine for running the bytecode program */
void run(uint32_t *program) {
    static void *labels[2][NUM_OF_INSTRUCTIONS] = {
        {&&load,&&load,&&load,&&load,        // Two Dimensional label array used in threaded
        &&load,&&load,&&load,&&load,         // code according to gcc extension (labels as values).
        &&load,&&load,&&load,&&load,         //See #define NEXT_INSTRUCTION for use
        &&load,&&load,&&load,&&load},

        {&&add,&&minus,&&mult,&&div,
        &&less,&&equal,&&and,&&not,
        &&dup,&&pop,&&swap,&&swap2,
        &&cjump,&&jump,&&get,&&put}
    };
    uint8_t *pc= (uint8_t *) &program[0];
    uint32_t a,b,c;
    int offset;
    uint32_t tmp;
    enum_opcode opcode;
    while (1) {
        tmp = pc[0] & 0x01;         //Mask for getting LSB is 0000-00001
        switch (tmp) {
            case 0:
            load:
                a = (pc[3] & 0x80) >> 7;  /*Check if negative or positive with mask 1000-0000 */
                if (a==1) {
                    /*Because MSB=1, data is negative so we set 1 (bitwise OR )
                    on shifted bit to get the correct data */
                    stack_push((get_4_bytes(pc) >> 1) | 0x80000000);
                    pc += 4;
                    NEXT_INSTRUCTION;
                }
                stack_push(get_4_bytes(pc) >> 1);
                pc += 4;
                NEXT_INSTRUCTION;
            case 1:
                opcode = (pc[0] & 0x1E) >> 1;   //Mask for getting opcode is 0001-1110
                switch (opcode) {
                    case op_add:
                    add:
                        stack_push(stack_pop() + stack_pop());
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_minus:
                    minus:
                        stack_push(-stack_pop());
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_mult:
                    mult:
                        stack_push(stack_pop() * stack_pop());
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_div:
                    div:
                        b = stack_pop();
                        if (b==0) {
                            fprintf(stderr,"Division Error. Cannot divide with zero");
                            exit(-1);
                        }
                        a = stack_pop();
                        stack_push(a/b);
                        stack_push(a%b);
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_less:
                    less:
                        stack_push(stack_pop() > stack_pop());
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_equal:
                    equal:
                        stack_push(stack_pop() == stack_pop());
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_and:
                    and:
                        stack_push(stack_pop() & stack_pop());
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_not:
                    not:
                        stack_push(~stack_pop());
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_dup:
                    dup:
                        a = stack_pop();
                        stack_push(a);
                        stack_push(a);
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_pop:
                    pop:
                        stack_pop();
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_swap:
                    swap:
                        a = stack_pop();
                        b = stack_pop();
                        stack_push(a);
                        stack_push(b);
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_swap2:
                    swap2:
                        c = stack_pop();
                        b = stack_pop();
                        a = stack_pop();
                        stack_push(c);
                        stack_push(a);
                        stack_push(b);
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_cjump:
                    cjump:
                        if (stack_pop()) {
                            a = pc[3] & 0x80 >> 7;  /*Check if negative or positive with mask 1000-0000 */
                            offset = (get_4_bytes(pc) & 0xFFFFFFE0) >> 5 ;
                            //Because offset is positive, shifting 5 bits is enough for correct offset 
                            if (a==1) {
                                /*Because MSB=1, offset is negative so we set 1 (bitwise OR )
                                on shifted bits to get the correct offset */
                                offset = offset | 0xF8000000; 
                            }
                            pc += 4*offset;
                        }                           
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_jump:
                    jump:
                        a = pc[3] & 0x80 >> 7;  /*Check if negative or positive with mask
                                                            1000-0000-0000-0000-0000-0000-0000-0000
                                                            */
                        offset = (get_4_bytes(pc) & 0xFFFFFFE0) >> 5 ;
                        //Because offset is positive, shifting 5 bits is enough for correct offset 
                        if (a==1) {
                            /*Because MSB=1, offset is negative so we set 1 (bitwise OR )
                            on shifted bits to get the correct offset */
                            offset = offset | 0xF8000000; 
                        }
                        pc += 4*offset;
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_get:
                    get:
                        a=fgetc(stdin);
                        if (a==EOF) stack_push(-1);
                        else stack_push(a);
                        pc += 4;
                        NEXT_INSTRUCTION;
                    case op_put:
                    put:
                        if (get_4_bytes(pc) == 0xB00B1E5F) //Magic Code for halting
                            HALT;
                        else {
                            a = stack_pop();
                            fputc(a,stdout);
                            fflush(stdout);
                            pc += 4;
                            NEXT_INSTRUCTION;
                        }
                    default:
                        //Never to come here
                        fprintf(stderr,"Error. Opcode <>15\n");
                        break;
                }
                NEXT_INSTRUCTION;
        }
    }
    out:
    return;
}


int main (int argc,char **argv) {
    if (argc<2) {
        printf("Please enter name of program bytecode file. Usage: %s bytecode_program_file\n",argv[0]);
        return -1;
    }
    FILE *fp = fopen(argv[1],"rb");
    if (fp==NULL) perror("fopen");
    fseek(fp,0,SEEK_END);                                                   //Fast-forward to end to find size of program
    int sz= (int) ftell(fp);
    fseek(fp,0,SEEK_SET);                                                       //Rewind
    uint32_t *program = malloc(sz*sizeof(uint32_t));
    int i=0;
    uint32_t tmp = 0;
    while (fread(&tmp,sizeof(uint32_t),1,fp)>0) program[i++] = htobe32(tmp);    //To-Big-Endian
    program[i] = 0xB00B1E5F;        //Magic number for halt instruction
    if (stack_init()<0) {
        perror("Error Initializing stack");
        return 0;
    }
    run(program);
    stack_destroy();
    free(program);
    return 0;
}
