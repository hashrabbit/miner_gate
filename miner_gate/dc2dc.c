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

#include "i2c.h"
#include "dc2dc.h"
#include "nvm.h"
#include "ac2dc.h"
#include "asic.h"
#include "pll.h"

#include <time.h>
#include <pthread.h>

#include "defines.h"
extern pthread_mutex_t i2c_mutex;



extern MINER_BOX vm;
#if 0
int volt_to_vtrim[ASIC_VOLTAGE_COUNT] = 
  { 0, 0xFFC4, 0xFFCF, 0xFFE1, 0xFFd4, 0xFFd6, 0xFFd8, 
//     550      580     630      675    681    687                               
       0xFFda,  0xFFdc, 0xFFde, 0xFFE1, 0xFFE3,   
//     693       700     705       710    715  
  0xFFE5,  0xfff7, 0x0, 0x8,
//  720      765   790   810 


};
   
int volt_to_vmargin[ASIC_VOLTAGE_COUNT] = { 0, 0x14, 0x14, 0x14, 0x0, 0x0, 0x0, 
                                            0x0,0x0, 0x0,  0x0,  0x0, 0x0, 0x0,
                                            0x0, 0x0 };
#endif

static int dc2dc_addr[2] = {I2C_DC2DC_0,I2C_DC2DC_1};
static void dc2dc_i2c_close();
static void dc2dc_select_i2c(int addr, int *err);
static void dc2dc_set_phase(int phaze_addr, int channel_mask, int *err);


/*
FET 72b
i2cset -y 0 0x18 0x00 0x81
i2cset -y 0 0x18 0x35 0xf028 w
i2cset -y 0 0x18 0x36 0xf018 w
i2cset -y 0 0x18 0x38 0x8021 w
i2cset -y 0 0x18 0x39 0xe024 w
i2cset –y 0 0x18 0x46 0xf850 w
i2cset –y 0 0x18 0x4a 0xf846 w
i2cset –y 0 0x18 0x47 0x3F
i2cset –y 0 0x18 0xd7 0x03
i2cset –y 0 0x18 0x02 0x02
i2cset –y 0 0x18 0x51 0x0087 w
i2cset –y 0 0x18 0x4f 0x0096 w
i2cset –y 0 0x18 0xe6 0x0007 w
i2cset –y 0 0x18 0xe5 0x7f00 w
i2cset –y 0 0x18 0x15
usleep 10000
i2cset -y 0 0x18 0x03
*/

