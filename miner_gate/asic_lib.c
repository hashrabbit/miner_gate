/*
 * Copyright 2014 Zvi (Zvisha) Shteingart - Spondoolies-tech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 *
 * Note that changing this SW will void your miners guaranty
 */


#include "squid.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>
#include <unistd.h>
#include "mg_proto_parser_sp30.h"
#include "asic.h"
#include "queue.h"
#include <string.h>
#include "asic_lib.h"
#include <pthread.h>
#include <spond_debug.h>
#include <sys/time.h>
#include "real_time_queue.h"
#include "miner_gate.h"
#include "dc2dc.h"
#include "ac2dc.h"
#include "pll.h"
#include "board.h"
#include "math.h"

#include "pwm_manager.h"


#include <sched.h>

extern int rt_queue_size;
extern int rt_queue_sw_write;
extern int rt_queue_hw_done;


extern int kill_app;
extern pthread_mutex_t network_hw_mutex;
pthread_mutex_t asic_mutex = PTHREAD_MUTEX_INITIALIZER;
MINER_BOX vm = { 0 };
void print_adapter(FILE *f , bool syslog);
void poll_win_temp_rt();
static int one_sec_counter = 0;
int total_devices = 0;
extern int enable_reg_debug;
int update_ac2dc_power_measurments();

RT_JOB *add_to_sw_rt_queue(const RT_JOB *work);
void reset_sw_rt_queue();
RT_JOB *peak_rt_queue(uint8_t hw_id);
void try_push_job_to_mq();
void on_failed_bist(int addr, bool store_limit,bool step_down_if_failed);

void stop_all_work_rt() {
  // wait to finish real time queue requests
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_COMMAND, BIT_ADDR_COMMAND_END_CURRENT_JOB);
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_COMMAND, BIT_ADDR_COMMAND_END_CURRENT_JOB);
  //usleep(100);
  RT_JOB work;

  // Move real-time q to complete.
  while (one_done_sw_rt_queue(&work)) {
    push_work_rsp(&work, 1);
  }

  // Move pending to complete
  // RT_JOB w;
  /*
  while (pull_work_req(&work)) {
    push_work_rsp(&work);
  }*/
 // passert(read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_BR_CONDUCTOR_BUSY) == 0);
  vm.stopped_all_work = 1;
}

void resume_all_work() {
  if (vm.stopped_all_work) {
    vm.stopped_all_work = 0;
  }
}

void print_devreg(int reg, const char *name) {
  printf("%2x:  BR:%8x ", reg, read_reg_asic(ANY_ASIC, NO_ENGINE,reg));
  int i;
  for (i=0; i< ASICS_COUNT; i++) {
    if (vm.asic[i].asic_present) {
      printf("DEV-%04x:%8x ", i, read_reg_asic(i, NO_ENGINE,reg));
    }
  }
  printf("  %s\n", name);
}

void parse_int_register(const char *label, int reg) {
  /*
  printf("%s :%x: ", label, reg);
  if (reg & BIT_INTR_NO_ADDR)
    printf("NO_ADDR ");
  if (reg & BIT_INTR_WIN)
    printf("WIN ");
  if (reg & BIT_INTR_FIFO_FULL)
    printf("FIFO_FULL ");
  if (reg & BIT_INTR_FIFO_ERR)
    printf("FIFO_ERR ");
  if (reg & BIT_INTR_CONDUCTOR_IDLE)
    printf("CONDUCTOR_IDLE ");
  if (reg & BIT_INTR_CONDUCTOR_BUSY)
    printf("CONDUCTOR_BUSY ");
  if (reg & BIT_INTR_RESERVED1)
    printf("RESERVED1 ");
  if (reg & BIT_INTR_FIFO_EMPTY)
    printf("FIFO_EMPTY ");
  if (reg & BIT_INTR_BIST_FAIL)
    printf("BIST_FAIL ");
  if (reg & BIT_INTR_THERMAL_SHUTDOWN)
    printf("THERMAL_SHUTDOWN ");
  if (reg & BIT_INTR_NOT_WIN)
    printf("NOT_WIN ");
  if (reg & BIT_INTR_0_OVER)
    printf("0_OVER ");
  if (reg & BIT_INTR_0_UNDER)
    printf("0_UNDER ");
  if (reg & BIT_INTR_1_OVER)
    printf("1_OVER ");
  if (reg & BIT_INTR_1_UNDER)
    printf("1_UNDER ");
  if (reg & BIT_INTR_PLL_NOT_READY)
    printf("PLL_NOT_READY ");
  printf("\n");
  */
}



void thermal_init(int addr) {
  write_reg_asic(addr, NO_ENGINE,ADDR_TS_EN_0, 1);
  write_reg_asic(addr, NO_ENGINE,ADDR_TS_EN_1, 1);
  write_reg_asic(addr, NO_ENGINE,ADDR_SHUTDOWN_ACTION, 0);
  write_reg_asic(addr, NO_ENGINE,ADDR_TS_RSTN_0, 0);
  write_reg_asic(addr, NO_ENGINE,ADDR_TS_RSTN_1, 0);
  // Set to MAX_ASIC_TEMPERATURE-1
  write_reg_asic(addr, NO_ENGINE,ADDR_TS_SET_0, (MAX_ASIC_TEMPERATURE-2));
  // Set to MAX_ASIC_TEMPERATURE
  write_reg_asic(addr, NO_ENGINE,ADDR_TS_SET_1, (MAX_ASIC_TEMPERATURE-1));
  write_reg_asic(addr, NO_ENGINE,ADDR_TS_RSTN_0, 1);
  write_reg_asic(addr, NO_ENGINE,ADDR_TS_RSTN_1, 1);
  flush_spi_write();
}

void act_on_temperature(int addr, int* can_upscale) {
  int err;
  ASIC *a = &vm.asic[addr];
  if ((a->asic_temp >= MAX_ASIC_TEMPERATURE)) {
    if (a->dc2dc.max_vtrim_temperature > VTRIM_MIN) {
      // Down 1 click on voltage, full down on FREQ 
      if (dc2dc_can_down(addr)) {
        if ((a->dc2dc.max_vtrim_temperature > VTRIM_MIN)) {
          a->dc2dc.max_vtrim_temperature = a->dc2dc.vtrim - 1;
          dc2dc_down(addr,&err,"too hot A");
          asic_down_freq_completly(addr,1,1,"too hot B");
          if (!a->cooling_down) {            
            a->cooling_down = 1;
            vm.board_cooling_now[ASIC_TO_BOARD_ID(addr)]++;
            vm.board_cooling_ever |= (1 << addr);
          }
        } else {
          disable_asic_forever_rt(addr,"ASIC too hot 1");
          return;
        }
      }
    }
  } else if (a->cooling_down && *can_upscale) {
    // Cooling down complete
    asic_up_freq_max(addr,1,1, "Cooling done");
    *can_upscale = 0;
  }
    //printf("Set pll %x %d to %d\n",i,vm.hammer[i].freq_hw,vm.hammer[i].freq_wanted);
    
}


void update_ac2dc_stats() {
  passert (!vm.i2c_busy_with_bug);
  //struct timeval tv;
  //start_stopper(&tv);
  update_ac2dc_power_measurments();
  //end_stopper(&tv, "update_ac2dc_stats1");  
}


int current_reading = ASIC_TEMP_125;


void temp_measure_next() {
    current_reading = current_reading + 1;
    if (current_reading >= ASIC_TEMP_COUNT) {
       current_reading = MAX_ASIC_TEMPERATURE-2;
    }
}


void set_temp_reading_rt(int measure_temp_addr, uint32_t* intr) {
    //flush_spi_write();
    write_reg_asic(measure_temp_addr,NO_ENGINE, ADDR_MNG_COMMAND, BIT_ADDR_MNG_TS_RESET_0 | BIT_ADDR_MNG_TS_RESET_0);
    write_reg_asic(measure_temp_addr,NO_ENGINE, ADDR_TS_SET_0, (current_reading-1));
    write_reg_asic(measure_temp_addr,NO_ENGINE, ADDR_TS_SET_1, (current_reading-1));
    write_reg_asic(measure_temp_addr,NO_ENGINE, ADDR_MNG_COMMAND, BIT_ADDR_MNG_TS_RESET_0 | BIT_ADDR_MNG_TS_RESET_0);
    push_asic_read(measure_temp_addr,NO_ENGINE, ADDR_INTR_RAW, intr);
}

void proccess_temp_reading_rt(ASIC *a, int intr) {
     static int over_bits[] = {BIT_INTR_TS_0_OVER, BIT_INTR_TS_1_OVER};
     int over_bit = over_bits[rand()%2]; // choose random sensor
     
     if (!(intr & over_bit)) {
        //printf("TMP %d LOW %d\n",a->address, current_reading);      
       if (a->asic_temp > (current_reading-1)) {
           a->asic_temp = (ASIC_TEMP)(current_reading-1);
       }
     } else {
        // the first sensor to show 125 might be brocken, don't use it
        if ((vm.mining_time < 20) &&
            (current_reading == ASIC_TEMP_125) && 
            (over_bits[0] != over_bits[1])) {
            a->faults |= 0x1;
          if (over_bit == BIT_INTR_TS_0_OVER) {
            over_bits[0] = over_bits[1] = BIT_INTR_TS_1_OVER;
          }  else {
            over_bits[0] = over_bits[1] = BIT_INTR_TS_0_OVER;
          }
        }
        //printf("TMP %d HIGH %d\n",a->address, current_reading);
        if ((a->asic_temp < current_reading)) {
          a->asic_temp = (ASIC_TEMP)(current_reading);
        }
     }     
}




void save_rate_temp(int back_tmp_t, int back_tmp_b, int front_tmp, int total_mhash) {
    FILE *f = fopen("/var/run/mg_rate_temp", "w");
    if (!f) {
      psyslog("Failed to create watchdog file\n");
      return;
    }
    fprintf(f, "%d %d %d %d\n", total_mhash, front_tmp, back_tmp_t, back_tmp_b);
    fclose(f);
}

