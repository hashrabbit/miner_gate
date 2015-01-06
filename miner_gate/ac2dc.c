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


#include "ac2dc_const.h"
#include "i2c.h"
#include "asic.h"
#include "ac2dc.h"
#include "dc2dc.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 


extern MINER_BOX vm;
extern pthread_mutex_t i2c_mutex;
static int mgmt_addr[5] = {0, AC2DC_MURATA_NEW_I2C_MGMT_DEVICE, 0, AC2DC_EMERSON_1200_I2C_MGMT_DEVICE, AC2DC_EMERSON_1600_I2C_MGMT_DEVICE};
static int eeprom_addr[5] = {0, AC2DC_MURATA_NEW_I2C_EEPROM_DEVICE, 0, AC2DC_EMERSON_1200_I2C_EEPROM_DEVICE,AC2DC_EMERSON_1600_I2C_EEPROM_DEVICE};
static int revive_code_off[5] = {0, 0x0, 0x0, 0x40, 0x40};
static int revive_code_on[5] = {0, 0x80, 0x0, 0x80, 0x80};
static int psu_addr[PSU_COUNT]  = {PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN, PRIMARY_I2C_SWITCH_AC2DC_PSU_1_PIN}; 
int get_fake_power(int psu_id);




static char psu_names[5][20] = {"UNKNOWN","NEW-MURATA","OLD-MURATA","EMERSON1200","EMERSON1600"};

char* psu_get_name(int id, int type) {
  if(type < 5) {
      return psu_names[type];
  } else {
      psyslog("ILEGAL PSU %d TYPE %d\n", id, type);
      passert(0);
  } 
}


// Return Watts
/*
static int ac2dc_get_power(AC2DC *ac2dc, int psu_id) {
  int err = 0;
  static int warned = 0;
  int r;

  if (err) {
    psyslog("RESET I2C BUS?\n");  
    /*
    system("echo 111 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio111/direction");
    system("echo 0 > /sys/class/gpio/gpio111/value");
    system("echo 1 > /sys/class/gpio/gpio111/value");
    usleep(1000000);
    system("echo 111 > /sys/class/gpio/unexport");
    passert(0);
    * /
  }
  if (err) {
    if ((warned++) < 10)
      psyslog( "FAILED TO INIT AC2DC\n" );
    if ((warned) == 9)
      psyslog( "FAILED TO INIT AC2DC, giving up :(\n" );
    return 100;
  }

  return power;
}
*/


bool ac2dc_check_connected(int psu_id) {
  int err;
  bool ret = false;
  i2c_write(PRIMARY_I2C_SWITCH, psu_addr[psu_id] | PRIMARY_I2C_SWITCH_DEAULT, &err);  
  
  i2c_read_word(AC2DC_EMERSON_1200_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err, 0);
  if (!err) {
   ret= true;
  }

  i2c_read_word(AC2DC_MURATA_NEW_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err, 0);
  if (!err) {
    ret= true;
  }

  i2c_read_word(AC2DC_EMERSON_1600_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err, 0);
  if (!err) {
    ret= true;
  }

  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT, &err);          
  return ret;
}


