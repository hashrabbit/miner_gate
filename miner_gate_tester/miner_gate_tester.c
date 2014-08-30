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

#include "mg_proto_parser_sp30.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <time.h> 
#include "squid.h"
#include "spond_debug.h"


int leading_zeroes  = 0x20;
int jobs_per_time   = 50;
int jobs_period   = 50;
int wins = 0;
int req = 0;
int done = 0;
int not_done = 0;
int first_run = 1;


void send_minergate_pkt(const minergate_req_packet_sp30* mp_req, 
            minergate_rsp_packet_sp30* mp_rsp,
            int  socket_fd) {
  int   nbytes;    
  write(socket_fd, (const void*)mp_req, sizeof(minergate_req_packet_sp30));
  nbytes = read(socket_fd, (void*)mp_rsp, sizeof(minergate_rsp_packet_sp30));  
  passert(nbytes > 0);
 // printf("got %d(%d) bytes\n",mp_rsp->data_length, nbytes);
  passert(mp_rsp->magic == 0xcaf4);
}


int init_socket() {
  struct sockaddr_un address;
  int  socket_fd ;
  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
   if(socket_fd < 0)
   {
    psyslog("socket() failed\n");
    perror("Err:");
    return 0;
   }
   
   /* start with a clean address structure */
   memset(&address, 0, sizeof(struct sockaddr_un));
   
   address.sun_family = AF_UNIX;
   sprintf(address.sun_path, MINERGATE_SOCKET_FILE_SP30);
   
   if(connect(socket_fd, 
        (struct sockaddr *) &address, 
        sizeof(struct sockaddr_un)) != 0)
   {
    psyslog("connect() failed\n");
    perror("Err:");
    return 0;
   }

  return socket_fd;
}





void fill_random_work2(minergate_do_job_req_sp30* work) {
  static int id = 1;
  id++;
  memset(work, 0, sizeof(minergate_do_job_req_sp30));
  work->ntime_limit = 60;
  //work->
#if 1
  work->midstate[0] = (rand()<<16) + rand()+rand();
  work->midstate[1] = (rand()<<16) + rand();
  work->midstate[2] = (rand()<<16) + rand();
  work->midstate[3] = (rand()<<16) + rand();
  work->midstate[4] = (rand()<<16) + rand();
  work->midstate[5] = (rand()<<16) + rand();
  work->midstate[6] = (rand()<<16) + rand();
  work->midstate[7] = (rand()<<16) + rand();
  work->work_id_in_sw = id%0xFFFF;
  work->mrkle_root = (rand()<<16) + rand();
  work->leading_zeroes = leading_zeroes;
  work->timestamp  = (rand()<<16) + rand();
  work->difficulty = (rand()<<16) + rand();
#else 
  work->midstate[0] = 0xBC909A33;
  work->midstate[1] = 0x6358BFF0;
  work->midstate[2] = 0x90CCAC7D;
  work->midstate[3] = 0x1E59CAA8;
  work->midstate[4] = 0xC3C8D8E9;
  work->midstate[5] = 0x4F0103C8;
  work->midstate[6] = 0x96B18736;
  work->midstate[7] = 0x4719F91B;
  work->work_id_in_sw = id%0xFFFF;
  work->mrkle_root = 0x4B1E5E4A;
  work->leading_zeroes = leading_zeroes;
  work->timestamp  = 0x29AB5F49;
  work->difficulty = 0xFFFF001D;
#endif
  //work_in_queue->winner_nonce
}