void print_state() {
  static int print = 0;
  print++;
  dprintf("**************** DEBUG PRINT %x ************\n", print);
  print_devreg(ADDR_CURRENT_NONCE, "CURRENT NONCE");
  /*
  print_devreg(ADDR_BR_FIFO_FULL, "FIFO FULL");
  print_devreg(ADDR_BR_FIFO_EMPTY, "FIFO EMPTY");
  print_devreg(ADDR_BR_CONDUCTOR_BUSY, "BUSY");
  print_devreg(ADDR_BR_CONDUCTOR_IDLE, "IDLE");
  */
  dprintf("*****************************\n");

  print_devreg(ADDR_CHIP_ADDR, "ADDR_CHIP_ADDR");
  print_devreg(ADDR_GOT_ADDR, "ADDR_GOT_ADDR ");
  print_devreg(ADDR_CONTROL, "ADDR_CONTROL ");
  print_devreg(ADDR_COMMAND, "ADDR_COMMAND ");
  /*
  print_devreg(ADDR_RESETING0, "ADDR_RESETING0");
  print_devreg(ADDR_RESETING1, "ADDR_RESETING1");
  */
  print_devreg(ADDR_CONTROL_SET0, "ADDR_CONTROL_SET0");
  print_devreg(ADDR_CONTROL_SET1, "ADDR_CONTROL_SET1");
  /*
  print_devreg(ADDR_SW_OVERRIDE_PLL, "ADDR_SW_OVERRIDE_PLL");
  print_devreg(ADDR_PLL_RSTN, "ADDR_PLL_RSTN");
  */
  print_devreg(ADDR_INTR_MASK, "ADDR_INTR_MASK");
  print_devreg(ADDR_INTR_CLEAR, "ADDR_INTR_CLEAR ");
  print_devreg(ADDR_INTR_RAW, "ADDR_INTR_RAW ");
  print_devreg(ADDR_INTR_SOURCE, "ADDR_INTR_SOURCE ");
  /*
  print_devreg(ADDR_INTR_BC_GOT_ADDR_NOT, "ADDR_INTR_BC_GOT_ADDR_NOT ");
  print_devreg(ADDR_BR_WIN, "ADDR_BR_WIN ");
  print_devreg(ADDR_BR_FIFO_FULL, "ADDR_BR_FIFO_FULL ");
  print_devreg(ADDR_BR_FIFO_ERROR, "ADDR_BR_FIFO_ERROR ");
  print_devreg(ADDR_BR_CONDUCTOR_IDLE, "ADDR_BR_CONDUCTOR_IDLE ");
  print_devreg(ADDR_BR_CONDUCTOR_BUSY, "ADDR_BR_CONDUCTOR_BUSY ");
  print_devreg(ADDR_BR_CRC_ERROR, "ADDR_BR_CRC_ERROR ");
  print_devreg(ADDR_BR_FIFO_EMPTY, "ADDR_BR_FIFO_EMPTY ");
  print_devreg(ADDR_BR_BIST_FAIL, "ADDR_BR_BIST_FAIL ");
  print_devreg(ADDR_BR_THERMAL_VIOLTION, "ADDR_BR_THERMAL_VIOLTION ");
  print_devreg(ADDR_BR_NOT_WIN, "ADDR_BR_NOT_WIN ");
  print_devreg(ADDR_BR_0_OVER, "ADDR_BR_0_OVER ");
  print_devreg(ADDR_BR_0_UNDER, "ADDR_BR_0_UNDER ");
  print_devreg(ADDR_BR_1_OVER, "ADDR_BR_1_OVER ");
  print_devreg(ADDR_BR_1_UNDER, "ADDR_BR_1_UNDER ");

  print_devreg(ADDR_MID_STATE, "ADDR_MID_STATE ");
  print_devreg(ADDR_MERKLE_ROOT, "ADDR_MERKLE_ROOT ");
  */
  print_devreg(ADDR_TIMESTEMP, "ADDR_TIMESTEMP ");
  print_devreg(ADDR_DIFFICULTY, "ADDR_DIFFICULTY ");
  print_devreg(ADDR_WIN_LEADING_0, "ADDR_WIN_LEADING_0 ");
  /*
  print_devreg(ADDR_JOB_ID, "ADDR_JOB_ID");

  print_devreg(ADDR_WINNER_JOBID_WINNER_ENGINE,
               "ADDR_WINNER_JOBID_WINNER_ENGINE ");
               */
  print_devreg(ADDR_WINNER_NONCE, "ADDR_WINNER_NONCE");
  print_devreg(ADDR_CURRENT_NONCE, CYAN "ADDR_CURRENT_NONCE" RESET);
//  print_devreg(ADDR_LEADER_ENGINE, "ADDR_LEADER_ENGINE");
  print_devreg(ADDR_VERSION, "ADDR_VERSION");
/*
  print_devreg(ADDR_ENGINES_PER_CHIP_BITS, "ADDR_ENGINES_PER_CHIP_BITS");
  print_devreg(ADDR_LOOP_LOG2, "ADDR_LOOP_LOG2");
  print_devreg(ADDR_ENABLE_ENGINE, "ADDR_ENABLE_ENGINE");
  print_devreg(ADDR_FIFO_OUT, "ADDR_FIFO_OUT");
  print_devreg(ADDR_CURRENT_NONCE_START, "ADDR_CURRENT_NONCE_START");
  print_devreg(ADDR_CURRENT_NONCE_START + 1, "ADDR_CURRENT_NONCE_START+1");
  print_devreg(ADDR_CURRENT_NONCE_START + 1, "ADDR_CURRENT_NONCE_START+2");

  print_devreg(ADDR_CURRENT_NONCE_RANGE, "ADDR_CURRENT_NONCE_RANGE");
  print_devreg(ADDR_FIFO_OUT_MID_STATE_LSB, "ADDR_FIFO_OUT_MID_STATE_LSB");
  print_devreg(ADDR_FIFO_OUT_MID_STATE_MSB, "ADDR_FIFO_OUT_MID_STATE_MSB");
  print_devreg(ADDR_FIFO_OUT_MARKEL, "ADDR_FIFO_OUT_MARKEL");
  print_devreg(ADDR_FIFO_OUT_TIMESTEMP, "ADDR_FIFO_OUT_TIMESTEMP");
  print_devreg(ADDR_FIFO_OUT_DIFFICULTY, "ADDR_FIFO_OUT_DIFFICULTY");
  print_devreg(ADDR_FIFO_OUT_WIN_LEADING_0, "ADDR_FIFO_OUT_WIN_LEADING_0");
  print_devreg(ADDR_FIFO_OUT_WIN_JOB_ID, "ADDR_FIFO_OUT_WIN_JOB_ID");
  */
  print_devreg(ADDR_BIST_NONCE_START, "ADDR_BIST_NONCE_START");
  print_devreg(ADDR_BIST_NONCE_RANGE, "ADDR_BIST_NONCE_RANGE");
  /*
  print_devreg(ADDR_BIST_NONCE_EXPECTED, "ADDR_BIST_NONCE_EXPECTED");
  print_devreg(ADDR_EMULATE_BIST_ERROR, "ADDR_EMULATE_BIST_ERROR");
  print_devreg(ADDR_BIST_PASS, "ADDR_BIST_PASS");
  */
  parse_int_register("ADDR_INTR_RAW", read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_RAW));
  parse_int_register("ADDR_INTR_MASK", read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_MASK));
  parse_int_register("ADDR_INTR_SOURCE", read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_SOURCE));

  // print_engines();
}



// Queues work to actual HW
// returns 1 if found nonce
void push_to_hw_queue_rt(RT_JOB *work) {
  //flush_spi_write();
  int i;
  // printf("Push!\n");
  //
   

  //PUSH_JOB(ADDR_COMMAND,BIT_ADDR_COMMAND_END_CURRENT_JOB_IF_FIFO_FULL);
  PUSH_JOB(ADDR_TIMESTEMP, work->timestamp);
  for (i = 0; i < 8; i++) {
    PUSH_JOB((ADDR_MID_STATE0 + i), work->midstate[i]);
  }
  PUSH_JOB(ADDR_MARKEL_ROOT, work->mrkle_root);
  PUSH_JOB(ADDR_DIFFICULTY, work->difficulty);
#ifdef  REMO_STRESS
  PUSH_JOB(ADDR_WIN_LEADING_0, work->leading_zeroes);
  vm.cur_leading_zeroes = work->leading_zeroes;
#else
  if ((vm.cur_leading_zeroes != work->leading_zeroes)) {
    PUSH_JOB(ADDR_WIN_LEADING_0, work->leading_zeroes);
    vm.cur_leading_zeroes = work->leading_zeroes;
  }
#endif  
  
  passert(work->work_id_in_hw < (0xFF - MQ_INCREMENTS));
  PUSH_JOB(ADDR_JOBID, work->work_id_in_hw);
  //DBG(DBG_WINS,"PUSH:JOB ID:%d  ---\n",work->work_id_in_hw);  
  //write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_COMMAND, BIT_CMD_END_JOB_IF_Q_FULL);
  PUSH_JOB(ADDR_COMMAND, BIT_ADDR_COMMAND_FIFO_LOAD);
  FLUSH_JOB();
  uint32_t status = read_spi(ADDR_SQUID_STATUS);
//  parse_squid_status(status);
  uint32_t q_status = read_spi(ADDR_SQUID_QUEUE_STATUS);
//  parse_squid_q_status(q_status);

}



// destribute range between 0 and max_range
void set_nonce_range_in_asic(int addr,unsigned int max_range) {
  int e,i;
  unsigned int current_nonce = 0;
  unsigned int engine_size;
  int total_engines = ENGINES_PER_ASIC * ASICS_COUNT;
  engine_size = ((max_range) / (total_engines));

  // for (d = FIRST_CHIP_ADDR; d < FIRST_CHIP_ADDR+total_devices; d++) {

  // int h, l;
  for (i=0;i< ASICS_COUNT;i++) {
    if (vm.asic[i].asic_present) {
      if ((i == addr) && (vm.asic[i].asic_present)) {
        psyslog("%d range: %x (%x * 193 )\n", i, current_nonce, engine_size);
        write_reg_asic(i, ANY_ENGINE, ADDR_NONCE_RANGE, engine_size);
      }
     
      for (e = 0; e < ENGINES_PER_ASIC; e++) {        
        if ((i == addr) && (vm.asic[i].asic_present)) {
          write_reg_asic(i, e, ADDR_NONCE_START, current_nonce);
        }
        current_nonce += engine_size;
      }
    }
  }
  //
  flush_spi_write();
  // print_state();
}




// destribute range between 0 and max_range
void set_nonce_range_in_engines(unsigned int max_range) {
  int i;
  for (i=0;i< ASICS_COUNT;i++) {
      set_nonce_range_in_asic(i, max_range);
  }
}


int revive_asics_if_one_got_reset(const char *why) {
  // Validate all got address
  if (read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT) != 0) {
      restart_asics_full(1, "revive_asics_if_one_got_reset");
  }
  return 0;
}



int allocate_addresses_to_devices() {
  int reg;
  int l;
  total_devices = 0;

  // Give resets to all ASICs
  DBG(DBG_HW, "Unallocate all chip addresses\n");
  write_reg_asic(ANY_ASIC, NO_ENGINE, ADDR_GOT_ADDR, 0);

  //  validate address reset worked - not really testing ALL ASICS.
  reg = read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT);
  if (reg == 0) {
    passert(0);
  }

  for (l = 0; l < LOOP_COUNT; l++) {
    // Only in loops discovered in "enable_good_loops_ok()"
    if (vm.loop[l].enabled_loop) {
      // Disable all other loops
      unsigned int bypass_loops = (~(1 << l)) & SQUID_LOOPS_MASK;
   
      psyslog("ADDR_SQUID_LOOP_BYPASS = %x\n", bypass_loops);      
      write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);

      // Give 8 addresses
      int h = 0;
      int asics_in_loop = 0;
      for (h = 0; h < ASICS_PER_LOOP; h++) {
        int addr = l * ASICS_PER_LOOP + h;
        
        vm.asic[addr].address = addr;
        vm.asic[addr].loop_address = l;
        vm.asic[addr].stacked_interrupt_mask = 0xcafebabe;
        if (  vm.asic[addr].dc2dc.dc2dc_present &&
              read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT)) {
          write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_CHIP_ADDR, addr << 8);
          total_devices++;
          asics_in_loop++;
          vm.asic_count++;
          for (int i = 0; i < ENGINE_BITMASCS-1; i++) {
            vm.asic[addr].not_brocken_engines[i] = ENABLED_ENGINES_MASK;
          }
          vm.asic[addr].not_brocken_engines[ENGINE_BITMASCS-1] = 0x1;
          
          vm.asic[addr].not_brocken_engines_count = 
                  count_ones(vm.asic[addr].not_brocken_engines[0]) +
                  count_ones(vm.asic[addr].not_brocken_engines[1]) +
                  count_ones(vm.asic[addr].not_brocken_engines[2]) +
                  count_ones(vm.asic[addr].not_brocken_engines[3]) +
                  count_ones(vm.asic[addr].not_brocken_engines[4]) +
                  count_ones(vm.asic[addr].not_brocken_engines[5]) +
                  count_ones(vm.asic[addr].not_brocken_engines[6]);

          vm.asic[addr].asic_present = 1;
          vm.asic[addr].freq_bist_limit =    (ASIC_FREQ_MAX); // todo - discover
   
        } else {
          if (!vm.asic[addr].dc2dc.dc2dc_present) {
              write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_CHIP_ADDR, addr << 8);
          }

          vm.asic[addr].asic_present = 0;
          for (int i = 0; i < ENGINE_BITMASCS; i++) {
            vm.asic[addr].not_brocken_engines[i] = 0;
          }

          int err;
          //dc2dc_disable_dc2dc(addr,&err);
          //vm.asic[addr].dc2dc.dc2dc_present = 0;
          psyslog("Disabling asic %d dc2dc present: (%d)\n", 
            addr, vm.asic[addr].dc2dc.dc2dc_present);

        }
      }


      
      // Dont remove this print - used by scripts!!!!
      psyslog("%s ASICS in loop %d: %d%s\n",
             (asics_in_loop == 3) ? RESET : RED, l,
             asics_in_loop, RESET);
      int no_addr = read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT);
      if (no_addr != 0) {
        mg_event_x("no_addr = %x", no_addr);
        passert(0);
      }

    } else {
      psyslog(RED "ASICS in loop %d: 0\n" RESET, l);
    }
  }
  psyslog("ADDR_SQUID_LOOP_BYPASS = %x\n", (~(vm.good_loops))&SQUID_LOOPS_MASK);
  write_spi(ADDR_SQUID_LOOP_BYPASS, (~(vm.good_loops))&SQUID_LOOPS_MASK);

  // Validate all got address
  if (read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT) != 0) {
    psyslog(RED "allocate_addresses_to_devices: Someone got no address?? - look:\n" RESET);
    for (int i = 0; i < ASICS_COUNT; i++) { 
      int reg = read_reg_asic(i, NO_ENGINE,ADDR_VERSION);
      psyslog("ASIC %d says:%x\n",i , reg);
    }
    passert(0);
  }

  

  
  //  passert(read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_VERSION) != 0);
  DBG(DBG_HW, "Number of ASICs found: %d (LOOPS=%X) %X\n",
              total_devices,(~(vm.good_loops))&SQUID_LOOPS_MASK,read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_VERSION));
  passert(test_serial(-1) == 1);
  return total_devices;

  // set_bits_offset(i);
}

void memprint(const void *m, size_t n) {
  unsigned int i;
  for (i = 0; i < n; i++) {
    printf("%02x", ((const unsigned char *)m)[i]);
  }
}