void ac2dc_init_one(AC2DC* ac2dc, int psu_id) {
  
 int err;
 
 i2c_write(PRIMARY_I2C_SWITCH, psu_addr[psu_id] | PRIMARY_I2C_SWITCH_DEAULT, &err);  
      {
        i2c_read_word(AC2DC_EMERSON_1200_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err, 0);
        if (!err) {
#ifdef MINERGATE
          psyslog("EMERSON 1200 AC2DC LOCATED\n");
#endif
          ac2dc->ac2dc_type = AC2DC_TYPE_EMERSON_1_2;
        } else {
          // NOT EMERSON 1200
          i2c_read_word(AC2DC_MURATA_NEW_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err, 0);
          if (!err) {
#ifdef MINERGATE
           psyslog("NEW MURATA AC2DC LOCATED\n");
#endif
           ac2dc->ac2dc_type = AC2DC_TYPE_MURATA_NEW;
          } else {
          // NOT EMERSON 1200
          i2c_read_word(AC2DC_EMERSON_1600_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err, 0);
          if (!err) {
#ifdef MINERGATE
           psyslog("EMERSON 1600 AC2DC LOCATED\n");
#endif
           ac2dc->ac2dc_type = AC2DC_TYPE_EMERSON_1_6;
          } else {
            // NOT MURATA 1200
#ifdef MINERGATE
              psyslog("UNKNOWN AC2DC\n");
#endif
            ac2dc->ac2dc_type = AC2DC_TYPE_UNKNOWN;
            ac2dc->voltage = 0;
          }
        }
      }
    }
    i2c_write(PRIMARY_I2C_SWITCH, psu_addr[psu_id] | PRIMARY_I2C_SWITCH_DEAULT, &err);  
    ac2dc->voltage = i2c_read_word(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_VIN_WORD, &err);
    ac2dc->voltage = i2c_getint(ac2dc->voltage );  
#ifdef MINERGATE
    psyslog("INPUT VOLTAGE=%d\n", ac2dc->voltage);
#endif
    // Fix Emerson BUG
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT, &err);  
}


#ifdef MINERGATE
void ac2dc_init() {
  for (int i = 0 ; i < PSU_COUNT; i++) {
    psyslog("DISCOVERING AC2DC %i\n",i);
    vm.ac2dc[i].ac2dc_type = AC2DC_TYPE_UNKNOWN;
    if (!vm.ac2dc[i].force_generic_psu) {
      ac2dc_init_one(&vm.ac2dc[i], i);
    }
  }
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
  read_ac2dc_errors(1);
}
#endif

#ifndef MINERGATE

void ac2dc_init2(AC2DC * ac2dc) {

	 int err;

	 ac2dc[1].ac2dc_type = AC2DC_TYPE_UNKNOWN;
	 ac2dc[0].ac2dc_type = AC2DC_TYPE_UNKNOWN;

	 i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PSU_1_PIN | PRIMARY_I2C_SWITCH_DEAULT, &err);
	 if (err) {
		 psyslog("GENERAL I2C AC2DC ERROR. EXIT\n");
		 assert(0);
	 }
	 ac2dc_init_one(&ac2dc[1],1);

	 i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN | PRIMARY_I2C_SWITCH_DEAULT, &err);
	 if (err) {
		 psyslog("GENERAL I2C AC2DC ERROR. EXIT\n");
		 assert(0);
	 }

	 ac2dc_init_one(&ac2dc[0],0);
	 i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);

  }

