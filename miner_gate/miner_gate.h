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

#ifndef MINER_GATE_H
#define MINER_GATE_H
#include "defines.h"
int pull_work_req(RT_JOB *w);
void exit_nicely(int seconds_sleep_before_exit , const char* p);
int test_serial(int loopid);
void configure_mq(uint32_t interval, uint32_t increments, int pause);
void restart_asics_part(const char* why);
void restart_asics_full(int reason, const char* why);



#endif
