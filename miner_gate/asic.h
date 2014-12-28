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

#ifndef _____ASIC_H____
#define _____ASIC_H____

#include "nvm.h"
#define dprintf printf

#define SHA256_HASH_BITS 256
#define SHA256_HASH_BYTES (SHA256_HASH_BITS / 8)
#define SHA256_BLOCK_BITS 512
#define SHA256_BLOCK_BYTES (SHA256_BLOCK_BITS / 8)

typedef struct {
  uint32_t h[8];
  uint32_t length;
} sha2_small_common_ctx_t;

/** \typedef sha256_ctx_t
 * \brief SHA-256 context type
* 
 * A variable of this type may hold the state of a SHA-256 hashing process
 */
typedef sha2_small_common_ctx_t sha256_ctx_t;



#define ANY_ASIC 0xff
#define ANY_ENGINE 0xff
#define NO_ENGINE 0xfe


// ENGINE REGISTERS
#define ADDR_CONTROL 0x2
#define BIT_ADDR_CONTROL_BIST_MODE            (1<<0)
#define BIT_ADDR_CONTROL_USE_1XCK             (1<<1)
#define BIT_ADDR_CONTROL_FIFO_RESET_N         (1<<2)
#define BIT_ADDR_CONTROL_IDLE_WHEN_FIFO_EMPTY (1<<3)

#define ADDR_CONTROL_SET0 0x6
#define ADDR_CONTROL_SET1 0x7 

#define ADDR_COMMAND 0x3
#define BIT_ADDR_COMMAND_FIFO_LOAD       0x1
#define BIT_ADDR_COMMAND_END_CURRENT_JOB 0x2
#define BIT_ADDR_COMMAND_END_CURRENT_JOB_IF_FIFO_FULL 0x4
#define BIT_ADDR_COMMAND_WIN_CLEAR       0x8

#define ADDR_ENGINE_STATUS 0xA
#define BIT_ADDR_ENGINE_FIFO_STATUS (1<<0)
#define BIT_ADDR_ENGINE_WINNER_EXIST (1<<2)
#define BIT_ADDR_ENGINE_CONDUCTOR_IDLE_SC (1<<3)
#define BIT_ADDR_ENGINE_BIST_MODE (1<<4)
#define BIT_ADDR_ENGINE_FIFO_RESET_N (1<<5)
#define BIT_ADDR_ENGINE_SOFT_HASH_RESET_N (1<<6)
#define BIT_ADDR_ENGINE_SOFT_HASH_CLK_EN (1<<7)
#define BIT_ADDR_ENGINE_JOBID (1<<8)

#define ADDR_MID_STATE0 0x50
#define ADDR_MID_STATE1 0x51
#define ADDR_MID_STATE2 0x52
#define ADDR_MID_STATE3 0x53 
#define ADDR_MID_STATE4 0x54
#define ADDR_MID_STATE5 0x55
#define ADDR_MID_STATE6 0x56
#define ADDR_MID_STATE7 0x57
#define ADDR_MARKEL_ROOT 0x58
#define ADDR_TIMESTEMP 0x59
#define ADDR_DIFFICULTY 0x5A
#define ADDR_WIN_LEADING_0 0x5B
#define ADDR_JOBID 0x5C
#define ADDR_WINNER_JOBID 0x60
#define ADDR_WINNER_NONCE 0x61
#define ADDR_WINNER_EXIST 0x62
#define ADDR_CURRENT_NONCE 0x63
#define ADDR_SOFT_HASH_RESET_N 0x98
#define ADDR_SOFT_HASH_CLK_EN 0x88
#define ADDR_NONCE_START 0xB0 
#define ADDR_NONCE_RANGE 0xC3 
#define ADDR_BIST_NONCE_START 0xD0
#define ADDR_BIST_NONCE_RANGE 0xD1
#define ADDR_BIST_CRC_EXPECTED 0xD2
#define ADDR_BIST_PARITY 0xDF
#define ADDR_SPARE0 0xEA
#define ADDR_SPARE1 0xEB