// not locked
// returns success 
static int dc2dc_init_rb(int addr) {
    int err;
    assert(addr < ASICS_COUNT);
    assert(addr >= 0);

    if ((vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_ERROR)) {
      disable_asic_forever_rt(addr, 1, "FET read error");
      return 0;
    }
  

    if (dc2dc_is_removed(addr)){
    	psyslog("skipping DC2DC %d, REMOVED/DISABLED\n",addr);
    	passert(0);
    	return 0;
    }

    dc2dc_select_i2c(addr, &err);
    if (err) {
      psyslog(RED "FAILED TO INIT DC2DC 1 %d\n" RESET, addr);
      dc2dc_i2c_close();
      return 0;
    }

    
    i2c_write_byte(dc2dc_addr[0], 0x00, 0x81, &err);
    if (err) {
      psyslog(RED "FAILED TO INIT DC2DC 2 %d\n" RESET, addr);
      dc2dc_i2c_close();
      return 0;
    }

    if ((vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B) ||
        (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B_I50)) {
      i2c_write_word(dc2dc_addr[0], 0x35, 0xf028, &err);
      i2c_write_word(dc2dc_addr[0], 0x36, 0xf018,&err);
      i2c_write_word(dc2dc_addr[0], 0x38, 0x8021,&err);
      i2c_write_word(dc2dc_addr[0], 0x39, 0xe024,&err);
      if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B) {
        i2c_write_word(dc2dc_addr[0], 0x46, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[0], 0x4a, 0xf850,&err);
        i2c_write_word(dc2dc_addr[0], 0x46, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[0], 0x4a, 0xf850,&err);
      } else {
        i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
        i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85a,&err);
        i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
        i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85a,&err);
      }
      i2c_write_byte(dc2dc_addr[0], 0x47, 0x3F,&err);
      i2c_write_byte(dc2dc_addr[0], 0xd7, 0x03,&err);
      i2c_write_byte(dc2dc_addr[0], 0x02, 0x02,&err);
      i2c_write_word(dc2dc_addr[0], 0x51, 0x006e,&err);
      i2c_write_word(dc2dc_addr[0], 0x4f, 0x007d ,&err);
      i2c_write_word(dc2dc_addr[0], 0xe6, 0x0007,&err);
      i2c_write_word(dc2dc_addr[0], 0xe5, 0x7f00,&err);
    }else if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_78B_I50)  {
      i2c_write_word(dc2dc_addr[0], 0x35, 0xf028, &err);
      i2c_write_word(dc2dc_addr[0], 0x36, 0xf018,&err);
      i2c_write_word(dc2dc_addr[0], 0x38, 0x8021,&err);
      i2c_write_word(dc2dc_addr[0], 0x39, 0xe024,&err);
      i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
      i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85a,&err);
      i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
      i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85a,&err);
      i2c_write_byte(dc2dc_addr[0], 0x47, 0x3F,&err);
      i2c_write_byte(dc2dc_addr[0], 0xd7, 0x03,&err);
      i2c_write_byte(dc2dc_addr[0], 0x02, 0x02,&err);
      i2c_write_word(dc2dc_addr[0], 0x51, 0x006e,&err);
      i2c_write_word(dc2dc_addr[0], 0x4f, 0x007d ,&err);
      i2c_write_word(dc2dc_addr[0], 0xe6, 0x0007,&err);
      i2c_write_word(dc2dc_addr[0], 0xe5, 0x7f00,&err);
    }

    else if ((vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A) ||
             (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A_I50))  {  // FET_T_72A
      i2c_write_word(dc2dc_addr[0], 0x35, 0xf028, &err);
      i2c_write_word(dc2dc_addr[0], 0x36, 0xf018,&err);
      if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A) {
        i2c_write_word(dc2dc_addr[0], 0x38, 0x8017,&err);
        i2c_write_word(dc2dc_addr[0], 0x46, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[0], 0x4a, 0xf850,&err);
        i2c_write_word(dc2dc_addr[0], 0x46, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[0], 0x4a, 0xf850,&err);
      } else {
        i2c_write_word(dc2dc_addr[0], 0x38, 0x8018,&err);      
        i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
        i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
        i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85A,&err);
      }
      i2c_write_byte(dc2dc_addr[0], 0x47, 0x3F,&err);
      i2c_write_byte(dc2dc_addr[0], 0xd7, 0x03,&err);
      i2c_write_byte(dc2dc_addr[0], 0x02, 0x02,&err);
      i2c_write_word(dc2dc_addr[0], 0x51, 0x006e,&err);
      i2c_write_word(dc2dc_addr[0], 0x4f, 0x007d ,&err);
      i2c_write_word(dc2dc_addr[0], 0xe6, 0x0007,&err);
      i2c_write_word(dc2dc_addr[0], 0xe5, 0x7c00,&err);
    } else if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A_3PHASE)  {  // FET_T_72A
      i2c_write_word(dc2dc_addr[0], 0x35, 0xf028, &err);
      i2c_write_word(dc2dc_addr[0], 0x36, 0xf018,&err);
      i2c_write_word(dc2dc_addr[0], 0x38, 0x8018,&err);
      i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
      i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85A,&err);
      i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
      i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85A,&err);
      i2c_write_byte(dc2dc_addr[0], 0x47, 0x3F,&err);
      i2c_write_byte(dc2dc_addr[0], 0xd7, 0x03,&err);
      i2c_write_byte(dc2dc_addr[0], 0x02, 0x02,&err);
      i2c_write_word(dc2dc_addr[0], 0x51, 0x006e,&err);
      i2c_write_word(dc2dc_addr[0], 0x4f, 0x007d ,&err);
      i2c_write_word(dc2dc_addr[0], 0xe6, 0x0006,&err);
      i2c_write_word(dc2dc_addr[0], 0xe5, 0x7c00,&err);
    } else if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B_3PHASE)  {
      i2c_write_word(dc2dc_addr[0], 0x35, 0xf028, &err);
       i2c_write_word(dc2dc_addr[0], 0x36, 0xf018,&err);
       i2c_write_word(dc2dc_addr[0], 0x38, 0x8021,&err);
       i2c_write_word(dc2dc_addr[0], 0x39, 0xe024,&err);    
       i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
       i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85a,&err);
       i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
       i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85a,&err);
       i2c_write_byte(dc2dc_addr[0], 0x47, 0x3F,&err);
       i2c_write_byte(dc2dc_addr[0], 0xd7, 0x03,&err);
       i2c_write_byte(dc2dc_addr[0], 0x02, 0x02,&err);
       i2c_write_word(dc2dc_addr[0], 0x51, 0x0087,&err);
       i2c_write_word(dc2dc_addr[0], 0x4f, 0x0096 ,&err);
       i2c_write_word(dc2dc_addr[0], 0xe6, 0x0006,&err);
       i2c_write_word(dc2dc_addr[0], 0xe5, 0x7f00,&err);
    } else if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_78B_3PHASE)  {
       i2c_write_word(dc2dc_addr[0], 0x35, 0xf028, &err);
       i2c_write_word(dc2dc_addr[0], 0x36, 0xf018,&err);
       i2c_write_word(dc2dc_addr[0], 0x38, 0x8021,&err);
       i2c_write_word(dc2dc_addr[0], 0x39, 0xe024,&err);
       i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
       i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85a,&err);
       i2c_write_word(dc2dc_addr[0], 0x46, 0xf864,&err);
       i2c_write_word(dc2dc_addr[0], 0x4a, 0xf85a,&err);
       i2c_write_byte(dc2dc_addr[0], 0x47, 0x3F,&err);
       i2c_write_byte(dc2dc_addr[0], 0xd7, 0x03,&err);
       i2c_write_byte(dc2dc_addr[0], 0x02, 0x02,&err);
       i2c_write_word(dc2dc_addr[0], 0x51, 0x006e,&err);
       i2c_write_word(dc2dc_addr[0], 0x4f, 0x007d ,&err);
       i2c_write_word(dc2dc_addr[0], 0xe6, 0x0006,&err);
       i2c_write_word(dc2dc_addr[0], 0xe5, 0x7f00,&err);
    } else {
      passert("Unknown FET type");
    }

    // Save
    i2c_write(dc2dc_addr[0], 0x15 ,&err);    
    usleep(20000);
    i2c_write(dc2dc_addr[0], 0x3 ,&err);  

    i2c_write_byte(dc2dc_addr[1], 0x00, 0x81 ,&err);
    if (err) {
      psyslog(RED "FAILED TO INIT DC2DC 2 %d\n" RESET, addr);
      dc2dc_i2c_close();
      return 0;
    }

    if ((vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B)  ||
        (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B_I50)) {
      i2c_write_word(dc2dc_addr[1], 0x35, 0xf028 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x36, 0xf018 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x38, 0x8021 ,&err);
      if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B) {
        i2c_write_word(dc2dc_addr[1], 0x46, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[1], 0x4a, 0xf850,&err);
        i2c_write_word(dc2dc_addr[1], 0x46, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[1], 0x4a, 0xf850,&err);
      } else {
        i2c_write_word(dc2dc_addr[1], 0x46, 0xf864,&err);
        i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85a,&err);
        i2c_write_word(dc2dc_addr[1], 0x46, 0xf864,&err);
        i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85a,&err);
      }
      i2c_write_byte(dc2dc_addr[1], 0x47, 0x3F ,&err);
      i2c_write_word(dc2dc_addr[1], 0x51, 0x0087 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x0096 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x51, 0x0087 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x0096 ,&err);
      i2c_write_word(dc2dc_addr[1], 0xe6, 0x001f ,&err);
      i2c_write_word(dc2dc_addr[1], 0xe5, 0x7f00 ,&err);
    }
    if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_78B_I50) {
      i2c_write_word(dc2dc_addr[1], 0x35, 0xf028 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x36, 0xf018 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x38, 0x8021 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x46, 0xf864,&err);
      i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85a,&err);
      i2c_write_word(dc2dc_addr[1], 0x46, 0xf864,&err);
      i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85a,&err);
      i2c_write_byte(dc2dc_addr[1], 0x47, 0x3F ,&err);
      i2c_write_word(dc2dc_addr[1], 0x51, 0x006e ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x007d ,&err);
      i2c_write_word(dc2dc_addr[1], 0x51, 0x006e ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x007d ,&err);
      i2c_write_word(dc2dc_addr[1], 0xe6, 0x001f ,&err);
      i2c_write_word(dc2dc_addr[1], 0xe5, 0x7f00 ,&err);
    }

    if ((vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A)  ||
        (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A_I50))  {  // FET_T_72A
      i2c_write_word(dc2dc_addr[1], 0x35, 0xf028, &err);
      i2c_write_word(dc2dc_addr[1], 0x36, 0xf018,&err);
      if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A) {
        i2c_write_word(dc2dc_addr[1], 0x38, 0x8017,&err);
        i2c_write_word(dc2dc_addr[1], 0x46, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[1], 0x4a, 0xf850,&err);
        i2c_write_word(dc2dc_addr[1], 0x46, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[1], 0x4a, 0xf850,&err);
      } else {
        i2c_write_word(dc2dc_addr[1], 0x38, 0x8018,&err);      
        i2c_write_word(dc2dc_addr[1], 0x46, 0xf864,&err);
        i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85A,&err);
        i2c_write_word(dc2dc_addr[1], 0x46, 0xf864,&err);
        i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85A,&err);
      }
      i2c_write_byte(dc2dc_addr[1], 0x47, 0x3F,&err);
