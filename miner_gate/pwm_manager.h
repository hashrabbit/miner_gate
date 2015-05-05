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

#ifndef _____PWMLIB_H__B__
#define _____PWMLIB_H__B__

#include <stdint.h>
#include <unistd.h>
#include <defines.h>

//#include "dc2dc.h"
// DONT USE UNLESS YOU KNOW WHAT YOU DOING
void kill_fan();
void init_pwm();
void set_fan_level(int fan_level_percent);



#define LIGHT_GREEN  0
#define LIGHT_YELLOW 1

typedef enum {
  LIGHT_MODE_OFF = 0,
  LIGHT_MODE_ON = 1,
  LIGHT_MODE_SLOW_BLINK = 2,
  LIGHT_MODE_FAST_BLINK = 3,
} LIGHT_MODE;


void set_light(int light_id, LIGHT_MODE m);
void leds_periodic_100_msecond();
// Low level function
void set_light_on_off(int light_id, int on);


#endif