int ac2dc_get_vpd(ac2dc_vpd_info_t *pVpd, int psu_id, AC2DC *ac2dc) {

	  if (ac2dc->ac2dc_type == AC2DC_TYPE_UNKNOWN) {
	     return 0;
	  }


	  int rc = 0;
	  int err = 0;
	  int pnr_offset = 0x34;
	  int pnr_size = 15;
	  int model_offset = 0x57;
	  int model_size = 4;
	  int serial_offset = 0x5b;
	  int serial_size = 10;
	  int revision_offset = 0x61;
	  int revision_size = 2;

	  if (ac2dc->ac2dc_type == AC2DC_TYPE_MURATA_NEW) // MURATA
	  {
		  fprintf(stderr, "MURATA_NEW\n");
		  pnr_offset = 0x1C;
		  pnr_size = 23;
		  model_offset = 0x16;
		  model_size = 5;
		  serial_offset = 0x35;
		  serial_size = 12;
		  revision_offset = 0x39;
		  revision_size = 2;
	  }else if (ac2dc->ac2dc_type == AC2DC_TYPE_EMERSON_1_2) // EMRSN1200
	  {
		  /*
		   *
	5 (0x2c , 0x30) Vendor ID
	12 (0x32, 0x3d) Product Name
	12 (0x3F , 0x4A ) Product NR
	2  (0x4c ,0x4d) REV
	13 (0x4f , 0x5B)SNR
		   */

		  pnr_offset = 0x3F;
		  pnr_size = 12;
		  model_offset = 0x3F;
		  model_size = 12;
		  serial_offset = 0x4f;
		  serial_size = 13;
		  revision_offset = 0x4c;
		  revision_size = 2;

	  }
	  else if (ac2dc->ac2dc_type == AC2DC_TYPE_EMERSON_1_6) // EMRSN1600
 	  {
		  pnr_offset = 0x34;
		  pnr_size = 15;
		  model_offset = 0x57;
		  model_size = 4;
		  serial_offset = 0x57;
		  serial_size = 13; // this includes the model and the serial . serial naked is 0x5b/9 (instead of 57/13)
		  revision_offset = 0x61;
		  revision_size = 2;
	  }

	  if (NULL == pVpd) {
	    psyslog("call ac2dc_get_vpd performed without allocating sturcture first\n");
	    return 1;
	  }
	  //psyslog("psu_id %d\n",psu_id);


	  pthread_mutex_lock(&i2c_mutex);

	  i2c_write(PRIMARY_I2C_SWITCH, ((psu_id == 0)?PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN:PRIMARY_I2C_SWITCH_AC2DC_PSU_1_PIN) | PRIMARY_I2C_SWITCH_DEAULT , &err);

	  if (err) {
	    fprintf(stderr, "Failed writing to I2C address 0x%X (err %d)",
	            PRIMARY_I2C_SWITCH, err);
	     pthread_mutex_unlock(&i2c_mutex);
	    return err;
	  }

	  for (int i = 0; i < pnr_size; i++) {
	    pVpd->pnr[i] = ac2dc_get_eeprom_quick(pnr_offset + i,ac2dc, &err);
	    if (err)
	      goto ac2dc_get_eeprom_quick_err;
	  }

	  for (int i = 0; i < model_size; i++) {
	    pVpd->model[i] = ac2dc_get_eeprom_quick(model_offset + i, ac2dc, &err);
	    if (err)
	      goto ac2dc_get_eeprom_quick_err;
	  }

	  for (int i = 0; i < revision_size; i++) {
	    pVpd->revision[i] = ac2dc_get_eeprom_quick(revision_offset + i, ac2dc, &err);
	    if (err)
	      goto ac2dc_get_eeprom_quick_err;
	  }


	  for (int i = 0; i < serial_size; i++) {
	    pVpd->serial[i] = ac2dc_get_eeprom_quick(serial_offset + i, ac2dc, &err);
	    if (err)
	      goto ac2dc_get_eeprom_quick_err;
	  }


	ac2dc_get_eeprom_quick_err:

	  if (err) {
	    fprintf(stderr, RED            "Failed reading AC2DC eeprom (err %d)\n" RESET, err);
	    rc =  err;
	  }

	  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
	  pthread_mutex_unlock(&i2c_mutex);

	  return rc;
}

unsigned char ac2dc_get_eeprom_quick(int offset, AC2DC *ac2dc, int *pError) {

  if (ac2dc->ac2dc_type == AC2DC_TYPE_UNKNOWN) {
     return 0;
  }

  //pthread_mutex_lock(&i2c_mutex); no lock here !!

  unsigned char b =
      (unsigned char)i2c_read_byte(eeprom_addr[ac2dc->ac2dc_type], offset, pError);

  return b;
}

int ac2dc_get_eeprom(int offset, int psu_id, AC2DC *ac2dc, int *pError) {
  // Stub for remo
  if (ac2dc->ac2dc_type == AC2DC_TYPE_UNKNOWN) {
    return 0;
  }
  //printf("%s:%d\n",__FILE__, __LINE__);
  pthread_mutex_lock(&i2c_mutex);
  int b;
  i2c_write(PRIMARY_I2C_SWITCH, psu_addr[psu_id] | PRIMARY_I2C_SWITCH_DEAULT, pError);
  if (pError && *pError) {
     pthread_mutex_unlock(&i2c_mutex);
    return *pError;
  }

  b = i2c_read_byte(eeprom_addr[ac2dc->ac2dc_type], offset, pError);
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
   pthread_mutex_unlock(&i2c_mutex);
  return b;
}
#endif





