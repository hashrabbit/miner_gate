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

#include "squid.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>
#include <string.h>
#include "asic.h"
#include "spond_debug.h"
#include <time.h>
#include <syslog.h>
#include <spond_debug.h>
#include "asic_lib.h"
 #include "sched.h"
#include "pwm_manager.h"
#include "miner_gate.h"
#ifdef MINERGATE
#include "asic.h"
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
//#define BROADCAST_ADDR 0xffff

static const char *device = "/dev/spidev1.0";
static uint8_t mode = 0;
static uint8_t bits = 8;
static uint32_t speed = 10000000;
static int fd = 0;
int assert_serial_failures = true;
int spi_ioctls_read = 0;
int spi_ioctls_write = 0;
int enable_reg_debug = DBG_REGS;

#define SPI_CMD_READ 0x01
#define SPI_CMD_WRITE 0x41

typedef struct {
  uint8_t addr;
  uint8_t cmd;
  uint32_t i[64];
} __attribute__((__packed__)) cpi_cmd;

int purge_fpga_queue(const char* why);

unsigned long usec_stamp ()
{
    static unsigned long long int first_usec = 0;
    struct timeval tv;
    unsigned long long int curr_usec;
    gettimeofday(&tv, NULL);
    curr_usec = tv.tv_sec * 1000000 + tv.tv_usec;
    if (first_usec == 0) {
	first_usec = curr_usec;
	curr_usec = 0;
    } else
	curr_usec -= first_usec;
    return curr_usec;
}
#ifdef MINERGATE
void reset_asic_queue();

void test_all_loops() {
  mg_event_x("Testing LOOPs");
  reset_asic_queue();
  for (int l = 0 ; l < LOOP_COUNT; l++) {
    if (!vm.loop[l].enabled_loop) {
      continue;
    }
    
    unsigned int bypass_loops = ((~(1 << l)) & SQUID_LOOPS_MASK);
    //psyslog("Bad loop %x ", bypass_loops);
    write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);
    if (test_serial(l) != 1) {
      mg_event_x("Loop serial failed %d", l);
    }
  }
  write_spi(ADDR_SQUID_LOOP_BYPASS, (~(vm.good_loops))&SQUID_LOOPS_MASK);
  mg_event_x("Testing LOOPs done");
}
#endif
void parse_squid_status(int v) {
  if (v & BIT_STATUS_SERIAL_Q_TX_FULL)
    printf("BIT_STATUS_SERIAL_Q_TX_FULL       ");
  if (v & BIT_STATUS_SERIAL_Q_TX_NOT_EMPTY)
    printf("BIT_STATUS_SERIAL_Q_TX_NOT_EMPTY  ");
  if (v & BIT_STATUS_SERIAL_Q_TX_EMPTY)
    printf("BIT_STATUS_SERIAL_Q_TX_EMPTY      ");
  if (v & BIT_STATUS_SERIAL_Q_RX_FULL)
    printf("BIT_STATUS_SERIAL_Q_RX_FULL       ");
  if (v & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY)
    printf("BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY  ");
  if (v & BIT_STATUS_SERIAL_Q_RX_EMPTY)
    printf("BIT_STATUS_SERIAL_Q_RX_EMPTY      ");
  if (v & BIT_STATUS_SERVICE_Q_FULL)
    printf("BIT_STATUS_SERVICE_Q_FULL         ");
  if (v & BIT_STATUS_SERVICE_Q_NOT_EMPTY)
    printf("BIT_STATUS_SERVICE_Q_NOT_EMPTY    ");
  if (v & BIT_STATUS_SERVICE_Q_EMPTY)
    printf("BIT_STATUS_SERVICE_Q_EMPTY        ");
  if (v & BIT_STATUS_FIFO_SERIAL_TX_ERR)
    printf("BIT_STATUS_FIFO_SERIAL_TX_ERR     ");
  if (v & BIT_STATUS_FIFO_SERIAL_RX_ERR)
    printf("BIT_STATUS_FIFO_SERIAL_RX_ERR     ");
  if (v & BIT_STATUS_FIFO_SERVICE_ERR)
    printf("BIT_STATUS_FIFO_SERVICE_ERR       ");
  if (v & BIT_STATUS_CHAIN_EMPTY)
    printf("BIT_STATUS_CHAIN_EMPTY            ");
  if (v & BIT_STATUS_LOOP_TIMEOUT_ERROR)
    printf("BIT_STATUS_LOOP_TIMEOUT_ERROR     ");
  if (v & BIT_STATUS_LOOP_CORRUPTION_ERROR)
    printf("BIT_STATUS_LOOP_CORRUPTION_ERROR  ");
  if (v & BIT_STATUS_ILLEGAL_ACCESS)
    printf("BIT_STATUS_ILLEGAL_ACCESS         ");
  if (v & BIT_STATUS_MQ_FULL)
    printf("BIT_STATUS_MQ_FULL         ");
  if (v & BIT_STATUS_MQ_NOT_EMPTY)
    printf("BIT_STATUS_MQ_NOT_EMPTY         ");
  if (v & BIT_STATUS_MQ_EMPTY)
    printf("BIT_STATUS_MQ_EMPTY         ");
  if (v & BIT_STATUS_MQ_ERROR)
    printf("BIT_STATUS_MQ_ERROR         ");
  printf("\n");
}

