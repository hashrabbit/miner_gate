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

#ifndef _____AC2DC__4544R_H____
#define _____AC2DC__4544R_H____
#include "defines.h"



typedef struct {
  unsigned char pnr[32];
  unsigned char model[32];
  unsigned char revision[32];
  unsigned char serial[32]; // WW[2]+SER[4]+REV[2]+PLANT[2]
} ac2dc_vpd_info_t;

//int ac2dc_get_vpd(ac2dc_vpd_info_t *pVpd);

void ac2dc_init();

#ifndef MINERGATE
int ac2dc_get_vpd(ac2dc_vpd_info_t *pVpd, int psu_id, AC2DC *ac2dc);
void ac2dc_init2(AC2DC * ac2dc);
unsigned char ac2dc_get_eeprom_quick(int offset,AC2DC *ac2dc, int *pError = 0);
#endif

char* psu_get_name(int id,int type);
void test_fix_ac2dc_limits();
void read_ac2dc_errors();

bool ac2dc_check_connected(int top_or_bottom);
void PSU12vON(int psu);
void PSU12vOFF(int psu);
void PSU12vPowerCycle(int psu);
void PSU12vPowerCycleALL();

// PSU types.
#define AC2DC_TYPE_UNKNOWN     0
#define AC2DC_TYPE_MURATA_NEW  1
#define AC2DC_TYPE_MURATA_OLD  2 // Removed!! Should not be used
#define AC2DC_TYPE_EMERSON_1_2 3
#define AC2DC_TYPE_EMERSON_1_6 4

int update_ac2dc_power_measurments();


#endif