#ifdef MINERGATE
int update_work_mode(int decrease_top, int decrease_bottom, bool to_alternative);
void exit_nicely(int seconds_sleep_before_exit, const char* p);
static pthread_t ac2dc_thread;

// returns non-zero in case of error
int read_ac2dc_errors(int to_event) {  
  int err;
  int r79_0 = 0;
  int r79_1 = 0;
  int r7b_0 = 0;
  int r7b_1 = 0;  
#ifndef SP2x   
  if (vm.ac2dc[PSU_0].ac2dc_type != AC2DC_TYPE_UNKNOWN) {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN | PRIMARY_I2C_SWITCH_DEAULT);      
    AC2DC* ac2dc = &vm.ac2dc[PSU_0];
    r79_0 = i2c_read_word(mgmt_addr[ac2dc->ac2dc_type], 0x79, &err);
    r7b_0 = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x7b, &err);    
    r79_0 &= (~0x3);
    if (r79_0 && (r79_0 != 0x4000)) {
      psyslog("AC2DC TOP 79:%x 7b:%x", r79_0, r7b_0);
     // psyslog(" 7a:%x",i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x7a, &err));
     // psyslog(" 7b:%x",r7b_0);
     // psyslog(" 7c:%x",i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x7c, &err));
     // psyslog(" 7d:%x",i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x7d, &err));
     // psyslog(" 81:%x\n",i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x81, &err));
    }
    i2c_write(mgmt_addr[ac2dc->ac2dc_type], 0x03);
  }
  if (vm.ac2dc[PSU_1].ac2dc_type != AC2DC_TYPE_UNKNOWN) {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PSU_1_PIN | PRIMARY_I2C_SWITCH_DEAULT);      
    AC2DC* ac2dc = &vm.ac2dc[PSU_1];
    r79_1 = i2c_read_word(mgmt_addr[ac2dc->ac2dc_type], 0x79, &err);
    r7b_1 = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x7b, &err);    
    r79_1 &= (~0x3);
    if(r79_1 && (r79_1 != 0x4000)) {
      psyslog("AC2DC TOP 79:%x 7b:%x", r79_1, r7b_1);
      //psyslog(" 7a:%x",i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x7a, &err));
      //psyslog(" 7b:%x",r7b_1);
      //psyslog(" 7c:%x",i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x7c, &err));
      //psyslog(" 7d:%x",i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x7d, &err));
      //psyslog(" 81:%x\n",i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], 0x81, &err));
    }
    i2c_write(mgmt_addr[ac2dc->ac2dc_type], 0x03);
  }
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);  

  int problem_top = (r79_0 != 0);
  if ((r79_0 == 0x4000) && (r7b_0 == 0x20)) {
    problem_top = 0;
  }

  int problem_bot = (r79_1 != 0);
  if ((r79_1 == 0x4000) && (r7b_1 == 0x20)) {
    problem_bot = 0;
  }
  
  int problem = problem_top || problem_bot;
  
  if (problem) {
    vm.err_murata++;
    //mg_event_x(RED "AC2DC status: %x %x" RESET,r79_0,r79_1);
    psyslog(RED  "AC2DC status: %x:%x %x:%x\n" RESET,r79_0,r7b_0,r79_1,r7b_1);
  }
  
  int ppp = 0;
  if ((r79_0 & 0x8000) && (vm.board_working_asics[BOARD_0] > 0)) {
      // in this error - restore loop error count
      for (int l = 0; l < ASICS_PER_BOARD ; l++) { 
        vm.loop[l].bad_loop_count = 0;
      }
      ppp=1;
  }

  if ((r79_1 & 0x8000) && (vm.board_working_asics[BOARD_1] > 0)) {
      // in this error - restore loop error count
      for (int l = ASICS_PER_BOARD; l < ASICS_COUNT; l++) { 
        vm.loop[l].bad_loop_count = 0;
      }
      ppp=1;
  }
  
  return (ppp);