static uint32_t status_bit_selector(uint32_t val, uint8_t offset, uint8_t length)
{
  uint32_t tmp = val >> offset;
  uint32_t mask = (1 << length) - 1;
  return tmp & mask;
}

void parse_squid_q_status(int v)
{
  printf("QUEUE STATUS:\tTX_FIFO: %d\tRX_FIFO: %d\tMQ_FIFO: %d\tMQ_BUFFER: %d\n",
	 status_bit_selector(v, BIT_QUEUE_STATUS_SERIAL_TX_FIFO_USED_OFFSET,
			     BIT_QUEUE_STATUS_SERIAL_TX_FIFO_USED_LENGTH),
	 status_bit_selector(v, BIT_QUEUE_STATUS_SERIAL_RX_FIFO_USED_OFFSET,
			     BIT_QUEUE_STATUS_SERIAL_RX_FIFO_USED_LENGTH),
	 status_bit_selector(v, BIT_QUEUE_STATUS_MQ_FIFO_USED_OFFSET,
			     BIT_QUEUE_STATUS_MQ_FIFO_USED_LENGTH),
	 status_bit_selector(v, BIT_QUEUE_STATUS_MQ_BUFFER_USED_OFFSET,
			     BIT_QUEUE_STATUS_MQ_BUFFER_USED_LENGTH));
}

void read_spi_mult(uint8_t addr, int count, uint32_t values[]) {
  int ret;
  cpi_cmd tx;
  cpi_cmd rx;

  tx.addr = addr;
  tx.cmd = count;
  passert(count < 16);
  memset(tx.i, 0, 16);
  spi_ioctls_read++;
  struct spi_ioc_transfer tr;
  tr.tx_buf = (unsigned long)&tx;
  tr.rx_buf = (unsigned long)&rx;
  tr.len = 2 + 4 * count;
  tr.delay_usecs = 0;
  tr.speed_hz = speed;
  tr.bits_per_word = bits;

  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  int i;
  for (i = 0; i < count; i++) {
    values[i] = htonl(rx.i[i]);
  }
  if (ret < 1)
    pabort("can't send spi message");

  if (enable_reg_debug && 
          (addr != ADDR_SQUID_STATUS) &&    
       (addr != ADDR_SQUID_SERIAL_WRITE) &&
       (addr != ADDR_SQUID_SERIAL_READ) &&
       (addr != ADDR_SQUID_SERVICE_READ) &&
       (addr != ADDR_SQUID_MQ_WRITE) &&
       (addr != ADDR_SQUID_QUEUE_STATUS)
       ) {
       for (int j = 0 ; j < count ; j++) {
         printf("SPI[%x]<=%x\n", addr, values[j]);
       }
  }

  // printf("%02X %02X %08X\n",rx.addr, rx.cmd, rx.i);
}

uint32_t read_spi(uint8_t addr) {
  int ret;
  cpi_cmd tx;
  cpi_cmd rx;

  tx.addr = addr;
  tx.cmd = SPI_CMD_READ;
  tx.i[0] = 0;
  spi_ioctls_read++;

  struct spi_ioc_transfer tr;
  tr.tx_buf = (unsigned long)&tx;
  tr.rx_buf = (unsigned long)&rx;
  tr.len = 2 + 4 * 1;
  tr.delay_usecs = 0;
  tr.speed_hz = speed;
  tr.bits_per_word = bits;

  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  rx.i[0] = htonl(rx.i[0]);
  if (ret < 1)
    pabort("can't send spi message");

  if (enable_reg_debug && 
          (addr != ADDR_SQUID_STATUS) &&    
       (addr != ADDR_SQUID_SERIAL_WRITE) &&
       (addr != ADDR_SQUID_SERIAL_READ) &&
       (addr != ADDR_SQUID_SERVICE_READ) &&
       (addr != ADDR_SQUID_MQ_WRITE) &&
       (addr != ADDR_SQUID_QUEUE_STATUS)
       ) {
     printf("SPI[%x]<=%x\n", addr, rx.i[0]);
   }

  //printf("SPI[%x]=>%x\n", addr,rx.i[0]);
  return rx.i[0];
}

