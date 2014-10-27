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

#ifndef __AC2DC_280114_CONSTANTS_H_
#define __AC2DC_280114_CONSTANTS_H_

#include "defines.h"

// imp from ac2dc.c

// imp from ac2dc.c
#define AC2DC_MURATA_NEW_I2C_EEPROM_DEVICE 0x57
#define AC2DC_MURATA_NEW_I2C_MGMT_DEVICE 0x5F

//#define AC2DC_MURATA_OLD_I2C_EEPROM_DEVICE 0
//#define AC2DC_MURATA_OLD_I2C_MGMT_DEVICE 0

#define AC2DC_EMERSON_1200_I2C_EEPROM_DEVICE 0x57
#define AC2DC_EMERSON_1200_I2C_MGMT_DEVICE 0x3F

#define AC2DC_EMERSON_1600_I2C_EEPROM_DEVICE 0x53
#define AC2DC_EMERSON_1600_I2C_MGMT_DEVICE 0x5b


// MGMT i2c registers
#define AC2DC_I2C_STATUS_WORD 0x79
#define AC2DC_I2C_VOUT_STATUS 0x7A
#define AC2DC_I2C_VIN_STATUS 0x7C
#define AC2DC_I2C_TEMP_STATUS 0x7D
#define AC2DC_I2C_CML_STATUS 0x7E
#define AC2DC_I2C_FAN_STATUS 0x81

#define AC2DC_I2C_READ_VIN_WORD 0x88
#define AC2DC_I2C_READ_IIN_WORD 0x89
#define AC2DC_I2C_READ_VCAP_WORD 0x8A
#define AC2DC_I2C_READ_VOUT_WORD 0x8B
#define AC2DC_I2C_READ_IOUT_WORD 0x8C
#define AC2DC_I2C_READ_TEMP_WORD 0x8E
#define AC2DC_I2C_READ_FANSPEED_WORD 0x90
#define AC2DC_I2C_READ_POUT_WORD 0x96
#define AC2DC_I2C_READ_PIN_WORD 0x97
#define AC2DC_I2C_READ_FAULTS   0xEF

#define AC2DC_I2C_READ_STATUS_IOUT   0x7b
#define AC2DC_I2C_READ_STATUS_IOUT_OP_WARN   0x1
#define AC2DC_I2C_READ_STATUS_IOUT_OP_ERR    0x2
#define AC2DC_I2C_READ_STATUS_IOUT_OC_WARN   0x20
#define AC2DC_I2C_READ_STATUS_IOUT_OC_ERR    0x80


#define AC2DC_I2C_WRITE_PROTECT 0x10
#define AC2DC_I2C_ON_OFF        0x1



// status/fault bit flags relates to AC2DC_I2C_STATUS_WORD 0x79
#define AC2DC_I2C_STATUS_VOUT ((1 << 7) << 8)
#define AC2DC_I2C_STATUS_LOUTPOUT ((1 << 6) << 8)
#define AC2DC_I2C_STATUS_INPUT ((1 << 5) << 8)
#define AC2DC_I2C_STATUS_MFR ((1 << 4) << 8)
#define AC2DC_I2C_STATUS_POWERGOOD ((1 << 3) << 8)
#define AC2DC_I2C_STATUS_FAN ((1 << 2) << 8)
#define AC2DC_I2C_STATUS_OTHER ((1 << 1) << 8)
#define AC2DC_I2C_STATUS_UNKNOWN ((1 << 0) << 8)
#define AC2DC_I2C_STATUS_BUSY (1 << 7)
#define AC2DC_I2C_STATUS_OFF (1 << 6)
#define AC2DC_I2C_STATUS_OV (1 << 5)
#define AC2DC_I2C_STATUS_OC (1 << 4)
#define AC2DC_I2C_STATUS_UV (1 << 3)
#define AC2DC_I2C_STATUS_TEMP (1 << 2)
#define AC2DC_I2C_STATUS_CML (1 << 1)
#define AC2DC_I2C_STATUS_NONE (1 << 0)

