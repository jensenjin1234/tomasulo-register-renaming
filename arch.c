#include <stdio.h>
#include "arch.h"

REG regs[NUM_REGS];
int mem_arr[MEM_SIZE];
RS rs_array[NUM_RS_ENTRIES];

bool is_add_available;
bool is_mul_available;
bool is_mem_available;

int logical_to_physical[NUM_ARCH_REGS];
static bool phys_in_use[NUM_REGS];
static int phys_owner[NUM_REGS];

static int find_free_phys()
{
#if ENABLE_RENAMING
  int i;
  for(i=NUM_ARCH_REGS;i<NUM_REGS;i++) {
    if(!phys_in_use[i]) return i;
  }
  return -1;
#else
  return -1;
#endif
}

bool is_rs_active()
{
  int i;
  for(i=0;i<NUM_RS_ENTRIES;i++)
    if(rs_array[i].is_busy) return true;
  return false;
}

void set_mem(int addr, int val)
{
  mem_arr[addr] = val;
  return;
}

int get_mem(int addr)
{
  return mem_arr[addr];
}

void init_mem()
{
  int i;
  for(i=0;i<MEM_SIZE;i++) mem_arr[i] = i%16;
  return;
}

void init_fu()
{
  is_add_available=true;
  is_mul_available=true;
  is_mem_available=true;
  return;
}

void init_regs()
{
  int i=0;
  for(i=0;i<NUM_REGS;i++) {
    regs[i].num = i;
    regs[i].val = i;
    regs[i].Qi = 0;
  }
  for(i=0;i<NUM_ARCH_REGS;i++) {
    logical_to_physical[i] = i;
    phys_in_use[i] = true;
    phys_owner[i] = i;
  }
#if ENABLE_RENAMING
  for(i=NUM_ARCH_REGS;i<NUM_REGS;i++) {
    regs[i].val = 0;
    phys_in_use[i] = false;
    phys_owner[i] = -1;
  }
#endif
}

void print_regs() 
{
  int i=0;
  printf("=====================================================================\n");
  printf("Logical Registers\n");
  printf("---------------------------------------------------------------------\n");
  for(i=0;i<NUM_ARCH_REGS;i++)
  {
    if(i%8==7) { 
      printf(" \tr%d\tr%d\tr%d\tr%d\tr%d\tr%d\tr%d\tr%d\n",
              i-7,
              i-6,
              i-5,
              i-4,
              i-3,
              i-2,
              i-1,
              i);
      printf("---------------------------------------------------------------------\n");
      printf("val\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
              regs[logical_to_physical[i-7]].val,
              regs[logical_to_physical[i-6]].val,
              regs[logical_to_physical[i-5]].val,
              regs[logical_to_physical[i-4]].val,
              regs[logical_to_physical[i-3]].val,
              regs[logical_to_physical[i-2]].val,
              regs[logical_to_physical[i-1]].val,
              regs[logical_to_physical[i]].val);
      printf("Qi\t#%d\t#%d\t#%d\t#%d\t#%d\t#%d\t#%d\t#%d\n",
              regs[logical_to_physical[i-7]].Qi,
              regs[logical_to_physical[i-6]].Qi,
              regs[logical_to_physical[i-5]].Qi,
              regs[logical_to_physical[i-4]].Qi,
              regs[logical_to_physical[i-3]].Qi,
              regs[logical_to_physical[i-2]].Qi,
              regs[logical_to_physical[i-1]].Qi,
              regs[logical_to_physical[i]].Qi);
      printf("---------------------------------------------------------------------\n");
    }
  }
#if ENABLE_RENAMING && (NUM_RENAME_REGS>0)
  printf("Rename Pool (physical registers %d-%d)\n", NUM_ARCH_REGS, NUM_REGS-1);
  printf("---------------------------------------------------------------------\n");
  printf("Phys\tOwner\tBusy\tVal\tQi\n");
  for(i=NUM_ARCH_REGS;i<NUM_REGS;i++) {
    printf("p%d\t",i);
    if(phys_owner[i]>=0) printf("r%d\t",phys_owner[i]);
    else printf("none\t");
    printf("%s\t",phys_in_use[i]?"Yes":"No");
    printf("%d\t#%d\n",regs[i].val,regs[i].Qi);
  }
  printf("---------------------------------------------------------------------\n");
#endif
  printf("=====================================================================\n");
  return;
  
}

void print_logical_regs()
{
  print_regs();
}