#endif  
}



void test_fix_ac2dc_limits() {
#ifdef SP2x
#else
  int err;
  if (vm.ac2dc[PSU_0].ac2dc_type != AC2DC_TYPE_UNKNOWN &&
      vm.ac2dc[PSU_0].ac2dc_type != AC2DC_TYPE_MURATA_NEW) {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN | PRIMARY_I2C_SWITCH_DEAULT);      
    AC2DC* ac2dc = &vm.ac2dc[PSU_0];
    int p = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_STATUS_IOUT, &err);
    if (!err) {
      int q = 0x1f;
      if (ac2dc->ac2dc_type == AC2DC_TYPE_EMERSON_1_2) {
        q = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_FAULTS, &err);     
      }
      psyslog("TOP PSU STAT:%x %x\n", p, q);
      if ((p & (AC2DC_I2C_READ_STATUS_IOUT_OP_ERR | AC2DC_I2C_READ_STATUS_IOUT_OC_ERR)) ||
              ((ac2dc->ac2dc_type == AC2DC_TYPE_EMERSON_1_2) && (q != 0x1f))) {            
        psyslog("AC2DC OVERCURRENT 0:%x\n",p);
        PSU12vPowerCycleALL();
        if ((p & AC2DC_I2C_READ_STATUS_IOUT_OC_ERR) &&
              (
                (vm.ac2dc[PSU_0].ac2dc_type == AC2DC_TYPE_EMERSON_1_2) && (vm.ac2dc[PSU_0].ac2dc_power_limit > 1270)
                ||
                (vm.ac2dc[PSU_0].ac2dc_type == AC2DC_TYPE_EMERSON_1_6) && (vm.ac2dc[PSU_0].ac2dc_power_limit > 1560)
               )
            ){
          mg_event_x("update 0 work mode %d", vm.ac2dc[PSU_0].ac2dc_power_limit);
          update_work_mode(5, 0, false);
        }     
        mg_event_x("AC2DC top fail on %d", vm.ac2dc[PSU_0].ac2dc_power_limit);
        i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
        exit_nicely(10,"AC2DC fail, restart minergate please");
      }
    }else {
      mg_event("top i2c miss");
    }
  }
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);

  if (vm.ac2dc[PSU_1].ac2dc_type != AC2DC_TYPE_UNKNOWN &&
      vm.ac2dc[PSU_1].ac2dc_type != AC2DC_TYPE_MURATA_NEW) {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PSU_1_PIN | PRIMARY_I2C_SWITCH_DEAULT);      
    AC2DC* ac2dc = &vm.ac2dc[PSU_1];
    int p = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_STATUS_IOUT, &err);
    if (!err) {
      int q = 0x1f;
      if (ac2dc->ac2dc_type == AC2DC_TYPE_EMERSON_1_2) {
        q = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_FAULTS, &err);    
      }
      psyslog("BOT PSU STAT:%x %x\n", p, q);
      if ((p & (AC2DC_I2C_READ_STATUS_IOUT_OP_ERR | AC2DC_I2C_READ_STATUS_IOUT_OC_ERR)) ||
           ((ac2dc->ac2dc_type == AC2DC_TYPE_EMERSON_1_2) && (q != 0x1f))) {
        psyslog("AC2DC OVERCURRENT BOTTOM:%x\n",p);
        PSU12vPowerCycleALL();
/*
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_WRITE_PROTECT,0x0,&err);
        usleep(3000000);
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_ON_OFF,revive_code_off[ac2dc->ac2dc_type],&err);
        usleep(3000000);
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_ON_OFF,0x80,&err);
        usleep(3000000);
*/
        if ((p & AC2DC_I2C_READ_STATUS_IOUT_OC_ERR) &&
             (
               (vm.ac2dc[PSU_1].ac2dc_type == AC2DC_TYPE_EMERSON_1_2) && (vm.ac2dc[PSU_1].ac2dc_power_limit > 1270)
               ||
               (vm.ac2dc[PSU_1].ac2dc_type == AC2DC_TYPE_EMERSON_1_6) && (vm.ac2dc[PSU_1].ac2dc_power_limit > 1570)
             ) 
             ){
          mg_event_x("update bottom work mode %d", vm.ac2dc[PSU_1].ac2dc_power_limit);
          update_work_mode(0, 5, false);
        }
        mg_event_x("AC2DC bottom fail on %d", vm.ac2dc[PSU_1].ac2dc_power_limit);
        i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
        exit_nicely(10,"AC2DC fail, restart minergate please");
      }
    } else {
      mg_event("bottom i2c miss");
    }
  }
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);  
#endif
}




