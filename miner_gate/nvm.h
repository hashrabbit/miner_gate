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

#ifndef _____NVMLIB_H__B__
#define _____NVMLIB_H__B__

#include <stdint.h>
#include <unistd.h>
#include "defines.h"

#define NVM_VERSION (0xCAF10000 + sizeof(SPONDOOLIES_NVM))
#define RECOMPUTE_NVM_TIME_SEC (60 * 60 * 24 * 7) // once every 24 hours

#define LOOP_COUNT 10
#define ASICS_PER_LOOP 3
#define ASICS_COUNT (LOOP_COUNT *ASICS_PER_LOOP)
#define ASICS_PER_BOARD        15 
#define LOOPS_PER_BOARD        5 

#define ASIC_TO_PSU_ID(ID)    (ID>=ASICS_PER_BOARD)   // psu_top = 0
#define ASIC_TO_BOARD_ID(ID)  (ID>=ASICS_PER_BOARD)   // psu_top = 0
#define LOOP_TO_PSU_ID(ID)    (ID>=LOOPS_PER_BOARD)   // psu_top = 0
#define LOOP_TO_BOARD_ID(ID)  (ID>=LOOPS_PER_BOARD)   // psu_top = 0

#define NVM_FILE_NAME "/etc/mg_nvm.bin"
#define ENGINES_PER_ASIC 193
#define ENGINE_BITMASCS  7


#define ASIC_TEMP_MIN 77
#define ASIC_TEMP_DELTA 6
typedef enum {
  ASIC_TEMP_85 = 0, // under the radar
  // -------------------
  ASIC_TEMP_90 = 1,
  ASIC_TEMP_95 = 2,
  ASIC_TEMP_100 = 3,
  ASIC_TEMP_105 = 4,
  ASIC_TEMP_110 = 5,
  ASIC_TEMP_115 = 6,
  ASIC_TEMP_120 = 7,
  ASIC_TEMP_125 = 8,
  ASIC_TEMP_COUNT = 9,
} ASIC_TEMP;


typedef struct _spondoolies_nvm {
  uint32_t nvm_version;
  uint32_t store_time;
  uint8_t dirty;  
  uint32_t crc32;
} SPONDOOLIES_NVM;

extern SPONDOOLIES_NVM nvm;
int load_nvm_ok();
void spond_save_nvm();
void spond_delete_nvm();
void print_nvm();

#endif
