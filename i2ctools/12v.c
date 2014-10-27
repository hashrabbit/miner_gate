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

///
///simple util that reads AC2DC VPD info
///and prints it out.
///

//#define drpintf printf
#include <stdio.h>


#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
//#include "hammer.h"
#include <spond_debug.h>
#include "asic.h"
#include "squid.h"
#include "i2c.h"
#include "ac2dc_const.h"
#include "ac2dc.h"
#include "dc2dc.h"

static int mgmt_addr[4] = {0, AC2DC_MURATA_NEW_I2C_MGMT_DEVICE, 0, AC2DC_EMERSON_1200_I2C_MGMT_DEVICE};
extern pthread_mutex_t i2c_mutex;
static int eeprom_addr[4] = {0, AC2DC_MURATA_NEW_I2C_EEPROM_DEVICE, 0, AC2DC_EMERSON_1200_I2C_EEPROM_DEVICE};
static int revive_code[4] = {0, 0x0, 0x0, 0x40};
static int revive_code_ON[4] = {0, 0x80, 0x80, 0x80};
static int psu_addr[PSU_COUNT]  = {PRIMARY_I2C_SWITCH_AC2DC_PSU_0_PIN, PRIMARY_I2C_SWITCH_AC2DC_PSU_1_PIN};


using namespace std;
//pthread_mutex_t network_hw_mutex = PTHREAD_MUTEX_INITIALIZER;

int usage(char * app ,int exCode ,const char * errMsg = NULL)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }
    
    printf ("Usage: %s [[-cycle|-c]|-on|-off] [-top|-bottom|-both]\n\n" , app);

    printf ("       -c : power cycle\n");
    printf ("       -on : power on\n");
    printf ("       -off : power off\n");
    printf ("       -top : top PSU\n");
    printf ("       -bottom : bottom PSU (default)\n");
    printf ("       -both : both top & bottom PSUs\n");

    if (0 == exCode) // exCode ==0 means - just print usage and get back to app for business. other value - exit with it.
    {
        return 0;
    }
    else 
    {
        exit(exCode);
    }
}


static void myPSU12vONOFF (int psu , AC2DC* ac2dc, bool ON){
	int err = 0;

	int * cmd_code_arr = ON ? revive_code_ON : revive_code;

	if (psu >= PSU_COUNT || psu < 0)
	{
		psyslog("Wrong PSU number %d in PSU12vONOFF\n",psu);
		return;
	}

	 pthread_mutex_lock(&i2c_mutex);

	int _type = ac2dc[psu].ac2dc_type;
	printf ("I2C_WRITE(0x%X , 0x%X )\n" , PRIMARY_I2C_SWITCH,psu_addr[psu]);
	i2c_write(PRIMARY_I2C_SWITCH, psu_addr[psu]|PRIMARY_I2C_SWITCH_DEAULT , &err);

	usleep(5000);

	printf ("I2C_WRITE_BYTE(0x%X , 0x%X , 0x%X ) \n" , mgmt_addr[_type], AC2DC_I2C_WRITE_PROTECT , 0x00 );
	i2c_write_byte(mgmt_addr[_type], AC2DC_I2C_WRITE_PROTECT , 0x00 , &err);


	usleep(5000);

	printf ("I2C_WRITE_BYTE(0x%X , 0x%X , 0x%X ) \n" , mgmt_addr[_type], AC2DC_I2C_ON_OFF , cmd_code_arr[_type] );
	i2c_write_byte(mgmt_addr[_type], AC2DC_I2C_ON_OFF , cmd_code_arr[_type] , &err);


	usleep(5000);

	pthread_mutex_unlock(&i2c_mutex);

}

void myPSU12vON (int psu, AC2DC* ac2dc){

	myPSU12vONOFF(psu , ac2dc,true);
	return;
}


void myPSU12vOFF (int psu, AC2DC* ac2dc){

	myPSU12vONOFF(psu ,ac2dc ,false);
	return;
}

int main(int argc, char *argv[])
{
     int rc = 0;
     int rctemp=0;
     AC2DC ac2dc[2];

     bool callUsage = false;
     bool badParm = false;

     bool showTop = false;
     bool showBot = false;

     bool bOFF = false;
     bool bON = false;

  
  for (int i = 1 ; i < argc ; i++){

	  printf ("arg %d : %s \n",i,argv[i]);

	  if ( 0 == strcmp(argv[i],"-h")){
		  callUsage = true;
	  }

	  else if ( strcmp(argv[i],"-c")== 0 || strcmp(argv[i],"-cycle") == 0)
    {
		  	  bOFF = true;
		  	  bON = true;
    }

    else if ( 0 == strcmp(argv[i],"-on"))
        bON = true;
    else if ( 0 == strcmp(argv[i],"-off"))
        bOFF = true;
    else if ( 0 == strcmp(argv[i],"-top"))
        showTop = true;
     else if ( 0 == strcmp(argv[i],"-bottom"))
       showBot = true;
     else if ( 0 == strcmp(argv[i],"-both")){
    	 showBot = true;
    	 showTop = true;
     }
      else
      badParm = true;
  }


  // default is showBottom

  if ( showTop == false && showBot == false)
	  showBot = true;


  if(badParm)
  {
	  usage(argv[0],1,"Bad arguments");
  }



  if (callUsage)
	  return usage(argv[0] , 0);

  printf ("%s  %s  %s %s\n", showBot ? "BOTTOM , ":"" , showTop ? "TOP , ":"" , bOFF?" OFF":"" , bON?" , ON.":".");

  i2c_init();
  ac2dc_init2(ac2dc);

  if (bOFF)
  {
	if (showTop)
		myPSU12vOFF(PSU_0 , ac2dc);
	if(showBot)
		myPSU12vOFF(PSU_1 , ac2dc);
  }

  if (bOFF && bON){
	  sleep (2);
  }

  if (bON)
  {
		if (showTop)
			myPSU12vON(PSU_0 , ac2dc);
		if(showBot)
			myPSU12vON(PSU_1 , ac2dc);

  }
  return rc;
}