void disable_engine(int addr, int eng) {
  enable_reg_debug = 1;
     int bad_eng_word = eng/32;
     int bad_eng_bit  = eng%32;
     psyslog("Disable engine %d:%d 1\n", addr, eng);
     vm.asic[addr].not_brocken_engines[bad_eng_word] =
     vm.asic[addr].not_brocken_engines[bad_eng_word] & ~(1<<bad_eng_bit);
     psyslog("Disable engine %d:%d %x X\n", addr, eng, 
                                           vm.asic[addr].not_brocken_engines[bad_eng_word]);
     disable_engines_asic(addr, 0);
     enable_engines_asic(addr, 
                 vm.asic[addr].not_brocken_engines, 0,0);
     enable_reg_debug = 0;
     vm.asic[addr].not_brocken_engines_count = 
                 count_ones(vm.asic[addr].not_brocken_engines[0]) +
                 count_ones(vm.asic[addr].not_brocken_engines[1]) +
                 count_ones(vm.asic[addr].not_brocken_engines[2]) +
                 count_ones(vm.asic[addr].not_brocken_engines[3]) +
                 count_ones(vm.asic[addr].not_brocken_engines[4]) +
                 count_ones(vm.asic[addr].not_brocken_engines[5]) +
                 count_ones(vm.asic[addr].not_brocken_engines[6]);
}




int SWAP32(int x);
void compute_hash(const unsigned char *midstate, unsigned int mrkle_root,
                  unsigned int timestamp, unsigned int difficulty,
                  unsigned int winner_nonce, unsigned char *hash);
int get_leading_zeroes(const unsigned char *hash);


int get_print_win(int winner_device, int winner_engine) {
  // TODO - find out who one!
  // int winner_device = winner_reg >> 16;
  // enable_reg_debug = 1;
 
  uint32_t winner_nonce; // = read_reg_asic(winner_device, NO_ENGINE,ADDR_WINNER_NONCE);
  uint32_t job_id;    // = read_reg_asic(winner_device, NO_ENGINE,ADDR_WINNER_JOBID);
  uint32_t next_win_reg = 0;
  write_reg_asic(winner_device ,NO_ENGINE, ADDR_INTR_CLEAR, BIT_INTR_WIN);
  write_reg_asic(winner_device, winner_engine, ADDR_COMMAND, BIT_ADDR_COMMAND_WIN_CLEAR);
  push_asic_read(winner_device, winner_engine, ADDR_WINNER_NONCE, &winner_nonce);
  push_asic_read(winner_device, winner_engine, ADDR_WINNER_JOBID, &job_id);
  push_asic_read(ANY_ASIC,NO_ENGINE ,ADDR_INTR_BC_WIN, &next_win_reg);
  squid_wait_asic_reads();

  // Winner ID holds ID and Engine info
  job_id = job_id & 0xFF;

  //DBG(DBG_WINS,"WON:JOB ID:%d,RT_QUEUE:%d, DELTA:%d ---\n",job_id,job_id/MQ_INCREMENTS,job_id%MQ_INCREMENTS);
  RT_JOB *work_in_hw = peak_rt_queue(job_id/MQ_INCREMENTS);

  if ((work_in_hw->work_state == WORK_STATE_HAS_JOB)) {
//  passert((work_in_hw->winner_nonce == 0));
    
//  struct timeval tv;
  if (winner_nonce != 0) {
#if 1
   //start_stopper(&tv);
   DBG(DBG_WINS,"------ FOUND WIN:----\n");
   DBG(DBG_WINS,"- midstate:\n");
   int i;
   for (i = 0; i < 8; i++) {
       DBG(DBG_WINS,"-   %08x\n", work_in_hw->midstate[i]);
   }
   DBG(DBG_WINS,"- mrkle_root: %08x\n", work_in_hw->mrkle_root);
   DBG(DBG_WINS,"- timestamp : %08x\n", work_in_hw->timestamp);
   DBG(DBG_WINS,"- timestamp real: %08x (+%d)\n", SWAP32(SWAP32(work_in_hw->timestamp)+(job_id%MQ_INCREMENTS)),job_id%MQ_INCREMENTS);   
   DBG(DBG_WINS,"- difficulty: %08x\n", work_in_hw->difficulty);
   DBG(DBG_WINS,"--- NONCE = %08x \n- ", winner_nonce);

    memcpy((unsigned char*)vm.last_win.midstate, (const unsigned char*)work_in_hw->midstate, sizeof(work_in_hw->midstate));
    vm.last_win.mrkle_root = work_in_hw->mrkle_root;
    vm.last_win.timestamp = SWAP32(SWAP32(work_in_hw->timestamp)+(job_id%MQ_INCREMENTS));
    vm.last_win.difficulty = work_in_hw->difficulty;
    vm.last_win.winner_nonce = winner_nonce;
    vm.last_win.winner_engine = winner_engine;
    vm.last_win.winner_device = winner_device;
#endif
    DBG(DBG_WINS,"Win. NONCE = %08x asic: %d engine %d\n", winner_nonce, winner_device, winner_engine);

   static unsigned char hash[32];
   //memset(hash,0, 32);
   compute_hash((const unsigned char*)work_in_hw->midstate,
       SWAP32(work_in_hw->mrkle_root),
       SWAP32(work_in_hw->timestamp)+(job_id%MQ_INCREMENTS),
       SWAP32(work_in_hw->difficulty),
       SWAP32(winner_nonce),
       hash);
   //memprint((void*)hash, 32);
   int leading_zeroes = get_leading_zeroes(hash);
   DBG(DBG_WINS,"%sWin leading Zeroes: %d\n" RESET,(leading_zeroes > 8)?GREEN:RED , leading_zeroes);
   if (leading_zeroes < 25) {
       psyslog(RED "HW error job 0x%x, nonce=0x%x, asic %d, consec =%d!!!\n" RESET,
                job_id,  winner_nonce, winner_device, vm.concecutive_hw_errs);
       vm.hw_errs++;
       vm.concecutive_hw_errs++;
       if (vm.asic[winner_device].last_bad_win_engine == winner_engine+1) {
          // Disable engine
          vm.asic[winner_device].last_bad_win_engine_count++;
          if (vm.asic[winner_device].last_bad_win_engine_count >= 4) {
            disable_engine(winner_device,winner_engine);
          }
       } else {
          vm.asic[winner_device].last_bad_win_engine = winner_engine+1;
          vm.asic[winner_device].last_bad_win_engine_count = 0;
       }

#ifdef DOWNSCALE_ON_ERRORS
       if (asic_can_down_freq(winner_device)) {
            vm.asic[winner_device].freq_bist_limit = (vm.asic[winner_device].freq_hw-5);
            asic_down_freq(winner_device, 1,1, "failed bist");
       }
#endif        
       goto done_win;
   }
   //passert(leading_zeroes > 8);
   //end_stopper(&tv, "Win compute");

    //printf("Win\n");
    vm.asic[winner_device].solved_jobs++;
    vm.solved_jobs_total++;
    vm.this_minute_wins++;
    work_in_hw->winner_nonce = winner_nonce;
    work_in_hw->ntime_offset = job_id%MQ_INCREMENTS;
    push_work_rsp(work_in_hw, 0);
    vm.concecutive_hw_errs = 0;
    work_in_hw->ntime_offset = 0;    
    work_in_hw->winner_nonce = 0;
    vm.asic[winner_device].last_bad_win_engine = 0;
   }
  } else {
    psyslog( "!!!!!  Warning !!!!: Win orphan/double job 0x%x, asic %d:%d, nonce=0x%x, %x, consec = %d!!!\n" ,
      job_id, winner_device,  winner_engine, winner_nonce, work_in_hw->winner_nonce,vm.concecutive_hw_errs);
    // FIX FALSE WINS FROM SINGLE ENGINES.
    if (vm.asic[winner_device].last_bad_win_engine == winner_engine+1) {
      // Disable engine
      disable_engine(winner_device,winner_engine);
    } else {
      vm.asic[winner_device].last_bad_win_engine = winner_engine+1;
    }
    vm.concecutive_hw_errs++;
  }
done_win:  
  if (vm.concecutive_hw_errs > 10) {
    // Asics out of sync.
    psyslog( "EXIT::: !!!!!  Warning !!!!: Win orphan/double job\n" );
    vm.concecutive_hw_errs = 0;
    //print_chiko(1);
    vm.err_too_much_fake_wins++;      
    restart_asics_full(3, "consec HW errors");
    return 0;
  }

  return next_win_reg;
}

/*
void fill_random_work(RT_JOB *work) {
 static int id = 1;
 id++;
 memset(work, 0, sizeof(RT_JOB));
 work->midstate[0] = (rand()<<16) + rand();
 work->midstate[1] = (rand()<<16) + rand();
 work->midstate[2] = (rand()<<16) + rand();
 work->midstate[3] = (rand()<<16) + rand();
 work->midstate[4] = (rand()<<16) + rand();
 work->midstate[5] = (rand()<<16) + rand();
 work->midstate[6] = (rand()<<16) + rand();
 work->midstate[7] = (rand()<<16) + rand();
 work->work_id_in_sw = id;
 work->mrkle_root = (rand()<<16) + rand();
 work->timestamp  = (rand()<<16) + rand();
 work->difficulty = (rand()<<16) + rand();
 //work_in_queue->winner_nonce
}*/

void init_scaling();

int init_asics(int addr) {
  // enable_reg_debug = 1;
  // Clearing all interrupts
  write_reg_asic(addr, NO_ENGINE,ADDR_INTR_MASK, 0x0);
  write_reg_asic(addr, NO_ENGINE,ADDR_INTR_CLEAR, 0xffff);
  write_reg_asic(addr,ANY_ENGINE,ADDR_COMMAND, BIT_ADDR_COMMAND_WIN_CLEAR);
  write_reg_asic(addr, NO_ENGINE,ADDR_INTR_CLEAR, 0x0);
  flush_spi_write();
  // print_state();
  return 0;
}


typedef struct {
  uint32_t nonce_winner;
  uint32_t midstate[8];
  uint32_t mrkl;
  uint32_t timestamp;
  uint32_t difficulty;
  uint32_t leading;
} BIST_VECTOR;

/*
#define TOTAL_BISTS 8
BIST_VECTOR bist_tests[TOTAL_BISTS] =
{
{0x1DAC2B7C,
  {0xBC909A33,0x6358BFF0,0x90CCAC7D,0x1E59CAA8,0xC3C8D8E9,0x4F0103C8,0x96B18736,0x4719F91B},
    0x4B1E5E4A,0x29AB5F49,0xFFFF001D,0x26},

{0x227D01DA,
  {0xD83B1B22,0xD5CEFEB4,0x17C68B5C,0x4EB2B911,0x3C3D668F,0x5CCA92CA,0x80E63EEC,0x77415443},
    0x31863FE8,0x68538051,0x3DAA011A,0x20},

{0x481df739,
  {0x4a41f9ac,0x30309e3f,0x06d49e05,0x938f5ceb,0x90ef75e3,0x94fb77e4,0x9e851664,0x3a89aa95},
  0x73ae0eb3, 0xb8d6fb46, 0xf72e17e4, 36},


{0xbf9d8004,
  { 0x55a41303,0xeea9d4d3,0x39aa38f4,0x985a231f,0xbea0d01f,0x23218acc,0x08c65f3f,0x9535c3f7},
  0x86fb069c, 0xb0a01a68, 0xaf7f89b0, 36},


{0xbbff18c5,
  {0x02c397d8,0xececa9bc,0xc31e67f0,0x9a6c1b50,0x684206bc,0xd09e1e48,0x32a30c61,0x4f7f9717},
0xb9a63c7e,0x0fa4fe30,0x7ebf0578,34},



{0x331f38d8,
  { 0x72608c6a,0xb1dae622,0x74e0fd9d,0x80af02b4,0x0d3c4f66,0x3a0868d6,0x30d6d1cd,0x54b0e059},
  0x4dc7d05e, 0x45716f49, 0x25b433d9, 36},

{0x339bc585,
  { 0xa40ebbec,0x6aa3645e,0xc0ef8027,0xb83c0010,0x0eeda850,0xbc407eae,0x0d892b47,0x6b1f7541},
  0xd6b747c9, 0xd12764f2, 0xd026d972, 35},

{0xbbed44a1 ,
  {   0x0dd441c1,0x5802c0da,0xf2951ee4,0xd77909da,0xb25c02cb,0x009b6349,0xfe2d9807,0x257609a8 },
  0x5064218c, 0x57332e1b, 0xcef3abae, 35}
};
*/