// MNGMT REGISTERS
#define ADDR_CHIP_ADDR 0x0
#define ADDR_GOT_ADDR 0x1
#define ADDR_GLOBAL_CLK_EN 0x4
#define ADDR_GLOBAL_HASH_RESETN 0x5

#define ADDR_MNG_COMMAND 0x8 
#define BIT_ADDR_MNG_TS_RESET_0         0x01
#define BIT_ADDR_MNG_TS_RESET_1         0x02
#define BIT_ADDR_MNG_BAD_ACCEESS_CLEAR  0x04
#define BIT_ADDR_MNG_ZERO_IDLE_COUNTER  0x08 

#define ADDR_DEBUG_CONTROL 0x9
#define BIT_ADDR_DEBUG_DISABLE_TRANSMIT   1
#define BIT_ADDR_DEBUG_STOP_SERIAL_CHAIN  2
#define BIT_ADDR_DEBUG_DISABLE_ENGINE_TRANSACTIONS 4

#define ADDR_TS_RSTN_0 0xE
#define ADDR_TS_RSTN_1 0xF
#define ADDR_PLL_CONFIG 0x11 
#define ADDR_PLL_ENABLE 0x12
#define ADDR_PLL_BYPASS 0x14
#define ADDR_PLL_TEST_MODE 0x15
#define ADDR_PLL_TEST_CONFIG 0x16
#define ADDR_PLL_DLY_INC 0x17
#define ADDR_PLL_SW_OVERRIDE 0x18
#define ADDR_PLL_POWER_DOWN 0x19
#define ADDR_PLL_RESET 0x1A
#define ADDR_PLL_STATUS 0x1B
#define ADDR_TS_SET_0 0x1C
#define ADDR_TS_SET_1 0x1D
#define ADDR_TS_DATA_0 0x1E
#define ADDR_TS_DATA_1 0x1F
#define ADDR_TS_EN_0 0x20
#define ADDR_TS_EN_1 0x21
#define ADDR_SHUTDOWN_ACTION 0x22
#define ADDR_TS_TS_0 0x2E
#define ADDR_TS_TS_1 0x2F
#define ADDR_INTR_MASK 0x30 
#define ADDR_INTR_CLEAR 0x31
#define ADDR_INTR_RAW 0x32
#define ADDR_INTR_SOURCE 0x33

