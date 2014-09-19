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

#ifndef _____DC2DEFINES__45R_H____
#define _____DC2DEFINES__45R_H____

//#define SP2x 

#define MG_CUSTOM_MODE_FILE "/etc/mg_custom_mode"


// compilation flags
#define CHECK_GARBAGE_READ
#define RESET_MQ_ON_BIST
#define RUNTIME_BIST
//#define REMO_STRESS

//#define DOWNSCALE_ON_ERRORS
//#define INFILOOP_TEST

//#define ASSAFS_EXPERIMENTAL_PLL
//#define ZVISHA_TEST
//#define DUMMY_I2C_EMERSON_FIX


#define SQUID_LOOPS_MASK 0xFFFFFFF



//#define ENABLED_ENGINES_MASK   0xFFFFFFFF
#define ENABLED_ENGINES_MASK     0xFFFFFFFF
#define ALLOWED_BIST_FAILURE_ENGINES    2
#define LEADER_ASIC_ADDR             3        




// System parameters
// Above this not allowed
#ifdef SP2x
#define MAX_ASIC_TEMPERATURE ASIC_TEMP_125
#else
#define MAX_ASIC_TEMPERATURE ASIC_TEMP_120
#endif
#define MAX_FAN_LEVEL                   100

#define TIME_FOR_DLL_USECS  1000
#define MIN_COSECUTIVE_JOBS_FOR_SCALING 100
#define MAX_CONSECUTIVE_JOBS_TO_COUNT (200)

// In seconds
#define BIST_PERIOD_SECS                                   16 // MUST BE MORE THEN 16
#define BIST_PERIOD_SECS_LONG                              (BIST_PERIOD_SECS*3) // MUST BE MORE THEN 16


#define DC2DCS_TO_UPVOLT_EACH_BIST_PER_BOARD          13
#if (BIST_PERIOD_SECS < 11) 
  Error!
#endif


#define MAX_BOTTOM_TEMP 95
#define MAX_TOP_TEMP 95
#define MAX_MGMT_TEMP 50

#define AC2DC_POWER_DECREASE_START_VOLTAGE     (1270)
#define AC2DC_POWER_LIMIT_105_EM  (800)
#define AC2DC_POWER_LIMIT_114_EM  (1050)
#define AC2DC_POWER_LIMIT_119_EM  (1100)
#define AC2DC_POWER_LIMIT_200_EM  (1100)
#define AC2DC_POWER_LIMIT_210_EM  (1250)

#define AC2DC_POWER_LIMIT_105_MU  (800)
#define AC2DC_POWER_LIMIT_114_MU  (1050)
#define AC2DC_POWER_LIMIT_119_MU  (1200)
#define AC2DC_POWER_LIMIT_200_MU  (1200)
#define AC2DC_POWER_LIMIT_210_MU  (1300)




//#define NO_AC2DC_I2C
#define IDLE_TIME_TO_PAUSE_ENGINES 30
#define DC_TEMP_LIMIT              140

#ifndef REMO_STRESS
#define MQ_TIMER_USEC 500
#else
#define MQ_TIMER_USEC 160
#endif
//#define MQ_TIMER_USEC 450  // for 10MB
//#define MQ_TIMER_USEC 225  // for 5MB

#define MQ_INCREMENTS 60
// MQ_TIMER_USEC*MQ_INCREMENTS = 30000
#define MQ_TOO_FULL   50  // 15 per job - 2 jobs inside only

#define RT_THREAD_PERIOD   (2000)
#define JOB_TRY_PUSH_MILLI (2)   // Try each 10 milli

#define MIN(A,B) (((A)<(B))?(A):(B))
#define MAX(A,B) (((A)<(B))?(B):(A))

#endif
