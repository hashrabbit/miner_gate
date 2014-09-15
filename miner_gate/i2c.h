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

#ifndef _____ASIC_I2C_H____
#define _____ASIC_I2C_H____

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include "spond_debug.h"

//#include "i2cbusses.h"

// set voltage test
// i2c_write_word(0x1b, 0xd4, 0xFFD4);
// i2c_write_byte(0x1b, 0x01, 0x00);
// usleep(1000000);
// return 0;

#define PRIMARY_I2C_SWITCH 0x70
#define PRIMARY_I2C_SWITCH_DEAULT 0//PRIMARY_I2C_SWITCH_AC2DC_PIN

#define PRIMARY_I2C_SWITCH_AC2DC_PSU_1_PIN 0x01
#define PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN 0x10

#define PRIMARY_I2C_SWITCH_TEMP_SENSOR_PIN  0x02
#define PRIMARY_I2C_SWITCH_BOARD0_MAIN_PIN 0x04
#define PRIMARY_I2C_SWITCH_BOARD1_MAIN_PIN 0x08

#ifdef SP2x
#define I2C_DC2DC_SWITCH_GROUP0 0x71 // 0 to 7
#define I2C_MGMT_THERMAL_SENSOR 0x48 // 0 to 7
#define I2C_MAIN_THERMAL_SENSOR 0x49 // 0 to 7
#else 
#define I2C_DC2DC_SWITCH_GROUP0 0x71 // 0 to 7
#define I2C_DC2DC_SWITCH_GROUP1 0x72 // 8 to 11
#define I2C_MGMT_THERMAL_SENSOR 0x48 // 0 to 7
#define I2C_MAIN_THERMAL_SENSOR 0x49 // 0 to 7
#endif

#define I2C_DC2DC_0 0x18
#define I2C_DC2DC_1 0x1A


int i2c_getint_x1000(int source);
int i2c_getint(int source);

extern int ignorable_err;
void my_i2c_set_address(int address, int *pError);
int __i2c_write_block_data(uint8_t command, uint8_t len ,const unsigned char * buff );
void i2c_init(int *pError = &ignorable_err);
uint8_t i2c_read(uint8_t addr, int *pError = &ignorable_err);
void i2c_write(uint8_t addr, uint8_t value, int *pError = &ignorable_err);
uint8_t i2c_read_byte(uint8_t addr, uint8_t command, int *pError = &ignorable_err);
void i2c_write_byte(uint8_t addr, uint8_t command, uint8_t value,
                    int *pError = &ignorable_err);
uint16_t i2c_read_word(uint8_t addr, uint8_t command, int *pError = &ignorable_err);
void i2c_write_word(uint8_t addr, uint8_t command, uint16_t value, int *pError = &ignorable_err);
uint8_t i2c_waddr_read_byte(uint8_t addr, uint16_t dev_addr, int *pError = &ignorable_err);
void i2c_waddr_write_byte(uint8_t addr, uint16_t dev_addr, uint8_t value, int *pError = &ignorable_err);
uint16_t read_mgmt_temp();

#endif
