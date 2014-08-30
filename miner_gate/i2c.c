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
#include "i2c-mydev.h"
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "spond_debug.h"

static int file;
pthread_mutex_t i2cm = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t i2cm_trans = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER;

int ignorable_err;



#define SLEEP_TIME_I2C 500

static char buf[10] = { 0 };


// set the I2C slave address for all subsequent I2C device transfers
static void i2c_set_address(int address, int *pError) {
  passert(file);
  int ioctl_err;
  if ((ioctl_err = ioctl(file, I2C_SLAVE, address)) < 0) {
    passert(0, "i2c-ff");
  }  
  *pError = ioctl_err;
}

int __i2c_write_block_data(unsigned char command, unsigned char len ,const unsigned char * buff ){
	//return i2c_smbus_write_block_data( file, command,len, buff);
	return 1;
}


void my_i2c_set_address(int address, int *pError) {
	i2c_set_address(address , pError);
}



void reset_i2c() {
  static int exported = 0;
  FILE *f;

  if (!exported) {
    f = fopen("/sys/class/gpio/export", "w");
    if (!f)
      return;
    fprintf(f, "111");
    fclose(f);
    f = fopen("/sys/class/gpio/gpio111/direction", "w");
    if (!f)
      return;
    fprintf(f, "out");
    fclose(f);
    exported = 1;
  }
  
  f = fopen("/sys/class/gpio/gpio111/value", "w");
  if (!f)
    return;
  fprintf(f, "0");
  usleep(100000);
  fprintf(f, "1");
  usleep(100000);
  fclose(f);
}


// Multiplyes by 1000
int i2c_getint_x1000(int source) {
  //printf("SOURCE=%x\n", source);  
  int n = (source & 0xF800) >> 11;
  int negative = false;
  //printf("N=0x%x\n", n);
  // This is shitty 2s compliment on 5 bits.
  if (n & 0x10) {
    negative = true;
    n = (n ^ 0x1F) + 1;
    //printf("Negative, new n=%x\n", n);
  }
  int y = (source & 0x07FF);
  //printf("Y=0x%x\n", y);
  if (negative) {
    //printf("RES:0x%x\n", (y * 1000) / (1 << n));
    return (y * 1000) / (1 << n);
  } else {
    //printf("RES:0x%x\n", (y * 1000) * (1 << n));    
    return (y * 1000) * (1 << n);
  }
}

// Multiplyes by 1000
int i2c_getint(int source) {
  //printf("SOURCE=%x\n", source);  
  int n = (source & 0xF800) >> 11;
  int negative = false;
  //printf("N=0x%x\n", n);
  // This is shitty 2s compliment on 5 bits.
  if (n & 0x10) {
    negative = true;
    n = (n ^ 0x1F) + 1;
    //printf("Negative, new n=%x\n", n);
  }
  int y = (source & 0x07FF);
  //printf("Y=0x%x\n", y);
  if (negative) {
    //printf("RES:0x%x\n", (y * 1000) / (1 << n));
    return (y) / (1 << n);
  } else {
    //printf("RES:0x%x\n", (y * 1000) * (1 << n));    
    return (y) * (1 << n);
  }
}


// 0 = no error
uint8_t i2c_read(uint8_t addr, int *pError) {
  uint8_t res;
   passert(pError);
  pthread_mutex_lock(&i2cm);
  i2c_set_address(addr, pError);
  if (*pError != 0) {
    psyslog(RED "i2c read 0x%x error1\n" RESET, addr);
     passert(0);
    res = 0;
  } else // no reason to call read, if we failed to set address.
      {
    res = i2c_smbus_read_byte(file, pError);
  }
  // usleep(SLEEP_TIME_I2C);
  pthread_mutex_unlock(&i2cm);
  DBG(DBG_I2C,"i2c_read [%x] = %x\n",addr,res);
  return res;
}

