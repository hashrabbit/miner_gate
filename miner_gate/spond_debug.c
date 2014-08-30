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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <execinfo.h>

#include <linux/types.h>
#include <time.h>
#include <syslog.h>
#include <spond_debug.h>
#include <ctime>
#include <unistd.h>


char glob_buf_x[700];


#define SIZE 100
#ifdef MINERGATE
void exit_nicely(int seconds_sleep_before_exit, const char* p);
#endif

void print_stack() {
	int j, nptrs;
	 void *buffer[SIZE];
	 char **strings;
	 psyslog("STACK:\n");
	
	 nptrs = backtrace(buffer, SIZE);
	 strings = backtrace_symbols(buffer, nptrs);
	 for (j = 0; j < nptrs; j++) {
	   psyslog(" %s\n", strings[j]);
	 }

}


void _pabort(const char *s) {
	 if (s) {
	   //perror(orig_buf);
	   psyslog("PABORT (%s)", s);
	 }
  print_stack();
#ifdef MINERGATE  
  psyslog( "EXIT::: !!!!!  pAbort\n" );
  exit_nicely(3,"Abort");
#else
  exit(0);
#endif
}

void _passert(int cond, const char *s) {
  if (!cond) {
    pabort(s);
  }
}

void start_stopper(struct timeval *tv) {gettimeofday(tv, NULL);}

int end_stopper(struct timeval *tv, const char *name) {
  int usec;
  struct timeval end;
  gettimeofday(&end, NULL);
  usec = (end.tv_sec - tv->tv_sec) * 1000000;
  usec += (end.tv_usec - tv->tv_usec);
  *tv = end;
  if (name) {
    psyslog("%s took %d usec\n", name, usec);
  }
  return usec;
}



void mg_event(const char *s) {
   time_t rawtime;
   struct tm * timeinfo;
   char buffer[80];
  psyslog("Critical: %s\n", s);
   time (&rawtime);
   timeinfo = localtime(&rawtime);
  
   strftime(buffer,80,"%d/%m %H:%M:%S",timeinfo);


  
	  FILE *f = fopen("/tmp/mg_event", "a"); 
    if (!f) {
      psyslog("Failed to create watchdog file\n");
      return;
    }

    fseek(f, 0, SEEK_END); // seek to end of file
    int size = ftell(f); // get current file pointer
    fseek(f, 0, SEEK_SET); // seek back to beginning of file
    if (size > 3000) {
       fclose(f);
       unlink("/tmp/mg_event");
       f = fopen("/tmp/mg_event", "a");
    }    
    fprintf(f, "%s:%s\n", buffer, s);
    fclose(f);

    f = fopen("/tmp/mg_event_log", "a");
    if (!f) {
      psyslog("Failed to create watchdog log file\n");
      return;
    }
    fseek(f, 0, SEEK_END); // seek to end of file
    size = ftell(f); // get current file pointer
    fseek(f, 0, SEEK_SET); // seek back to beginning of file
    if (size > 100000) {
      fclose(f);
      unlink("/tmp/mg_event_log");
      f = fopen("/tmp/mg_event_log", "a");
    }    
    fprintf(f, "%s:%s\n", buffer, s);
    fclose(f);
}

void mg_status(const char *s) {
	FILE *f = fopen("/tmp/mg_status", "w");
    if (!f) {
      psyslog("Failed to create watchdog file\n");
      return;
    }
    fprintf(f, "%s\n", s);
    fclose(f);
}