#define BIT_INTR_GOT_ADDRESS_NOT  (1<<0)
#define BIT_INTR_WIN              (1<<1)
#define BIT_INTR_CONDUCTOR_IDLE   (1<<4)
#define BIT_INTR_CONDUCTOR_BUSY   (1<<5)
#define BIT_INTR_BIST_FAILURE     (1<<8)
#define BIT_INTR_THERMAL_SHUTDOWN (1<<9)
#define BIT_INTR_RESERVED_10      (1<<10)
#define BIT_INTR_TS_0_OVER        (1<<11)
#define BIT_INTR_TS_0_UNDER       (1<<12)
#define BIT_INTR_TS_1_OVER        (1<<13)
#define BIT_INTR_TS_1_UNDER       (1<<14)
#define BIT_INTR_PLL_NOT_READY    (1<<15)
#define ADDR_INTR_BC_GOT_ADDR_NOT  0x40
#define ADDR_INTR_BC_WIN 0x41
#define ADDR_INTR_BC_CONDUCTOR_IDLE 0x44
#define ADDR_INTR_BC_CONDUCTOR_BUSY 0x45
#define ADDR_INTR_BC_BIST_EXP_FAIL 0x48
#define ADDR_INTR_BC_THERMAL_SHUTDOWN 0x49 
#define ADDR_INTR_BC_RESERVED_10 0x4A 
#define ADDR_INTR_BC_TS_0_OVER 0x4B
#define ADDR_INTR_BC_TS_0_UNDER 0x4C
#define ADDR_INTR_BC_TS_1_OVER 0x4D
#define ADDR_INTR_BC_TS_1_UNDER 0x4E
#define ADDR_INTR_BC_PLL_NOT_READY 0x4F
#define ADDR_VERSION 0x70
#define ADDR_ENGINES_PER_CHIP 0x71
#define ADDR_REG_BAD_ACCESS 0x7F
#define ADDR_ENABLE_ENGINES_0 0x80
#define ADDR_ENABLE_ENGINES_1 0x81
#define ADDR_ENABLE_ENGINES_2 0x82
#define ADDR_ENABLE_ENGINES_3 0x83 
#define ADDR_ENABLE_ENGINES_4 0x84
#define ADDR_ENABLE_ENGINES_5 0x85
#define ADDR_ENABLE_ENGINES_6 0x86
#define ADDR_ENABLE_ENGINES_7 0x87
#define ADDR_SERIAL_RESETN_ENGINES_0 0x90
#define ADDR_SERIAL_RESETN_ENGINES_1 0x91 
#define ADDR_SERIAL_RESETN_ENGINES_2 0x92
#define ADDR_SERIAL_RESETN_ENGINES_3 0x93
#define ADDR_SERIAL_RESETN_ENGINES_4 0x94
#define ADDR_SERIAL_RESETN_ENGINES_5 0x95
#define ADDR_SERIAL_RESETN_ENGINES_6 0x96
#define ADDR_SERIAL_RESETN_ENGINES_7 0x97
#define ADDR_WINNER_ENGINE_ID 0xA0
#define ADDR_BIST_FAILED_INDEX 0xA1
#define ADDR_CONDUCTOR_IDLE_INDEX 0xA2
#define ADDR_IDLE_COUNTER 0xA3
#define ADDR_BIST_ALLOWED_FAILURE_NUM 0xD3
#define ADDR_BIST_PASS_0 0xD4
#define ADDR_BIST_PASS_1 0xD5
#define ADDR_BIST_PASS_2 0xD6
#define ADDR_BIST_PASS_3 0xD7 
#define ADDR_BIST_PASS_4 0xD8
#define ADDR_BIST_PASS_5 0xD9
#define ADDR_BIST_PASS_6 0xDA
#define ADDR_BIST_PASS_7 0xDB
#define ADDR_WIN_0 0xE0
#define ADDR_WIN_1 0xE1
#define ADDR_WIN_2 0xE2
#define ADDR_WIN_3 0xE3
#define ADDR_WIN_4 0xE4
#define ADDR_WIN_5 0xE5
#define ADDR_WIN_6 0xE6
#define ADDR_WIN_7 0xE7
#define ADDR_SPARE_0 0xE8
#define ADDR_SPARE_1 0xE9



#define WORK_STATE_FREE 0
#define WORK_STATE_HAS_JOB 1


typedef struct {
  uint8_t work_state; //
  uint8_t work_id_in_hw;
  uint8_t adapter_id;
  uint8_t leading_zeroes;
  uint32_t work_id_in_sw;

  uint32_t difficulty;
  uint32_t timestamp; // BIG ENDIAN !!
  uint32_t mrkle_root;
  uint32_t midstate[8];
  uint32_t winner_nonce; // 0 means no nonce. This means we loose 0.00000000001%
                         // results. Fuck it.
  // uint32_t work_state;
  // uint32_t nonce;
  uint8_t ntime_max;
  uint8_t ntime_offset; // for reply only
} RT_JOB;


// 24 dc2dc
typedef struct {
  uint8_t dc_temp_limit;
  uint8_t dc_temp;
  uint8_t dc2dc_present;
  uint8_t revolted;
  //int dc_current_16s_arr_ptr;
  //int dc_current_16s_arr[4]; // do median - its noisy?
  int dc_current_16s; // in 1/16 of amper. 0 = bad reading
  int dc_power_watts_16s; 
  int last_voltage_change_time;
  int vtrim;
  int loop_margin_low;
  int max_vtrim_user;
  int max_vtrim_currentwise;
  int max_vtrim_temperature;  
  int voltage_from_dc2dc;
  int before_failure_vtrim;
  
  // Guessing added current
} DC2DC;