int do_bist_loop_push_job(const char* why) {
  int f;
  int i = 0;
  // Save engines needed only at minimal freq so we can disable them.
  if (vm.i2c_busy_with_bug) {
    psyslog(RED "SKIPPED BIST BECAUSE OF AC2DC BUG (%s)\n", why);
    return 0;
  }
  psyslog(YELLOW "Do BIST LOOP- %s\n" RESET, why);
  while ((f = do_bist_ok(true,true,1 , why))) {
    DBG(DBG_SCALING, "BIST %x FAILED: %x asics failed bist\n", i, f);
    i++;
  }

   try_push_job_to_mq();
   try_push_job_to_mq();  
}


void on_failed_bist(int addr, bool store_limit, bool step_down_if_failed) {
  DBG(DBG_SCALING, "FAILED BIST %d: sl:%d sd:%d, rate:%d\n",
          addr,
          store_limit,
          step_down_if_failed,
          (vm.asic[addr].freq_hw));

  passert(vm.asic[addr].asic_present);
  if (asic_can_down_freq(addr)) { 
    DBG(DBG_SCALING, "ASIC down XXT:%d (%d)\n", addr, vm.asic[addr].freq_bist_limit);
    passert(vm.asic[addr].freq_bist_limit > ASIC_FREQ_MIN);
    vm.asic[addr].bist_failed_called = 1;
    if (store_limit) {
      vm.asic[addr].freq_bist_limit = (int)(vm.asic[addr].freq_hw-5);
      DBG(DBG_SCALING, "Store bist limit %d:%d\n", addr, vm.asic[addr].freq_bist_limit );      
    }
    if (step_down_if_failed) {
      asic_down_freq(addr, 1,1, "failed bist");
    }
  } else {
    // Disable engines.
    push_asic_read(addr, NO_ENGINE, ADDR_WIN_0, &vm.asic[addr].not_brocken_engines[0]);
    push_asic_read(addr, NO_ENGINE, ADDR_WIN_1, &vm.asic[addr].not_brocken_engines[1]);
    push_asic_read(addr, NO_ENGINE, ADDR_WIN_2, &vm.asic[addr].not_brocken_engines[2]);
    push_asic_read(addr, NO_ENGINE, ADDR_WIN_3, &vm.asic[addr].not_brocken_engines[3]);
    push_asic_read(addr, NO_ENGINE, ADDR_WIN_4, &vm.asic[addr].not_brocken_engines[4]);
    push_asic_read(addr, NO_ENGINE, ADDR_WIN_5, &vm.asic[addr].not_brocken_engines[5]);
    push_asic_read(addr, NO_ENGINE, ADDR_WIN_6, &vm.asic[addr].not_brocken_engines[6]);
    
    squid_wait_asic_reads();
    vm.asic[addr].not_brocken_engines[6] &= 0x1;

    vm.asic[addr].not_brocken_engines_count = 
            count_ones(vm.asic[addr].not_brocken_engines[0]) +
            count_ones(vm.asic[addr].not_brocken_engines[1]) +
            count_ones(vm.asic[addr].not_brocken_engines[2]) +
            count_ones(vm.asic[addr].not_brocken_engines[3]) +
            count_ones(vm.asic[addr].not_brocken_engines[4]) +
            count_ones(vm.asic[addr].not_brocken_engines[5]) +
            count_ones(vm.asic[addr].not_brocken_engines[6]);

    // Disable limit?
      // If minimal rate - disable engines!
    psyslog("Disabling engines on ASIC %d: %x-%x-%x-%x %x-%x-%x\n", addr,
      vm.asic[addr].not_brocken_engines[0],
      vm.asic[addr].not_brocken_engines[1],
      vm.asic[addr].not_brocken_engines[2],
      vm.asic[addr].not_brocken_engines[3],
      vm.asic[addr].not_brocken_engines[4],
      vm.asic[addr].not_brocken_engines[5],
      vm.asic[addr].not_brocken_engines[6]
    );
    if (vm.asic[addr].not_brocken_engines_count < 100) {
      disable_asic_forever_rt(addr, "Asic all engines fail BIST");
    } else {
      disable_engines_asic(addr,0);
      enable_engines_asic(addr, vm.asic[addr].not_brocken_engines, 0,1);
    }
    vm.asic[addr].freq_bist_limit = ASIC_FREQ_MAX-200;
  }

  // Silence all interrupts temporary
  push_asic_read(addr, NO_ENGINE, ADDR_INTR_MASK, &vm.asic[addr].stacked_interrupt_mask);
  write_reg_asic(addr, NO_ENGINE, ADDR_INTR_MASK, 0xFFFF);
  squid_wait_asic_reads();
}


int count_ones(uint32_t i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}


// returns 1 on failed
// If ASIC rate is minimal, we still store the engines.
int do_bist_ok(bool store_limit, bool step_down_if_failed, int fast_bist ,const char *why) {
  int some_one_failed = 0;

  // PAUSE FPGA MQ TODO
#ifndef RESET_MQ_ON_BIST  
  configure_mq(MQ_TIMER_USEC, MQ_INCREMENTS, 1);
#endif

  // Wait for IDLE - todo - is this needed??
  int reg;
  int i = 0;
  //DBG(DBG_BIST,"WAITING MQ (%s)\n",why);
  //struct timeval tv;
  //start_stopper(&tv);
  revive_asics_if_one_got_reset("do_bist_ok");
#ifdef RESET_MQ_ON_BIST  
  write_spi(ADDR_SQUID_COMMAND, BIT_COMMAND_MQ_FIFO_RESET);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_COMMAND, BIT_ADDR_COMMAND_END_CURRENT_JOB);  
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_COMMAND, BIT_ADDR_COMMAND_END_CURRENT_JOB);
  flush_spi_write();    
#endif


  i=0;
  //return 0;
  while ((reg = read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_CONDUCTOR_BUSY)) != 0) {
    poll_win_temp_rt();
    if (i++ > 3000) {
      //end_stopper(&tv,"MQ WAIT BAD");
      psyslog(RED "JOB %x stuck, killing ASIC X 2\n" RESET, reg);
      //disable_asic_forever_rt(addr, "bist cant start, stuck Y");
      print_chiko(1);
      vm.err_stuck_bist++;        
      restart_asics_full(4, "stuck ASIC");
      return 0;
    }
    usleep(1);
  }
  
  //end_stopper(&tv,"MQ WAIT OK");
  //DBG(DBG_BIST,"MQ-DONE\n");

  // ENTER BIST MODE
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_CONTROL_SET1, BIT_ADDR_CONTROL_BIST_MODE);


  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MID_STATE0, 0xBC909A33);//0xBC909A33
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MID_STATE1, 0x6358BFF0);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MID_STATE2, 0x90CCAC7D);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MID_STATE3, 0x1E59CAA8);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MID_STATE4, 0xC3C8D8E9);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MID_STATE5, 0x4F0103C8);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MID_STATE6, 0x96B18736);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MID_STATE7, 0x4719F91B);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_MARKEL_ROOT,0x4B1E5E4A);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_TIMESTEMP,  0x29AB5F49);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_DIFFICULTY, 0xFFFF001D);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_WIN_LEADING_0, 37); 
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_JOBID, 33);
#if 1 
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_BIST_NONCE_RANGE, 0x2200);  // 
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_BIST_NONCE_START, 0x1dac0a7d+rand()%6); //  
#else
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_BIST_NONCE_RANGE, 0x200);  // 0x200
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_BIST_NONCE_START, 0x1dac2a7d); //0x1dac2a7d  
#endif
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_BIST_CRC_EXPECTED, 0xbde23fff);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_BIST_ALLOWED_FAILURE_NUM, ALLOWED_BIST_FAILURE_ENGINES);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_COMMAND, BIT_ADDR_COMMAND_FIFO_LOAD);
  flush_spi_write();

   while ((reg = read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_CONDUCTOR_BUSY)) != 0) {
    if (i++ > 10000) {
      //end_stopper(&tv,"MQ WAIT BAD");
      psyslog(RED "JOB %x stuck, killing ASIC X 3\n" RESET, reg);
      //return 0;
      int addr = BROADCAST_READ_ADDR(reg);
      disable_asic_forever_rt(addr, "bist cant start, stuck X");
    }
    usleep(1);
  }
  // READ BIST FAILURES
  //bool engines_disabled = false;
  for (int addr = 0; addr < ASICS_COUNT; addr++) {
    ASIC* a = &vm.asic[addr];
    if (a->asic_present) { 
     push_asic_read(addr, NO_ENGINE, ADDR_WIN_0, &a->last_bist_passed_engines[0]);
     push_asic_read(addr, NO_ENGINE, ADDR_WIN_1, &a->last_bist_passed_engines[1]);     
     if (!fast_bist) {
       push_asic_read(addr, NO_ENGINE, ADDR_WIN_1, &a->last_bist_passed_engines[1]);
       push_asic_read(addr, NO_ENGINE, ADDR_WIN_2, &a->last_bist_passed_engines[2]);
       push_asic_read(addr, NO_ENGINE, ADDR_WIN_3, &a->last_bist_passed_engines[3]);
       push_asic_read(addr, NO_ENGINE, ADDR_WIN_4, &a->last_bist_passed_engines[4]);
       push_asic_read(addr, NO_ENGINE, ADDR_WIN_5, &a->last_bist_passed_engines[5]);
       push_asic_read(addr, NO_ENGINE, ADDR_WIN_6, &a->last_bist_passed_engines[6]);
     }
    }
  }
  squid_wait_asic_reads();



  for (int addr = 0; addr < ASICS_COUNT; addr++) {
    uint32_t bist_passed[ENGINE_BITMASCS]; //     
    ASIC* a = &vm.asic[addr];
    if (a->asic_present) {
        int passed = 0;
     
      if (!fast_bist) {
        
        int bad_asic = (a->last_bist_passed_engines[6] > 0x1);
        if (bad_asic) {
          psyslog("ASIC %d is evil\n", addr);
          disable_asic_forever_rt(addr, "Very Evil ASIC");
          continue;
        }
        
        passed =
          (a->last_bist_passed_engines[0] == a->not_brocken_engines[0]) &&
          (a->last_bist_passed_engines[1] == a->not_brocken_engines[1]) &&
          (a->last_bist_passed_engines[2] == a->not_brocken_engines[2]) &&
          (a->last_bist_passed_engines[3] == a->not_brocken_engines[3]) &&
          (a->last_bist_passed_engines[4] == a->not_brocken_engines[4]) &&
          (a->last_bist_passed_engines[5] == a->not_brocken_engines[5]) &&
          (a->last_bist_passed_engines[6] == a->not_brocken_engines[6]);
        
          DBG(DBG_SCALING, "BIST:%s\n"
                "%d: WORKING ENG: %x-%x-%x-%x %x-%x-%x\n"
                "    WINS:        %x-%x-%x-%x %x-%x-%x\n"
                "    PASSED 1s:   %d of %d\n", 
                    why,
                    addr,
                    a->not_brocken_engines[0],
                    a->not_brocken_engines[1],
                    a->not_brocken_engines[2],
                    a->not_brocken_engines[3],
                    a->not_brocken_engines[4],
                    a->not_brocken_engines[5],
                    a->not_brocken_engines[6],
                    a->last_bist_passed_engines[0],
                    a->last_bist_passed_engines[1],
                    a->last_bist_passed_engines[2],
                    a->last_bist_passed_engines[3],
                    a->last_bist_passed_engines[4],
                    a->last_bist_passed_engines[5],
                    a->last_bist_passed_engines[6],
                    0,
                    vm.asic[addr].not_brocken_engines_count);
          
      } else {
          passed = (vm.asic[addr].not_brocken_engines[0] == 
                    vm.asic[addr].last_bist_passed_engines[0]) &&
                   (vm.asic[addr].not_brocken_engines[1] == 
                    vm.asic[addr].last_bist_passed_engines[1]);
      }

  
     if  (!passed) {   
        some_one_failed++;
        vm.this_min_failed_bist++;
        DBG(DBG_SCALING, RED "FAILED BIST %d at freq %d\n" RESET, addr, vm.asic[addr].freq_hw);
        on_failed_bist(addr, store_limit, step_down_if_failed);
     }
    }  
  }
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_COMMAND, BIT_ADDR_COMMAND_WIN_CLEAR);
  
  DBG(DBG_SCALING, "FAILED BIST %d ASICS\n", some_one_failed);
  //end_stopper(&tv,"READ BIST RES");


  // Return all interrupts
  for (int i = 0 ; i < ASICS_COUNT; i++) {
    ASIC *a = &vm.asic[i];
    if (a->asic_present && (vm.asic[i].stacked_interrupt_mask != 0xcafebabe)) {
      write_reg_asic(i, NO_ENGINE, ADDR_INTR_MASK, vm.asic[i].stacked_interrupt_mask);
      vm.asic[i].stacked_interrupt_mask = 0xcafebabe;
    }
  }
  flush_spi_write();



  enable_good_engines_all_asics_ok(0);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_WIN_LEADING_0, vm.cur_leading_zeroes);
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_CONTROL_SET0, BIT_ADDR_CONTROL_BIST_MODE);
  // write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_CONTROL_SET1, BIT_ADDR_CONTROL_FIFO_RESET_N);


  // Continue MQ
