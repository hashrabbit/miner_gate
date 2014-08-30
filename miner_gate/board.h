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

#ifndef _____BOARD_45R_H____
#define _____BOARD_45R_H____


#include "defines.h"
#include <stdint.h>
#include <unistd.h>


int get_mng_board_temp();
int get_top_board_temp();
int get_bottom_board_temp();


#endif
