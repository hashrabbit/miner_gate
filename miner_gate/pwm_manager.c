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
 

#include "pwm_manager.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <spond_debug.h>
#include "asic.h"
#include "asic_lib.h"
#include "pthread.h"

extern pthread_mutex_t i2cm;




// XX in percent - 0 to 100
// #define PWM_VALUE(XX)  (12000+XX*(320-120)) 
#define PWM_VALUE(XX)  (12000+XX*(250))  // TODO - can go up to 250
//int pwm_values[] = { 12000 , 22000, 32000 };
void init_pwm() {
  // cd /sys/devices/bone_capemgr.*

  FILE *f;
  f = fopen("/sys/devices/bone_capemgr.9/slots", "w");
  fprintf(f, "am33xx_pwm");
  fclose(f);
  f = fopen("/sys/devices/bone_capemgr.9/slots", "w");
  fprintf(f, "bone_pwm_P9_31");
  fclose(f);

  f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.12/period", "w");
  passert(f != NULL);
  fprintf(f, "40000");
  fclose(f);
  // pwm_values
  f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.12/duty", "w");
  passert(f != NULL);
  fprintf(f, "%d", PWM_VALUE(0));
  fclose(f);
  set_fan_level(40);
}

void kill_fan() {
  FILE *f;
  int val = 0;
  f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.12/duty", "w");
  if (f <= 0) {
    psyslog(RED "Fan PWM not found XXX\n" RESET);
    passert(0);
  } else {
    fprintf(f, "%d", val);
    fclose(f);
  }
}


void set_fan_level(int fan_level) {
  if (vm.fan_level != fan_level) {
    FILE *f;    
	  passert(fan_level <= 100 && fan_level >=0);
	  int val = PWM_VALUE(fan_level);
	  f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.12/duty", "w");
	  if (f <= 0) {
	    psyslog(RED "Fan PWM not found YYY\n" RESET);
      passert(0);
	  } else {
	    fprintf(f, "%d", val);
	    fclose(f);
	    vm.fan_level = fan_level;
	  }
  	}
}












//// LEDS



static LIGHT_MODE lm_green;
static LIGHT_MODE lm_yellow;
int green_on;
int yellow_on;





// 2 lamps - 0=green, 1=yellow
void set_light_on_off(int light_id, int on) {
  LIGHT_MODE* lm;
  int* ls;
  if (light_id==LIGHT_YELLOW) {
   ls = &yellow_on;
  } else {
   ls = &green_on;
  }
  
  if (*ls == on) {
    return;
  }

  static FILE *fy = NULL;
  static FILE *fg = NULL;  
  
  FILE *f;
  if (light_id==LIGHT_YELLOW) {
    if (fy == NULL) {
  	  fy = fopen("/sys/class/gpio/gpio22/value", "w");
    }
    f = fy;
  } else {
     if (fg == NULL) {
  	  fg = fopen("/sys/class/gpio/gpio51/value", "w");
    }
    f = fg;
  }
  passert(f);
  fprintf(f, (on)?"1":"0");
  *ls = on;
  fflush(f);
  //fclose(f);
}


void leds_periodic_100_msecond_led(int light_id,int counter) {
  LIGHT_MODE* lm = (light_id==LIGHT_GREEN)?&lm_green:&lm_yellow;
  int* l_on = (light_id==LIGHT_GREEN)?&green_on:&yellow_on;

  if (*lm == LIGHT_MODE_FAST_BLINK) {
    set_light_on_off(light_id, (counter)%2);
  } else if (*lm == LIGHT_MODE_SLOW_BLINK) {
    set_light_on_off(light_id, (counter/10)%2);
  }
}



void leds_periodic_100_msecond() {
   static int counter=0;
   counter++;
  leds_periodic_100_msecond_led(LIGHT_GREEN,counter);
  //leds_periodic_100_msecond_led(LIGHT_YELLOW,counter);
}





// 2 lamps - 0=green, 1=yellow
void set_light(int light_id, LIGHT_MODE m) {

//  echo 1 > /sys/class/gpio/gpio22/value
  LIGHT_MODE* lm;
  int* ls;

  if (light_id==LIGHT_YELLOW) {
    passert(0);
    lm = &lm_yellow;
    ls = &yellow_on;
  } else {
    lm = &lm_green;
    ls = &green_on;
  }
 

  if (*lm != m) {
    *lm = m;
    if (m == LIGHT_MODE_ON) {
  	  set_light_on_off(light_id, true);
    } else if (m == LIGHT_MODE_OFF) {
      set_light_on_off(light_id, false); 
    }
  }
}




void leds_init() {
  FILE *f;
  f = fopen("/sys/class/gpio/export", "w");
  fprintf(f, "22");
  fclose(f);
  f = fopen("/sys/class/gpio/gpio22/direction", "w");
  fprintf(f, "out");
  fclose(f);
  f = fopen("/sys/class/gpio/export", "w");
  fprintf(f, "51");
  fclose(f);
  f = fopen("/sys/class/gpio/gpio51/direction", "w");
  fprintf(f, "out");
  fclose(f);
}




