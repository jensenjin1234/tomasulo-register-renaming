/*******************************************
  operations
*********************************************/
#define NUM_OF_OP_TYPES 6
enum op_type {ADD, SUB, MUL, DIV, LD, ST};

#ifndef USE_CUSTOM_SEQUENCE
#define USE_CUSTOM_SEQUENCE 0
#endif

/* data structure for an instruction */
typedef struct instruction {
  int num; /* number: starting from 1 */
  enum op_type op; /* operation type */
  int rd; /* destination register id */
  int rs; /* source regsiter id or base register for ld/st */
  int rt; /* target regsiter id or addr offset for ld/st */
} INST;


#if USE_CUSTOM_SEQUENCE
#define NUM_OF_INST 7
#else
#define NUM_OF_INST 6
#endif
extern INST inst[NUM_OF_INST]; /* instruction array */ 

void init_inst();
void print_inst(INST ins);
void print_program();