//#ifndef RESET_MQ_ON_BIST      
    configure_mq(MQ_TIMER_USEC, MQ_INCREMENTS, 0);
//#endif
    return some_one_failed;
}



int pull_work_req(RT_JOB *w);

int has_work_req();


void set_initial_voltage_freq_on_restart() {
   int f;
   int err;
   // SET VOLTAGE
   for (int i = 0 ; i < ASICS_COUNT; i++) {
      ASIC *a = &vm.asic[i];
      if (a->asic_present) {
        // Set voltage
        dc2dc_set_vtrim(i, a->dc2dc.vtrim-1, 1, &err, "RESTART custom vtrim per loop");
      }
   }
   usleep(1000);
  
   // 1) Set initial freq to minimum.
   for (int i = 0 ; i < ASICS_COUNT; i++) {
     ASIC *a = &vm.asic[i];
     if (a->asic_present) {
       DBG(DBG_SCALING, CYAN "TRY ENABLE ASIC %d\n" RESET, i);
       DBG(DBG_SCALING, CYAN "TRY ENABLE PRESENT ASIC %d\n" RESET, i);
       set_pll(i, a->freq_hw, 1,1, "RESTART Initial freq");
     } else {
        
     }
   }
   //usleep(50000);
   read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT);
   wait_dll_ready(ANY_ASIC, "after initial");
  
   // 2) Those who fail - disable failed engines
   do_bist_ok(true,true, 0, "restart bist");
   do_bist_ok(true,true, 0, "restart bist");


   psyslog("RESTART ASICS DONE\n");
}


void set_initial_voltage_freq() {
  int f;
  int err;
  // SET VOLTAGE
  for (int i = 0 ; i < ASICS_COUNT; i++) {
     ASIC *a = &vm.asic[i];
     if (a->asic_present) {
       // Set voltage
       if (a->dc2dc.before_failure_vtrim) {
          int last_vtrim = a->dc2dc.before_failure_vtrim; // for heat
          int default_vtrim = (IS_TOP_ASIC(i))?vm.vtrim_start_top:vm.vtrim_start_bottom;
          dc2dc_set_vtrim(i, 
                          MAX(last_vtrim, default_vtrim), 
                          1, &err, "Initial custom vtrim per loop");
       } else {
         dc2dc_set_vtrim(i, 
                          (IS_TOP_ASIC(i))?vm.vtrim_start_top:vm.vtrim_start_bottom, 
                          1, &err, "Initial vtrim per loop");
       }
       a->dc2dc.max_vtrim_user = VOLTAGE_TO_VTRIM_MILLI(vm.voltage_max);
     }
  }
  usleep(1000);

  // 1) Set initial freq to minimum.
  for (int i = 0 ; i < ASICS_COUNT; i++) {
    DBG(DBG_SCALING, CYAN "TRY ENABLE ASIC %d\n" RESET, i);
    ASIC *a = &vm.asic[i];
    if (a->asic_present) {
      DBG(DBG_SCALING, CYAN "TRY ENABLE PRESENT ASIC %d\n" RESET, i);
      // Set FREQ
      a->freq_bist_limit = ASIC_FREQ_MAX-2;
      disable_engines_asic(i, 0);
      set_pll(i, ASIC_FREQ_MIN, 1,0, "Initial freq");
      //usleep(500);
      enable_engines_asic(i,vm.asic[i].not_brocken_engines, 0,0);
    }
  }
  //usleep(50000);
  read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT);
  wait_dll_ready(ANY_ASIC, "after initial");

  // 2) Those who fail - disable failed engines
  int jj = 0;
  while ((f = do_bist_ok(true,true, 0, "init and disable bad engines")) || (jj++ < 10)) {
    psyslog("MINIAL BIST FAILED: %x asics failed bist\n", f);
  }
  psyslog("MINIAL BIST DONE\n");

  

  // 3) start raising frequency
  //     - those that fail - store bist limit.
  bool all_limits_found = false;
  while (!all_limits_found) {
    //psyslog("! all_limits_found\n");
    all_limits_found = true;
    do_bist_ok(true,true,1,"all_limits_found why");
    for (int i = 0 ; i < ASICS_COUNT; i++) {
     ASIC *a = &vm.asic[i];
     if (a->asic_present) {
       //a->freq_bist_limit = ASIC_FREQ_MAX-3;
       //psyslog(CYAN "LOOOKING AT ASIC %d (limit = %d)\n" RESET, i, (a->freq_bist_limit));
       if (!a->bist_failed_called) {
         all_limits_found = false;
         if (a->freq_hw < ASIC_FREQ_MAX - 20) {
          set_pll(i, (int)(a->freq_hw+10), 1,1, "initial stepup");
         } else {
          disable_asic_forever_rt(i, "Runaway ASIC Bob");
         }
       } 
     }
   }
  }
  psyslog("UPSCALE DONE\n");

  // Give 2 clicks bonus
  /*
  for (int i = 0 ; i < ASICS_COUNT; i++) {
   ASIC *a = &vm.asic[i];
   if (a->asic_present) {    
      if (a->freq_bist_limit < (ASIC_FREQ_MAX - 50)) {
         a->freq_bist_limit += 20;
      }
      asic_up_freq_max(i,1,1,"Bonus");
    }
  }
  */
}

 
  //returns better ASIC to upvolt of the 2
int better_asic_to_upvolt(int i, int j) {
    ASIC *ai = &vm.asic[i];
    ASIC *aj = &vm.asic[j];  
    if (i == -1 || (!ai->asic_present) || ai->dc2dc.revolted) return j;
    if (j == -1 || (!aj->asic_present) || ai->dc2dc.revolted) return i;

  
    if (!dc2dc_can_up(i)) return j;
    if (!dc2dc_can_up(j)) return i;
    
    // Return one with lower WATs
#if 1    
    return (ai->dc2dc.vtrim > aj->dc2dc.vtrim)? j : i ;
#else
// Return one with lower WATs
   return (
           (ai->dc2dc.dc_power_watts_16s*10000)/ai->freq_hw > 
           (aj->dc2dc.dc_power_watts_16s*10000)/aj->freq_hw)? j : i ;

#endif
}



 
//returns better ASIC to upvolt of the 2
int better_asic_to_downvolt(int i, int j) {
    ASIC *ai = &vm.asic[i];
    ASIC *aj = &vm.asic[j];  
    if (i == -1 || (!ai->asic_present) || ai->dc2dc.revolted) return j;
    if (j == -1 || (!aj->asic_present) || ai->dc2dc.revolted) return i;

  
    if (!dc2dc_can_down(i)) return j;
    if (!dc2dc_can_down(j)) return i;
    
    // Return one with lower WATs
#if 1    
    return (ai->dc2dc.vtrim < aj->dc2dc.vtrim)? j : i ;
#else
// Return one with lower WATs
   return (
           (ai->dc2dc.dc_power_watts_16s*10000)/ai->freq_hw > 
           (aj->dc2dc.dc_power_watts_16s*10000)/aj->freq_hw)? j : i ;

#endif
}



int best_asic_to_upvolt(int psu_id) {
  int best = -1;
  if (vm.ac2dc[psu_id].ac2dc_power_limit - vm.ac2dc[psu_id].ac2dc_power < 5) {
    return -1;
  }
  int delta = (psu_id == PSU_TOP)?0:ASICS_PER_BOARD;
  for (int i = delta; i<delta+ASICS_PER_BOARD; i++) {
    //printf("Who better of (%d %d)?", best, i);    
    best =  better_asic_to_upvolt(i, best);
    //printf(" %d\n", best);        
  }
  return best;
}




int best_asic_to_downvolt(int psu_id) {
  int best = -1;
  int delta = (psu_id == PSU_TOP)?0:ASICS_PER_BOARD;
  for (int i = delta; i<delta+ASICS_PER_BOARD; i++) {
    //printf("Who better of (%d %d)?", best, i);    
    best =  better_asic_to_downvolt(i, best);
    //printf(" %d\n", best);        
  }
  return best;
}


void print_chiko(int with_asics) {
  print_stack();
  for (int xxx = 0; xxx < ASICS_COUNT ; xxx++) {
      ASIC *a = &vm.asic[xxx];
      if (a->asic_present) {
        if (vm.asic[xxx].dc2dc.dc_current_16s < 7*16) {
          //mg_event_x("Lazy asic Chiko1 %d",xxx);
        }
        psyslog("ASIC %d power:%d volt:%d ISR:%x\n",xxx,
                vm.asic[xxx].dc2dc.dc_current_16s/16,
                vm.asic[xxx].dc2dc.voltage_from_dc2dc,
                (with_asics)?read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_RAW):0xcafebabe
                );
      }
  }
}

void once_second_scaling_logic() {
  int can_upscale = 1;
  int err;
  struct timeval tv;
  DBG(DBG_SCALING,"once_second_scaling_logic start\n");

  if (((one_sec_counter % BIST_PERIOD_SECS) == 0) ||
      ((one_sec_counter % BIST_PERIOD_SECS) == 7) ||
      ((one_sec_counter % BIST_PERIOD_SECS) == 11)       
      ) {
    //printf("TIME=%d\n", time(NULL));
    update_ac2dc_stats();
  }

#ifdef RUNTIME_BIST
    static int current_dc2dc_to_up = 0; 
    // Periodcally up DC voltage
    if (
        (
         (vm.mining_time < 60*10) 
          && 
         ((one_sec_counter % BIST_PERIOD_SECS) == 5)
        )
        ||
        ((one_sec_counter  % BIST_PERIOD_SECS_LONG) == 5)
       )
      {
      passert((!vm.i2c_busy_with_bug));
      DBG(DBG_SCALING,"BIST!\n");
      int i=0; 

      for (int xx = 0; xx < ASICS_COUNT ; xx++) {
        vm.asic[xx].dc2dc.revolted = 0;
      }
      
      //end_stopper(&tv, "UPVOLT1");
      if (vm.board_cooling_now[PSU_TOP] == 0) {
        while(i < DC2DCS_TO_UPVOLT_EACH_BIST_PER_BOARD) {
          // Find most optimal to up-volt
          int best = best_asic_to_upvolt(PSU_TOP);
          if ((best == -1) || (!dc2dc_can_up(best))) {
            DBG(DBG_SCALING,"CANNOT UPSCALE %d\n", best);
            break;
          } else {
            i++;
            ASIC *a = &vm.asic[best];
            DBG(DBG_SCALING,"UPSCALE %d\n", best);
            dc2dc_up(best,&err,"upvolt time 1");
          }
        }
      }
      //end_stopper(&tv, "UPVOLT2");
      i=0; 
      if (vm.board_cooling_now[PSU_BOTTOM] == 0) {
        while(i < DC2DCS_TO_UPVOLT_EACH_BIST_PER_BOARD) {
            // Find most optimal to up-volt
            int best = best_asic_to_upvolt(PSU_BOTTOM);
            if ((best == -1) || (!dc2dc_can_up(best))) {
              DBG(DBG_SCALING,"CANNOT UPSCALE %d\n", best);
              break;
            } else {
              i++;
              ASIC *a = &vm.asic[best];
              dc2dc_up(best,&err,"upvolt time 2");
            }
         }
      }
      start_stopper(&tv);      
      do_bist_loop_push_job("BIST_PERIOD_SECS");
      end_stopper(&tv, "BIST");     
    }
#endif
  int needs_reset = 0;
  for (int xx = 0; xx < ASICS_COUNT ; xx++) {
    ASIC *a = &vm.asic[xx];
    if ((a->asic_present) && (a->dc2dc.dc_current_16s < (7*16))) {
        needs_reset = 1;
        psyslog("Lazy ASIC %d\n", xx);
    } 
    vm.asic[xx].dc2dc.revolted = 0;
  }

  if (needs_reset) {
    restart_asics_full(5, "Lazy asic");
    return;
  }
  
  // If AC2DC too high - reduce voltage on ASIC
  if ((!vm.i2c_busy_with_bug)) {
    int do_bist = 0;
    if ((vm.ac2dc[PSU_TOP].ac2dc_power_limit+4) < (vm.ac2dc[PSU_TOP].ac2dc_power_now
                                                  +vm.ac2dc[PSU_TOP].ac2dc_power_last)/2) {
      // down random DC2DC
      int i = 5;
      do_bist = 1;
      vm.ac2dc[PSU_TOP].ac2dc_power_last = vm.ac2dc[PSU_TOP].ac2dc_power_now;
      vm.ac2dc[PSU_TOP].ac2dc_power_now = 0;
      while(i) {
        int addr = best_asic_to_downvolt(PSU_TOP);  
        ASIC *a = &vm.asic[addr];
        if (a->asic_present) {
          if (dc2dc_can_down(addr)) {
            dc2dc_down(addr,&err,"TOP AC2DC too high A");
            i--;
          }
        }
      }
    }

    if ((vm.ac2dc[PSU_BOTTOM].ac2dc_power_limit+4) < (vm.ac2dc[PSU_BOTTOM].ac2dc_power_now +
                                                     vm.ac2dc[PSU_BOTTOM].ac2dc_power_last)/2) {
      // down random DC2DC
      int i = 5;
      do_bist = 1;
      vm.ac2dc[PSU_BOTTOM].ac2dc_power_last = vm.ac2dc[PSU_BOTTOM].ac2dc_power_now;
      vm.ac2dc[PSU_BOTTOM].ac2dc_power_now = 0;
      while(i) {
        int addr = best_asic_to_downvolt(PSU_BOTTOM);  
        ASIC *a = &vm.asic[addr];
        if (a->asic_present) {
          if (dc2dc_can_down(addr)) {
            dc2dc_down(addr,&err,"TOP AC2DC too high B");
            i--;
          }
        }
      }
    }
    
    for (int xx = 0; xx < ASICS_COUNT ; xx++) {
        vm.asic[xx].dc2dc.revolted = 0;
    }
    if (do_bist) {
      do_bist_loop_push_job("AC2DC too high");
    }
  }



  // Periodic temperature loop
  for (int i = 0 ; i < ASICS_COUNT; i++) {
    ASIC *a = &vm.asic[i];
    
    if (a->asic_present && (!vm.i2c_busy_with_bug)) {
      act_on_temperature(i, &can_upscale);
    }

    
    int max_freq = a->freq_bist_limit;

    if (a->asic_present &&
        !a->cooling_down && 
        (a->freq_hw < max_freq) &&
        asic_can_up_freq(i)) {
      asic_up_freq(i, 1,1, "because possible");
    }
  }
    DBG(DBG_SCALING,"once_second_scaling_logic stop\n");
}