void write_spi_mult(uint8_t addr, int count, int values[]) {
  int ret;
  cpi_cmd tx;
  cpi_cmd rx;
  passert(count < 64);
  spi_ioctls_write++;

  tx.addr = addr;
  tx.cmd = 0x40 | count;
  int i;
  for (i = 0; i < count; i++) {
    tx.i[i] = htonl(values[i]);
  }

  struct spi_ioc_transfer tr;
  tr.tx_buf = (unsigned long)&tx;
  tr.rx_buf = (unsigned long)&rx;
  tr.len = 2 + 4 * count;
  tr.delay_usecs = 0;
  tr.speed_hz = speed;
  tr.bits_per_word = bits;

  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

  if (ret < 1) {
    pabort("can't send spi message");
    passert(0);
  }

  if (enable_reg_debug && 
      (addr != ADDR_SQUID_STATUS) &&    
      (addr != ADDR_SQUID_SERIAL_WRITE) &&
      (addr != ADDR_SQUID_SERIAL_READ) &&
      (addr != ADDR_SQUID_SERVICE_READ) &&
      (addr != ADDR_SQUID_MQ_WRITE) &&
      (addr != ADDR_SQUID_QUEUE_STATUS)
      ) {
    for (int j = 0 ; j < count ; j++) {        
  	  printf("SPI[%x]<=%x\n", addr, values[j]);
    }
  }
}

void write_spi(uint8_t addr, uint32_t data) {
  int ret;
  cpi_cmd tx;
  cpi_cmd rx;

  tx.addr = addr;
  tx.cmd = SPI_CMD_WRITE;
  tx.i[0] = htonl(data);
  spi_ioctls_write++;

  struct spi_ioc_transfer tr;
  tr.tx_buf = (unsigned long)&tx;
  tr.rx_buf = (unsigned long)&rx;
  tr.len = 2 + 4 * 1;
  tr.delay_usecs = 0;
  tr.speed_hz = speed;
  tr.bits_per_word = bits;

  if (enable_reg_debug && 
          (addr != ADDR_SQUID_STATUS) &&    
         (addr != ADDR_SQUID_SERIAL_WRITE) &&
         (addr != ADDR_SQUID_SERIAL_READ) &&
         (addr != ADDR_SQUID_SERVICE_READ) &&
         (addr != ADDR_SQUID_MQ_WRITE) &&
         (addr != ADDR_SQUID_QUEUE_STATUS)
         ) {
       printf("SPI[%x]<=%x\n", addr, data);
     }


  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

  if (ret < 1) {
    pabort("can't send spi message");
    passert(0);
  }

  //printf("SPI[%x]<=%x\n", addr, data);
}

void init_spi() {
  int ret = 0;
  fd = open(device, O_RDWR);
  if (fd < 0)
    pabort("can't open spi device. Did you init eeprom?\n");

  /*
  * spi mode
  */
  ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
  if (ret == -1)
    pabort("can't set spi mode");

  ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
  if (ret == -1)
    pabort("can't get spi mode");

  /*
  * bits per word
  */
  ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1)
    pabort("can't set bits per word");

  ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
  if (ret == -1)
    pabort("can't get bits per word");

  /*
  * max speed hz
  */
  ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (ret == -1)
    pabort("can't set max speed hz");

  ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
  if (ret == -1)
    pabort("can't get max speed hz");

  /*
  //printf("SPI mode: %d\n", mode);
  printf("bits per word: %d\n", bits);
  printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
  */
}

// Create READ and WRITE queues.
#define MAX_FPGA_CMD_QUEUE 64
typedef struct {
  uint32_t addr;     // SET by SW
  uint32_t offset;   // SET by SW
  uint32_t value;    // SET by SW or FPGA
  uint32_t *p_value; // SET by SW or FPGA
  uint8_t b_read;
  uint32_t data[2];
} QUEUED_REG_ELEMENT;

QUEUED_REG_ELEMENT cmd_queue[MAX_FPGA_CMD_QUEUE];
int current_cmd_queue_ptr = 0;
int current_cmd_hw_queue_ptr = 0;

