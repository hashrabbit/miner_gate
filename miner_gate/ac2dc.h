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
int ac2dc_get_vpd(ac2dc_vpd_info_t *pVpd, int psu_id, AC2DC *ac2dc);

void ac2dc_init();

#ifndef MINERGATE
void ac2dc_init2(AC2DC * ac2dc);
#endif

unsigned char ac2dc_get_eeprom_quick(int offset, int *pError = 0);
char* psu_get_name(int type);
void test_fix_ac2dc_limits();
bool ac2dc_check_connected(int top_or_bottom);

// PSU types.
#define AC2DC_TYPE_UNKNOWN     0
#define AC2DC_TYPE_MURATA_NEW  1
#define AC2DC_TYPE_MURATA_OLD  2
#define AC2DC_TYPE_EMERSON_1_2 3


#endif