int main(int argc, char* argv[])
{
 if (argc == 4) {
    leading_zeroes = atoi(argv[1]);
    jobs_per_time= atoi(argv[2]);    
    jobs_period= atoi(argv[3]);        
 } else {
    printf("Usage: %s <leading zeroes> <jobs per send> <send period milli>\n", argv[0]);
    return 0;
 }
 

    
 int  socket_fd = init_socket();
 passert(socket_fd != 0);

 
 
 minergate_req_packet_sp30* mp_req = allocate_minergate_packet_req_sp30(0xca, 0xfe);
 minergate_rsp_packet_sp30* mp_rsp = allocate_minergate_packet_rsp_sp30(0xca, 0xfe);
 int i;
 int requests = 0;
 int responces = 0;
 int rate = 0;
 
 //nbytes = snprintf(buffer, 256, "hello from a client");
 //srand (time(NULL));
 int start_time = time(NULL);
 while (1) {
    if ((time(NULL) - start_time == 5) && (first_run == 1)) {
      first_run = 0;
      start_time = time(NULL);
      wins = 0;
      printf("NULLIFY stats\n");
    }
  
    passert(jobs_per_time <= MAX_REQUESTS_SP30);
    for (i = 0; i < (jobs_per_time); i++) {
        req++;
        //printf("L %d %d\n", jobs_per_time, jobs_period);
        minergate_do_job_req_sp30* p = mp_req->req+i;
        fill_random_work2(p);
        p->ntime_limit = 100;
        requests++;
        //printf("DIFFFFFFF %d\n", p->leading_zeroes);
     }
     mp_req->req_count = jobs_per_time;
     //printf("MESSAGE TO SERVER: %x\n", mp_req->request_id);
     send_minergate_pkt(mp_req,  mp_rsp, socket_fd);
     //printf("MESSAGE FROM SERVER: %x\n", mp_rsp->request_id);

   
    //DBG(DBG_NET, "GOT minergate_do_job_req_sp30: %x/%x\n", sizeof(minergate_do_job_req_sp30), md->data_length);
    int array_size = mp_rsp->rsp_count;
    int i;
    for (i = 0; i < array_size; i++) { // walk the jobs
       responces++;
       minergate_do_job_rsp_sp30* work = mp_rsp->rsp+i;
       if (work->job_complete) {
         done++;
       } else {
         not_done++;

       }
       
       if (work->winner_nonce) {
         wins++;
  //     printf("!!!GOT minergate job rsp %08x %08x\n",work->work_id_in_sw,work->winner_nonce);
         int job_difficulty = 1<<(leading_zeroes-20); // in 1MH units
         rate += job_difficulty;
       }
    }

   
   //printf("REQUESTS: %d RESPONCES: %d, DELTA: %d\n",requests, responces, requests - responces);
   usleep(jobs_period*1000);
   static int last_time = 0;
   int t = time(NULL);
   static int counter = 0;
   counter++;

   // Show rate each X seconds 
   if ((counter%((1000/jobs_period))) == 0) {
      int global_rate_cnt=time(NULL) - start_time;
      printf(MAGENTA "HASH RATE=%4dGH (TOTAL=%fGH) [REQ %d DONE %d/%d ]\n" RESET,
        rate/1000,
        (((1.0*(float)wins*(1<<(leading_zeroes-20)))/(float)global_rate_cnt))/1000, 
        req, done, not_done);
      rate=0;
      last_time = t;
   }
   if ((counter%((60*1000/jobs_period))) == 0) {
      static int last_wins = 0;
       printf(MAGENTA "LASTMIN=%fMH\n" RESET,
        ((1.0*(float)(wins - last_wins)*(1<<(leading_zeroes-20)))/(float)60),
        req, done, not_done);
      last_wins = wins;
   }
 }
 close(socket_fd);
 free(mp_req);
 return 0;
}

/*

int main(int argc, char *argv[]) {
  minergate_packet *mp = allocate_minergate_packet(1000, 1, 2);
  minergate_data* md1 =  get_minergate_data(mp,  100, 1);
  minergate_data* md2 =  get_minergate_data(mp,  200, 2);
  minergate_data* md3 =  get_minergate_data(mp,  300, 3);
  printf("Got:\n");

  parse_minergate_packet(mp, minergate_data_processor);

  return 0;
}
*/