void create_serial_pkt(uint32_t *d1, uint32_t *d2, uint8_t reg_offset,
                       uint8_t wr_rd, uint16_t dst_chip_addr, uint32_t reg_val,
                       uint8_t general_bits) {
  *d1 = reg_offset << 24;
  *d1 |= wr_rd << 23;
  *d1 |= dst_chip_addr << 7;
  *d1 |= reg_val >> 26;
  *d2 = reg_val << 6;
  *d2 |= general_bits;
}

int asic_serial_stack[64] = { 0 };
int asic_serial_stack_size = 0;
int asic_mq_stack[64] = { 0 };
int asic_mq_stack_size = 0;

void reset_asic_queue() {
  //memset(cmd_queue, 0, current_cmd_queue_ptr * sizeof(QUEUED_REG_ELEMENT));
  current_cmd_queue_ptr = 0;
  current_cmd_hw_queue_ptr = 0;
}


void push_asic_serial_packet_to_hw(uint32_t d1, uint32_t d2) {
  if(asic_serial_stack_size >= 64) {
	psyslog("asic_serial_stack_size = %d\n",asic_serial_stack_size);
	passert(0);
  }
  //printf("asic_serial_stack_size=%d\n",asic_serial_stack_size);
  asic_serial_stack[asic_serial_stack_size++] = d1;
  asic_serial_stack[asic_serial_stack_size++] = d2;
  if (asic_serial_stack_size == 60) {
    flush_spi_write();
  }
}

void flush_spi_write() {
  if (asic_serial_stack_size) {
    write_spi_mult(ADDR_SQUID_SERIAL_WRITE, asic_serial_stack_size,
                   asic_serial_stack);
    asic_serial_stack_size = 0;
  }
}

void push_mq_write(uint8_t asic_addr, uint8_t engine_addr, uint32_t offset, uint32_t value) {
  uint32_t l_d1;
  uint32_t l_d2;
  if(asic_mq_stack_size >= 64) {
  	psyslog("asic_mq_stack_size = %d\n",asic_mq_stack_size);
  	passert(0);
  }
  //printf("asic_serial_stack_size=%d\n",asic_serial_stack_size);
  if (enable_reg_debug) {
      printf("%d - MQ: reg mq %04x %02x %x\n", usec_stamp(), (((int)asic_addr)<<8)|engine_addr, offset, value);
  }

  create_serial_pkt(&l_d1, &l_d2, offset, 0, (((int)asic_addr)<<8)|engine_addr, value, 0);

  asic_mq_stack[asic_mq_stack_size++] = l_d1;
  asic_mq_stack[asic_mq_stack_size++] = l_d2;
  if (asic_mq_stack_size >= 60) {
    flush_mq_write();
  }
}


int mq_size() {
  int status = read_spi(ADDR_SQUID_QUEUE_STATUS);
  //printf("status %x\n",status);    
  status = (status >> 17) & 0xFF;
  return status;
}


void flush_mq_write() {
  if (asic_mq_stack_size) {
    write_spi_mult(ADDR_SQUID_MQ_WRITE, asic_mq_stack_size, asic_mq_stack);
    asic_mq_stack_size = 0;
  }
}


int purge_fpga_queue(const char* why) {
#ifdef MINERGATE
#ifdef CHECK_GARBAGE_READ
    if (assert_serial_failures) {
       int i = 0;
       int sq_int;
       while (i<200 && ((sq_int = read_spi(ADDR_SQUID_STATUS)) & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY)) {  
         //set_light_on_off(LIGHT_YELLOW, true);
         int x = read_spi(ADDR_SQUID_SERIAL_READ);         
         if ((i<10) || (i>195)) {
#ifdef MINERGATE            
            vm.err_purge_queue3++;
#endif
            psyslog("%s read garbage %x (%d)/int %x\n", why, x, i, sq_int);
            mg_event_x("%s read garbage %x (%d)/int %x", why, x, i, sq_int);
         }
         //set_light_on_off(LIGHT_YELLOW, false);
         i++;
       }
       if(i>195) {
#ifdef MINERGATE        
          reset_asic_queue();
          print_chiko(0);
          vm.err_purge_queue++;  
          restart_asics_full(10,"purge_fpga_queue");
          return -1;
#endif
       }
    }
#endif
#endif
  return 0;
}