#if 1    // 110/120 
      i2c_write_word(dc2dc_addr[1], 0x51, 0x006e,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x007d ,&err);
#else     // 120/140
      i2c_write_word(dc2dc_addr[1], 0x51, 0x0078,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x008c ,&err);
#endif
      i2c_write_word(dc2dc_addr[1], 0xe6, 0x001f,&err);
      i2c_write_word(dc2dc_addr[1], 0xe5, 0x7c00,&err);
    }
    else if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A_3PHASE)  {  // FET_T_72A
      i2c_write_word(dc2dc_addr[1], 0x35, 0xf028, &err);
      i2c_write_word(dc2dc_addr[1], 0x36, 0xf018,&err);
      i2c_write_word(dc2dc_addr[1], 0x38, 0x8018,&err);
      i2c_write_word(dc2dc_addr[1], 0x46, 0xf864,&err);
      i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85A,&err);
      i2c_write_word(dc2dc_addr[1], 0x46, 0xf864,&err);
      i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85A,&err);
      i2c_write_byte(dc2dc_addr[1], 0x47, 0x3F,&err);
#if 1    // 110/120 
      i2c_write_word(dc2dc_addr[1], 0x51, 0x006e,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x007d ,&err);
#else     // 120/140
      i2c_write_word(dc2dc_addr[1], 0x51, 0x0078,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x008c ,&err);