void reset_rs_entry(RS * t)
{ 
    t->is_busy = false;
    t->op = -1;
    t->Qj = 0;
    t->Qk = 0;
    t->Vj = -1;
    t->Vk = -1;
    t->A = -1;
    t->exec_cycles = -1;
    t->result = -1;
    t->in_exec = false;
    t->is_result_ready = false;
    t->inst_num = -1;
    t->dest_reg = -1;
    t->dest_alias = -1;
    return;
}
/**************************************
* initialize RS 
***************************************/
void init_rs()
{
  int i;
  int curr_ind=0;
  RS * curr_entry;
  
  /* common for all */
  for(i=0;i<NUM_RS_ENTRIES;i++) {
    curr_entry = &(rs_array[i]);
    curr_entry->id = i+1;
    curr_entry->is_busy = false;
    curr_entry->op = -1;
    curr_entry->Qj = 0;
    curr_entry->Qk = 0;
    curr_entry->Vj = -1;
    curr_entry->Vk = -1;
    curr_entry->A = -1;
    curr_entry->exec_cycles = -1;
    curr_entry->result = -1;
    curr_entry->in_exec = false;
    curr_entry->is_result_ready = false;
    curr_entry->inst_num = -1;
  }
  /* types */
  for(i=0;i<NUM_LD_BUF;i++) rs_array[curr_ind++].type=LD_BUF;
  for(i=0;i<NUM_ST_BUF;i++) rs_array[curr_ind++].type=ST_BUF;
  for(i=0;i<NUM_ADD_RS;i++) rs_array[curr_ind++].type=ADD_RS;
  for(i=0;i<NUM_MUL_RS;i++) rs_array[curr_ind++].type=MUL_RS;
}

/**************************************
* print RS 
***************************************/
void print_rs()
{
  int i;

  printf("==================================================================================================\n");
  printf("RS_id\ttype\tBusy\tinst#\tOp\tVj\tVk\tQj\tQj\tA\tExec\tDone\n");
  printf("--------------------------------------------------------------------------------------------------\n");
  for(i=0;i<NUM_RS_ENTRIES;i++) {
    printf("#%d\t",rs_array[i].id); // id
    if(rs_array[i].type==LD_BUF) printf("LD\t"); // RS buff type
    else if(rs_array[i].type==ST_BUF) printf("ST\t"); 
    else if(rs_array[i].type==ADD_RS) printf("ADD\t"); 
    else if(rs_array[i].type==MUL_RS) printf("MUL\t"); 
    else printf("NONE\t");
    if(rs_array[i].is_busy) printf("Yes\t"); else printf("No\t"); // Busy
    printf("I#%d\t",rs_array[i].inst_num);
    if(rs_array[i].op==ADD) printf("add\t"); // instr type
    else if(rs_array[i].op==SUB) printf("sub\t"); 
    else if(rs_array[i].op==MUL) printf("mul\t"); 
    else if(rs_array[i].op==DIV) printf("div\t"); 
    else if(rs_array[i].op==LD) printf("ld\t"); 
    else if(rs_array[i].op==ST) printf("st\t"); 
    else printf("NONE\t");
    printf("%d\t%d\t#%d\t#%d\t%d\t",rs_array[i].Vj,rs_array[i].Vk,rs_array[i].Qj,rs_array[i].Qk,rs_array[i].A);
    if(rs_array[i].in_exec) printf("Yes\t"); else printf("No\t"); // in Execution?
    if(rs_array[i].is_result_ready) printf("Yes\n"); else printf("No\n"); // done Execution?
    
  }
  printf("==================================================================================================\n");
  
  return;
}


int obtain_available_rs(enum rs_type t)
{
  int i;
  for(i=0;i<NUM_RS_ENTRIES;i++) {
    if((rs_array[i].type==t) && !rs_array[i].is_busy) return rs_array[i].id;
  }
  return -1; /* no available RS entriy */
}

RS * get_rs(int id)
{
  int i=0;
  for(i=0;i<NUM_RS_ENTRIES;i++) {
    if(rs_array[i].id == id) return &(rs_array[i]);
  }
  return NULL;
}

int get_phys_reg(int logical_reg)
{
  if(logical_reg<0 || logical_reg>=NUM_ARCH_REGS) return logical_reg;
  return logical_to_physical[logical_reg];
}

int allocate_phys_reg(int logical_reg)
{
#if ENABLE_RENAMING
  int alias = find_free_phys();
  int old_alias = get_phys_reg(logical_reg);
  if(alias==-1) {
    return old_alias;
  }
  phys_in_use[alias] = true;
  phys_owner[alias] = logical_reg;
  logical_to_physical[logical_reg] = alias;
  regs[alias].Qi = 0;
  if(old_alias>=NUM_ARCH_REGS && regs[old_alias].Qi==0) {
    release_phys_reg(old_alias);
  }
  return alias;
#else
  return logical_reg;
#endif
}

void release_phys_reg(int phys_reg)
{
#if ENABLE_RENAMING
  if(phys_reg>=NUM_ARCH_REGS && phys_reg<NUM_REGS) {
    phys_in_use[phys_reg] = false;
    phys_owner[phys_reg] = -1;
    regs[phys_reg].Qi = 0;
  }
#else
  (void)phys_reg;
#endif
}

bool is_current_mapping(int logical_reg, int phys_reg)
{
  if(logical_reg<0 || logical_reg>=NUM_ARCH_REGS) return false;
  return logical_to_physical[logical_reg]==phys_reg;
}
