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
static int mgmt_addr[4] = {0, AC2DC_MURATA_NEW_I2C_MGMT_DEVICE, AC2DC_MURATA_OLD_I2C_MGMT_DEVICE, AC2DC_EMERSON_1200_I2C_MGMT_DEVICE};
static int eeprom_addr[4] = {0, AC2DC_MURATA_NEW_I2C_EEPROM_DEVICE, AC2DC_MURATA_OLD_I2C_EEPROM_DEVICE, AC2DC_EMERSON_1200_I2C_EEPROM_DEVICE};
static int revive_code[4] = {0, 0x0, 0x0, 0x40};
static int revive_code_ON[4] = {0, 0x80, 0x80, 0x80};
static int psu_addr[PSU_COUNT]  = {PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN, PRIMARY_I2C_SWITCH_AC2DC_PSU_1_PIN}; 
int get_fake_power(int psu_id);




static char psu_names[4][20] = {"UNKNOWN","NEW-MURATA","OLD-MURATA","EMERSON1200"};

char* psu_get_name(int type) {
  passert(type < 4);
  return psu_names[type];
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
  i2c_read_word(AC2DC_MURATA_OLD_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err);
  if (!err) {
    ret= true;
  }

  
  i2c_read_word(AC2DC_EMERSON_1200_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err);
  if (!err) {
   ret= true;
  }

  i2c_read_word(AC2DC_MURATA_NEW_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err);
  if (!err) {
    ret= true;
  }
    
  return ret;

  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT, &err);  
}


