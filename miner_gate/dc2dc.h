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


#ifndef _____DC2DC__45R_H____
#define _____DC2DC__45R_H____

#include "defines.h"
#include <stdint.h>
#include <unistd.h>


#define VTRIM_MIN   0x0FFc4 // 555 with vmargin low,  0.635 with
#define VTRIM_MAX   0x1001E // 788 with vmargin low,  0.860 with  



#define VTRIM_TO_VOLTAGE_MILLI(XX)    ((55500 + (XX-0x0FFc4)*(266))/100)  
#define VOLTAGE_TO_VTRIM_MILLI(XXX)    ((((XXX)*100-55500)/266) + 0x0FFc4) 

void dc2dc_set_vtrim(int addr, uint32_t vtrim, bool vmargin_75low  , int *err,const char* why);
void dc2dc_select_rb(int addr, int *err) ;
void dc2dc_disable_dc2dc(int addr, int *err);
void dc2dc_enable_dc2dc(int addr, int *err);
void dc2dc_init();
void update_dc2dc_stats();
int dc2dc_is_removed(int addr);
int dc2dc_is_removed_or_disabled(int addr);
int dc2dc_is_user_disabled(int addr);
int loop_is_removed_or_disabled(int l);
void test_all_dc2dc();


int dc2dc_get_all_stats(
      int addr, 
      int* overcurrent_err, 
      int* overcurrent_warning,
      uint8_t *temp,
      int* current,      
      int *err);


// in takes 0.2second for voltage to be stable.
// Remember to wait after you exit this function


// Returns value like 810, 765 etc`

#endif