typedef struct {
  // asic present and used
  // If loop not enabled set to false
  int faults;
  int user_disabled;
  uint8_t asic_present;
  const char* why_disabled;
  uint8_t asic_problem;  
  // address - loop*8 + offset
  uint8_t address;
  uint8_t loop_address;
  
  int last_bist_result;
  int engines_down;
  int cooling_down;
  int ot_warned_a;
  int ot_warned_b;  
  // Asic temperature/frequency (polled periodicaly)
  ASIC_TEMP asic_temp;   
  int freq_hw;            // current frequency
  //int freq_thermal_limit; // max frequency in this voltage based on termal 
  int freq_bist_limit;    // max frequency in this voltage based on BIST 
  int bist_failed_called;   
    
  // Set durring initialisation currently enabled engines
  uint32_t not_brocken_engines[ENGINE_BITMASCS]; // 
  uint32_t last_bist_passed_engines[ENGINE_BITMASCS];  
  int last_bad_win_engine;
  int wanted_pll_freq;
  int last_bad_win_engine_count; 
  int not_brocken_engines_count;
  int too_hot_concecutive_reads;
//  int bist_failed;  
  // Jobs solved by ASIC
  uint32_t solved_jobs;
  uint32_t stacked_interrupt_mask; // 0xcafebabe - means not used.

  uint32_t lazy_asic_count;
  uint32_t idle_asic_cycles_sec;
  uint32_t idle_asic_cycles_last_sec;
  uint32_t idle_asic_cycles_this_min;  
  uint32_t idle_asic_cycles_last_min;    
  ASIC_TEMP max_temp_by_asic;
  DC2DC dc2dc;
} ASIC;


typedef struct {
  int voltage;
  int board_cooling_now;    
  int ac2dc_type;  // DISABLED=4
  int ac2dc_temp;
  int ac2dc_power_last_last;
  int ac2dc_power_last;
  int ac2dc_power_now;
  int ac2dc_power_assumed;
  int ac2dc_power_measured; 

  int ac2dc_power_last_last_fake;    
  int ac2dc_power_last_fake;  
  int ac2dc_power_now_fake;  
  int ac2dc_power_fake;  
  
  int ac2dc_in_power;  
  int ac2dc_power_limit;
  int force_generic_psu;
  int voltage_start; 
  int vtrim_start;
  int total_hashrate;
} AC2DC;


typedef struct {
  uint8_t id;
  // Last time ac2dc scaling changed limit.
  int     last_ac2dc_scaling_on_loop;
  uint8_t enabled_loop;
  const char* why_disabled;
  int     asic_temp_sum; // if asics disabled or missing give them fake temp
  int     asic_hz_sum; // if asics disabled or missing give them fake temp
  int asics_failing_bist;
  int wins_this_ten_min;
  int crit_temp_downscale;
  int power_throttled;
  int user_disabled;
  int bad_loop_count;
} LOOP;


typedef struct {
  uint32_t winner_device;    
  uint32_t winner_nonce;
  uint32_t winner_engine;  
  uint32_t winner_id;
  uint32_t mrkle_root;
  uint32_t timestamp;
  uint32_t difficulty;
  uint32_t midstate[8];
} WIN;  


// Global data
#define BIST_SM_NADA          0
#define BIST_SM_DO_SCALING    1
#define BIST_SM_CHANGE_FREQ1  2
#define BIST_SM_CHANGE_FREQ2  3
#define BIST_SM_DO_BIST_AGAIN 4

#define BOARD_0     0
#define BOARD_1     1

// ASIC status per /etc/mg_disabled 0=enabled , 1=disabled , 2=removed
#define ASIC_STATUS_ENABLED 0
#define ASIC_STATUS_DISABLED 1
#define ASIC_STATUS_REMOVED 2


#ifndef SP2x
#define PSU_0     0
#define PSU_1     1
#define PSU_COUNT 2
#else 
/*
#define PSU_0     0
#define PSU_1     1
#define PSU_2     0
#define PSU_3     1
*/
#define PSU_COUNT 4
#endif

#define FET_T_72A            0
#define FET_T_72B            1
#define FET_T_72A_3PHASE     2
#define FET_T_72B_3PHASE     3
#define FET_T_72A_I50        4
#define FET_T_72B_I50        5
//#define FET_T_78A_I50		   8
#define FET_T_78B_I50		     9
//#define FET_T_78A_3PHASE	 10
#define FET_T_78B_3PHASE	   11