#endif
      i2c_write_word(dc2dc_addr[1], 0xe6, 0x001e,&err);
      i2c_write_word(dc2dc_addr[1], 0xe5, 0x7c00,&err);
    } else if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B_3PHASE)  {  // FET_T_72A
      i2c_write_word(dc2dc_addr[1], 0x35, 0xf028 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x36, 0xf018 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x38, 0x8021 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x46, 0xf864 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85a ,&err);
      i2c_write_word(dc2dc_addr[1], 0x46, 0xf864 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85a ,&err);
      i2c_write_byte(dc2dc_addr[1], 0x47, 0x3F ,&err);
      i2c_write_word(dc2dc_addr[1], 0x51, 0x0087 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x0096 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x51, 0x0087 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x0096 ,&err);
      i2c_write_word(dc2dc_addr[1], 0xe6, 0x001e ,&err);
      i2c_write_word(dc2dc_addr[1], 0xe5, 0x7f00 ,&err);    
    } else if (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_78B_3PHASE)  {  // FET_T_72A
      i2c_write_word(dc2dc_addr[1], 0x35, 0xf028 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x36, 0xf018 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x38, 0x8021 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x46, 0xf864 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85a ,&err);
      i2c_write_word(dc2dc_addr[1], 0x46, 0xf864 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4a, 0xf85a ,&err);
      i2c_write_byte(dc2dc_addr[1], 0x47, 0x3F ,&err);
      i2c_write_word(dc2dc_addr[1], 0x51, 0x0087 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x0096 ,&err);
      i2c_write_word(dc2dc_addr[1], 0x51, 0x006e ,&err);
      i2c_write_word(dc2dc_addr[1], 0x4f, 0x007d ,&err);
      i2c_write_word(dc2dc_addr[1], 0xe6, 0x001e ,&err);
      i2c_write_word(dc2dc_addr[1], 0xe5, 0x7f00 ,&err);
    } else {
      passert("Unknown FET type");
    }

    // Save    
    i2c_write(dc2dc_addr[1], 0x15 ,&err);    
    usleep(20000);
    i2c_write(dc2dc_addr[1], 0x3 ,&err);  
    psyslog("DC2DC Init %d done OK\n", addr);
    dc2dc_i2c_close();
    pthread_mutex_unlock(&i2c_mutex);
    dc2dc_enable_dc2dc(addr, &err);
    dc2dc_set_vtrim(addr, VOLTAGE_TO_VTRIM_MILLI(660), 1, &err, "init 1");
    //dc2dc_disable_dc2dc(addr,&err);
    pthread_mutex_lock(&i2c_mutex);
    vm.asic[addr].dc2dc.dc_temp_limit = DC_TEMP_LIMIT;
    return 1;
}

