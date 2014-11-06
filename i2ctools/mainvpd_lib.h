/* 
 * File:   mainvpd_lib.h
 * Author: zvisha
 *
 * Created on October 14, 2014, 2:49 PM
 */

#ifndef MAINVPD_LIB_H
#define	MAINVPD_LIB_H

#include <stdio.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
//#include "hammer.h"
#include <spond_debug.h>
#include "squid.h"
#include "i2c.h"
#include "defines.h"
#include "asic.h"

using namespace std;
extern pthread_mutex_t i2c_mutex;

#define MAIN_VPD_REV 2
#define FET_SUPPORT_VPD_REV 2

#define TOP_BOARD 0
#define BOTTOM_BOARD 1
#define BOTH_MAIN_BOARDS 2
#define CUSTOM_LOOPS_SELECTION 3

#define MAIN_BOARD_I2C_SWITCH_EEPROM_PIN 0x80
#define MAIN_BOARD_I2C_EEPROM_DEV_ADDR 0x57
#define MAIN_BOARD_VPD_EEPROM_SIZE 0x80

#define MAIN_BOARD_VPD_ADDR_START 0x00

#define MAIN_BOARD_VPD_VPDREV_ADDR_START 0x00
#define MAIN_BOARD_VPD_VPDREV_ADDR_LENGTH 0x01 // end address included.

#define MAIN_BOARD_VPD_SERIAL_ADDR_START 0x01
#define MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH 0x0C // end address included.

#define MAIN_BOARD_VPD_PNR_ADDR_START 0x0D
#define MAIN_BOARD_VPD_PNR_ADDR_LENGTH 0x08 // end address included.

#define MAIN_BOARD_VPD_PNRREV_ADDR_START 0x15
#define MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH 0x03 // end address included.

#define MAIN_BOARD_VPD_FET_STR_ADDR_START 0x18
#define MAIN_BOARD_VPD_FET_STR_ADDR_LENGTH 0x0E

#define MAIN_BOARD_VPD_FET_FLAG_ADDR_START 0x26
#define MAIN_BOARD_VPD_FET_FLAG_ADDR_LENGTH 0x01

#define MAIN_BOARD_VPD_FET_CODE_ADDR_START 0x27
#define MAIN_BOARD_VPD_FET_CODE_ADDR_LENGTH 0x01

#define MAIN_BOARD_VPD_RESERVED_ADDR_START 0x28
#define MAIN_BOARD_VPD_RESERVED_ADDR_LENGTH 0x22 // end address included.

#define MAIN_BOARD_VPD_ADDR_END 0x49 // end address included.
#define MAIN_BOARD_VPD_ADDR_LENGTH 0x4A // end address included.





//#define PRIMARY_I2C_SWITCH_TOP_MAIN_PIN 0x04 - in i2c.h
//#define PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN 0x08 - in i2c.h

typedef struct {
	//unsigned char vpdrev[MAIN_BOARD_VPD_VPDREV_ADDR_LENGTH + 1]; // 1+1
	char serial[ MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH + 1]; // WW[2]+SER[4]+REV[2]+PLANT[2]
	char pnr[MAIN_BOARD_VPD_PNR_ADDR_LENGTH + 1];
	char revision[MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH + 1];
} mainboard_vpd_info_t;

void resetI2CSwitches();
inline int fix_max_cap(int val, int max);
int usage(char * app ,int exCode ,const char * errMsg );
int usage(char * app ,int exCode );
int setI2CSwitches(int tob);
int set_fet(int topOrBottom , int fet_type);
int get_fet(int topOrBottom );
int get_fet_str(int topOrBottom , char * fet);
int mainboard_set_vpd(  int topOrBottom, const char * vpd);
int readMain_I2C_eeprom (char * vpd_str , int topOrBottom , int startAddress , int length);
int mainboard_get_vpd(int topOrBottom ,  mainboard_vpd_info_t *pVpd);






#endif	/* MAINVPD_LIB_H */

