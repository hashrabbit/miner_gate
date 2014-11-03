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
#include "spond_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

void push_asic_read(uint8_t asic_addr, uint8_t engine_addr ,  uint32_t offset, uint32_t* p_value);
int squid_wait_asic_reads();
void push_asic_write(uint8_t asic_addr, uint8_t engine_addr ,  uint32_t offset, uint32_t value);
int squid_wait_asic_reads();



int main(int argc, char *argv[]) {

  init_spi();
  
  if (argc < 3 ) {
  printf("Usage for squid read/write  : %s s <h`addr> [h`value]\n" , argv[0]);
  printf(" for asic read/write : %s h <h`addr> <h`offset> [h`value]\n" , argv[0]);
  printf(" for asic MQ broadcast write : %s mq <h`offset> [h`value]\n" , argv[0]);
  
  return 0;
  }

 // printf("Enabling loop\n");
 // write_spi(0x10, 0xffffff);
 // write_spi(0x11, 0xfffffe);


  if (argv[1][0] == 's') {
    if (argc < 4) {
    // READ
    unsigned int addr = strtoul(argv[2], NULL, 16);
    unsigned int value = read_spi(addr);
    printf("[%x] => %x\n",addr,value);
      } else {
    unsigned int addr  = strtoul(argv[2], NULL, 16);
    unsigned int value = strtoul(argv[3], NULL, 16);
    write_spi(addr, value);
    printf("[%x] <= %x\n",addr,value);
    }
  } else if (argv[1][0] == 'h') {
    int d1;

      uint32_t q_status;
    //  q_status = read_spi(ADDR_SQUID_PONG);
    // passert((q_status == 0xDEADBEEF), "ERROR: no 0xdeadbeef in squid pong register!\n");


      int i = 0;
    while (read_spi(ADDR_SQUID_STATUS) & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) {
       psyslog("read garbage %x\n", read_spi(ADDR_SQUID_SERIAL_READ));
             i++;
             if (i == 1000) {
                psyslog("GARBAGE OVERFLOW!\n");
                return(0);
             }
    }
      

  
    if (argc < 5) {
    // READ
        unsigned int addr = strtoul(argv[2], NULL, 16);
        unsigned int offset = strtoul(argv[3], NULL, 16);
        unsigned int value;
        push_asic_read( (addr&0xff00)>>8,addr&0xff,  offset, &value);
        squid_wait_asic_reads();
        printf("[%x:%x] => %x\n",addr,offset,value);
      } else {
        unsigned int addr  = strtoul(argv[2], NULL, 16);
        unsigned int offset = strtoul(argv[3], NULL, 16);
        unsigned int value = strtoul(argv[4], NULL, 16);
        push_asic_write((addr&0xff00)>>8,addr&0xff, offset, value);
        squid_wait_asic_reads();
        printf("[%x:%x] <= %x\n",addr,offset,value);
    }
    
  } else if (argv[1][0] == 'm') {
       unsigned int offset = strtoul(argv[2], NULL, 16);
       unsigned int value = strtoul(argv[3], NULL, 16);;
       push_mq_write(0xff, 0xff, offset,  value);
       flush_mq_write();
       printf("[%x] => %x\n",offset,value);


 } else  {
    printf("Usage for squid read/write  : %s squid <h`addr> [h`value]\n" , argv[0]);
    printf(" for asic read/write : %s asic <h`addr> [h`value]\n" , argv[0]);
    return 0;
  }  

}