void dc2dc_init() {
    int err = 0;
  //printf("%s:%d\n",__FILE__, __LINE__);
   psyslog("DC2DC stop all\n");
   for (int addr = 0; addr < ASICS_COUNT; addr++) {
    if (!vm.board_not_present[ASIC_TO_BOARD_ID(addr)]) {
      if (!dc2dc_is_removed(addr))
    	dc2dc_disable_dc2dc(addr,&err);
    }
  }
  usleep(500000);
  
  pthread_mutex_lock(&i2c_mutex);

  // static int warned = 0;
  // Write defaults
  
  for (int addr = 0; addr < ASICS_COUNT; addr++) {
    int success = 0;
    if (dc2dc_is_removed(addr)) {
        printf("dc2dc %d skipped (REMOVED)\n", addr);
        success = 0;
    } else if (!vm.board_not_present[ASIC_TO_BOARD_ID(addr)]) {
      success = dc2dc_init_rb(addr);
      vm.asic[addr].dc2dc.max_vtrim_currentwise = VTRIM_MAX;
      vm.asic[addr].dc2dc.max_vtrim_temperature = VTRIM_MAX;
    } else {
      printf("dc2dc %d (PSU ID=%d) disabled because no board\n", addr, PSU_ID(addr));
      success = 0;
    }
    
    if (dc2dc_is_removed_or_disabled(addr)) {
    	printf("dc2dc %d REMOVED - setting accordingly\n", addr);
    	vm.asic[addr].dc2dc.dc2dc_present = 0;
    	vm.asic[addr].asic_present = 0;
    	vm.asic[addr].why_disabled = "REMOVED/DISABLED";
    } else if (!success) {
      pthread_mutex_unlock(&i2c_mutex);
      dc2dc_disable_dc2dc(addr,&err);
      printf("DC2DC error : %d\n",addr);
      vm.asic[addr].dc2dc.dc2dc_present = 0;
      vm.asic[addr].asic_problem = 1;
      pthread_mutex_lock(&i2c_mutex);
    } else {
      vm.asic[addr].dc2dc.dc2dc_present = 1;
    }
  }
  pthread_mutex_unlock(&i2c_mutex);

  //dc2dc_disable_dc2dc(0, &err);
  
}

static void dc2dc_set_phase(int phase_addr, int channel_mask, int *err) {
  i2c_write_byte(phase_addr, 0, channel_mask, err);
}

void dc2dc_disable_dc2dc(int addr, int *err) {  
   //printf("%s:%d\n",__FILE__, __LINE__);
  passert(!dc2dc_is_removed(addr));
  pthread_mutex_lock(&i2c_mutex);
  //printf("%s:%d\n",__FILE__, __LINE__);
  dc2dc_select_i2c(addr, err);
  i2c_write_byte(dc2dc_addr[0], 0x02, 0x12, err);
  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
}

int dc2dc_is_removed(int addr) {
	return (vm.asic[addr].user_disabled == ASIC_STATUS_REMOVED ? 1 : 0);
}

int dc2dc_is_removed_or_disabled(int addr) {
	return (vm.asic[addr].user_disabled != ASIC_STATUS_ENABLED ? 1 : 0);
}

int dc2dc_is_user_disabled(int addr) {
	return (vm.asic[addr].user_disabled == ASIC_STATUS_DISABLED? 1 : 0);
}

int loop_is_removed_or_disabled(int l) {
  int loop_enabled = 0;
	for (int i = 0 ; i < ASICS_PER_LOOP; i++) {
    if (vm.asic[l*ASICS_PER_LOOP + i].user_disabled == ASIC_STATUS_ENABLED) {
      loop_enabled = 1;
    }
	}
  return !loop_enabled;
}


void dc2dc_enable_dc2dc(int addr, int *err) {
	passert(!dc2dc_is_removed(addr));
// disengage from scale manager if not needed
#ifdef MINERGATE
  if (vm.asic[addr].dc2dc.dc2dc_present) {
#endif
	pthread_mutex_lock(&i2c_mutex);
    dc2dc_select_i2c(addr, err);
    i2c_write_byte(dc2dc_addr[0], 0x02, 0x02, err);
    dc2dc_i2c_close();
    pthread_mutex_unlock(&i2c_mutex);
#ifdef MINERGATE
  }
#endif
}


static void dc2dc_i2c_close() {
    int err;
    i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0, &err);    
#ifdef SP2x
#else
    i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0, &err);    
#endif
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
}

static void dc2dc_select_i2c_ex(int top,          // 1 or 0
                         int i2c_group,    // 0 or 1
                         int dc2dc_offset, // 0..7
                         int *err) { // 0x00=first, 0x01=second, 0x81=both
  if (top) {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOARD0_MAIN_PIN | PRIMARY_I2C_SWITCH_DEAULT,err); // TOP
  } else {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOARD1_MAIN_PIN | PRIMARY_I2C_SWITCH_DEAULT, err); // BOTTOM
  }

#ifdef SP2x
  i2c_write(I2C_DC2DC_SWITCH_GROUP0, 1 << dc2dc_offset, err); 