void i2c_write(uint8_t addr, uint8_t value, int *pError) {
  
  pthread_mutex_lock(&i2cm);
   passert(pError);
  i2c_set_address(addr, pError);

  if (pError != NULL && *pError != 0) {
       psyslog(RED "i2c write 0x%x = 0x%x error1\n" RESET, addr, value); 
       //print_stack();
  } else {
    if (i2c_smbus_write_byte(file, value) == -1) {
      psyslog(RED "i2c write 0x%x = 0x%x error2\n" RESET, addr, value); 
      //print_stack();
      if (pError != NULL)
        *pError = -1;
    }
    //usleep(SLEEP_TIME_I2C);
  }
  DBG(DBG_I2C,"i2c_write [%x] = %x\n",addr,value);
 // end_stopper(&tv,"i2c_write B");
  pthread_mutex_unlock(&i2cm);
}

uint8_t i2c_read_byte(uint8_t addr, uint8_t command, int *pError) {
  uint8_t res = 0;
  __s32 r;
  pthread_mutex_lock(&i2cm);
  passert(pError);
  i2c_set_address(addr, pError);

  i2c_set_address(addr, pError);
  if (*pError != 0) {
    printf("i2c_read_byte(%d): call to i2c_set_address returned with err %d\n",__LINE__ , *pError);    
    psyslog(RED "i2c read byte 0x%x 0x%x error1\n" RESET, addr, command);
  //  passert(0);
  //  res = 0xFF;
  } else {
    r = i2c_smbus_read_byte_data(file, command, pError);
    res = r & 0xFF;
    if (*pError) {
      psyslog(RED "i2c read byte 0x%x 0x%x error2\n" RESET, addr, command);
     // passert(0);
     // res = 0;
    }
  }
  pthread_mutex_unlock(&i2cm);
  DBG(DBG_I2C,"i2c_read [%x:%x] = %x\n",addr,command ,res);
  return res;
}


//TODO
uint8_t i2c_waddr_read_byte(uint8_t addr, uint16_t dev_addr, int *pError) {
  uint8_t res = 0;
  //__s32 r;
  passert(pError);
  pthread_mutex_lock(&i2cm);
  i2c_set_address(addr, pError);

  uint8_t command = (uint8_t)(dev_addr & 0xFF);
  uint8_t addr_data = (uint8_t)(dev_addr & 0xFF00);

  union i2c_smbus_data data;
  data.byte = addr_data;

  i2c_set_address(addr, pError);
  if (*pError != 0) {
    printf("i2c_read_byte(%d): call to i2c_set_address returned with err %d\n",__LINE__ , *pError);
    psyslog(RED "i2c read byte 0x%x 0x%x error1\n" RESET, addr, command);
    passert(0);
  //  res = 0xFF;
  } else {
    __s32 ioctl_err = i2c_smbus_access(file , I2C_SMBUS_READ , command , I2C_SMBUS_WORD_DATA , &data);

    if (0 != ioctl_err) {
    	printf("i2c error caught in i2c_smbus_read_byte_data %d\n" , ioctl_err);
        if (NULL != pError)
          * pError = ioctl_err;
    }

    else{
    	res = data.byte;
    }
  }
  pthread_mutex_unlock(&i2cm);
  DBG(DBG_I2C,"i2c_read [%x:%x] = %x\n",addr,command ,res);
  return res;
}

void i2c_waddr_write_byte(uint8_t addr, uint16_t dev_addr, uint8_t value, int *pError) {
	  pthread_mutex_lock(&i2cm);
	  passert(pError);
	  i2c_set_address(addr, pError);
	  if (*pError != 0) {
	    psyslog(RED "i2c write byte 0x%x 0x%x = 0x%x error3\n" RESET, addr, dev_addr, value);
      print_stack();

	  } else {
		  uint8_t command = (uint8_t)(dev_addr & 0xFF);
		  uint8_t block[] = { (uint8_t)(dev_addr >> 8),value};
		  if (i2c_smbus_write_block_data(file , command , 2 , block) == -1){
	    	  psyslog(RED "i2c_waddr_write_byte 0x%x 0x%x = 0x%x error4 ; c=%d , b0=%d , b1=%d\n" RESET, addr, dev_addr, value , command , block[0] , block[1]);
	        *pError = -1;
	    }
	    //usleep(SLEEP_TIME_I2C);
	  }
	  DBG(DBG_I2C,"i2c_write [%x] = %x\n",addr,value);
	  pthread_mutex_unlock(&i2cm);
}