void one_minute_tasks() {
  // Give them chance to raise over 3 hours if system got colder
  vm.last_minute_wins = vm.this_minute_wins;
  //vm.this_min_failed_bist = 0;
  // basic win at 20lz lz is 1MB, divide by 60 seconds.
  vm.last_minute_rate_mb = ((1<<(vm.cur_leading_zeroes-20)) * vm.last_minute_wins); 
  vm.last_minute_rate_mb /= 60;
  vm.idle_asic_cycles_last_min = vm.idle_asic_cycles_this_min;  
  psyslog("***********************************\n");  
  psyslog("Last minute rate: %d (mining:%d, notmining:%d, idle promil=%d)\n", 
    (vm.last_minute_rate_mb),1<<(vm.cur_leading_zeroes-20),
    ((1<<(vm.cur_leading_zeroes-20)) * vm.last_minute_wins),
    vm.last_minute_wins, vm.mining_time, vm.not_mining_time,
    (vm.idle_asic_cycles_last_min*100) / 6000000
  )
  psyslog("***********************************\n");      


  vm.idle_asic_cycles_this_min = 0;
  vm.this_minute_wins = 0;   
}

// For Remo use only
void print_production() {
  int err;
  int hash_power = 0;
  FILE *f = fopen("/var/log/production", "w");
  if (!f) {
    psyslog("Failed to save ASIC production state\n");
    return;
  }
  
  fprintf(f,  "PSU-TOP[%s]: %dw, %dv\n",
      psu_get_name(vm.ac2dc[PSU_TOP].ac2dc_type),
      vm.ac2dc[PSU_TOP].ac2dc_power,
      vm.ac2dc[PSU_TOP].voltage
    );
  fprintf(f,  "PSU-BOT[%s]: %dw, %dv\n",
      psu_get_name(vm.ac2dc[PSU_BOTTOM].ac2dc_type),
      vm.ac2dc[PSU_BOTTOM].ac2dc_power,
      vm.ac2dc[PSU_BOTTOM].voltage
    );

  fprintf(f,  "OH: 0x%x\n",
    vm.board_cooling_ever);

  fprintf(f,  "AC2DCBUG: %x %x\n", vm.bad_ac2dc[PSU_TOP], vm.bad_ac2dc[PSU_BOTTOM]);


  int good_loops = 0;
  int good_asics = 0;  
  for (int l = 0; l < LOOP_COUNT; l++) {
    
    if (vm.loop[l].enabled_loop) {
      fprintf(f,"LOOP[%d] ON \n" ,l);
      good_loops++;
    } else {
      fprintf(f,"LOOP[%d] OFF \n" ,l);
    }

  for (int addr = l*ASICS_PER_LOOP; addr < (l+1)*ASICS_PER_LOOP; addr++) {
     ASIC *a = &vm.asic[addr];
     DC2DC* dc2dc = &vm.asic[addr].dc2dc;
  
     if (dc2dc->dc2dc_present && vm.loop[l].enabled_loop) {
        fprintf(f, "%2d: DC2DC:[vlt:%3d %2dW %2dA %2dc]",
           addr,
           VTRIM_TO_VOLTAGE_MILLI(dc2dc->vtrim),
           dc2dc->dc_power_watts_16s/16,
           dc2dc->dc_current_16s/16,
           dc2dc->dc_temp
        );

        hash_power +=  (a->freq_hw)*ENGINES_PER_ASIC;
        good_asics++;

        fprintf(f, " ASIC:[%3dhz %dc %08x %08x %08x %08x %08x %08x %08x (%d) [F:%x] W:%d]\n",
            (a->freq_hw),
            (a->asic_temp*5)+85,
           a->not_brocken_engines[0],
           a->not_brocken_engines[1],
           a->not_brocken_engines[2],
           a->not_brocken_engines[3],
           a->not_brocken_engines[4],
           a->not_brocken_engines[5],
           a->not_brocken_engines[6],
           a->not_brocken_engines_count,
           a->faults,
           a->solved_jobs
           );
       }else {
         fprintf(f,"%d: NA\n", addr);
       }
     }      
  }
  fprintf(f,"LOOPS:%d, ASICS:%d RATE:%dmh/s\n",good_loops,good_asics, hash_power);

  fprintf(f, "%d err_restarted\n", vm.err_restarted);  
  fprintf(f, "%d err_unexpected_reset\n", vm.err_unexpected_reset);  
  fprintf(f, "%d err_unexpected_reset2\n", vm.err_unexpected_reset2);  
  fprintf(f, "%d err_too_much_fake_wins\n", vm.err_too_much_fake_wins);  
  fprintf(f, "%d err_stuck_bist\n", vm.err_stuck_bist);  
  fprintf(f, "%d err_low_power_chiko\n", vm.err_low_power_chiko);  
  fprintf(f, "%d err_stuck_pll\n", vm.err_stuck_pll);  
  fprintf(f, "%d err_runtime_disable\n", vm.err_runtime_disable);  
  fprintf(f, "%d err_purge_queue\n", vm.err_purge_queue);  
  fprintf(f, "%d err_read_timeouts\n", vm.err_read_timeouts);    
  fprintf(f, "%d err_dc2dc_oc\n", vm.err_dc2dc_oc);    
  fprintf(f, "%d err_read_timeouts2\n", vm.err_read_timeouts2);  
  fprintf(f, "%d err_read_corruption\n", vm.err_read_corruption);  
  fprintf(f, "%d err_purge_queue3\n", vm.err_purge_queue3);  
  fclose(f);  
}

//void store_last_voltage();

void print_scaling() {
  int err;
  vm.total_mhash = 0;
  int total_hash[2];
  total_hash[0] = total_hash[1] = 0;
  FILE *f = fopen("/var/log/asics", "w");
  if (!f) {
    psyslog("Failed to save ASIC state\n");
    return;
  }

  //asic_iter_init(&hi);

  int wanted_hash_power=0;
  int total_loops=0;
  int total_asics=0;
  int expected_rate=0;
  fprintf(f, GREEN "Uptime:%d | FPGA ver:%d\n" , time(NULL) - vm.start_run_time, vm.fpga_ver);



  fprintf(f,  "%s-----TOP-BOARD-----\n"
    "PSU-TOP[%s]: %d->%dw[%d %d %d] (lim=%d) %dc cooling:%d/0x%x\n" RESET,
      ((vm.ac2dc[PSU_TOP].ac2dc_type == AC2DC_TYPE_NONE)?RED:GREEN),
      psu_get_name(vm.ac2dc[PSU_TOP].ac2dc_type),
      vm.ac2dc[PSU_TOP].ac2dc_in_power,
      vm.ac2dc[PSU_TOP].ac2dc_power, 
      vm.ac2dc[PSU_TOP].ac2dc_power_now, 
      vm.ac2dc[PSU_TOP].ac2dc_power_last, 
      vm.ac2dc[PSU_TOP].ac2dc_power_last_last,       
      vm.ac2dc[PSU_TOP].ac2dc_power_limit,
      vm.ac2dc[PSU_TOP].ac2dc_temp,
      vm.board_cooling_now[PSU_TOP],
      vm.board_cooling_ever
      
    );


  int total_watt=0;

  for (int l = 0; l < LOOP_COUNT; l++) {
    if (l == LOOP_COUNT/2) {
      fprintf(f, "%s----BOTTOM BOARD------\n"
      "PSU-BOT[%s]: %d->%dw[%d %d %d] (lim=%d) %dc cooling:%d/0x%x\n" RESET,
      ((vm.ac2dc[PSU_BOTTOM].ac2dc_type == AC2DC_TYPE_NONE)?RED:GREEN),
        psu_get_name(vm.ac2dc[PSU_BOTTOM].ac2dc_type),
        vm.ac2dc[PSU_BOTTOM].ac2dc_in_power,
        vm.ac2dc[PSU_BOTTOM].ac2dc_power,
        vm.ac2dc[PSU_BOTTOM].ac2dc_power_now, 
        vm.ac2dc[PSU_BOTTOM].ac2dc_power_last, 
        vm.ac2dc[PSU_BOTTOM].ac2dc_power_last_last,              
        vm.ac2dc[PSU_BOTTOM].ac2dc_power_limit,
        vm.ac2dc[PSU_BOTTOM].ac2dc_temp,
        vm.board_cooling_now[PSU_BOTTOM],
        vm.board_cooling_ever
      );

    }



    if (vm.loop[l].enabled_loop) {
      fprintf(f,GREEN "LOOP[%d] ON\n" RESET,l);
    } else {
      fprintf(f,RED "LOOP[%d] OFF (%s)\n" RESET,l,vm.loop[l].why_disabled);
    }


  for (int addr = l*ASICS_PER_LOOP; addr < (l+1)*ASICS_PER_LOOP; addr++) {
    ASIC *a = &vm.asic[addr];
    DC2DC* dc2dc = &vm.asic[addr].dc2dc;

    if (a->asic_present) {
      fprintf(f, "%2d: DC2DC/%d/:[vlt1:%d vlt2:%3d(DCl:%3d Tl:%3d Ul:%3d) %2dW %3dA %3dc]",
          addr,
          1-a->user_disabled,
          dc2dc->voltage_from_dc2dc,
          VTRIM_TO_VOLTAGE_MILLI(dc2dc->vtrim),
          VTRIM_TO_VOLTAGE_MILLI(dc2dc->max_vtrim_currentwise),
          VTRIM_TO_VOLTAGE_MILLI(dc2dc->max_vtrim_temperature),
          VTRIM_TO_VOLTAGE_MILLI(dc2dc->max_vtrim_user),          
          dc2dc->dc_power_watts_16s/16,
          dc2dc->dc_current_16s/16,
          dc2dc->dc_temp
          );

      total_watt+=dc2dc->dc_power_watts_16s;

      vm.total_mhash +=  (a->freq_hw)*ENGINES_PER_ASIC;
      total_hash[ASIC_TO_BOARD_ID(addr)] +=  (a->freq_hw)*ENGINES_PER_ASIC;
   
      total_asics++;

      fprintf(f, GREEN RESET " ASIC:[%s%3dc%s " GREEN,
        (a->asic_temp>=MAX_ASIC_TEMPERATURE-1)?((a->asic_temp>=MAX_ASIC_TEMPERATURE)?RED:YELLOW):GREEN,((a->asic_temp*5)+85),
        ((a->cooling_down)?("*"):(" ")));

      fprintf(f, "%s%3dhz%s(BL:%4d) %4d" RESET /*"%08x%08x%08x%08x%08x%08x%08x"*/ " (E:%d) F:%x]\n",
         ((a->freq_hw>=800)? (MAGENTA) : ((a->freq_hw<=600)?(CYAN):(YELLOW))), (a->freq_hw),GREEN,
          (a->freq_bist_limit),
          vm.asic[addr].solved_jobs,
          /*
          a->not_brocken_engines[0],
          a->not_brocken_engines[1],
          a->not_brocken_engines[2],
          a->not_brocken_engines[3],
          a->not_brocken_engines[4],
          a->not_brocken_engines[5],
          a->not_brocken_engines[6],
          */
          a->not_brocken_engines_count,
          a->faults

      );
      }else {
          fprintf(f,RED "%d: disabled (%s)\n" RESET, addr, a->why_disabled);
      }
    }
  }
  // print last loop
  // print total hash power
  fprintf(f, RESET "\n[H:HW:%dGh[%d+%d],W:%d,L:%d,A:%d,MMtmp:%d TMP:(%d)=>=>=>(%d,%d)]\n",
                (vm.total_mhash)/1000,total_hash[BOARD_TOP]/1000, total_hash[BOARD_BOTTOM]/1000,
                total_watt/16,
                total_loops,
                total_asics,
                vm.mgmt_temp_max,
                vm.temp_mgmt,
                vm.temp_top,
                vm.temp_bottom
  );

   fprintf(f, "Pushed %d jobs , in HW queue %d jobs (sw:%d, hw:%d)!\n",
             vm.last_second_jobs, rt_queue_size, rt_queue_sw_write, rt_queue_hw_done);
   fprintf(f, "min:%d wins:%d[this/last min:%d/%d] bist-fail:%d, hw-err:%d\n",
           one_sec_counter%60,
           vm.solved_jobs_total, vm.this_minute_wins, vm.last_minute_wins,
           vm.this_min_failed_bist,
           vm.hw_errs);
   fprintf(f, "leading-zeroes:%d idle promils[s/m]:%d/%d, rate:%dgh/s\n",            
           vm.cur_leading_zeroes, 
           vm.idle_asic_cycles_sec*10/100000, (vm.idle_asic_cycles_last_min*10) / 6000000, 
           vm.last_minute_rate_mb/1000);
   fprintf(f, "Fan:%d, conseq:%d\n", vm.fan_level, vm.consecutive_jobs);
   fprintf(f, "AC2DC BAD: %d %d\n" , vm.bad_ac2dc[PSU_TOP], vm.bad_ac2dc[PSU_BOTTOM]);
   fprintf(f, "R/NR: %d/%d\n", vm.mining_time, vm.not_mining_time);
   fprintf(f, "RTF asics: %d\n",vm.run_time_failed_asics);

   fprintf(f, "%d err_restarted\n", vm.err_restarted);  
   fprintf(f, "%d err_unexpected_reset\n", vm.err_unexpected_reset);  
   fprintf(f, "%d err_unexpected_reset2\n", vm.err_unexpected_reset2);  
   fprintf(f, "%d err_too_much_fake_wins\n", vm.err_too_much_fake_wins);  
   fprintf(f, "%d err_stuck_bist\n", vm.err_stuck_bist);  
   fprintf(f, "%d err_low_power_chiko\n", vm.err_low_power_chiko);  
   fprintf(f, "%d err_stuck_pll\n", vm.err_stuck_pll);  
   fprintf(f, "%d err_runtime_disable\n", vm.err_runtime_disable);  
   fprintf(f, "%d err_purge_queue\n", vm.err_purge_queue);  
   fprintf(f, "%d err_read_timeouts\n", vm.err_read_timeouts);    
   fprintf(f, "%d err_dc2dc_oc\n", vm.err_dc2dc_oc);    
   fprintf(f, "%d err_read_timeouts2\n", vm.err_read_timeouts2);  
   fprintf(f, "%d err_read_corruption\n", vm.err_read_corruption);  
   fprintf(f, "%d err_purge_queue3\n", vm.err_purge_queue3);  
   fprintf(f, "%d err_bad_idle\n", vm.err_bad_idle);     
   

   
   print_adapter(f, false);
   fclose(f);
}