void ac2dc_init_one(AC2DC* ac2dc, int psu_id) {
  
 int err;
 
 i2c_write(PRIMARY_I2C_SWITCH, psu_addr[psu_id] | PRIMARY_I2C_SWITCH_DEAULT, &err);  
 
     int res = i2c_read_word(AC2DC_MURATA_OLD_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err);
      if (!err) {
#ifdef MINERGATE
        psyslog("OLD MURATA AC2DC LOCATED\n");
#endif
        ac2dc->ac2dc_type = AC2DC_TYPE_MURATA_OLD;
      } else {
        i2c_read_word(AC2DC_EMERSON_1200_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err);
        if (!err) {
#ifdef MINERGATE
          psyslog("EMERSON 1200 AC2DC LOCATED\n");
#endif
          ac2dc->ac2dc_type = AC2DC_TYPE_EMERSON_1_2;
        } else {
          // NOT EMERSON 1200
          i2c_read_word(AC2DC_MURATA_NEW_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP_WORD, &err);
          if (!err) {
#ifdef MINERGATE
           psyslog("NEW MURATA AC2DC LOCATED\n");
#endif
           ac2dc->ac2dc_type = AC2DC_TYPE_MURATA_NEW;
          } else {
            // NOT MURATA 1200
            psyslog("UNKNOWN AC2DC\n");
            ac2dc->ac2dc_type = AC2DC_TYPE_UNKNOWN;
            ac2dc->voltage = 0;
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
  psyslog("DISCOVERING AC2DC 0\n");
  vm.ac2dc[PSU_0].ac2dc_type = AC2DC_TYPE_UNKNOWN;
  if (!vm.ac2dc[PSU_0].force_generic_psu) {
    ac2dc_init_one(&vm.ac2dc[PSU_0], PSU_0);
  }
  psyslog("DISCOVERING AC2DC 1\n");
  vm.ac2dc[PSU_1].ac2dc_type = AC2DC_TYPE_UNKNOWN;
  if (!vm.ac2dc[PSU_1].force_generic_psu) {
    ac2dc_init_one(&vm.ac2dc[PSU_1], PSU_1);
  } 
  
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
  // Disable boards if no DC2DC
  vm.ac2dc[PSU_1].psu_present = 1;//(vm.ac2dc[PSU_1].ac2dc_type != AC2DC_TYPE_UNKNOWN); 
  vm.ac2dc[PSU_0].psu_present = 1;//(vm.ac2dc[PSU_0].ac2dc_type != AC2DC_TYPE_UNKNOWN);   
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
		  pnr_offset = 0x1D;
		  pnr_size = 21;
		  model_offset = 0x16;
		  model_size = 6;
		  serial_offset = 0x34;
		  serial_size = 12;
		  revision_offset = 0x3a;
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
		   *
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

void test_fix_ac2dc_limits() {
  int err;
  if (vm.ac2dc[PSU_0].ac2dc_type != AC2DC_TYPE_UNKNOWN) {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN | PRIMARY_I2C_SWITCH_DEAULT);      
    AC2DC* ac2dc = &vm.ac2dc[PSU_0];
    int p = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_STATUS_IOUT, &err);
    if (!err) {
      int q = 0x1f;
      if (ac2dc->ac2dc_type == AC2DC_TYPE_EMERSON_1_2) {
        q = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_FAULTS, &err);     
      }
      psyslog("0 PSU STAT:%x %x\n", p, q);
      if ((p & (AC2DC_I2C_READ_STATUS_IOUT_OP_ERR | AC2DC_I2C_READ_STATUS_IOUT_OC_ERR)) ||
              ((ac2dc->ac2dc_type == AC2DC_TYPE_EMERSON_1_2) && (q != 0x1f))) {            
        psyslog("AC2DC OVERCURRENT 0:%x\n",p);
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_WRITE_PROTECT,0x0,&err);
        usleep(3000000);
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_ON_OFF,revive_code[ac2dc->ac2dc_type],&err);
        usleep(1000000);
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_ON_OFF,0x80,&err);

         if ((p & AC2DC_I2C_READ_STATUS_IOUT_OC_ERR) &&
            (vm.ac2dc[PSU_0].ac2dc_power_limit > 1270)) {
          mg_event_x("update 0 work mode %d", vm.ac2dc[PSU_0].ac2dc_power_limit);
          update_work_mode(5, 0, false);
        }     
        mg_event_x("AC2DC top fail on %d", vm.ac2dc[PSU_0].ac2dc_power_limit);
        i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
        exit_nicely(4,"AC2DC fail 1");
      }
    }else {
      mg_event("top i2c miss");
    }
  }
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);

  if (vm.ac2dc[PSU_1].ac2dc_type != AC2DC_TYPE_UNKNOWN) {
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
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_WRITE_PROTECT,0x0,&err);
        usleep(3000000);
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_ON_OFF,revive_code[ac2dc->ac2dc_type],&err);
        usleep(1000000);
        i2c_write_byte(mgmt_addr[ac2dc->ac2dc_type],AC2DC_I2C_ON_OFF,0x80,&err);
        if ((p & AC2DC_I2C_READ_STATUS_IOUT_OC_ERR) &&
            (vm.ac2dc[PSU_1].ac2dc_power_limit > 1270)) {
          mg_event_x("update bottom work mode %d", vm.ac2dc[PSU_0].ac2dc_power_limit);
          update_work_mode(0, 5, false);
        }
        mg_event_x("AC2DC bottom fail on %d", vm.ac2dc[PSU_1].ac2dc_power_limit);
        i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
        exit_nicely(4,"AC2DC fail 2"); 
      }
    } else {
      mg_event("bottom i2c miss");
    }
  }
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);  


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
     DBG( DBG_SCALING ,"PowerIn: %d\n", ac2dc->ac2dc_in_power);
   } else {
     //ac2dc->ac2dc_in_power = 0;
     DBG( DBG_SCALING ,"PowerIn: Error\n", ac2dc->ac2dc_in_power);
   }
   
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);  
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);      
   p = i2c_read_word(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_POUT_WORD, &err);
   if (!err) {
      int pp = i2c_read_byte(mgmt_addr[ac2dc->ac2dc_type], AC2DC_I2C_READ_STATUS_IOUT, &err);
      DBG( DBG_SCALING ,"PSU STAT:%x\n", pp);
      ac2dc->ac2dc_power_last_last = ac2dc->ac2dc_power_last;
      ac2dc->ac2dc_power_last = ac2dc->ac2dc_power_now;      
      ac2dc->ac2dc_power_now = i2c_getint(p); 
      ac2dc->ac2dc_power = MAX(ac2dc->ac2dc_power_now, ac2dc->ac2dc_power_last);
      ac2dc->ac2dc_power = MAX(ac2dc->ac2dc_power, ac2dc->ac2dc_power_last_last);        

      DBG( DBG_SCALING ,"PowerOut: R:%d PM:%d [PL:%d PLL:%d PLLL:%d]\n", 
               p,
               ac2dc->ac2dc_power,
               ac2dc->ac2dc_power_now,
               ac2dc->ac2dc_power_last,
               ac2dc->ac2dc_power_last_last);
   } else {
     DBG( DBG_SCALING ,"PowerOut: Error\n", ac2dc->ac2dc_power);
   }
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);  
//   i2c_write(PRIMARY_I2C_SWITCH, i2c_switch);      
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);  
   
   int usecs = end_stopper(&tv,NULL);
   if ((usecs > 500000)) {
     mg_event_x("Bad PSU FW on PSU %d", psu_id);  
     exit_nicely(0,"Bad PSU Firmware");
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
      ac2dc->ac2dc_power = MAX(ac2dc->ac2dc_power_now, ac2dc->ac2dc_power_last);
      ac2dc->ac2dc_power = MAX(ac2dc->ac2dc_power, ac2dc->ac2dc_power_last_last);
      if (!ac2dc->force_generic_psu) {
        
        if (ac2dc_check_connected(psu_id)) {
          exit_nicely(10,"AC2DC connected");
        }
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

	int * cmd_code_arr = ON ? revive_code_ON : revive_code;

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
	usleep(2000 * 1000);
	PSU12vON(psu );
	return;
}
void PSU12vPowerCycleALL (){

	for (int psu = 0 ; psu < PSU_COUNT ; psu++){
		PSU12vOFF(psu );
	}

	usleep(2000 * 1000);

	for (int psu = 0 ; psu < PSU_COUNT ; psu++){
		PSU12vON(psu );
	}

	return;
}
#endif

