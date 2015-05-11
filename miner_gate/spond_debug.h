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

#ifndef ___SPOND_DBG____
#define ___SPOND_DBG____

#include <assert.h>
#include <syslog.h>
#include <sys/time.h>

#define DBG_TMP 0
#define DBG_NET 0
#define DBG_WINS 0
#define DBG_HW 0
#define DBG_SCALING 0
#define DBG_SCALING_TMP 0
#define DBG_SCALING_INIT 0
#define DBG_SCALING_HZ 0
#define DBG_SCALING_BIST 0
#define DBG_SCALING_AC2DC 0
#define DBG_SCALING_UP 0
#define DBG_VTRIM 0
#define DBG_DC2DC 0


#define DBG_POWER 0
#define DBG_TEMP 0

#define DBG_HASH 0
#define DBG_I2C 0
#define DBG_IDLE 0
#define DBG_BIST 0
#define DBG_REGS 0

#define DBG_FAN 0


#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0;0m"


#define RED_BK "\x1b[41m"
#define GREEN_BK "\x1b[42m"
#define YELLOW_BK "\x1b[43m"
#define BLUE_BK "\x1b[44m"
#define MAGENTA_BK "\x1b[45m"
#define CYAN_BK "\x1b[46m"
#define WHITE_BK "\x1b[47m"

// Bold format is 1;
#define RED_BOLD "\x1b[1;31m"
#define GREEN_BOLD "\x1b[1;32m"
#define YELLOW_BOLD "\x1b[1;33m"
#define BLUE_BOLD "\x1b[1;34m"
#define MAGENTA_BOLD "\x1b[1;35m"
#define CYAN_BOLD "\x1b[1;36m"
#define WHITE_BOLD "\x1b[1;37m"

/*
#define RED ""
#define GREEN ""
#define YELLOW ""
#define BLUE ""
#define MAGENTA ""
#define CYAN ""
#define RESET ""
#define RED_BK ""
#define GREEN_BK ""
#define YELLOW_BK ""
#define BLUE_BK ""
#define MAGENTA_BK ""
#define CYAN_BK ""
#define WHITE_BK ""
#define RED_BOLD ""
#define GREEN_BOLD ""
#define YELLOW_BOLD ""
#define BLUE_BOLD ""
#define MAGENTA_BOLD ""
#define CYAN_BOLD ""
#define WHITE_BOLD ""
*/

#define DBG(DBG_TYPE, STR...)                                                  \
  if (DBG_TYPE) {                                                              \
    psyslog(STR);                                                               \
  }
int print_time_delta();

#define passert(X...)                                                          \
  {                                                                            \
    if ((X) == 0) {                                                              \
      psyslog("FATAL: %s:%d\n", __FILE__, __LINE__);                            \
      mg_event_x("FATAL: %s:%d", __FILE__, __LINE__);                       \
      _passert(0);                                                             \
    }                                                                          \
  }
#define pabort(X...)                                                           \
  { _pabort(X); }

#ifdef MINERGATE
#define psyslog(X...)                                                          \
  {                                                                            \
    if (!vm.dont_psyslog) {                                                    \
      print_time_delta();                                                        \
      syslog(LOG_INFO, X);                                                    \
      printf(X);                                                                 \
    }                                                  \
  }
#else     
#define psyslog(X...)                                                          \
  {                                                                            \
    print_time_delta();                                                        \
    syslog(LOG_INFO, X);                                                    \
    printf(X);                                                                 \
  }
#endif      


void _passert(int cond, const char *s = NULL);
void _pabort(const char *s);
void print_stack();

void start_stopper(struct timeval *tv);
int end_stopper(struct timeval *tv, const char *name);
extern char glob_buf_x[700];
#define mg_event_x(S...) { sprintf(glob_buf_x, S); mg_event(glob_buf_x, 1);} 
#define mg_event_x_nonl(S...) { sprintf(glob_buf_x, S); mg_event(glob_buf_x, 0);} 


void mg_event(const char *s, int nl=1);
void mg_status(const char *s);


#endif