// status/fault bit flags relates to AC2DC_I2C_VOUT_STATUS 0x7A
#define AC2DC_I2C_VOUT_OV_FLT (1 << 7)
#define AC2DC_I2C_VOUT_OV_WRN (1 << 6)
#define AC2DC_I2C_VOUT_UV_WRN (1 << 5)
#define AC2DC_I2C_VOUT_UV_FLT (1 << 4)
#define AC2DC_I2C_VOUT_MAX_WRN (1 << 3)
#define AC2DC_I2C_VOUT_TON_MAX_WRN (1 << 2)
#define AC2DC_I2C_VOUT_TOFF_MAX_WRN (1 << 1)
#define AC2DC_I2C_VOUT_PWR_ON_TRK_ERR (1 << 0)

// status/fault bit flags relates to AC2DC_I2C_READ_STATUS_IOUT 0x7B
#define AC2DC_I2C_IOUT_OC_FLT (1 << 7)
#define AC2DC_I2C_IOUT_OVC_LV_SHTDWN (1 << 6)
#define AC2DC_I2C_IOUT_OC_WRN (1 << 5)
#define AC2DC_I2C_IOUT_UC_FLT (1 << 4)
#define AC2DC_I2C_IOUT_CURR_SHARE (1 << 3)
#define AC2DC_I2C_IOUT_IN_PWR_LIM_MODE (1 << 2)
#define AC2DC_I2C_IOUT_POUT_OP_FLT (1 << 1)
#define AC2DC_I2C_IOUT_POUT_OP_WRN (1 << 0)

// status/fault bit flags relates to AC2DC_I2C_VIN_STATUS 0x7C
#define AC2DC_I2C_VIN_OV_FLT (1 << 7)
#define AC2DC_I2C_VIN_OV_WRN (1 << 6)
#define AC2DC_I2C_VIN_UV_WRN (1 << 5)
#define AC2DC_I2C_VIN_UV_FLT (1 << 4)
#define AC2DC_I2C_VIN_UNIT_OFF_LOW_VIN (1 << 3)
#define AC2DC_I2C_VIN_OC_FLT (1 << 2)
#define AC2DC_I2C_VIN_OC_WRN (1 << 1)
#define AC2DC_I2C_VIN_PIN_OP_WRN (1 << 0)

// status/fault bit flags relates to AC2DC_I2C_TEMP_STATUS 0x7D
#define AC2DC_I2C_TEMP_OT_FLT (1 << 7)
#define AC2DC_I2C_TEMP_OT_WRN (1 << 6)
#define AC2DC_I2C_TEMP_UT_WRN (1 << 5)
#define AC2DC_I2C_TEMP_UT_FLT (1 << 4)
// BIT 0-3 in 0x7D Reserved

// status/fault bit flags relates to AC2DC_I2C_CML_STATUS 0x7E
#define AC2DC_I2C_CML_INVALID_CMD (1 << 7)
#define AC2DC_I2C_CML_INVALID_DATE (1 << 6)
#define AC2DC_I2C_CML_PCK_ERR_CHK_FLT (1 << 5)
#define AC2DC_I2C_CML_MEM_FLT_DETECTED (1 << 4)
#define AC2DC_I2C_CML_PROC_FLT_DETECTED (1 << 3)
//#define Reserved      (1 << 2)
#define AC2DC_I2C_CML_OTHER_COMM_FLT (1 << 1)
#define AC2DC_I2C_CML_OTHER_MEM_FLT (1 << 0)

// status/fault bit flags relates to AC2DC_I2C_FAN_STATUS 0x81
#define AC2DC_I2C_FAN1_FLT (1 << 7)
#define AC2DC_I2C_FAN2_FLT (1 << 6)
#define AC2DC_I2C_FAN1_WRN (1 << 5)
#define AC2DC_I2C_FAN2_WRN (1 << 4)
#define AC2DC_I2C_FAN1_SPD_OVRDN (1 << 3)
#define AC2DC_I2C_FAN2_SPD_OVRDN (1 << 2)
#define AC2DC_I2C_AIRFLOW_FLT (1 << 1)
#define AC2DC_I2C_AIRFLOW_WRN (1 << 0)

#endif