#else
  if (i2c_group == 0) {
    //if (dc2dc_offset ==0 && top) return;
    i2c_write(I2C_DC2DC_SWITCH_GROUP0, 1 << dc2dc_offset, err); // TOP
    i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0); // TO
  } else {
    i2c_write(I2C_DC2DC_SWITCH_GROUP1, 1 << dc2dc_offset, err); // TOP    
    i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0);                 // TOP   
  }
#endif
}

#if 0  
int dc2dc_get_dcr_inductor_cat(int addr){
	int rc = 0;
	int err = 0;
	//fprintf(stderr, "---> Entered dc2dc_set_dcr_inductor_cat %d %d\n", addr , value);
	dc2dc_select_i2c(addr , &err);
	if (0 != err) {
		//fprintf(stderr, "LOOP %2d SELECT FAILED\n", addr);
		return -1;
	}

	//fprintf(stderr, "writing %d to LOOP %2d \n", (uint16_t)(0xFFFF & value),addr );
	usleep(1000);

	rc = (0x000F & i2c_read_word(I2C_DC2DC , 0xD0  , &err));

	if (0 != err) {
		return -2;
	}

	return rc;
}

int dc2dc_set_dcr_inductor_cat(int addr,int value){
	int rc = 0;
	int err = 0;
	//fprintf(stderr, "---> Entered dc2dc_set_dcr_inductor_cat %d %d\n", addr , value);

	if (value < 0 || value	> 15){
		fprintf(stderr, "DCR Value of %4d is Invalid. Only 0-15 are supported\n", value);
		return 4;
	}

	dc2dc_select_i2c(addr , &err);
	if (0 != err) {
		//fprintf(stderr, "LOOP %2d SELECT FAILED\n", addr);
		return 1;
	}

	int current_value = i2c_read_word(I2C_DC2DC , 0xD0  , &err);
	int current_dcr = current_value & 0x000F;

	if (0 != err) {
		//fprintf(stderr, "LOOP %2d GET DCR INDUCTOR to %d FAILED\n", addr , value);
		return 3;
	}

	if (current_dcr == value)
	{
		// nothing to do. value already set correctly.
		return 0;
	}

	int set_value = (current_value & 0xFFFFFFF0) | value;

	fprintf(stderr, "writing %d to LOOP %2d (%2d)\n", (uint16_t)(0xFFFF & set_value),value,addr );
	usleep(1000);
	i2c_write_word(I2C_DC2DC, 0xD0, (uint16_t)(0xFFFF & set_value), &err);
	if (0 != err) {
		//fprintf(stderr, "LOOP %2d SET DCR INDUCTOR to %d FAILED\n", addr , value);
		return 2;
	}

	usleep(1000);
	i2c_write(I2C_DC2DC, 0x15, &err);
	usleep(5000);
	if (0 != err) {
		//fprintf(stderr, "LOOP %2d SET DCR INDUCTOR to %d FAILED SAVING CONF\n", addr , value);
		return 3;
	}

	return rc;
}
#endif  


static void dc2dc_select_i2c(int addr, int *err) { // 1 or 0
  passert(!dc2dc_is_removed(addr));

  int top = (addr < ASICS_PER_BOARD);
#ifndef SP2x
  int i2c_group = ((addr % ASICS_PER_BOARD) >= 8);
  int dc2dc_offset = addr % ASICS_PER_PSU;
  dc2dc_offset = dc2dc_offset % 8;
  dc2dc_select_i2c_ex(top,          // 1 or 0
                      i2c_group,    // 0 or 1
                      dc2dc_offset, // 0..7
                      err);
#else
  int dc2dc_offset = addr % ASICS_PER_BOARD;
  dc2dc_select_i2c_ex(top,          // 1 or 0
                      0,    // 0 or 1
                      dc2dc_offset, // 0..7
                      err);

#endif
}



void dc2dc_set_vtrim(int addr, uint32_t vtrim, bool vmargin_75low  , int *err, const char* why) {
  DBG(DBG_SCALING, "Set VOLTAGE ASIC %d Milli:%d Vtrim:%x (%s)\n",addr, VTRIM_TO_VOLTAGE_MILLI(vtrim),vtrim, why);
  passert(!dc2dc_is_removed(addr));
#ifdef MINERGATE
  passert((vtrim >= VTRIM_MIN) && (vtrim <= vm.vtrim_max));
#endif

  pthread_mutex_lock(&i2c_mutex);

  // printf("%d\n",v);
  // int err = 0;
  dc2dc_select_i2c(addr, err);

  i2c_write_word(I2C_DC2DC_0, 0xd4, vtrim&0xFFFF);

  if (vmargin_75low) {
    i2c_write_byte(I2C_DC2DC_0, 0x01, 0x14);
  } else {
    i2c_write_byte(I2C_DC2DC_0, 0x01, 0x0);
  }

  // disengage from scale manager if not needed
#ifdef MINERGATE
  vm.asic[addr].dc2dc.vtrim = vtrim;
  vm.asic[addr].dc2dc.loop_margin_low = vmargin_75low;
  vm.asic[addr].dc2dc.last_voltage_change_time = time(NULL);
#endif

  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
  //FAKE_BIST_PRINT(addr, vm.asic[addr].dc2dc.vtrim , vm.asic[addr].freq_hw);
}

