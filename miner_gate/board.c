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


#include "dc2dc.h"
#include "i2c.h"
#include "nvm.h"
#include "asic.h"
#include <time.h>
#include <pthread.h>

#include "defines.h"





int get_mng_board_temp(int *real_temp) {
  int err;
  int reg;  
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TEMP_SENSOR_PIN | PRIMARY_I2C_SWITCH_DEAULT);
  reg = i2c_read_word(I2C_MGMT_THERMAL_SENSOR, 0x0, &err);
  reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
  *real_temp = (reg*625)/10000;
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);   
  if (*real_temp > 200) {
      return 0;
  }
  return *real_temp;
}

int get_top_board_temp(int *real_temp) {
  int err;
   int reg;
   
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOARD0_MAIN_PIN | PRIMARY_I2C_SWITCH_DEAULT, &err, 0);    
#ifdef SP2x
   i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0x80, &err, 0);
#else
   i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0x80, &err, 0);
#endif
   if (err && !vm.board_not_present[0]) {
    psyslog(RED "LOST TOP BOARD!\n" RESET);
   }
   vm.board_not_present[0] = (err != 0);
   reg = i2c_read_word(I2C_MAIN_THERMAL_SENSOR, 0x0, &err, 0);
   reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
   *real_temp = (reg*625)/10000;
#ifdef SP2x
   i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0, &err, 0);
#else
   i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0, &err, 0);
#endif   
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT, &err, 0);   
   if (*real_temp > 200) {
      return 0;
   }
   return *real_temp;
}

int get_bottom_board_temp(int *real_temp) {
   int err;
   int reg;  
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOARD1_MAIN_PIN | PRIMARY_I2C_SWITCH_DEAULT, &err, 0);   
#ifdef SP2x
      i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0x80, &err, 0);
#else
      i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0x80, &err, 0);
#endif

   if (err && !vm.board_not_present[1]) {
    psyslog(RED "LOST BOTTOM BOARD!\n" RESET);
   }   
   vm.board_not_present[1] = (err != 0);      
   reg = i2c_read_word(I2C_MAIN_THERMAL_SENSOR, 0x0, &err, 0);
   reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
   *real_temp = (reg*625)/10000;
#ifdef SP2x
      i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0, &err, 0);
#else
      i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0, &err, 0);
#endif  

   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT, &err, 0);   
   if (*real_temp > 200) {
       return 0;
   }

   return *real_temp;
}




