#include <stdio.h>
#include <stdlib.h>
#include "arch.h"


/*******************************
main
*******************************/
int main()
{
  int i,j;
  int done = 0;
  int cycle = 0;
  int num_issued_inst = 0;

  init_inst();
  init_fu();

  printf("============== TEST INSTRUCTION SEQUENCE ===========\n");
  print_program();

  init_rs();	// initialize RS entries
  init_regs();	// initalize registers
  init_mem();	// initialize memory

  printf("* CYCLE %d (initial state)\n",cycle);
  print_rs();	// print initial RS state
  print_regs();	// print initial register state

  /* simulation loop main */
  while(!done){

    /* increment the cycle */
    cycle++;
    
    /********************************
     *     Step III: Write result 
     ********************************/
     for(i=0;i<NUM_RS_ENTRIES;i++) {
       RS * r = &rs_array[i];
       
       /* Check if execution is complete and result is ready */
       if(r->is_busy && r->is_result_ready) {
         
         /* For FP operations and Load */
         if(r->op==ADD || r->op==SUB || r->op==MUL || r->op==DIV || r->op==LD) {
           
           /* Update all registers waiting for this RS result */
           for(j=0;j<NUM_REGS;j++) {
             if(regs[j].Qi == r->id) {
               regs[j].val = r->result;
               regs[j].Qi = 0;
             }
           }
           
           /* Update all RS entries waiting for this result (broadcast on CDB) */
           for(j=0;j<NUM_RS_ENTRIES;j++) {
             if(rs_array[j].Qj == r->id) {
               rs_array[j].Vj = r->result;
               rs_array[j].Qj = 0;
             }
             if(rs_array[j].Qk == r->id) {
               rs_array[j].Vk = r->result;
               rs_array[j].Qk = 0;
             }
           }

           /* Release old physical register versions if this is no longer the latest */
           if(r->dest_alias!=-1 && r->dest_reg!=-1) {
             if(!is_current_mapping(r->dest_reg, r->dest_alias)) {
               release_phys_reg(r->dest_alias);
             }
           }
           
           /* Free the RS entry */
           reset_rs_entry(r);
           
         } else if(r->op==ST) {
           /* Store: write to memory */
           if(r->Qk == 0) {  /* Ensure store data is ready */
             set_mem(r->A, r->Vk);
             reset_rs_entry(r);
           }
         }
       }
     }


    /********************************
     *     Step II: Execute 
     ********************************/
     for(i=0;i<NUM_RS_ENTRIES;i++) {
       RS * r = &rs_array[i];
       
       /* Check if RS entry is busy and not already done */
       if(r->is_busy && !r->is_result_ready) {
         
         /* For FP operations (ADD, SUB, MUL, DIV) */
         if(r->op==ADD || r->op==SUB || r->op==MUL || r->op==DIV) {
           
           /* Check if operands are ready */
           if(r->Qj == 0 && r->Qk == 0) {
             
             /* Check if functional unit is available */
             bool fu_available = false;
             if((r->op==ADD || r->op==SUB) && is_add_available) {
               fu_available = true;
               is_add_available = false;  /* Reserve the functional unit */
             } else if((r->op==MUL || r->op==DIV) && is_mul_available) {
               fu_available = true;
               is_mul_available = false;  /* Reserve the functional unit */
             }
             
             if(fu_available) {
               if(!r->in_exec) {
                 /* Start execution */
                 r->in_exec = true;
               }
               
               /* Decrement execution cycles */
               r->exec_cycles--;
               
               /* Check if execution is complete */
               if(r->exec_cycles == 0) {
                 /* Compute result */
                 if(r->op == ADD) r->result = r->Vj + r->Vk;
                 else if(r->op == SUB) r->result = r->Vj - r->Vk;
                 else if(r->op == MUL) r->result = r->Vj * r->Vk;
                 else if(r->op == DIV) r->result = r->Vj / r->Vk;
                 
                 r->is_result_ready = true;
                 r->in_exec = false;
               }
             }
           }
           
         } else if(r->op == LD) {
           /* Load operation */
           if(r->Qj == 0 && is_mem_available) {
             is_mem_available = false;  /* Reserve memory unit */
             
             if(!r->in_exec) {
               /* Load step 1: Compute address */
               r->A = r->Vj + r->A;
               r->in_exec = true;
             }
             
             /* Decrement execution cycles */
             r->exec_cycles--;
             
             /* Check if execution is complete */
             if(r->exec_cycles == 0) {
               /* Load step 2: Read from memory */
               r->result = get_mem(r->A);
               r->is_result_ready = true;
               r->in_exec = false;
             }
           }
           
         } else if(r->op == ST) {
           /* Store operation */
           if(r->Qj == 0 && is_mem_available) {
             is_mem_available = false;  /* Reserve memory unit */
             
             if(!r->in_exec) {
               /* Compute address */
               r->A = r->Vj + r->A;
               r->in_exec = true;
             }
             
             /* Decrement execution cycles */
             r->exec_cycles--;
             
             /* Check if execution is complete */
             if(r->exec_cycles == 0) {
               r->is_result_ready = true;
               r->in_exec = false;
             }
           }
         }
       }
     }
     
     /* Release functional units for next cycle */
     is_add_available = true;
     is_mul_available = true;
     is_mem_available = true;

    /********************************
     *     Step I: Issue 
     ********************************/

    /*  wait if no RS entry is available */
    if(num_issued_inst < NUM_OF_INST) {
      int cand_rs_id; 
      if(inst[num_issued_inst].op==ADD) cand_rs_id = obtain_available_rs(ADD_RS);
      else if(inst[num_issued_inst].op==SUB) cand_rs_id = obtain_available_rs(ADD_RS);
      else if(inst[num_issued_inst].op==MUL) cand_rs_id = obtain_available_rs(MUL_RS);
      else if(inst[num_issued_inst].op==DIV) cand_rs_id = obtain_available_rs(MUL_RS);
      else if(inst[num_issued_inst].op==LD) cand_rs_id = obtain_available_rs(LD_BUF);
      else if(inst[num_issued_inst].op==ST) cand_rs_id = obtain_available_rs(ST_BUF);

      /* if there is an available RS entry */
      if(cand_rs_id!=-1) {
        /* issue the instruction: See Fig. 3.13 */
	RS * curr_rs = get_rs(cand_rs_id);
        if(curr_rs ==NULL) {
          printf("NO RS found with the given id\n");
          exit(1);
        }

        /* normal ALU operations */
        if(inst[num_issued_inst].op==ADD || inst[num_issued_inst].op==SUB ||
          inst[num_issued_inst].op== MUL ||  inst[num_issued_inst].op==DIV) {
          int rd, rs, rt;
          int phys_rs, phys_rt;
          rd = inst[num_issued_inst].rd;
          rs = inst[num_issued_inst].rs;
          rt = inst[num_issued_inst].rt;

          /* Rs */
          phys_rs = get_phys_reg(rs);
          if(regs[phys_rs].Qi!=0) curr_rs->Qj = regs[phys_rs].Qi;
          else curr_rs->Vj = regs[phys_rs].val;

          /* Rt */
          phys_rt = get_phys_reg(rt);
          if(regs[phys_rt].Qi!=0) curr_rs->Qk = regs[phys_rt].Qi;
          else curr_rs->Vk = regs[phys_rt].val;

          /* set busy */
          curr_rs->is_busy = true;
          curr_rs->op = inst[num_issued_inst].op;
          curr_rs->inst_num = inst[num_issued_inst].num;
          curr_rs->dest_reg = rd;
          curr_rs->dest_alias = allocate_phys_reg(rd);

          /* register update */
          regs[curr_rs->dest_alias].Qi = curr_rs->id;

          /* set exec cycles */
          if(inst[num_issued_inst].op==ADD) curr_rs->exec_cycles=LAT_ADD;
          else if(inst[num_issued_inst].op==SUB) curr_rs->exec_cycles=LAT_SUB;
          else if(inst[num_issued_inst].op==MUL) curr_rs->exec_cycles=LAT_MUL;
          else if(inst[num_issued_inst].op==DIV) curr_rs->exec_cycles=LAT_DIV;

          /* num issued ++ */
          num_issued_inst++;

        } else if (inst[num_issued_inst].op==LD) {

          int rd, rs, imm;
          int phys_rs;
          rd = inst[num_issued_inst].rd;
          rs = inst[num_issued_inst].rs;
          imm = inst[num_issued_inst].rt;

          /* Rs */
          phys_rs = get_phys_reg(rs);
          if(regs[phys_rs].Qi!=0) curr_rs->Qj = regs[phys_rs].Qi;
          else curr_rs->Vj = regs[phys_rs].val;

          /* addr */
          curr_rs->A = imm;

          /* set busy */
          curr_rs->is_busy = true;
          curr_rs->op = inst[num_issued_inst].op;
          curr_rs->inst_num = inst[num_issued_inst].num;
          curr_rs->dest_reg = rd;
          curr_rs->dest_alias = allocate_phys_reg(rd);

          /* set exec cycles */
          curr_rs->exec_cycles=LAT_LD;

          /* register update */
          regs[curr_rs->dest_alias].Qi = curr_rs->id;

          /* num issued ++ */
          num_issued_inst++;

        } else if (inst[num_issued_inst].op==ST) {

          int rd, rs, imm;
          int phys_rs, phys_rd;
          rd = inst[num_issued_inst].rd;
          rs = inst[num_issued_inst].rs;
          imm = inst[num_issued_inst].rt;

          /* Rs */
          phys_rs = get_phys_reg(rs);
          if(regs[phys_rs].Qi!=0) curr_rs->Qj = regs[phys_rs].Qi;
          else curr_rs->Vj = regs[phys_rs].val;

          /* Rd */
          phys_rd = get_phys_reg(rd);
          if(regs[phys_rd].Qi!=0) curr_rs->Qk = regs[phys_rd].Qi;
          else curr_rs->Vk = regs[phys_rd].val;

          /* addr */
          curr_rs->A = imm;

          /* set busy */
          curr_rs->is_busy = true;
          curr_rs->op = inst[num_issued_inst].op;
          curr_rs->inst_num = inst[num_issued_inst].num;
          curr_rs->dest_reg = -1;
          curr_rs->dest_alias = -1;

          /* set exec cycles */
          curr_rs->exec_cycles=LAT_ST;

          /* num issued ++ */
          num_issued_inst++;

        }
      }
    }


    /* print out the result */
    printf("* CYCLE %d\n",cycle);
    print_rs();
    print_regs();
   
    /* check the termination condition */ 
    if( (num_issued_inst>=NUM_OF_INST) && !is_rs_active())
      done =1;
  }
  return 0;
}
