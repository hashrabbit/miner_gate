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

#ifndef _____ASICLIBBBB_H____
#define _____ASICLIBBBB_H____

#include "nvm.h"
#include "asic.h"


typedef struct {
  int l;
  int h;
  int addr;
  int done;
  ASIC *a;
} asic_iter;
//#define ASIC_ITER_INITIALIZER  {-1, 0, -1, 0, NULL}

typedef struct {
  int l;
  int done;
} loop_iter;


int stop_all_work_rt_restart_if_error(int wait_ok = 1);
void resume_all_work();

extern int assert_serial_failures;
int one_done_sw_rt_queue(RT_JOB *work);

void *squid_regular_state_machine_rt(void *p);
int init_asics(int addr);
int do_bist_ok(bool store_limit = true, bool step_down_if_failed = true, int fast_bist = ALLOWED_BIST_FAILURE_ENGINES,const char* why="no reason");
int do_bist_loop_push_job(const char* why);

uint32_t crc32(uint32_t crc, const void *buf, size_t size);
//void push_asic_read(uint8_t asic_addr, uint8_t engine_addr ,uint32_t offset, uint32_t *p_value, int defval=0);
//void push_asic_write(uint8_t asic_addr, uint8_t engine_addr , uint32_t offset, uint32_t value);

int enable_good_loops_ok();
int allocate_addresses_to_devices();
void set_nonce_range_in_engines(unsigned int max_range);
void set_nonce_range_in_asic(int addr, unsigned int max_range);

void enable_all_engines_all_asics(int with_reset);
void thermal_init(int addr);
int count_ones(uint32_t i);


int dc2dc_can_down(int i);
int dc2dc_can_up(int i, int *pp);
void dc2dc_down(int i, int clicks,int *err, const char* why);
void dc2dc_up(int i, int *err, const char* why);
int asic_can_down_freq(int i);
int asic_can_up_freq(int i);
void asic_up_freq(int i, int wait_pll_lock,int disable_enable_engines,const char* why);
void asic_up_freq_max_request(int i, const  char* why);
void print_chiko(int with_asics);

void ping_watchdog();
void set_initial_voltage_freq();
void set_initial_voltage_freq_on_restart();
int get_fake_power(int psu_id); // get ACDC power by calculation



#endif