void store_last_voltage();

void dump_watts() {
	FILE *f = fopen("/tmp/mg_watts", "w");
    if (!f) {
      psyslog("Failed to create watts file\n");
      return;
    }
    fprintf(f, "T:%d[%d], B:%d[%d]\n", 
      vm.ac2dc[PSU_TOP].ac2dc_power, vm.ac2dc[PSU_TOP].ac2dc_power_limit,
      vm.ac2dc[PSU_BOTTOM].ac2dc_power, vm.ac2dc[PSU_BOTTOM].ac2dc_power_limit);
    fclose(f);
}

void ten_second_tasks() {
  /*
 printf("vm.consecutive_jobs = %d\n", vm.consecutive_jobs);
 printf("vm.last_second_jobs = %d\n", vm.last_second_jobs);
 printf("vm.cur_leading_zeroes = %d\n", vm.cur_leading_zeroes);
 printf("vm.needs_bist = %d\n", vm.needs_bist);
  printf("vm.mining_time = %d\n", vm.mining_time);
  printf("vm.not_mining_time = %d\n", vm.not_mining_time);
  printf("vm.asics_shut_down_powersave = %d\n", vm.asics_shut_down_powersave);
  printf("vm.start_mine_time = %d\n", vm.start_mine_time);
*/

  static char x[200]; 
  
  psyslog("MQ 10sec:%d\n", read_spi(ADDR_SQUID_MQ_SENT));
  sprintf(x, "uptime:%d lim:%d/%d rst:%d\n", 
    time(NULL) - vm.start_run_time, 
    vm.ac2dc[PSU_TOP].ac2dc_power_limit,vm.ac2dc[PSU_BOTTOM].ac2dc_power_limit,
    vm.err_restarted);
  mg_status(x);
  store_last_voltage();
  dump_watts();
  //restart_asics();
  
  //write_reg_asic(12, NO_ENGINE,ADDR_GOT_ADDR, 0);
  revive_asics_if_one_got_reset("ten_second_tasks");

 vm.temp_mgmt = get_mng_board_temp();
 if (vm.board_present[BOARD_TOP]) {
    vm.temp_top = get_top_board_temp();
 }
 if (vm.board_present[BOARD_BOTTOM]) {
    vm.temp_bottom = get_bottom_board_temp();
 }


 save_rate_temp(vm.temp_top, vm.temp_bottom,  vm.temp_mgmt, (vm.consecutive_jobs)?vm.total_mhash:0);
 if (vm.consecutive_jobs > 0) {
   ping_watchdog();
 }

 if (vm.temp_top > MAX_TOP_TEMP ||
   vm.temp_bottom > MAX_BOTTOM_TEMP ||  
   vm.temp_mgmt > MAX_MGMT_TEMP
  ) {
    psyslog("Thermal shutdown: %d %d %d\n",vm.temp_top,vm.temp_bottom,vm.temp_mgmt);
    mg_event_x("Thermal shutdown %d %d %d",vm.temp_top,vm.temp_bottom,vm.temp_mgmt);
    exit_nicely(180,"Too hot");
 } 
}

int dc2dc_can_down(int i) {
  return((vm.asic[i].dc2dc.vtrim > VTRIM_MIN));
}

int dc2dc_can_up(int i) {
  int ret, p;
  p=1;
  ret = ((vm.asic[i].dc2dc.dc_current_16s < (vm.max_dc2dc_current_16s)) && ++p &&
         (vm.asic[i].dc2dc.vtrim < vm.asic[i].dc2dc.max_vtrim_currentwise) && ++p &&
         (vm.asic[i].dc2dc.vtrim < vm.asic[i].dc2dc.max_vtrim_temperature) && ++p && 
         (vm.asic[i].dc2dc.vtrim < vm.asic[i].dc2dc.max_vtrim_user) && ++p &&
         (vm.asic[i].dc2dc.dc_temp < vm.asic[i].dc2dc.dc_temp_limit) && ++p &&         
         (vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power_limit - vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power > 5) && ++p);
  //DBG(DBG_SCALING,"P[%d]=%d\n", i,p);

}


void dc2dc_down(int i, int *err, const  char* why) {
  passert((vm.asic[i].dc2dc.vtrim > VTRIM_MIN));
  // - 2.7 milli-volt
  vm.asic[i].dc2dc.vtrim--;
  vm.asic[i].dc2dc.revolted=1;
  dc2dc_set_vtrim(i, vm.asic[i].dc2dc.vtrim, 1, err, why);
}


void dc2dc_up(int i, int *err, const  char* why) {
  passert((vm.asic[i].dc2dc.vtrim < VTRIM_MAX));
  vm.asic[i].dc2dc.revolted=1;
  vm.asic[i].dc2dc.vtrim++;
  vm.asic[i].dc2dc.dc_power_watts_16s += 7*16;
  vm.asic[i].dc2dc.dc_current_16s     += 7*16;  
  dc2dc_set_vtrim(i, vm.asic[i].dc2dc.vtrim, 1, err, why);
  if (vm.asic[i].freq_bist_limit < (ASIC_FREQ_MAX - 30)) {
    vm.asic[i].freq_bist_limit = (int)(vm.asic[i].freq_bist_limit+15);
    if (!vm.asic[i].cooling_down) {
      if (asic_can_up_freq(i)) {
        asic_up_freq_max(i, 1, 1,  "Upvolt2");
      }
    }
  } else {
    psyslog("Runaway ASIC %d with freq %d\n",i,(vm.asic[i].freq_bist_limit));
    disable_asic_forever_rt(i, "runaway freq 1"); 
  }
}

int asic_can_down_freq(int i) {
  return (vm.asic[i].freq_hw > ASIC_FREQ_MIN);
}

void asic_down_freq(int i, int wait_pll_lock, int disable_enable_engines,  const  char* why) {
  set_pll(i, (int)(vm.asic[i].freq_hw - 5),wait_pll_lock, disable_enable_engines,why);
  //vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power -= 1;
}

void asic_down_freq_completly(int i, int wait_pll_lock, int disable_enable_engines,  const  char* why) {
  set_pll(i, ASIC_FREQ_MIN,wait_pll_lock, disable_enable_engines,why); 
}

// wait 1 milli to lock!
void asic_up_freq_max(int i, int wait_pll_lock, int disable_enable_engines,  const  char* why) {
  int wanted = vm.asic[i].freq_bist_limit;
  int wanted_hz = wanted - vm.asic[i].freq_hw;
  int allowed_hz = (vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power_limit -
                       vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power) * 3;
  if (wanted_hz <= allowed_hz) {
    if (vm.asic[i].cooling_down) {
      vm.asic[i].cooling_down = 0;
      vm.board_cooling_now[ASIC_TO_BOARD_ID(i)]--;
    }
    DBG(DBG_SCALING, "%d Full upscaling from %d by %d\n", i, vm.asic[i].freq_hw, (wanted_hz)); 
  } else {
    wanted_hz = (allowed_hz/5)*5;
    DBG(DBG_SCALING, "%d Only upscaling from %d by %d\n", i, vm.asic[i].freq_hw, (wanted_hz)); 
  }
  
  if (wanted_hz >= 0) {
    vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power += wanted_hz/3;
    set_pll(i, wanted,wait_pll_lock,disable_enable_engines, why);  
  }
}



int asic_can_up_freq(int i) {
  return ((vm.asic[i].dc2dc.dc_current_16s < (vm.max_dc2dc_current_16s)) &&
          (vm.asic[i].freq_hw < ASIC_FREQ_MAX) &&
          (vm.asic[i].dc2dc.dc_temp < vm.asic[i].dc2dc.dc_temp_limit) &&
          (vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power_limit - vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power > 2) );
}


void asic_up_freq(int i, int wait_pll_lock, int disable_enable_engines, const char* why) {
  set_pll(i, (int)(vm.asic[i].freq_hw + 5),wait_pll_lock,disable_enable_engines, why);
  vm.asic[i].dc2dc.dc_power_watts_16s += 1; // for scaling
  vm.ac2dc[ASIC_TO_PSU_ID(i)].ac2dc_power += 1;
}





// Each DC2DC takes 10 milli to poll.
// All board takes 300 milli.
void update_dc2dc_stats(int i) {
  int overcurrent_err;
  int overcurrent_warning;
  uint8_t temp;
  int current;
  int err;

  if (vm.asic[i].dc2dc.dc2dc_present) {
    //printf("Updating dc2dc %d\n", i);
    dc2dc_get_all_stats(
              i,
              &overcurrent_err,
              &overcurrent_warning,
              &vm.asic[i].dc2dc.dc_temp,
              &vm.asic[i].dc2dc.dc_current_16s,
              &err);
    if (err) {
        vm.err_dc2dc_oc++;  
        if ((vm.err_dc2dc_oc % 10) == 0) {
          restart_asics_full(6, "Dc2Dc i2c error");
          return;
        }
    }
    //passert(err == 0);
    if (overcurrent_warning) {
      // Downscale ASIC voltage
      if (vm.asic[i].dc2dc.vtrim > VTRIM_MIN) {
        dc2dc_down(i, &err, "OC warning");
        vm.asic[i].dc2dc.max_vtrim_currentwise = vm.asic[i].dc2dc.vtrim;
        vm.needs_bist = 1;
      }
    }

    vm.asic[i].dc2dc.dc_power_watts_16s =
      vm.asic[i].dc2dc.dc_current_16s*VTRIM_TO_VOLTAGE_MILLI(vm.asic[i].dc2dc.vtrim)/1000;
  }

}


