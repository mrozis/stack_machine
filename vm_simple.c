#include <stdio.h>                      /****************************************************/
#include <sys/types.h>                  /* Author : Michalis Rozis                          */
#include <sys/stat.h>                   /* Virtual Machine for stack-based language         */
#include <fcntl.h>                      /* May-June 2014                                    */
#include <unistd.h>                     /*                                                  */
#include <stdlib.h>                     /*                                                  */
#include <stdint.h>                     /*                                                  */
#include <endian.h>                     /*                                                  */
                                        /****************************************************/

#define NUM_OF_INSTRUCTIONS 16
#define INITIAL_SIZE 1024       //Initial Size of Stack

typedef struct {
    uint32_t *s_stack;          //This is the Stack implemented as an array of INITIAL_SIZE size
    int top;
    int size;                   //Variable for holding the size of stack - Used for possible reallocation
} stackStr;

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

/* Main routine for running the bytecode program */
void run(uint32_t *program,int size) {
    int i=0;
    uint32_t a,b,c,offset;
    uint32_t tmp;
    while (i<size) {
        tmp = program[i];
        tmp = tmp & 0x00000001;         //Mask for getting LSB is 0000-0000-0000-0000-0000-0000-0000-00001
        switch (tmp) {
            case 0:
                a = program[i] & 0x80000000 >> 31;  /*Check if negative or positive with mask
                                                    1000-0000-0000-0000-0000-0000-0000-0000
                                                    */
                if (a==1) {
                    /*Because MSB=1, data is negative so we set 1 (bitwise OR )
                    on shifted bit to get the correct data */
                    stack_push((program[i] >> 1) | 0x80000000);
                }
                else  stack_push(program[i] >> 1);
                break;
            case 1:
                tmp = (program[i] & 0x0000001E) >> 1;   //Mask for getting opcode is 0000-0000-0000-0000-0000-0000-0001-1110
                switch (tmp) {
                    case 0:
                        //add
                        stack_push(stack_pop() + stack_pop());
                        break;
                    case 1:
                        //minus
                        stack_push(-stack_pop());
                        break;
                    case 2:
                        //mult
                        stack_push(stack_pop() * stack_pop());
                        break;
                    case 3:
                        //div
                        b = stack_pop();
                        if (b==0) {
                            fprintf(stderr,"Division Error. Cannot divide with zero");
                            exit(-1);
                        }
                        a = stack_pop();
                        stack_push(a/b);
                        stack_push(a%b);
                        break;
                    case 4:
                        //less
                        stack_push(stack_pop() > stack_pop());
                        break;
                    case 5:
                        //equal
                        stack_push(stack_pop() == stack_pop());
                        break;
                    case 6:
                        //and
                        stack_push(stack_pop() & stack_pop());
                        break;
                    case 7:
                        //not
                        stack_push(~stack_pop());
                        break;
                    case 8:
                        //dup
                        a = stack_pop();
                        stack_push(a);
                        stack_push(a);
                        break;
                    case 9:
                        //pop
                        stack_pop();
                        break;
                    case 10:
                        //swap
                        a = stack_pop();
                        b = stack_pop();
                        stack_push(a);
                        stack_push(b);
                        break;
                    case 11:
                        //swap2
                        c = stack_pop();
                        b = stack_pop();
                        a = stack_pop();
                        stack_push(c);
                        stack_push(a);
                        stack_push(b);
                        break;
                    case 12:
                        //cjump
                        if (stack_pop()) {
                            a = program[i] & 0x80000000 >> 31;  /*Check if negative or positive with mask
                                                                1000-0000-0000-0000-0000-0000-0000-0000
                                                                */
                            offset = (program[i] & 0xFFFFFFE0) >> 5 ;
                            //Because offset is positive, shifting 5 bits is enough for correct offset 
                            if (a==1) {
                                /*Because MSB=1, offset is negative so we set 1 (bitwise OR )
                                on shifted bits to get the correct offset */
                                offset = offset | 0xF8000000; 
                            }
                            i+=offset;
                        }                           
                        break;
                    case 13:
                        //jump
                        a = program[i] & 0x80000000 >> 31;  /*Check if negative or positive with mask
                                                            1000-0000-0000-0000-0000-0000-0000-0000
                                                            */
                        offset = (program[i] & 0xFFFFFFE0) >> 5;  
                        //Because offset is positive, shifting 5 bits is enough for correct offset 
                        if (a==1) {
                            /*Because MSB=1, offset is negative so we set 1 (bitwise OR )
                            on shifted bits to get the correct offset */
                            offset = offset | 0xF8000000; 
                        }
                        i+=offset;
                        break;
                    case 14:
                        //get
                        a=fgetc(stdin);
                        if (a==EOF) stack_push(-1);
                        else stack_push(a);
                        fflush(stdin);
                        break;
                    case 15:
                        //put
                        a = stack_pop();
                        fputc(a,stdout);
                        fflush(stdout);
                        break;
                    default:
                        //Never to come here. Used for debugging
                        fprintf(stderr,"Error. Opcode <>15\n");
                        break;
                }
                break;
        }
        i++;
    }
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
    sz = i;
    if (stack_init()<0) {
        perror("Error Initializing stack");
        return 0;
    }
    run(program,sz);
    stack_destroy();
    free(program);
    return 0;
}