void update_single_psu(AC2DC *ac2dc, int psu_id) {
   int p;
   int err;
   struct timeval tv;
   start_stopper(&tv);
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);  
   int i2c_switch = psu_addr[psu_id] | PRIMARY_I2C_SWITCH_DEAULT;

   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);
   
   p = i2c_read_word(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_PIN_WORD, &err);
   if (!err) {
     ac2dc->ac2dc_in_power = i2c_getint(p); 
     DBG( DBG_SCALING_AC2DC ,"PowerIn: %d\n", ac2dc->ac2dc_in_power);
   } else {
     //ac2dc->ac2dc_in_power = 0;
     DBG( DBG_SCALING_AC2DC ,"PowerIn: Error\n", ac2dc->ac2dc_in_power);
   }
   
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);  
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);      
   p = i2c_read_word(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_POUT_WORD, &err);
   if (!err) {
      int pp = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_STATUS_IOUT, &err);
      DBG( DBG_SCALING_AC2DC ,"PSU STAT:%x\n", pp);
      ac2dc->ac2dc_power_last_last = ac2dc->ac2dc_power_last;
      ac2dc->ac2dc_power_last = ac2dc->ac2dc_power_now;      
      ac2dc->ac2dc_power_now = i2c_getint(p); 
      ac2dc->ac2dc_power_assumed = MAX(ac2dc->ac2dc_power_now, ac2dc->ac2dc_power_last);
      ac2dc->ac2dc_power_assumed = MAX(ac2dc->ac2dc_power_assumed, ac2dc->ac2dc_power_last_last);        
      ac2dc->ac2dc_power_measured = ac2dc->ac2dc_power_assumed;       
      DBG( DBG_POWER ,"PowerOut %d: R:%d PM:%d [PL:%d PLL:%d PLLL:%d]\n", 
               psu_id,
               p,
               ac2dc->ac2dc_power_assumed,
               ac2dc->ac2dc_power_now,
               ac2dc->ac2dc_power_last,
               ac2dc->ac2dc_power_last_last);
   } else {
     DBG( DBG_SCALING_AC2DC ,"PowerOut: Error\n", ac2dc->ac2dc_power_assumed);
   }
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);  
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);      
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);  
   
   int usecs = end_stopper(&tv,NULL);
   if ((usecs > 500000)) {
     mg_event_x("Bad PSU FW on PSU %d", psu_id);  
     exit_nicely(20,"Bad PSU Firmware");
   }
}



