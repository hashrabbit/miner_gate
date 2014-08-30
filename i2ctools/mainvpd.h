#ifndef _____MAINVPD__1203_H____
#define _____MAINVPD__1203_H____
#include "defines.h"

#define TOP_BOARD 0
#define BOTTOM_BOARD 1
#define BOTH_MAIN_BOARDS 2

#define MAIN_BOARD_I2C_SWITCH_EEPROM_PIN 0x80
#define MAIN_BOARD_I2C_EEPROM_DEV_ADDR 0x57
#define MAIN_BOARD_VPD_ADDR_START 0x00
#define MAIN_BOARD_VPD_ADDR_END 0x2B // end address included.

#define MAIN_BOARD_VPD_ADDR_START 0x00
#define MAIN_BOARD_VPD_ADDR_END 0x2B // end address included.
#define MAIN_BOARD_VPD_ADDR_LENGTH 0x2C // end address included.

#define MAIN_BOARD_VPD_VPDREV_ADDR_START 0x00
#define MAIN_BOARD_VPD_VPDREV_ADDR_LENGTH 0x01 // end address included.

#define MAIN_BOARD_VPD_SERIAL_ADDR_START 0x01
#define MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH 0x0C // end address included.

#define MAIN_BOARD_VPD_PNR_ADDR_START 0x0D
#define MAIN_BOARD_VPD_PNR_ADDR_LENGTH 0x08 // end address included.

#define MAIN_BOARD_VPD_PNRREV_ADDR_START 0x15
#define MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH 0x03 // end address included.

#define MAIN_BOARD_VPD_RESERVED_ADDR_START 0x18
#define MAIN_BOARD_VPD_RESERVED_ADDR_LENGTH 0x14 // end address included.



//#define PRIMARY_I2C_SWITCH_TOP_MAIN_PIN 0x04 - in i2c.h
//#define PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN 0x08 - in i2c.h



typedef struct {
	//unsigned char vpdrev[MAIN_BOARD_VPD_VPDREV_ADDR_LENGTH + 1]; // 1+1
	unsigned char serial[ MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH + 1]; // WW[2]+SER[4]+REV[2]+PLANT[2]
	unsigned char pnr[MAIN_BOARD_VPD_PNR_ADDR_LENGTH + 1];
	unsigned char revision[MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH + 1];
} mainboard_vpd_info_t;


#endif
