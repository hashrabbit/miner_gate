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

#ifndef _____SPILIB_H____
#define _____SPILIB_H____

#include <stdint.h>
#include <unistd.h>

#define ADDR_SQUID_REVISION 0x0
#define ADDR_SQUID_STATUS 0x1
#define ADDR_SQUID_ISR 0x2
#define ADDR_SQUID_IMR 0x3
#define ADDR_SQUID_ICR 0x4
#define ADDR_SQUID_ITR 0x5
#define ADDR_SQUID_PONG 0xf          // 0xDEADBEEF
#define ADDR_SQUID_LOOP_RESET 0x10   // Each bit = a loop
#define ADDR_SQUID_LOOP_BYPASS 0x11  // Each bit = a loop
#define ADDR_SQUID_LOOP_TIMEOUT 0x12 // Each bit = a loop
#define ADDR_SQUID_SCRATCHPAD 0x1f
#define ADDR_SQUID_COMMAND 0x20
#define ADDR_SQUID_QUEUE_STATUS 0x21
#define ADDR_SQUID_SERVICE_STATUS 0x22
#define ADDR_SQUID_SR_CONTROL_BASE 0x30
#define ADDR_SQUID_SR_MEMORY_BASE 0x40
#define ADDR_SQUID_SERIAL_WRITE 0x80
#define ADDR_SQUID_SERIAL_READ 0x81
#define ADDR_SQUID_SERVICE_READ 0x82
#define ADDR_SQUID_MQ_WRITE 0x83
#define ADDR_SQUID_MQ_CONFIG 0x23
#define ADDR_SQUID_MQ_SENT 0x24


// ADDR_SQUID_STATUS, ADDR_SQUID_ISR, ADDR_SQUID_IMR, ADDR_SQUID_ICR,
// ADDR_SQUID_ITR
#define BIT_STATUS_SERIAL_Q_TX_FULL 0x00000001
#define BIT_STATUS_SERIAL_Q_TX_NOT_EMPTY 0x00000002
#define BIT_STATUS_SERIAL_Q_TX_EMPTY 0x00000004
#define BIT_STATUS_SERIAL_Q_RX_FULL 0x00000008
#define BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY 0x00000010
#define BIT_STATUS_SERIAL_Q_RX_EMPTY 0x00000020
#define BIT_STATUS_SERVICE_Q_FULL 0x00000040
#define BIT_STATUS_SERVICE_Q_NOT_EMPTY 0x00000080
#define BIT_STATUS_SERVICE_Q_EMPTY 0x00000100
#define BIT_STATUS_FIFO_SERIAL_TX_ERR 0x00000200
#define BIT_STATUS_FIFO_SERIAL_RX_ERR 0x00000400
#define BIT_STATUS_FIFO_SERVICE_ERR 0x00000800
#define BIT_STATUS_CHAIN_EMPTY 0x00001000
#define BIT_STATUS_LOOP_TIMEOUT_ERROR 0x00002000
#define BIT_STATUS_LOOP_CORRUPTION_ERROR 0x00004000
#define BIT_STATUS_ILLEGAL_ACCESS 0x00008000
#define BIT_STATUS_MQ_FULL 0x00010000
#define BIT_STATUS_MQ_NOT_EMPTY 0x00020000
#define BIT_STATUS_MQ_EMPTY 0x00040000
#define BIT_STATUS_MQ_ERROR 0x00080000

// ADDR_SQUID_COMMAND
#define BIT_COMMAND_SERIAL_Q_TX_FIFO_RESET 0x01
#define BIT_COMMAND_SERIAL_Q_RX_FIFO_RESET 0x02
#define BIT_COMMAND_SERVICE_Q_RX_FIFO_RESET 0x04
#define BIT_COMMAND_LOOP_LOGIC_RESET 0x08
#define BIT_COMMAND_SERVICE_ROUTINE_ENABLE 0x10
#define BIT_COMMAND_MQ_FIFO_RESET 0x20