// returns AMPERS
// Mutex locked from above!
int dc2dc_get_current_16s_of_amper_channel(
      int addr, 
      int chanel_id,
      int dc2dc_channel_i2c_addr,
      int* overcurrent_err, 
      int* overcurrent_warning,
      uint8_t *temp,
      int *current,      
      int *err) {
  
  uint8_t temp2;
  int temp_reg;
  passert(!dc2dc_is_removed_or_disabled(addr));
  dc2dc_set_phase(dc2dc_channel_i2c_addr,0 , err);
  temp_reg = i2c_read_word(dc2dc_channel_i2c_addr, 0x8e, err);
  if ((vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B) ||
    (vm.fet[ASIC_TO_BOARD_ID(addr)]   == FET_T_72B_I50) ||
     (vm.fet[ASIC_TO_BOARD_ID(addr)]  == FET_T_72B_3PHASE)) {
    *temp = ((temp_reg)&0x7FF)/4 - 25;
  } else {
    *temp = ((temp_reg)&0x7FF)/4;
  }
  DBG(DBG_TMP, "%d: temp=%d\n",addr, *temp);  
  
  if (*err) {
    return 0;
  }
  *current = (i2c_read_word(dc2dc_channel_i2c_addr, 0x8c) & 0x07FF);
//  psyslog("CURRENT %d:%d =%d\n",addr, chanel_id*2 ,(*current)/16);
  int gen_stat = i2c_read_byte(dc2dc_channel_i2c_addr, 0x78);
  int gen_stat2 = i2c_read_byte(dc2dc_channel_i2c_addr, 0x7A); 
  int problems = i2c_read_byte(dc2dc_channel_i2c_addr, 0x7b);
  *overcurrent_err |= (problems & 0x80);
  *overcurrent_warning |= (problems & 0x20);
  if (*overcurrent_err || (gen_stat & 0x8) || (gen_stat2 & 0x10)) {
    mg_event_x(RED "DC2DC[%d] 7b=0x%x, 7A=%x, 78=%x\n" RESET,addr,problems, gen_stat2, gen_stat);
    i2c_write(dc2dc_channel_i2c_addr, 0x03);
  }
  if (*overcurrent_warning) {
    i2c_write(dc2dc_channel_i2c_addr, 0x03);
  }

  int overtemp = i2c_read_byte(dc2dc_channel_i2c_addr, 0x7d);
  if (overtemp & 0xC0) {
    if (!vm.asic[addr].ot_warned_a) {
      psyslog(RED "DC2DC A TEMP WARN %d:p%d - 0x%x\n" RESET,addr,dc2dc_channel_i2c_addr,overtemp);
      vm.asic[addr].ot_warned_a = 1;
    }
    i2c_write(dc2dc_channel_i2c_addr, 0x03);
  }

  
  if ((chanel_id == 0) ||
      (
        (vm.fet[ASIC_TO_BOARD_ID(addr)] != FET_T_72B_3PHASE) &&
        (vm.fet[ASIC_TO_BOARD_ID(addr)] != FET_T_78B_3PHASE) &&
        (vm.fet[ASIC_TO_BOARD_ID(addr)] != FET_T_72A_3PHASE) 
      )
     ){
    
    //--------------
    dc2dc_set_phase(dc2dc_channel_i2c_addr,1 , err);
    //--------------
    int cur2=(i2c_read_word(dc2dc_channel_i2c_addr, 0x8c) & 0x07FF);
 //   psyslog("CURRENT %d:%d =%d\n",addr, chanel_id*2 +1 ,(cur2)/16);
    *current += cur2;

    temp_reg = i2c_read_word(dc2dc_channel_i2c_addr, 0x8e, err);
    if ((vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B) ||
        (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B_I50) ||
        (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72B_3PHASE)) {
      temp2 = (((temp_reg)&0x7FF)/4) - 25;
    } else {
      temp2 = (((temp_reg)&0x7FF)/4);
    }
     DBG(DBG_TMP,"%d: temp=%d\n",addr, temp2);  
    
    if (temp2>*temp) {
      *temp = temp2;
    }
    problems = i2c_read_word(dc2dc_channel_i2c_addr, 0x7b);  
    int gen_stat = i2c_read_byte(dc2dc_channel_i2c_addr, 0x78);
    int gen_stat2 = i2c_read_byte(dc2dc_channel_i2c_addr, 0x7A); 
    *overcurrent_err |= (problems & 0x80);
    *overcurrent_warning |= (problems & 0x20);  

    if (*overcurrent_err || (gen_stat & 0x8) || (gen_stat2 & 0x10)) {
       mg_event_x(RED "DC2DC<%d> 7b=0x%x, 7A=%x, 78=%x\n" RESET,addr,problems, gen_stat2, gen_stat);
       i2c_write(dc2dc_channel_i2c_addr, 0x03);
     }
     if (*overcurrent_warning) {
       i2c_write(dc2dc_channel_i2c_addr, 0x03);
     }

    
    overtemp = i2c_read_byte(dc2dc_channel_i2c_addr, 0x7d);
    if (overtemp & 0xC0) {
      if (!vm.asic[addr].ot_warned_b) {
        psyslog(RED "DC2DC B OVERTEMP WARN %d:p%d - 0x%x\n" RESET,addr,dc2dc_channel_i2c_addr,overtemp);
        vm.asic[addr].ot_warned_b = 1;
      }
      i2c_write(dc2dc_channel_i2c_addr, 0x03);
    }
  }
  dc2dc_set_phase(dc2dc_channel_i2c_addr, 0x81, err);
  if (*err) {
    return 0;
  }
  

}