void push_asic_read(uint8_t asic_addr, uint8_t engine_addr ,  uint32_t offset, uint32_t *p_value) {

  if (current_cmd_queue_ptr >= MAX_FPGA_CMD_QUEUE - 8) {
    squid_wait_asic_reads();
  }

  if (current_cmd_queue_ptr == 0) {
    if (purge_fpga_queue("Pre") != 0) {
      return;
    }
  }
  QUEUED_REG_ELEMENT *e = &cmd_queue[current_cmd_queue_ptr++];
  //printf("%x %x %x %x\n",current_cmd_queue_ptr,e->addr,e->offset,e->value);
  passert(current_cmd_queue_ptr < MAX_FPGA_CMD_QUEUE);
  //passert(e->addr == 0);
  //passert(e->offset == 0);
  //passert(e->value == 0);
  e->addr = (((int)asic_addr) << 8) | engine_addr;
  e->offset = offset;
  e->p_value = p_value;
  e->b_read = 1;

  uint32_t d1;
  uint32_t d2;
  create_serial_pkt(&d1, &d2, offset, 1, (((int)asic_addr) << 8) | engine_addr, 0, GENERAL_BITS_COMPLETION);
  e->data[0] = d1;
  e->data[1] = d2;

  if (0 && enable_reg_debug) {
      printf("%d - prepare read %04x %02x (%x %x)\n", usec_stamp(), e->addr, offset,d1, d2);
  }

  
  // printf("---> %x %x\n",d1,d2);
  push_asic_serial_packet_to_hw(d1, d2);
}


void push_asic_write(uint8_t asic_addr, uint8_t engine_addr, uint32_t offset, uint32_t value) {
  /*
  QUEUED_REG_ELEMENT *e = &cmd_queue[current_cmd_queue_ptr++];
  passert(current_cmd_queue_ptr <= MAX_FPGA_CMD_QUEUE);
  passert(e->addr == 0);
  passert(e->value == 0);
  passert(e->offset == 0);
  e->addr = addr;
  e->value = value;
  e->offset = offset;
  e->b_read = 0;
 */
  uint32_t d1;
  uint32_t d2;
  create_serial_pkt(&d1, &d2, offset, 0, (((int)asic_addr)<<8)|engine_addr, value, 0);
  // printf("---> %x %x\n",d1,d2);
  push_asic_serial_packet_to_hw(d1, d2);
  if (enable_reg_debug) {
      printf("%d - reg h %04x %02x %x\n", usec_stamp(), (((int)asic_addr)<<8)|engine_addr, offset, value);
  }
}
int wait_rx_queue_ready() {
  int loops = 0;
  uint32_t q_status = read_spi(ADDR_SQUID_STATUS);
  // printf("Q STATUS:%x \n", q_status);
  while ((++loops < 1000) &&
         ((q_status & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) == 0)) {
    //usleep(1);
	  sched_yield();
    q_status = read_spi(ADDR_SQUID_STATUS);
  }
  return ((q_status & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) != 0);
}

static int corruptions_count = 0;
static int spi_timeout_count = 0;
int read_ac2dc_errors(int to_event);