// ADDR_SQUID_QUEUE_STATUS - offsets!
#define BIT_QUEUE_STATUS_SERIAL_TX_FIFO_USED_OFFSET 0
#define BIT_QUEUE_STATUS_SERIAL_TX_FIFO_USED_LENGTH 8
#define BIT_QUEUE_STATUS_SERIAL_RX_FIFO_USED_OFFSET 8
#define BIT_QUEUE_STATUS_SERIAL_RX_FIFO_USED_LENGTH 9
#define BIT_QUEUE_STATUS_MQ_FIFO_USED_OFFSET 17
#define BIT_QUEUE_STATUS_MQ_FIFO_USED_LENGTH 9
#define BIT_QUEUE_STATUS_MQ_BUFFER_USED_OFFSET 26
#define BIT_QUEUE_STATUS_MQ_BUFFER_USED_LENGTH 4

// ADDR_SQUID_SR_CONTROL
#define BIT_SERVICE_ROUTINE_COUNTER_OFFSET 0
#define BIT_SERVICE_ROUTINE_ENABLE 0x80000000

#define GENERAL_BITS_COMPLETION 0x20
#define GENERAL_BITS_CONDITION 0x10

#if 1
#define PUSH_JOB(A,B) push_mq_write(ANY_ASIC, ANY_ENGINE,A,B)
#define FLUSH_JOB() flush_mq_write()
#else
#define PUSH_JOB(A,B) push_asic_write(ANY_ASIC, ANY_ENGINE, A,B)
#define FLUSH_JOB() flush_spi_write()
#endif


void write_spi(uint8_t addr, uint32_t data);
uint32_t read_spi(uint8_t addr);
void write_spi_mult(uint8_t addr, int count, int values[]);
void read_spi_mult(uint8_t addr, int count, int values[]);
void init_spi();
unsigned long usec_stamp();

int test_all_loops_and_dc2dc(int disabled_failed, int verbose);
void parse_squid_status(int v);
void parse_squid_q_status(int v);
// Flush write command to SPI
void flush_spi_write();
void push_mq_write(uint8_t asic_addr, uint8_t engine_addr, uint32_t offset, uint32_t value);
void flush_mq_write();
int mq_size();
int squid_wait_asic_reads_restart_if_error();

void write_reg_asic(uint8_t asic_addr, uint8_t engine_addr, uint8_t offset, uint32_t value);
uint32_t read_reg_asic(uint8_t asic_addr, uint8_t engine_addr, uint8_t offset);
void push_asic_read(uint8_t asic_addr, uint8_t engine_addr, uint32_t offset, uint32_t *p_value, int defval=0);
void push_asic_write(uint8_t asic_addr, uint8_t engine_addr , uint32_t offset, uint32_t value);


#define BROADCAST_READ_ADDR(X) ((X >> 24) & 0xFF)
#define BROADCAST_READ_ENGINE(X) ((X >> 16) & 0xFF)
#define BROADCAST_READ_DATA(X) (X & 0xFFFF)
#define ASIC_GET_LOOP_OFFSET(ADDR, LOOP, OFFSET)                               \
  {                                                                            \
    LOOP = (ADDR / ASICS_PER_LOOP);                                          \
    OFFSET = ADDR % ASICS_PER_LOOP;                                          \
  }
#define ASIC_GET_BY_ADDR(ADDR) (&vm.asic[ADDR])
extern int enable_reg_debug;

#define MQ_CONFIG_TIMER_OFFSET 0
#define MQ_CONFIG_INCREMENT_OFFSET 24
#define ADDR_SQUID_MQ_CONFIG_PAUSE_Q_OFFSET  31


#define MQ_CONFIG_TIMER_UNITS_IN_USEC_10MB  10
#define MQ_CONFIG_TIMER_UNITS_IN_USEC_5MB   5
#define MQ_CONFIG_TIMER_UNITS_IN_USEC_2_5MB 1




// FOR 10MB SERIAL INTERFACE TODO

#define MAKE_MQ_CONFIG(t,fpga_rate,i,pause) (((t*fpga_rate) << MQ_CONFIG_TIMER_OFFSET) | \
			     ((i-1) << MQ_CONFIG_INCREMENT_OFFSET) | (pause << ADDR_SQUID_MQ_CONFIG_PAUSE_Q_OFFSET))
#endif