#define FET_ERROR_CODES_START 			0xF0
#define FET_ERROR_ILLEGAL_VALUE 		0xF1

#define FET_ERROR_VPD_WRITE_ERROR 		0xF8
#define FET_ERROR_VPD_READ_ERROR 		0xF9
#define FET_EEPROM_DEV_ERROR 			0xFA
#define FET_ERROR_BLANK_VPD 			0xFB
#define FET_ERROR_VPD_FET_NOT_SET 		0xFC
#define FET_ERROR_VPD_FET_NOT_SUPPORT	0xFD
#define FET_ERROR_UNKNOWN_ELA  			0xFE
#define FET_ERROR_BOARD_NA 				0xFF

#define SLOW_START_STATE_HALF     2
#define SLOW_START_STATE_REST     1
#define SLOW_START_STATE_WORKING  0



typedef struct {
  // Fans set to high
  int fan_level;
  int in_exit;
  uint32_t good_loops;

  int mgmt_temp_max;
  int start_mine_time;
  int start_run_time;  
  unsigned int uptime;
  // pll can be changed
  int tryed_power_cycle_to_revive_loops;

  int last_chiko_time;
  int disasics;
  int err_restarted;  
  int err_unexpected_reset;  
  int err_unexpected_reset2;  
  int err_too_much_fake_wins;  
  int err_stuck_bist;  
  int err_low_power_chiko;  
  int err_stuck_pll;  
  int err_runtime_disable;  
  int err_purge_queue;  
  int err_read_timeouts;    
  int err_dc2dc_i2c_error;    
  int err_bad_idle;
  int err_murata;  
  int dc2dc_temp_ignore;  

  int err_read_timeouts2;  
  int err_read_corruption;  
  int err_purge_queue3;  


  // How many WINs we got
  uint32_t solved_jobs_total;
  uint32_t last_minute_rate_mb;


  
  // Pause all miner work to save electricity
  uint8_t asics_shut_down_powersave;
  uint32_t not_mining_time; // in seconds how long we are not mining
  uint32_t mining_time; 
  int32_t next_bist_countdown; 

  
  int temp_mgmt;
  int temp_top;
  int temp_bottom;  

  // Stoped all work
  uint8_t stopped_all_work;

  // jobs right one after another
  int consecutive_jobs;

/*
  int voltage_start_top;
  int voltage_start_bottom; 
  int vtrim_start_top;
  int vtrim_start_bottom;  
  */
  int voltage_max;
  int minimal_rate_gh;
  int flag_0;
  int flag_1;  
 
  // ac2dc current and temperature
  uint32_t max_dc2dc_current_16s;  
  int dc2dc_total_power; 
  int total_rate_mh; 
  int concecutive_hw_errs;
  int userset_fan_level;
  int vtrim_max;
  int last_second_jobs;
  int this_second_jobs;  
  int last_minute_wins;
  int this_minute_wins;  
  int hw_errs;
  int fpga_ver;
  int try_12v_fix;
  int cur_leading_zeroes;
  // We give less LZ then needed to do faster scaling.
  // When system just started, search optimal speed agressively
  int needs_bist;  
  int engine_size;
  // our ASIC data
  ASIC asic[ASICS_COUNT];
  //uint32_t not_brocken_engines[ASICS_COUNT];
  int in_asic_reset;
  int slowstart;
  int corruptions_count;
  int spi_timeout_count;

  
  // our loop and dc2dc data
  LOOP loop[LOOP_COUNT];
  int fet[BOARD_COUNT];
  int board_not_present[BOARD_COUNT];  
  int wins_last_minute[BOARD_COUNT];
  int board_working_asics[BOARD_COUNT];   
  AC2DC ac2dc[PSU_COUNT]; 
  int exiting;
  int did_asic_reset;
  int board_cooling_ever;   
  int asic_count;

  int this_min_failed_bist;
  WIN last_win;
  int run_time_failed_asics;
} MINER_BOX;

extern MINER_BOX vm;

#endif