int revive_asics_if_one_got_reset(const char *why);
uint32_t _read_reg_actual(QUEUED_REG_ELEMENT *e, int *err) {
  // uint32_t d1, d2;
  // printf("-!-read SERIAL: %x %x\n",d1,d2);
  int success = wait_rx_queue_ready();
  if (!success) {
    // TODO  - handle timeout?
    if (assert_serial_failures) {
      
#ifdef MINERGATE      
      //set_light_on_off(LIGHT_YELLOW, true);
      vm.err_read_timeouts2++;
#endif      
      psyslog("READ TIMEOUT 0x%x 0x%x\n", e->addr, e->offset);
      mg_event_x("Data Timeout on read %x:%x (%d)", e->addr, e->offset,spi_timeout_count);
      // Test loops?
      spi_timeout_count++;
#ifdef MINERGATE
      if (read_ac2dc_errors(1)) {
        //usleep(1000000);
        restart_asics_full(434, "read timeout AC2DC fail");
      }
      // wait milli
      //*err = 1;
      //set_light_on_off(LIGHT_YELLOW, false);
      test_all_loops();
      return 0;
#endif
    } else {
      return 0;
    }
  } 
  spi_timeout_count = 0;

  uint32_t values[2];
  read_spi_mult(ADDR_SQUID_SERIAL_READ, 2, values);
  // printf("read SERIAL rsp: %x %x\n",d1,d2);
  uint32_t got_addr = ((values[0] >> 7) & 0xFFFF);
  uint32_t got_offset = ((values[0] >> 24) & 0xFF);
  if ((e->addr != got_addr) || (e->offset != got_offset)) {
    // Data returned corrupted :(
    if (assert_serial_failures) {
      corruptions_count++;
#ifdef MINERGATE        
      //set_light_on_off(LIGHT_YELLOW, true);
#endif
      psyslog("Data corruption A: READ:0x%x 0x%x (0x%x 0x%x)/ GOT:0x%x 0x%x (0x%x 0x%x) \n", 
             e->addr,
             e->offset, 
             e->data[0],e->data[1],
             got_addr, got_offset,
             values[0], values[1]);

      // pull all messages in queue, later handle timeout and shit...
      /*
      while ((read_spi(ADDR_SQUID_STATUS)) & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) {
          read_spi(ADDR_SQUID_SERIAL_READ);
      }
      reset_asic_queue();*/
#ifdef MINERGATE      
      //set_light_on_off(LIGHT_YELLOW, false);
      vm.err_read_corruption++;

      mg_event_x("Data corruption %d on read %x:%x (%x:%x) got %x:%x (%x:%x) - ql=%d", 
              corruptions_count, e->addr, e->offset, e->data[0],e->data[1],
              got_addr, got_offset,
              values[0], values[1], current_cmd_queue_ptr);
      //set_light_on_off(LIGHT_YELLOW, true);
      if (corruptions_count > 100) {
        passert(0);
      }
      *err = 1;
      return 0;
#endif
      //passert(0);
    } else {
        psyslog("Data corruption B: READ:0x%x 0x%x / GOT:0x%x 0x%x (0x%x 0x%x) \n", 
             e->addr,
             e->offset, 
             got_addr, got_offset,
             values[0], values[1]);
        return 0;
    }
  }
 
  uint32_t value = ((values[0] & 0x3F) << (32 - 6)) | ((values[1] >> 6) & 0x03FFFFFF);

  if (enable_reg_debug) {
      printf("%d - reg h %04x %02x  #=%x\n", usec_stamp(), e->addr, e->offset, value);
  }

  return value;
}

#define FPGA_QUEUE_FREE 0
#define FPGA_QUEUE_WORKING 1
#define FPGA_QUEUE_FINISHED 2
int fpga_queue_status() {
  if (current_cmd_queue_ptr == 0) {
    passert(current_cmd_hw_queue_ptr == 0);
    return FPGA_QUEUE_FREE;
  } else if (current_cmd_hw_queue_ptr == current_cmd_queue_ptr) {
    return FPGA_QUEUE_FINISHED;
  } else {
    return FPGA_QUEUE_WORKING;
  }
}


int squid_wait_asic_reads() {
  int err = 0;
  //struct timeval tv;
  flush_spi_write();
  // printf("HERE!\n");
  corruptions_count = 0;
  while (fpga_queue_status() == FPGA_QUEUE_WORKING) {
    QUEUED_REG_ELEMENT *e = &cmd_queue[current_cmd_hw_queue_ptr++];
    if (e->b_read) {
      //start_stopper(&tv); 
      e->value = _read_reg_actual(e, &err);
      //end_stopper(&tv,"X");    
      *(e->p_value) = e->value;
    } else {
      passert(0);
    }
#ifdef MINERGATE  
    if (spi_timeout_count >= 15) {
          spi_timeout_count = 0;
          reset_asic_queue();
          print_chiko(0);
          vm.err_read_timeouts++;  
          restart_asics_full(11,"Too much timeouts");
          return 0;
    }
#endif
    if (current_cmd_hw_queue_ptr == current_cmd_queue_ptr) {
      reset_asic_queue();
      //purge_fpga_queue("post");      
      break;
    }
  }

#ifdef MINERGATE  
  

  if (err) { 
    reset_asic_queue();
    revive_asics_if_one_got_reset("squid_wait_asic_reads");
    return 0;
  }

  
#endif

  return 1;
}


void write_reg_asic(uint8_t addr, uint8_t engine, uint8_t offset, uint32_t value) {
  push_asic_write(addr, engine, offset, value);
}


/*
void write_reg_asic(ANY_ASIC, NO_ENGINE,uint8_t offset, uint32_t value) {
  write_reg_asic(ANY_ASIC, NO_ENGINE, offset, value);
  // squid_wait_asic_reads()
}
*/

uint32_t read_reg_asic(uint8_t addr, uint8_t engine, uint8_t offset) {
  uint32_t value;
  push_asic_read(addr, engine, offset, &value);
  squid_wait_asic_reads();
  return value;
}
