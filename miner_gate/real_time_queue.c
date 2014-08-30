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


#include "spond_debug.h"
#include "asic.h"
#include "squid.h"
#include "queue.h"
#include <string.h>
#include <spond_debug.h>
#include <sys/time.h>
#include "asic_lib.h"
#include "real_time_queue.h"

// How long we wait for winn? 666 is 1 second, 66 in 100ms, 22 in 33ms

// hash table
RT_JOB rt_queue[RT_QUEUE_SIZE]; // must be of that size because address is 0xXX
int rt_queue_sw_write;
int rt_queue_hw_done;
int rt_queue_size = RT_QUEUE_SIZE; // to check that init was called
//#define QUEUE_WORK_IN_PROGRESS 20

RT_JOB *peak_rt_queue(uint8_t hw_id) { return &rt_queue[hw_id]; }

void reset_sw_rt_queue() {
  memset(&rt_queue, 0, sizeof(rt_queue));
  // start with 3, why not? :)
  rt_queue_sw_write = rt_queue_hw_done = 3;
  rt_queue_size = 0;
}

// Returns pointer for RT_JOB to give HW.
RT_JOB *add_to_sw_rt_queue(const RT_JOB *work) {
  //printf("rt_queue_size = %d\n",rt_queue_size);
  if (rt_queue_size == MAX_PACKETS_IN_RT_QUEUE) {
    RT_JOB old_w;
    one_done_sw_rt_queue(&old_w);
    //DBG(DBG_WINS,"DONE:JOB ID:%d  ---\n",old_w.work_id_in_hw);  
    passert(old_w.winner_nonce == 0);
    push_work_rsp(&old_w, 1);
  }
  //passert(rt_queue[rt_queue_sw_write].work_state == WORK_STATE_FREE);
  RT_JOB *work_in_queue = &rt_queue[rt_queue_sw_write];
  *work_in_queue = *work;
  passert(rt_queue_sw_write < RT_QUEUE_SIZE*MQ_INCREMENTS);
  work_in_queue->work_id_in_hw = rt_queue_sw_write*MQ_INCREMENTS;
  work_in_queue->work_state = WORK_STATE_HAS_JOB;
  // to make sure...
  //passert(work_in_queue->winner_nonce == 0);
  //work_in_queue->work_id_in_hw = rt_queue_sw_write;
  rt_queue_sw_write = (rt_queue_sw_write + 1) % (RT_QUEUE_SIZE);
  rt_queue_size++;
  passert((rt_queue_sw_write - rt_queue_hw_done) % RT_QUEUE_SIZE <= MAX_PACKETS_IN_RT_QUEUE);
  //passert(rt_queue_size <= MAX_PACKETS_IN_RT_QUEUE);
  return work_in_queue;
}

int one_done_sw_rt_queue(RT_JOB *work) {
    if (rt_queue_size) {
      *work = rt_queue[rt_queue_hw_done];
      //passert(rt_queue[rt_queue_hw_done].work_state == WORK_STATE_HAS_JOB);
      rt_queue[rt_queue_hw_done].work_state = WORK_STATE_FREE;
      rt_queue_hw_done = (rt_queue_hw_done + 1) % RT_QUEUE_SIZE;
      rt_queue_size--;
      //passert((rt_queue_sw_write - rt_queue_hw_done) % 0x100 <= MAX_PACKETS_IN_RT_QUEUE);
      //passert(rt_queue_size <= MAX_PACKETS_IN_RT_QUEUE);
      return 1;
    }

  
  return 0;
}