int dc2dc_get_all_stats(
      int addr, 
      int* overcurrent_err, 
      int* overcurrent_warning,
      uint8_t *temp,
      int* current,      
      int *err) {
  // TODO - select addr!
  // int err = 0;
  passert(err != NULL);
  passert(!dc2dc_is_removed_or_disabled(addr));
  //printf("%s:%d\n",__FILE__, __LINE__);
  *err = 0;

  pthread_mutex_lock(&i2c_mutex);
  *overcurrent_err = 0;
  *overcurrent_warning = 0;  
  static int warned = 0;
  dc2dc_select_i2c(addr, err);
  int phaze_overcurrent_err[2]  = {0,0};
  int phaze_overcurrent_warning[2] = {0,0};
  uint8_t phaze_temp[2] = {0,0};
  int phaze_current[2] = {0,0};
  vm.asic[addr].dc2dc.voltage_from_dc2dc = i2c_read_word(dc2dc_addr[0], 0x8b)*1000/512;
  dc2dc_get_current_16s_of_amper_channel(
        addr, 
        0,
        dc2dc_addr[0],
        phaze_overcurrent_err+0, 
        phaze_overcurrent_warning+0,
        phaze_temp+0,
        phaze_current+0,      
        err);
  if (*err) {
    psyslog("i2c error on dc2dc  %i:0\n", addr);
    dc2dc_i2c_close();
    //vm.asic[addr].dc2dc.dc2dc_present = 0;
    pthread_mutex_unlock(&i2c_mutex);
    //*err = 1;
    return 0;
  }
 
  dc2dc_get_current_16s_of_amper_channel(
          addr, 
          1, 
          dc2dc_addr[1],
          phaze_overcurrent_err+1, 
          phaze_overcurrent_warning+1,
          phaze_temp+1,
          phaze_current+1,      
          err);
  if (*err) {
    psyslog("i2c error on dc2dc  %i:1\n", addr);
    dc2dc_i2c_close();
    //vm.asic[addr].dc2dc.dc2dc_present = 0;
    pthread_mutex_unlock(&i2c_mutex);
    //*err = 1;
    return 0;
  }

  *overcurrent_err = phaze_overcurrent_err[0] || phaze_overcurrent_err[1];
  *overcurrent_warning = phaze_overcurrent_warning[0] || phaze_overcurrent_warning[1];
  if (*overcurrent_err || *overcurrent_warning) {
    psyslog(YELLOW "DC %d ERROR:%d, WARNING:%d\n" RESET,addr,*overcurrent_err, *overcurrent_warning);
  }
  *temp = (phaze_temp[0] > phaze_temp[1])? phaze_temp[0] : phaze_temp[1];
  //printf("%d %d %d\n",*current,phaze_current[0],phaze_current[1]);
  *current = phaze_current[0] + phaze_current[1];
  if ((vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A) ||
      (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A_I50) ||
      (vm.fet[ASIC_TO_BOARD_ID(addr)] == FET_T_72A_3PHASE)){
    *current = (*current)*2;
  }

  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
  return 1;
}