void once_33milli_tasks_rt() {
  static int counter = 0;
  if (!vm.i2c_busy_with_bug) {
    update_dc2dc_stats(counter%ASICS_COUNT);
  }
  counter++;
}



void forget_all_limits() {
  for (int i = 0; i < ASICS_COUNT; i++) {
    ASIC *a = &vm.asic[i];
    a->dc2dc.max_vtrim_currentwise = VTRIM_MAX;
    a->dc2dc.max_vtrim_temperature = VTRIM_MAX;
  }
}



void once_second_tasks_rt() {
  //psyslog("One-secs %d\n", vm.consecutive_jobs);
  //struct timeval tv;
  //start_stopper(&tv);

  // If ASICs all done - finish all work?
  /*
  int someone_mining = read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_CONDUCTOR_BUSY);
  while ((!someone_mining) && rt_queue_size) {
     printf("Finishin work\n");
     RT_JOB old_w;
     one_done_sw_rt_queue(&old_w);
     passert(old_w.winner_nonce == 0);
     push_work_rsp(&old_w, 1);
  }
  */

  // I2C stats

  if (vm.consecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_SCALING) {
      if ((vm.start_mine_time == 0)) {
        vm.start_mine_time = time(NULL);
        psyslog( "Restarting mining timer\n" );
      }
  }

  //write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_MNG_COMMAND, BIT_ADDR_MNG_ZERO_IDLE_COUNTER);

  vm.idle_asic_cycles_sec = read_reg_asic(LEADER_ASIC_ADDR, NO_ENGINE, ADDR_IDLE_COUNTER);
  if ((vm.idle_asic_cycles_sec/100000 > 90) && 
      (vm.consecutive_jobs == MAX_CONSECUTIVE_JOBS_TO_COUNT)) {
    vm.err_bad_idle++;
    restart_asics_full(17,"Asic IDLE should not be");
    return;
  }
  
  vm.idle_asic_cycles_this_min += vm.idle_asic_cycles_sec;

  if (vm.idle_asic_cycles_sec) {
    psyslog("%sIDLE :%d% (0x%d), mqsize=%d cons=%d\n" RESET, 
      (vm.idle_asic_cycles_sec)?(RED):(CYAN), 
      vm.idle_asic_cycles_sec/100000 , 
      vm.idle_asic_cycles_sec, 
      mq_size(),
      vm.consecutive_jobs
      );
    print_adapter(NULL, true);
  }
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_MNG_COMMAND, BIT_ADDR_MNG_ZERO_IDLE_COUNTER);
  vm.last_second_jobs = vm.this_second_jobs;
  vm.this_second_jobs = 0;

  for (int i=0; i< ASICS_COUNT; i++) {
    ASIC *h = &vm.asic[i];
    if (!h->asic_present) {
      continue;
    }
  }

#if 0
  uint32_t engines_mask[7];
  
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_0,&engines_mask[0]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_1,&engines_mask[1]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_2,&engines_mask[2]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_3,&engines_mask[3]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_4,&engines_mask[4]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_5,&engines_mask[5]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_6,&engines_mask[6]);
  squid_wait_asic_reads();
  printf("RST: %x %x %x %x %x %x %x \n",
    engines_mask[0],
    engines_mask[1],
    engines_mask[2],
    engines_mask[3],
    engines_mask[4],
    engines_mask[5],
    engines_mask[6]
  );
  
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_ENABLE_ENGINES_0,&engines_mask[0]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_ENABLE_ENGINES_1,&engines_mask[1]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_ENABLE_ENGINES_2,&engines_mask[2]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_ENABLE_ENGINES_3,&engines_mask[3]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_ENABLE_ENGINES_4,&engines_mask[4]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_ENABLE_ENGINES_5,&engines_mask[5]);
  push_asic_read(LEADER_ASIC_ADDR, NO_ENGINE,ADDR_ENABLE_ENGINES_6,&engines_mask[6]);
  squid_wait_asic_reads();  
  printf("ENB: %x %x %x %x %x %x %x \n",
    engines_mask[0],
    engines_mask[1],
    engines_mask[2],
    engines_mask[3],
    engines_mask[4],
    engines_mask[5],
    engines_mask[6]
  );
#endif  



  if (vm.consecutive_jobs == 0) {
    set_light(LIGHT_GREEN, LIGHT_MODE_FAST_BLINK);
    vm.start_mine_time = 0;
    vm.not_mining_time++;
    vm.mining_time = 0;
  } else {
    set_light(LIGHT_GREEN, LIGHT_MODE_ON);
    vm.not_mining_time = 0;
    vm.mining_time++;
  }
  // See if we can stop engines
  //psyslog("not_mining_time %d\n", vm.not_mining_time);

  // Must update_ac2dc_stats before once_second_scaling_logic!
  if (vm.consecutive_jobs  >= MIN_COSECUTIVE_JOBS_FOR_SCALING) {
    once_second_scaling_logic();
  }
  if (one_sec_counter % 10 == 0) {
    ten_second_tasks();
  }
  if (one_sec_counter % 60 == 2) {
    one_minute_tasks();
  }

  if (one_sec_counter % (60*60) == 3) {
    // once per hour forget scaling data
    forget_all_limits();
  }
  //psyslog("One-secs done 1\n")
  print_scaling();
  print_production();
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_WIN_LEADING_0, vm.cur_leading_zeroes);
  ++one_sec_counter;
}



void poll_win_temp_rt() {
  static int measure_temp_addr = 0;
  uint32_t intr;
  uint32_t win_reg = 0;
  uint32_t range; 
  static int counter=0;
  counter++;
  if ((counter%ASICS_COUNT) == 0 ) {
    temp_measure_next();
  }

#ifdef  REMO_STRESS
  uint32_t bbb[5];
  for (int i = 0; i < 5; i++) {
    push_asic_read(ANY_ASIC, NO_ENGINE, ADDR_VERSION, &bbb[i]);
  }
#endif

      
  if (vm.asic[measure_temp_addr].asic_present) {
    set_temp_reading_rt(measure_temp_addr, &intr);
  }
  push_asic_read(ANY_ASIC, NO_ENGINE, ADDR_INTR_BC_WIN, &win_reg);
  //push_asic_read(ANY_ASIC, NO_ENGINE, ADDR_INTR_BC_WIN, &win_reg);

  //int eng = rand()%193;
  //push_asic_read(LEADER_ASIC_ADDR, eng, ADDR_NONCE_RANGE, &range);

//-------------------------------
  squid_wait_asic_reads();
//-------------------------------

  //printf("----------------------------- RANGE[%d] = %d\n",eng, range);
#ifdef  REMO_STRESS
  for (int i = 0; i < 5; i++) {
    if ((bbb[i] & 0xffFF) != 0x46) {
      vm.err_read_corruption++;
    }
  }
#endif
  // proccess TEMPERATURE reading
  if (vm.asic[measure_temp_addr].asic_present) {
    proccess_temp_reading_rt(&vm.asic[measure_temp_addr], intr);
  }
  measure_temp_addr = (measure_temp_addr+1)%ASICS_COUNT;


  // proccess WIN
  while (win_reg) {
       //struct timeval tv;;
       //start_stopper(&tv);
       uint16_t winner_device = BROADCAST_READ_ADDR(win_reg);
       uint16_t winner_engine = BROADCAST_READ_ENGINE(win_reg);
       //vm.last_minute_rate_mb += 1<<(vm.cur_leading_zeroes-32);
       //printf("WON:%x\n", winner_device);
       win_reg = get_print_win(winner_device, winner_engine);
       //end_stopper(&tv,"WIN STOPPER");
       //try_push_job_to_mq();
  }

}




void try_push_job_to_mq() {
  if (vm.asics_shut_down_powersave) {
    return;
  }
  
  RT_JOB work;
  RT_JOB *actual_work = NULL;
  int has_request = pull_work_req(&work);
  if (has_request) {
    // Update leading zeroes?
    vm.not_mining_time = 0;
    // TODO - every time 
    //flush_spi_write();
    actual_work = add_to_sw_rt_queue(&work);
    //DBG(DBG_WINS,"<PUSH> work.timestamp %x, job_id %x, queue_id %x\n",actual_work->timestamp,actual_work->work_id_in_hw,actual_work->work_id_in_hw/MQ_INCREMENTS)

    // write_reg_asic(0,NO_ENGINE, ADDR_CURRENT_NONCE_START, rand() + rand()<<16);
    // write_reg_asic(0, NO_ENGINE, ADDR_CURRENT_NONCE_START + 1, rand() + rand()<<16);
    push_to_hw_queue_rt(actual_work);
    vm.this_second_jobs++;
    if (vm.consecutive_jobs < MAX_CONSECUTIVE_JOBS_TO_COUNT) {
      vm.consecutive_jobs++;
      set_fan_level(vm.max_fan_level);      
    } 
  } else {
    if (vm.consecutive_jobs > 0) {
      vm.consecutive_jobs--;
    } else {
      set_fan_level(50);
    }
  }

}



void ping_watchdog() {
    psyslog("Ping WD %d %d %d\n",vm.total_mhash, vm.consecutive_jobs, time(NULL));
    FILE *f = fopen("/var/run/dont_reboot", "w");
    if (!f) {
      psyslog("Failed to create watchdog file\n");
      return;
    }
    fprintf(f, "minergate %d %d %d\n", vm.total_mhash, vm.consecutive_jobs, time(NULL));
    fclose(f);
}





// 666 times a second
void once_2_msec_tasks_rt() {
  static struct timeval tv;

  static int counter = 0;
  counter++;
  
  int mqsize = mq_size();
  //end_stopper(&tv,"2 msec");
  //start_stopper(&tv);

  
  if ((counter % ((JOB_TRY_PUSH_MILLI*1000)/RT_THREAD_PERIOD)) == 0) {
    if ((mqsize < MQ_TOO_FULL)) {
#ifdef INFILOOP_TEST      
      printf("\n<%d OK>\n",mqsize);      
#endif
      try_push_job_to_mq();
    } else {
      //printf("<%d>",mqsize);    
#ifdef INFILOOP_TEST
      printf("<%x>",read_reg_asic(ANY_ASIC, NO_ENGINE, ADDR_VERSION));
#endif      
    }
  }

#ifndef INFILOOP_TEST
  if (counter % (50) == 0) {
    //start_stopper(&tv);
    leds_periodic_100_msecond();
    //end_stopper(&tv,"100");
  }


  // ~30 times per second - ask one DC2DC and poll for win.
  if (counter % (16) == 0) {  // %16
    //struct timeval tv;
    //start_stopper(&tv);
    once_33milli_tasks_rt();
    //end_stopper(&tv,"33_MSEC");
  }
  

  if (counter % (4) == 0) 
  {
    poll_win_temp_rt();
  }
  if (counter % (500) == 0) {
    struct timeval tv;
    //start_stopper(&tv);
    once_second_tasks_rt();
    //end_stopper(&tv,CYAN "ONE_SEC" RESET);
  }
#endif  
}








void *squid_regular_state_machine_rt(void *p) {
  psyslog("Starting squid_regular_state_machine_rt!\n");
  flush_spi_write();
  //return NULL;
  // Do BIST before start
  vm.not_mining_time = 0;
  /*
  static struct sched_param param;
  param.sched_priority = sched_get_priority_max(SCHED_RR);
  int r = sched_setscheduler(0, SCHED_RR, &param);
  passert(r==0);
  */

/*
  while(1) {
    update_ac2dc_power_measurments();
  }
  */


  struct timeval tv;
  struct timeval last_job_pushed;
  struct timeval last_force_queue;
  gettimeofday(&last_job_pushed, NULL);
  gettimeofday(&last_force_queue, NULL);
  gettimeofday(&tv, NULL);
  int usec;
  print_scaling();
  print_production();
  //do_bist_loop_push_job("First BIST!");
  // Cancel wins:  
  vm.hw_errs = 0;
  for (;;) {
    gettimeofday(&tv, NULL);
    usec = (tv.tv_sec - last_job_pushed.tv_sec) * 1000000;
    usec += (tv.tv_usec - last_job_pushed.tv_usec);
    //struct timeval * tv2;
    if (usec >= RT_THREAD_PERIOD) {
      pthread_mutex_lock(&asic_mutex);
      once_2_msec_tasks_rt();
      pthread_mutex_unlock(&asic_mutex);

      last_job_pushed = tv;
      int drift = usec - RT_THREAD_PERIOD;
      if (last_job_pushed.tv_usec > drift) {
        last_job_pushed.tv_usec -= drift;
      }
    } else {
      if (kill_app) {
        printf("RT out\n");
        return NULL;
      }
      usleep(RT_THREAD_PERIOD - usec);
    }
  }
}