void *update_ac2dc_power_measurments_thread(void *ptr) {
  pthread_mutex_lock(&i2c_mutex);  
  int err;  
  AC2DC *ac2dc;
  int p;

  for (int psu_id = 0; psu_id < PSU_COUNT; psu_id++) {
    ac2dc = &vm.ac2dc[psu_id];
    ac2dc->ac2dc_power_last_last_fake= ac2dc->ac2dc_power_last_fake;
    ac2dc->ac2dc_power_last_fake = ac2dc->ac2dc_power_now_fake;      
    ac2dc->ac2dc_power_now_fake = get_fake_power(psu_id); 
    ac2dc->ac2dc_power_fake = MAX(ac2dc->ac2dc_power_now_fake, ac2dc->ac2dc_power_last_fake);
    ac2dc->ac2dc_power_fake = MAX(ac2dc->ac2dc_power_fake, ac2dc->ac2dc_power_last_last_fake);  
     
    if (ac2dc->ac2dc_type != AC2DC_TYPE_UNKNOWN) {
        update_single_psu(ac2dc, psu_id);
    } else {
      ac2dc->ac2dc_power_last_last= ac2dc->ac2dc_power_last;
      ac2dc->ac2dc_power_last = ac2dc->ac2dc_power_now;      
      ac2dc->ac2dc_power_now = ac2dc->ac2dc_power_now_fake; 
      ac2dc->ac2dc_power_assumed = MAX(ac2dc->ac2dc_power_now, ac2dc->ac2dc_power_last);
      ac2dc->ac2dc_power_assumed = MAX(ac2dc->ac2dc_power_assumed, ac2dc->ac2dc_power_last_last);
      ac2dc->ac2dc_power_measured = ac2dc->ac2dc_power_assumed;
      if (!ac2dc->force_generic_psu) {
#ifndef SP2x        
        if (ac2dc_check_connected(psu_id)) {
          exit_nicely(10,"AC2DC connected");
        }
#endif              
      }
    }
  }
  
  pthread_mutex_unlock(&i2c_mutex);
  //pthread_exit(NULL);
  return NULL;
}




int update_ac2dc_power_measurments() {
  update_ac2dc_power_measurments_thread(NULL);
}


static void PSU12vONOFF (int psu , bool ON){
	int err = 0;

	int * cmd_code_arr = ON ? revive_code_on : revive_code_off;

	if (psu >= PSU_COUNT || psu < 0)
	{
		psyslog("Wrong PSU number %d in PSU12vONOFF\n",psu);
		return;
	}

	pthread_mutex_lock(&i2c_mutex);

	int _type = vm.ac2dc[psu].ac2dc_type;
//	printf ("I2C_WRITE(0x%X , 0x%X )\n" , PRIMARY_I2C_SWITCH,psu_addr[psu]);
	i2c_write(PRIMARY_I2C_SWITCH, psu_addr[psu]|PRIMARY_I2C_SWITCH_DEAULT , &err);
	usleep(5000);


//	printf ("I2C_WRITE_BYTE(0x%X , 0x%X , 0x%X ) \n" , mgmt_addr[_type], AC2DC_I2C_WRITE_PROTECT , 0x00 );
	i2c_write_byte(mgmt_addr[_type], AC2DC_I2C_WRITE_PROTECT , 0x00 , &err);
	usleep(5000);

//	printf ("I2C_WRITE_BYTE(0x%X , 0x%X , 0x%X ) \n" , mgmt_addr[_type], AC2DC_I2C_ON_OFF , cmd_code_arr[_type] );
	i2c_write_byte(mgmt_addr[_type], AC2DC_I2C_ON_OFF , cmd_code_arr[_type] , &err);
  mg_event("PSU restart");
	usleep(5000);

	pthread_mutex_unlock(&i2c_mutex);

}

void PSU12vON (int psu){
	PSU12vONOFF(psu , true);
	return;
}


void PSU12vOFF (int psu){
	PSU12vONOFF(psu , false);
	return;
}
void PSU12vPowerCycle (int psu){
	PSU12vOFF(psu );
	usleep(3000 * 1000);
	PSU12vON(psu );
	return;
}

void PSU12vPowerCycleALL() {
  psyslog("Doing full power cycle\n");
	for (int psu = 0 ; psu < PSU_COUNT ; psu++){
		PSU12vOFF(psu );
	}
	usleep(4000 * 1000);
	for (int psu = 0 ; psu < PSU_COUNT ; psu++){
		PSU12vON(psu );
	}
  usleep(9000 * 1000);
	return;
}
#endif