void i2c_write_byte(uint8_t addr, uint8_t command, uint8_t value, int *pError) {
  pthread_mutex_lock(&i2cm);
  passert(pError);
  i2c_set_address(addr, pError);
  if (*pError != 0) {
    psyslog(RED "i2c write byte 0x%x 0x%x = 0x%x error5\n" RESET, addr, command, value);

  } else {
    if (i2c_smbus_write_byte_data(file, command, value) == -1) {
      psyslog(RED "i2c write byte 0x%x 0x%x = 0x%x error6\n" RESET, addr, command, value);
      *pError = -1;
    }
    //usleep(SLEEP_TIME_I2C);
  }
  DBG(DBG_I2C,"i2c_write [%x:%x] = %x\n",addr,command,value);
  pthread_mutex_unlock(&i2cm);
}

void emerson_workaround() {
/*  struct timeval tv;
   start_stopper(&tv);
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
   end_stopper(&tv,"emerson_workaround B");
   start_stopper(&tv);  
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
   end_stopper(&tv,"emerson_workaround C");
   //reset_i2c();
*/
}



uint16_t i2c_read_word(uint8_t addr, uint8_t command, int *pError) {
  __s32 r;
  pthread_mutex_lock(&i2cm);
  passert(pError);
  i2c_set_address(addr, pError);
  if ( *pError != 0) {
#ifdef MINERGATE
	    psyslog(RED "i2c read word 0x%x 0x%x error7\n" RESET, addr, command);
#else
    	perror("i2c read word error7\n" );
#endif

    r = 0xFFFF;
  } else {
    r = i2c_smbus_read_word_data(file, command, pError);
    if ( *pError != 0) {
#ifdef MINERGATE
    	psyslog(RED "i2c read word 0x%x 0x%x error8\n" RESET, addr, command, *pError);
#else
    	perror("i2c read word error8\n");
#endif

      r = 0x0;
      pthread_mutex_unlock(&i2cm);
      return r;
    } 
  }
  DBG(DBG_I2C,"i2c[%x:%x] -> %x\n",addr, command, r);
  // usleep(SLEEP_TIME_I2C);
  pthread_mutex_unlock(&i2cm);
  DBG(DBG_I2C,"i2c_read [%x:%x] = %x\n",addr,command ,r);
  return r;
}

void i2c_write_word(uint8_t addr, uint8_t command, uint16_t value,
                    int *pError) {
  pthread_mutex_lock(&i2cm);
  i2c_set_address(addr, pError);
  if (pError != NULL && *pError != 0) {
    psyslog(RED "i2c write word 0x%x 0x%x = 0x%x error9\n" RESET, addr, command, value);        
  } else {
    if ((i2c_smbus_write_word_data(file, command, value) == -1) &&
        (pError != NULL)) {
      psyslog(RED "i2c write word 0x%x 0x%x = 0x%x errora\n" RESET, addr, command, value);        
      *pError = -1;
    }

    //usleep(SLEEP_TIME_I2C);
  }
  DBG(DBG_I2C,"i2c_write [%x:%x] = %x\n",addr,command,value);
  pthread_mutex_unlock(&i2cm);
}

void i2c_init(int *pError) {
  reset_i2c();
  pthread_mutex_lock(&i2cm);
  if ((file = open("/dev/i2c-0", O_RDWR)) < 0) {
    psyslog("Failed to open the i2c bus1.\n");
    
    /* ERROR HANDLING; you can check errno to see what went wrong */
    if (pError != NULL) {      
      psyslog(RED "i2c init error\n" RESET);
      *pError = -1;
    }
  }
  pthread_mutex_unlock(&i2cm);
}

uint16_t read_mgmt_temp() {
  pthread_mutex_lock(&i2cm_trans);
  // set router
  //     ./i2cset -y 0 0x70 0x00 0xff
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TEMP_SENSOR_PIN | PRIMARY_I2C_SWITCH_DEAULT);
  // read value
  //     ./i2cget -y 0 0x48 0x00 w
  uint16_t value = i2c_read_word(0x48, 0x00);
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
  pthread_mutex_unlock(&i2cm_trans);
  return value;
}
