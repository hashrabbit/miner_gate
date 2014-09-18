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



/*
    Minergate server network interface.
    Multithreaded application, listens on socket and handles requests.
    Pushes mining requests from network to lock and returns
    asynchroniously responces from work_minergate_rsp.

    Each network connection has adaptor structure, that holds all the
    data needed for the connection - last packet with request,
    next packet with responce etc`.


    by Zvisha Shteingart
*/
#include "defines.h"
#include "mg_proto_parser_sp30.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h> //for sockets
#include <netinet/in.h> //for internet
#include <pthread.h>
#include "asic.h"
#include <queue>
#include "spond_debug.h"
#include "squid.h"
#include "asic_lib.h"
#include "miner_gate.h"
#include "pll.h"
#include <syslog.h>
#include "real_time_queue.h"
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <ac2dc.h>
#include <ac2dc_const.h>
#include <unistd.h>
#include <dc2dc.h>
#include <board.h>
#include <pwm_manager.h>
#include <i2c.h>


using namespace std;
pthread_mutex_t network_hw_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sigaction termhandler, inthandler;
extern int rt_queue_size;
int socket_fd = 0;

// SERVER
void compute_hash(const unsigned char *midstate, unsigned int mrkle_root,
                  unsigned int timestamp, unsigned int difficulty,
                  unsigned int winner_nonce, unsigned char *hash);
int get_leading_zeroes(const unsigned char *hash);
void memprint(const void *m, size_t n);

typedef class {
public:
  uint8_t adapter_id;
  // uint8_t  adapter_id;
  int connection_fd;
  pthread_t conn_pth;
  minergate_req_packet_sp30 *last_req;
  minergate_rsp_packet_sp30 *next_rsp;

  // pthread_mutex_t queue_lock;
  queue<minergate_do_job_req_sp30> work_minergate_req;
  queue<minergate_do_job_rsp_sp30> work_minergate_rsp;

  void minergate_adapter() {}
} minergate_adapter;

minergate_adapter *adapters[0x100] = { 0 };
int kill_app = 0;


extern void save_rate_temp(int back_tmp, int back_tmp_b, int front_tmp, int total_mhash);

const char *last_err_loop;
int test_serial(int loopid) {
  assert_serial_failures = false;
  int i = 0;
  int val1;

  /*if (loopid == 5 || loopid == 6) {
    last_err_loop = "fake bad"; 
    return 0;
  }*/


  while ((val1 = read_spi(ADDR_SQUID_STATUS)) &
         BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) {
    read_spi(ADDR_SQUID_SERIAL_READ);
    i++;
    if (i > 700) {
      // parse_squid_status(val1);
      // psyslog("ERR (%d) loop is too noisy: ADDR_SQUID_STATUS=%x,
      // ADDR_SQUID_SERIAL_READ=%x\n",i, val1, val2);
      psyslog("Loop NOISE\n");
      last_err_loop = "noise on loop";
      return 0;
    }
    usleep(10);
  }

  // psyslog("Ok %d!\n", i);

  // test for connectivity
  int ver = read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_VERSION);

  // printf("XTesting loop, version = %x\n", ver);
  if (BROADCAST_READ_DATA(ver) != 0x46) {
    // printf("Failed: Got version: %x!\n", ver);
    psyslog("Loop BAD DATA (%x) on %d\n",ver,loopid);
    last_err_loop = "no data on loop";
    return 0;
  }

  assert_serial_failures = true;
  return 1;
}


extern pthread_mutex_t i2c_mutex;
extern pthread_mutex_t i2cm;
void store_last_voltage();
void update_dc2dc_stats(int i);

void exit_nicely(int seconds_sleep_before_exit, const char* why) {
  int i, err2;
  set_light(LIGHT_GREEN, LIGHT_MODE_OFF);
  leds_periodic_100_msecond();
  set_light_on_off(LIGHT_GREEN, LIGHT_MODE_OFF);
  //execl("/usr/local/bin/kill_cgminer", "/usr/local/bin/kill_cgminer", NULL);
  psyslog("Closing socket %d\n", socket_fd);
  close(socket_fd);
  save_rate_temp(0,0,0,0);  
  mg_event(why);
  i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0, &err2);    
#ifndef SP2x
  i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0, &err2);    
#endif
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
  err2 = 0;

/*
  if ((time(NULL) - vm.start_run_time) > 20) {
    // if over minute - store voltages
    store_last_voltage();
  }
*/
  
  if (vm.exiting) {
    psyslog("Recursive exit (%s)!\n", why);
    set_fan_level(30);
    exit(0);
  }
  usleep(100*1000);
  pthread_mutex_unlock(&i2cm);
  pthread_mutex_unlock(&i2c_mutex);    
  vm.exiting = 1;
  kill_app = 1;
  // Let other threads finish.
  //set_light(LIGHT_YELLOW, LIGHT_MODE_OFF);
  usleep(1000*1000);
  int ii = 0;
  psyslog("Here comes unexpected death, lights off!\n");

/*
  for (i = 0; i < ASICS_COUNT ; i++) {
    //psyslog("Test DC2DC %d\n", i);
    if (vm.asic[i].dc2dc.dc2dc_present) {
      //update_dc2dc_stats(i);
    }
  }
  */
  for (i = 0; i < ASICS_COUNT ; i++) {
    dc2dc_disable_dc2dc(i, &err2);
  }
  psyslog("Dye (%s)!\n", why);
  //set_light(LIGHT_GREEN, LIGHT_MODE_OFF);
  usleep(seconds_sleep_before_exit*1000*1000);
  set_fan_level(20);  
  set_light_on_off(LIGHT_GREEN, LIGHT_MODE_OFF);
  exit(0);
}


static void sighandler(int sig)
{
  /* Restore signal handlers so we can still quit if kill_work fails */
  sigaction(SIGTERM, &termhandler, NULL);
  sigaction(SIGINT, &inthandler, NULL);
  psyslog( "EXIT::: got signal !\n" );
  exit_nicely(1,"Signal");
}


void print_adapter(FILE *f, bool b_syslog) {
  minergate_adapter *a = adapters[0];
  if (b_syslog && a) {
    psyslog("Adapter queues: rsp=%d, req=%d\n",
            a->work_minergate_rsp.size(),
            a->work_minergate_req.size());
  }

  if (f != NULL) {
    if (a) {
      fprintf(f, "Adapter queues: rsp=%d, req=%d\n",
              a->work_minergate_rsp.size(),
             a->work_minergate_req.size());
      
    } else {
      fprintf(f, "No Adapter\n");
    }
  }
}

void free_minergate_adapter(minergate_adapter *adapter) {
  close(adapter->connection_fd);
  free(adapter->last_req);
  free(adapter->next_rsp);
  delete adapter;
}

int SWAP32(int x) {
  return ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) |
          (((x) & 0xff0000) >> 8) | (((x) >> 24) & 0xff));
}

//
// Return the winner (or not winner) job back to the addapter queue.
//
void push_work_rsp(RT_JOB *work, int job_complete) {
  pthread_mutex_lock(&network_hw_mutex);
  minergate_do_job_rsp_sp30 r;
  uint8_t adapter_id = work->adapter_id;
  minergate_adapter *adapter = adapters[adapter_id];
  DBG(DBG_NET, "Packet to adapter\n");
  if (!adapter) {
    DBG(DBG_NET, "Adapter disconected! Packet to garbage\n");
    pthread_mutex_unlock(&network_hw_mutex);
    return;
  }
  r.mrkle_root = work->mrkle_root;
  r.winner_nonce = work->winner_nonce;
  r.work_id_in_sw = work->work_id_in_sw;
  r.ntime_offset = work->ntime_offset;
  r.job_complete = job_complete;
  r.res = 0;
  adapter->work_minergate_rsp.push(r);
  //passert(adapter->work_minergate_rsp.size() <= MINERGATE_ADAPTER_QUEUE_SP30 * 2);
  pthread_mutex_unlock(&network_hw_mutex);
}

//
// returns success, fills W with new job from adapter queue
//
int pull_work_req_adapter(RT_JOB *w, minergate_adapter *adapter) {
  minergate_do_job_req_sp30 r;

  if (!adapter->work_minergate_req.empty()) {
    r = adapter->work_minergate_req.front();
    adapter->work_minergate_req.pop();
    w->difficulty = r.difficulty;
    memcpy(w->midstate, r.midstate, sizeof(r.midstate));
    w->adapter_id = adapter->adapter_id;
    w->mrkle_root = r.mrkle_root;
    w->timestamp = r.timestamp;
    w->winner_nonce = 0;
    w->work_id_in_sw = r.work_id_in_sw;
    w->work_state = 0;
    w->leading_zeroes = r.leading_zeroes;
    w->ntime_max = r.ntime_limit;
    w->ntime_offset = 0;
    passert(w->ntime_max >= MQ_INCREMENTS);
    return 1;
  }
  return 0;
}
/*
// returns success
int has_work_req_adapter(minergate_adapter *adapter) {
  return (adapter->work_minergate_req.size());
}
*/

// returns success
int pull_work_req(RT_JOB *w) {
  // go over adapters...
  // TODO
  pthread_mutex_lock(&network_hw_mutex);
  int ret = false;
  minergate_adapter *adapter = adapters[0];
  if (adapter) {
      ret =pull_work_req_adapter(w, adapter);
  }
  pthread_mutex_unlock(&network_hw_mutex);
  return ret;
}
/*
int has_work_req() {
  pthread_mutex_lock(&network_hw_mutex);
  minergate_adapter *adapter = adapters[0];
  if (adapter) {
    has_work_req_adapter(adapter);
  }
  pthread_mutex_unlock(&network_hw_mutex);
}
*/
void push_work_req(minergate_do_job_req_sp30 *req, minergate_adapter *adapter) {
  pthread_mutex_lock(&network_hw_mutex);
  if (adapter->work_minergate_req.size() >= (MINERGATE_ADAPTER_QUEUE_SP30 - 2)) {
    minergate_do_job_rsp_sp30 rsp;
    rsp.mrkle_root = req->mrkle_root;
    rsp.winner_nonce = 0;
    rsp.ntime_offset = 0;
    rsp.work_id_in_sw = req->work_id_in_sw;
    rsp.job_complete = 1;
    rsp.res = 1;
    // printf("returning %d %d\n",req->work_id_in_sw,rsp.work_id_in_sw);
    adapter->work_minergate_rsp.push(rsp);
  } else {
    adapter->work_minergate_req.push(*req);
  }
  pthread_mutex_unlock(&network_hw_mutex);
}

// returns success
int pull_work_rsp(minergate_do_job_rsp_sp30 *r, minergate_adapter *adapter) {
  pthread_mutex_lock(&network_hw_mutex);
  if (!adapter->work_minergate_rsp.empty()) {
    *r = adapter->work_minergate_rsp.front();
    adapter->work_minergate_rsp.pop();
    pthread_mutex_unlock(&network_hw_mutex);
    return 1;
  }
  pthread_mutex_unlock(&network_hw_mutex);
  return 0;
}
extern pthread_mutex_t asic_mutex;
//
// Support new minergate client
//
void *connection_handler_thread(void *adptr) {
  psyslog("New adapter connected!\n");
  minergate_adapter *adapter = (minergate_adapter *)adptr;
  // DBG(DBG_NET,"connection_fd = %d\n", adapter->connection_fd);

  adapter->adapter_id = 0;
  adapters[0] = adapter;
  adapter->last_req = allocate_minergate_packet_req_sp30(0xca, 0xfe);
  adapter->next_rsp = allocate_minergate_packet_rsp_sp30(0xca, 0xfe);

  vm.solved_jobs_total = 0;


  RT_JOB work;

  while (one_done_sw_rt_queue(&work)) {
    //push_work_rsp(&work);
  }
  int nbytes;

  // Read packet
  struct timeval last_time;
  gettimeofday(&last_time, NULL);
  while ((nbytes = read(adapter->connection_fd, (void *)adapter->last_req,
                        sizeof(minergate_req_packet_sp30))) > 0) {
    struct timeval now;
    unsigned long long int usec;
    if (nbytes) {
      passert(adapter->last_req->protocol_version == MINERGATE_PROTOCOL_VERSION_SP30);
      passert(adapter->last_req->magic == 0xcaf4);
      gettimeofday(&now, NULL);

      usec = (now.tv_sec - last_time.tv_sec) * 1000000;
      usec += (now.tv_usec - last_time.tv_usec);

      pthread_mutex_lock(&asic_mutex);
      pthread_mutex_lock(&network_hw_mutex);
      vm.not_mining_time = 0;
      pthread_mutex_unlock(&network_hw_mutex);
      pthread_mutex_unlock(&asic_mutex);

      // Reset packet.
      int i;
      int array_size = adapter->last_req->req_count;
      for (i = 0; i < array_size; i++) { // walk the jobs
        minergate_do_job_req_sp30 *work = adapter->last_req->req + i;
        push_work_req(work, adapter);
      }


      if ((adapter->last_req->mask & 0x02)) {
         // Drop old requests
         int mqs = mq_size();
         //psyslog("%d ------------------->> Drop old requests = mq size:%d\n",usec_stamp(), mqs);
         
         // Cancel Adapter req queue queue         
         // Cancel HW queue
         pthread_mutex_lock(&asic_mutex);   
         while (one_done_sw_rt_queue(&work)) {
            //DBG(DBG_WINS,"CANCELED1::JOB ID HW:%d, SW:%d  ---\n",work.work_id_in_hw,work.work_id_in_sw);  
            push_work_rsp(&work,1);
         }   
         pthread_mutex_unlock(&asic_mutex);

         
         while (pull_work_req(&work)) {
           DBG(DBG_WINS,"CANCELED2::JOB ID HW:%d, SW:%d  ---\n",work.work_id_in_hw,work.work_id_in_sw);  
           push_work_rsp(&work, 1);
         }
         
         
         // Give "STOP" to FPGA
         write_spi(ADDR_SQUID_COMMAND, BIT_COMMAND_MQ_FIFO_RESET);
         write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_COMMAND, BIT_ADDR_COMMAND_END_CURRENT_JOB);
         write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_COMMAND, BIT_ADDR_COMMAND_END_CURRENT_JOB);  
         flush_spi_write();
       } else {
        int mqs = mq_size();
        //psyslog("%d ------------------>>Push OK:%d (%d pkts)\n",usec_stamp(), mqs, array_size);
       }



      
      // Return all previous responces
       int rsp_count = adapter->work_minergate_rsp.size();
       DBG(DBG_NET, "Sending %d minergate_do_job_rsp_sp30\n", rsp_count);
       if (rsp_count > MAX_RESPONDS_SP30) {
         rsp_count = MAX_RESPONDS_SP30;
       }
      
       for (i = 0; i < rsp_count; i++) {
         minergate_do_job_rsp_sp30 *rsp = adapter->next_rsp->rsp + i;
         int res = pull_work_rsp(rsp, adapter);
         passert(res);
       }
       adapter->next_rsp->rsp_count = rsp_count;
       int mhashes_done = (vm.total_mhash/1000)*(int)(usec/1000);
       adapter->next_rsp->gh_div_50_rate = ((mhashes_done/5)/930); // Don't know why
       //psyslog("mhash done=%d\n",adapter->next_rsp->gh_div_50_rate);
      // parse_minergate_packet(adapter->last_req, minergate_data_processor,
      // adapter, adapter);
      adapter->next_rsp->request_id = adapter->last_req->request_id;
      // Send response
      write(adapter->connection_fd, (void *)adapter->next_rsp,
            sizeof(minergate_rsp_packet_sp30));

      // Clear packet.
      adapter->next_rsp->rsp_count = 0;
      last_time = now;
    }
  }
  adapters[adapter->adapter_id] = NULL;
  free_minergate_adapter(adapter);
  // Clear the real_time_queue from the old packets
  adapter = NULL;
  return 0;
}

int init_socket() {
  struct sockaddr_un address;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    DBG(DBG_NET, "socket() failed\n");
    perror("Err:");
    return 0;
  }

  unlink(MINERGATE_SOCKET_FILE_SP30);

  /* start with a clean address structure */
  memset(&address, 0, sizeof(struct sockaddr_un));

  address.sun_family = AF_UNIX;
  sprintf(address.sun_path, MINERGATE_SOCKET_FILE_SP30);

  if (bind(socket_fd, (struct sockaddr *)&address,
           sizeof(struct sockaddr_un)) !=
      0) {
    perror("Err:");
    return 0;
  }

  if (listen(socket_fd, 5) != 0) {
    perror("Err:");
    return 0;
  }

  return socket_fd;
}


void test_asic_reset() {
  // Reset ASICs
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_GOT_ADDR, 0);

  // If someone not reseted (has address) - we have a problem
  int reg = read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT);
  if (reg == 0) {
    // Don't remove - used by tests
    printf("got reply from ASIC 0x%x\n", BROADCAST_READ_ADDR(reg));
    printf("RESET BAD\n");
  } else {
    // Don't remove - used by tests
    printf("RESET GOOD\n");
  }
  return;
}


void enable_sinal_handler() {
  struct sigaction handler;
  handler.sa_handler = &sighandler;
  handler.sa_flags = 0;
  sigemptyset(&handler.sa_mask);
  sigaction(SIGTERM, &handler, &termhandler);
  sigaction(SIGINT, &handler, &inthandler);

}

void reset_squid() {
  FILE *f = fopen("/sys/class/gpio/export", "w");
  if (!f)
    return;
  fprintf(f, "47");
  fclose(f);
  f = fopen("/sys/class/gpio/gpio47/direction", "w");
  if (!f)
    return;
  fprintf(f, "out");
  fclose(f);
  f = fopen("/sys/class/gpio/gpio47/value", "w");
  if (!f)
    return;
  fprintf(f, "0");
  usleep(10000);
  fprintf(f, "1");
  usleep(20000);
  fclose(f);
}



pid_t proc_find(const char* name)
{
    DIR* dir;
    struct dirent* ent;
    char buf[512];

    long  pid;
    char pname[100] = {0,};
    char state;
    FILE *fp=NULL;

    if (!(dir = opendir("/proc"))) {
        perror("can't open /proc");
        return -1;
    }

    while((ent = readdir(dir)) != NULL) {
        long lpid = atol(ent->d_name);
        if(lpid < 0)
            continue;
        snprintf(buf, sizeof(buf), "/proc/%ld/stat", lpid);
        fp = fopen(buf, "r");

        if (fp) {
            if ( (fscanf(fp, "%ld (%[^)]) %c", &pid, pname, &state)) != 3 ){
                printf("fscanf failed \n");
                fclose(fp);
                closedir(dir);
                return -1;
            }
            if (!strcmp(pname, name)) {
                fclose(fp);
                closedir(dir);
                return (pid_t)lpid;
            }
            fclose(fp);
        }
    }


closedir(dir);
return -1;
}



void discover_good_loops_restart_12v() {
  DBG(DBG_NET, "RESET SQUID\n");

  uint32_t good_loops = 0;
  int i;
  int good_loops_cnt = 0;

  write_spi(ADDR_SQUID_LOOP_BYPASS, SQUID_LOOPS_MASK);
  write_spi(ADDR_SQUID_LOOP_RESET, 0);
  write_spi(ADDR_SQUID_COMMAND, 0xF);
  write_spi(ADDR_SQUID_COMMAND, 0x0);
  write_spi(ADDR_SQUID_LOOP_BYPASS, 0);
  write_spi(ADDR_SQUID_LOOP_RESET, SQUID_LOOPS_MASK);


  for (i = 0; i < LOOP_COUNT; i++) {
    vm.loop[i].id = i;
    unsigned int bypass_loops = ((~(1 << i)) & SQUID_LOOPS_MASK);
    psyslog("ADDR_SQUID_LOOP_BYPASS = %x\n", bypass_loops);
    write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);
    if ((vm.loop[i].user_disabled == 0) &&
       (!vm.board_not_present[LOOP_TO_BOARD_ID(i)]) &&
       test_serial(i)
    ) {
      vm.loop[i].enabled_loop = 1;
      vm.loop[i].why_disabled = "All good, Rodrigez";
      for (int h = i * ASICS_PER_LOOP; h < (i + 1) * ASICS_PER_LOOP; h++) {
        vm.asic[h].asic_present = 1;
     
        for (int x = 0; x < ENGINE_BITMASCS-1; x++) {
          vm.asic[h].not_brocken_engines[x] = ENABLED_ENGINES_MASK;
        }
        vm.asic[h].not_brocken_engines[ENGINE_BITMASCS-1] = 0x1;
      }
      printf("loop %d enabled\n", i);
      good_loops |= 1 << i;
      good_loops_cnt++;
    } else {
      vm.loop[i].enabled_loop = 0;
      vm.loop[i].why_disabled = "test serial failed";        
      printf("loop %d disabled\n", i);

#ifndef SP2x
      if (!vm.tryed_power_cycle_to_revive_loops) {
        mg_event_x("Bad loop %d = trying power cycle");
        vm.tryed_power_cycle_to_revive_loops = 1;
        PSU12vPowerCycleALL();
        discover_good_loops_restart_12v();
        return;
      } else {
        mg_event_x("Bad loop %d = set loop as disabled");
      }
#endif
      
      for (int h = i * ASICS_PER_LOOP; h < (i + 1) * ASICS_PER_LOOP; h++) {
        int err;
        int overcurrent_err, overcurrent_warning;
        dc2dc_get_all_stats(
              i,
              &overcurrent_err,
              &overcurrent_warning,
              &vm.asic[i].dc2dc.dc_temp,
              &vm.asic[i].dc2dc.dc_current_16s,
              &err);
        dc2dc_disable_dc2dc(h,&err);
        if (vm.loop[i].user_disabled) {
          vm.asic[h].why_disabled = "Loop user disabled!";
        }
        if (err) {
          vm.asic[h].why_disabled = "i2c BAD, btw!";
        } else {
          if (overcurrent_err) {
            vm.asic[h].why_disabled = "i2c good but OC!!";
          } else {
            vm.asic[h].why_disabled = "i2c good, no OC";
          }
        }
        vm.asic[h].dc2dc.dc2dc_present = 0;
        psyslog("Disabling DC2DC %d because no ASIC A\n", h);
        vm.asic[h].asic_present = 0;
        for (int i = 0; i < ENGINE_BITMASCS; i++) {
          vm.asic[h].not_brocken_engines[i] = 0;
        }
      }
    }
  }
  psyslog("ADDR_SQUID_LOOP_BYPASS = %x\n", ~(good_loops) & SQUID_LOOPS_MASK);
  write_spi(ADDR_SQUID_LOOP_BYPASS, ~(good_loops) & SQUID_LOOPS_MASK);
  vm.good_loops = good_loops;
  test_serial(-3);
  psyslog("Found %d good loops\n", good_loops_cnt);
  passert(good_loops_cnt);
}

int get_print_win(int winner_device, int winner_engine);
int update_ac2dc_power_measurments();


void read_disabled_asics() {
  psyslog("Reading DISABLED ASICS\n");
  int r;
  FILE* file = fopen("/etc/mg_disabled_asics", "r");
  if (file < 0) {
    psyslog("/etc/mg_disabled_asics missing");
    passert(0);
    return;
  }
#ifndef SP2x  
  r = fscanf (file,  "0:%d 1:%d 2:%d\n", 
            &vm.asic[0].user_disabled,
            &vm.asic[1].user_disabled,            
            &vm.asic[2].user_disabled);
  passert(r == 3);
  if (vm.asic[0].user_disabled && vm.asic[1].user_disabled && vm.asic[2].user_disabled) {
    vm.loop[0].user_disabled = 1;
  }
  r = fscanf (file,  "3:%d 4:%d 5:%d\n" ,
            &vm.asic[3].user_disabled,
            &vm.asic[4].user_disabled,            
            &vm.asic[5].user_disabled);
  passert(r == 3);  
  if (vm.asic[3].user_disabled && vm.asic[4].user_disabled && vm.asic[5].user_disabled) {
    vm.loop[1].user_disabled = 1;
  }
  
  r = fscanf (file,  "6:%d 7:%d 8:%d\n" ,
            &vm.asic[6].user_disabled,
            &vm.asic[7].user_disabled,            
            &vm.asic[8].user_disabled);
  if (vm.asic[6].user_disabled && vm.asic[7].user_disabled && vm.asic[8].user_disabled) {
    vm.loop[2].user_disabled = 1;
  }
  passert(r == 3);    
  r = fscanf (file,  "9:%d 10:%d 11:%d\n", 
            &vm.asic[9].user_disabled,
            &vm.asic[10].user_disabled,            
            &vm.asic[11].user_disabled);
  
  if (vm.asic[9].user_disabled && vm.asic[10].user_disabled && vm.asic[11].user_disabled) {
    vm.loop[3].user_disabled = 1;
  }
  passert(r == 3);  
  r = fscanf (file,  "12:%d 13:%d 14:%d\n", 
            &vm.asic[12].user_disabled,
            &vm.asic[13].user_disabled,            
            &vm.asic[14].user_disabled);
  
  if (vm.asic[12].user_disabled && vm.asic[13].user_disabled && vm.asic[14].user_disabled) {
    vm.loop[4].user_disabled = 1;
  }
  passert(r == 3);  
  r = fscanf (file,  "15:%d 16:%d 17:%d\n" ,
            &vm.asic[15].user_disabled,
            &vm.asic[16].user_disabled,            
            &vm.asic[17].user_disabled);
  
  if (vm.asic[15].user_disabled && vm.asic[16].user_disabled && vm.asic[17].user_disabled) {
    vm.loop[5].user_disabled = 1;
  }
  passert(r == 3);  
  r = fscanf (file,  "18:%d 19:%d 20:%d\n" ,
            &vm.asic[18].user_disabled,
            &vm.asic[19].user_disabled,            
            &vm.asic[20].user_disabled);
  
  if (vm.asic[18].user_disabled && vm.asic[19].user_disabled && vm.asic[20].user_disabled) {
    vm.loop[6].user_disabled = 1;
  }
  passert(r == 3);  
  r = fscanf (file,  "21:%d 22:%d 23:%d\n" ,
            &vm.asic[21].user_disabled,
            &vm.asic[22].user_disabled,            
            &vm.asic[23].user_disabled);
  if (vm.asic[21].user_disabled && vm.asic[22].user_disabled && vm.asic[23].user_disabled) {
    vm.loop[7].user_disabled = 1;
  }
  passert(r == 3);  
  r = fscanf (file,  "24:%d 25:%d 26:%d\n" ,
            &vm.asic[24].user_disabled,
            &vm.asic[25].user_disabled,            
            &vm.asic[26].user_disabled);
  if (vm.asic[24].user_disabled && vm.asic[25].user_disabled && vm.asic[26].user_disabled) {
    vm.loop[8].user_disabled = 1;
  }
  passert(r == 3);  
  r = fscanf (file,  "27:%d 28:%d 29:%d\n" ,
            &vm.asic[27].user_disabled,
            &vm.asic[28].user_disabled,            
            &vm.asic[29].user_disabled);
  if (vm.asic[27].user_disabled && vm.asic[28].user_disabled && vm.asic[29].user_disabled) {
    vm.loop[9].user_disabled = 1;
  }
  passert(r == 3);

#else  // SP20
  r = fscanf (file,  "0:%d 1:%d\n", 
            &vm.asic[0].user_disabled,
            &vm.asic[1].user_disabled);
  passert(r == 2);
  if (vm.asic[0].user_disabled && vm.asic[1].user_disabled) {
    vm.loop[0].user_disabled = 1;
  }

  r = fscanf (file,  "2:%d 3:%d\n", 
            &vm.asic[2].user_disabled,
            &vm.asic[3].user_disabled);
  passert(r == 2);
  if (vm.asic[2].user_disabled && vm.asic[3].user_disabled) {
    vm.loop[1].user_disabled = 1;
  }

  r = fscanf (file,  "4:%d 5:%d\n", 
            &vm.asic[4].user_disabled,
            &vm.asic[5].user_disabled);
  passert(r == 2);
  if (vm.asic[4].user_disabled && vm.asic[5].user_disabled) {
    vm.loop[2].user_disabled = 1;
  }
  
  r = fscanf (file,  "6:%d 7:%d\n", 
            &vm.asic[6].user_disabled,
            &vm.asic[7].user_disabled);
  passert(r == 2);
  if (vm.asic[6].user_disabled && vm.asic[7].user_disabled) {
    vm.loop[3].user_disabled = 1;
  }
#endif
  fclose (file);
}




void read_generic_ac2dc() {
  FILE* file = fopen ("/etc/mg_generic_psu", "r");
#ifndef  SP2x
  if (file != 0) {
    int res = fscanf (file, "0:%d 1:%d",            
              &vm.ac2dc[PSU_0].force_generic_psu,
              &vm.ac2dc[PSU_1].force_generic_psu);
    fclose (file);
    psyslog("Generic i2c %d/%d\n",
            vm.ac2dc[PSU_0].force_generic_psu,
            vm.ac2dc[PSU_1].force_generic_psu);
    passert(res == 2);
  } 
#endif
}


void read_fet() {
  FILE* file = fopen ("/tmp/mg_fet", "r");
  if (file != 0) {
    int res = fscanf (file, "0:%d 1:%d",            
              &vm.fet[BOARD_0],
              &vm.fet[BOARD_1]);
    fclose (file);
    if(res < 2) {
      mg_event("Failed to parse fet\n");
      passert(0);
    }
  } else {
    printf("--------/tmp/mg_fet missing-------------\n");
    passert(0);
  }
  psyslog("----------------------------------\n");
  psyslog("--------FET TOP:%d BOT:%d---------\n", vm.fet[BOARD_0],vm.fet[BOARD_1]);
  psyslog("----------------------------------\n");
}


int update_work_mode(int decrease_top, int decrease_bottom, bool to_alternative_file) {
  FILE* file = fopen (MG_CUSTOM_MODE_FILE, "r");
  passert(file != NULL);
#ifdef SP2x
	passert(0);
#else 
  fscanf (file, "FAN:%d VST:%d VSB:%d VMAX:%d AC_TOP:%d AC_BOT:%d DC_AMP:%d", 
    &vm.max_fan_level, 
    &vm.ac2dc[PSU_0].voltage_start, 
    &vm.ac2dc[PSU_1].voltage_start,     
    &vm.voltage_max, 
    &vm.ac2dc[PSU_0].ac2dc_power_limit, 
    &vm.ac2dc[PSU_1].ac2dc_power_limit,    
    &vm.max_dc2dc_current_16s);

  if (decrease_top > 0) {
    vm.ac2dc[PSU_0].ac2dc_power_limit -= decrease_top;
  } 
  if (vm.ac2dc[PSU_0].ac2dc_power_limit < AC2DC_POWER_DECREASE_START_VOLTAGE) {
      vm.ac2dc[PSU_0].voltage_start = MIN(660,vm.ac2dc[PSU_0].voltage_start);
  }  

  if (decrease_bottom > 0) {
    vm.ac2dc[PSU_1].ac2dc_power_limit -= decrease_bottom;
  }
  if (vm.ac2dc[PSU_1].ac2dc_power_limit < AC2DC_POWER_DECREASE_START_VOLTAGE) {
    vm.ac2dc[PSU_1].voltage_start = MIN(660,vm.ac2dc[PSU_1].voltage_start) ;
  }
  fclose(file);

  if (to_alternative_file) {
    file = fopen ("/tmp/mg_custom_mode", "w");
  } else {
    file = fopen (MG_CUSTOM_MODE_FILE, "w");
  }
  passert(file != NULL);
  fprintf (file, "FAN:%d VST:%d VSB:%d VMAX:%d AC_TOP:%d AC_BOT:%d DC_AMP:%d", 
    vm.max_fan_level, 
    vm.ac2dc[PSU_0].voltage_start, 
    vm.ac2dc[PSU_1].voltage_start,     
    vm.voltage_max, 
    vm.ac2dc[PSU_0].ac2dc_power_limit, 
    vm.ac2dc[PSU_1].ac2dc_power_limit,    
    vm.max_dc2dc_current_16s);
  vm.max_dc2dc_current_16s *=16;
#endif
  fclose(file);
  system("/bin/sync");
}


static int apl_105[4] = {0,AC2DC_POWER_LIMIT_105_MU,AC2DC_POWER_LIMIT_105_MU,AC2DC_POWER_LIMIT_105_EM};
static int apl_114[4] = {0,AC2DC_POWER_LIMIT_114_MU,AC2DC_POWER_LIMIT_114_MU,AC2DC_POWER_LIMIT_114_EM};
static int apl_119[4] = {0,AC2DC_POWER_LIMIT_119_MU,AC2DC_POWER_LIMIT_119_MU,AC2DC_POWER_LIMIT_119_EM};
static int apl_200[4] = {0,AC2DC_POWER_LIMIT_200_MU,AC2DC_POWER_LIMIT_200_MU,AC2DC_POWER_LIMIT_200_EM};
static int apl_210[4] = {0,AC2DC_POWER_LIMIT_210_MU,AC2DC_POWER_LIMIT_210_MU,AC2DC_POWER_LIMIT_210_EM};

void store_last_voltage() {
  int r;
  psyslog("Storing voltages\n");
  FILE* file = fopen ("/tmp/mg_last_voltage", "w");
  if (file == NULL) {
    return;
  }
  r = fprintf (file, "0:%d 1:%d 2:%d\n", 
            (vm.asic[0].dc2dc.vtrim),
            (vm.asic[1].dc2dc.vtrim),     
            (vm.asic[2].dc2dc.vtrim));
  r = fprintf (file, "3:%d 4:%d 5:%d\n" ,
            (vm.asic[3].dc2dc.vtrim),
            (vm.asic[4].dc2dc.vtrim),            
            (vm.asic[5].dc2dc.vtrim));
  r = fprintf (file, "6:%d 7:%d 8:%d\n" ,
            (vm.asic[6].dc2dc.vtrim),
            (vm.asic[7].dc2dc.vtrim),            
            (vm.asic[8].dc2dc.vtrim));
  r = fprintf (file, "9:%d 10:%d 11:%d\n", 
            (vm.asic[9].dc2dc.vtrim),
            (vm.asic[10].dc2dc.vtrim),            
            (vm.asic[11].dc2dc.vtrim));
  r = fprintf (file, "12:%d 13:%d 14:%d\n", 
            (vm.asic[12].dc2dc.vtrim),
            (vm.asic[13].dc2dc.vtrim),            
            (vm.asic[14].dc2dc.vtrim));
  r = fprintf (file, "15:%d 16:%d 17:%d\n" ,
            (vm.asic[15].dc2dc.vtrim),
            (vm.asic[16].dc2dc.vtrim),            
            (vm.asic[17].dc2dc.vtrim));
  r = fprintf (file, "18:%d 19:%d 20:%d\n" ,
            (vm.asic[18].dc2dc.vtrim),
            (vm.asic[19].dc2dc.vtrim),            
            (vm.asic[20].dc2dc.vtrim));
  r = fprintf (file, "21:%d 22:%d 23:%d\n" ,
            (vm.asic[21].dc2dc.vtrim),
            (vm.asic[22].dc2dc.vtrim),            
            (vm.asic[23].dc2dc.vtrim));
  r = fprintf (file, "24:%d 25:%d 26:%d\n" ,
            (vm.asic[24].dc2dc.vtrim),
            (vm.asic[25].dc2dc.vtrim),            
            (vm.asic[26].dc2dc.vtrim));
  r = fprintf (file, "27:%d 28:%d 29:%d\n" ,
            (vm.asic[27].dc2dc.vtrim),
            (vm.asic[28].dc2dc.vtrim),            
            (vm.asic[29].dc2dc.vtrim));
  fclose(file);
}


void read_last_voltage() {
  int r;
  FILE* file = fopen ("/tmp/mg_last_voltage", "r");
  if (file == NULL) {
    return;
  }
  r = fscanf (file, "0:%d 1:%d 2:%d\n", 
            &vm.asic[0].dc2dc.before_failure_vtrim,
            &vm.asic[1].dc2dc.before_failure_vtrim,            
            &vm.asic[2].dc2dc.before_failure_vtrim);
  r = fscanf (file, "3:%d 4:%d 5:%d\n" ,
            &vm.asic[3].dc2dc.before_failure_vtrim,
            &vm.asic[4].dc2dc.before_failure_vtrim,            
            &vm.asic[5].dc2dc.before_failure_vtrim);
  r = fscanf (file, "6:%d 7:%d 8:%d\n" ,
            &vm.asic[6].dc2dc.before_failure_vtrim,
            &vm.asic[7].dc2dc.before_failure_vtrim,            
            &vm.asic[8].dc2dc.before_failure_vtrim);
  r = fscanf (file, "9:%d 10:%d 11:%d\n", 
            &vm.asic[9].dc2dc.before_failure_vtrim,
            &vm.asic[10].dc2dc.before_failure_vtrim,            
            &vm.asic[11].dc2dc.before_failure_vtrim);
  r = fscanf (file, "12:%d 13:%d 14:%d\n", 
            &vm.asic[12].dc2dc.before_failure_vtrim,
            &vm.asic[13].dc2dc.before_failure_vtrim,            
            &vm.asic[14].dc2dc.before_failure_vtrim);
  r = fscanf (file, "15:%d 16:%d 17:%d\n" ,
            &vm.asic[15].dc2dc.before_failure_vtrim,
            &vm.asic[16].dc2dc.before_failure_vtrim,            
            &vm.asic[17].dc2dc.before_failure_vtrim);
  r = fscanf (file, "18:%d 19:%d 20:%d\n" ,
            &vm.asic[18].dc2dc.before_failure_vtrim,
            &vm.asic[19].dc2dc.before_failure_vtrim,            
            &vm.asic[20].dc2dc.before_failure_vtrim);
  r = fscanf (file, "21:%d 22:%d 23:%d\n" ,
            &vm.asic[21].dc2dc.before_failure_vtrim,
            &vm.asic[22].dc2dc.before_failure_vtrim,            
            &vm.asic[23].dc2dc.before_failure_vtrim);
  r = fscanf (file, "24:%d 25:%d 26:%d\n" ,
            &vm.asic[24].dc2dc.before_failure_vtrim,
            &vm.asic[25].dc2dc.before_failure_vtrim,            
            &vm.asic[26].dc2dc.before_failure_vtrim);
  r = fscanf (file, "27:%d 28:%d 29:%d\n" ,
            &vm.asic[27].dc2dc.before_failure_vtrim,
            &vm.asic[28].dc2dc.before_failure_vtrim,            
            &vm.asic[29].dc2dc.before_failure_vtrim);
  fclose(file);
}



int read_work_mode() {
	FILE* file = fopen (MG_CUSTOM_MODE_FILE, "r");
 
	int i = 0;
  passert(file != NULL);
#ifdef SP2x 
  int ret =  fscanf (file, "FAN:%d VS0:%d VS1:%d VS2:%d VS3:%d VMAX:%d AC0:%d AC1:%d AC2:%d AC3:%d DC_AMP:%d", 
    &vm.max_fan_level, 
    &vm.ac2dc[0].voltage_start, 
    &vm.ac2dc[1].voltage_start,     
    &vm.ac2dc[2].voltage_start, 
    &vm.ac2dc[3].voltage_start,     
    &vm.voltage_max, 
    &vm.ac2dc[0].ac2dc_power_limit, 
    &vm.ac2dc[1].ac2dc_power_limit,    
    &vm.ac2dc[2].ac2dc_power_limit, 
    &vm.ac2dc[3].ac2dc_power_limit,      
    &vm.max_dc2dc_current_16s);

   assert(ret == 11);  
   assert(vm.max_fan_level <= 100);
   assert(vm.max_fan_level >= 0);
   for (int p = 0 ; p < PSU_COUNT; p++) {
     assert(vm.ac2dc[p].voltage_start <= 830);
     assert(vm.ac2dc[p].voltage_start >= 580);
     assert(vm.voltage_max >= vm.ac2dc[p].voltage_start);
     assert(vm.ac2dc[p].ac2dc_power_limit   >= 100);
     assert(vm.ac2dc[p].ac2dc_power_limit   <= 1800);
     vm.ac2dc[p].vtrim_start = VOLTAGE_TO_VTRIM_MILLI(vm.ac2dc[p].voltage_start);
     
   }
 vm.vtrim_max = VOLTAGE_TO_VTRIM_MILLI(vm.voltage_max);
 assert(vm.voltage_max <= 830);
 assert(vm.voltage_max >= 580);
 vm.max_dc2dc_current_16s*=16;
 passert(vm.vtrim_max <= VTRIM_MAX);



  // compute VTRIM
  psyslog(
    "vm.max_fan_level: %d, vm.voltage_start: %d/%d/%d/%d, vm.voltage_end: %d vm.vtrim_start: %x/%x/%x/%x, vm.vtrim_end: %x\n"
    ,vm.max_fan_level, 
    vm.ac2dc[0].voltage_start,vm.ac2dc[1].voltage_start, vm.ac2dc[2].voltage_start,vm.ac2dc[3].voltage_start, 
    vm.voltage_max, 
    vm.ac2dc[0].vtrim_start,vm.ac2dc[1].vtrim_start , vm.ac2dc[2].vtrim_start,vm.ac2dc[3].vtrim_start , 
    vm.vtrim_max);
  

#else
	fscanf (file, "FAN:%d VST:%d VSB:%d VMAX:%d AC_TOP:%d AC_BOT:%d DC_AMP:%d", 
    &vm.max_fan_level, 
    &vm.ac2dc[PSU_0].voltage_start, 
    &vm.ac2dc[PSU_1].voltage_start,    
    &vm.voltage_max, 
    &vm.ac2dc[PSU_0].ac2dc_power_limit, 
    &vm.ac2dc[PSU_1].ac2dc_power_limit,    
    &vm.max_dc2dc_current_16s);
  fclose(file);
  
  
  //vm.ac2dc[PSU_1].ac2dc_power_limit = vm.ac2dc[PSU_0].ac2dc_power_limit;
  assert(vm.max_fan_level <= 100);
  assert(vm.max_fan_level >= 0);
  assert(vm.ac2dc[PSU_0].voltage_start <= 830);
  assert(vm.ac2dc[PSU_0].voltage_start >= 580);
  assert(vm.ac2dc[PSU_1].voltage_start <= 830);
  assert(vm.ac2dc[PSU_1].voltage_start >= 580);
  assert(vm.voltage_max <= 830);
  assert(vm.voltage_max >= 580);
  assert(vm.voltage_max >= vm.ac2dc[PSU_0].voltage_start);
  assert(vm.voltage_max >= vm.ac2dc[PSU_1].voltage_start);  
  assert(vm.ac2dc[PSU_1].ac2dc_power_limit   >= 70);
  assert(vm.ac2dc[PSU_1].ac2dc_power_limit   <= 2500);
  assert(vm.ac2dc[PSU_0].ac2dc_power_limit   >= 70);
  assert(vm.ac2dc[PSU_0].ac2dc_power_limit   <= 2500);
  vm.max_dc2dc_current_16s*=16;

  FILE* ignore_fcc_file = fopen ("/etc/mg_ignore_110_fcc", "r");
  if (ignore_fcc_file != NULL) {
    // ignore_fcc_file present
    fclose (file);
  } else {
    int top_fix = 0;
    int bot_fix = 0;
      
    int psu_type = vm.ac2dc[PSU_0].ac2dc_type;
    int voltage  = vm.ac2dc[PSU_0].voltage;
    int limit    = vm.ac2dc[PSU_0].ac2dc_power_limit;
    if ((psu_type != AC2DC_TYPE_UNKNOWN) &&
        !vm.ac2dc[PSU_0].force_generic_psu) {
      psyslog("Voltage %d\n", voltage);
      if (voltage < 88) {
         exit_nicely(4,"Voltage low top");
      }
      
      if (voltage < 105) {
        if ((vm.ac2dc[PSU_0].voltage_start > 660) || (limit    > apl_105[psu_type])) {
          top_fix = vm.ac2dc[PSU_0].ac2dc_power_limit - apl_105[psu_type];
          goto ddd1;
        }
      }

      if (voltage < 114) {
        if ((vm.ac2dc[PSU_0].voltage_start > 660) || (limit    > apl_114[psu_type])) {
          top_fix = vm.ac2dc[PSU_0].ac2dc_power_limit - apl_114[psu_type]; 
          goto ddd1;
        }
      }

      if (voltage < 119) {
        if ((vm.ac2dc[PSU_0].voltage_start > 660) || (limit    > apl_119[psu_type])) {
          top_fix = vm.ac2dc[PSU_0].ac2dc_power_limit - apl_119[psu_type]; 
          goto ddd1;
        }
      }

      if (voltage < 200) {
        if ((vm.ac2dc[PSU_0].voltage_start > 660) || (limit    > apl_200[psu_type])) {
          top_fix = vm.ac2dc[PSU_0].ac2dc_power_limit - apl_200[psu_type]; 
          goto ddd1;
        }
      }

      if (voltage < 210) {
        if ((vm.ac2dc[PSU_0].voltage_start > 660) || (limit    > apl_210[psu_type])) {
         top_fix = vm.ac2dc[PSU_0].ac2dc_power_limit - apl_210[psu_type]; 
         goto ddd1;
        }
      }
    }
  
ddd1:
    psu_type = vm.ac2dc[PSU_1].ac2dc_type;
    voltage  = vm.ac2dc[PSU_1].voltage;
    limit   =  vm.ac2dc[PSU_1].ac2dc_power_limit;
    psyslog("HERE Voltage %d\n", voltage);    
    if ((psu_type != AC2DC_TYPE_UNKNOWN) &&
        !vm.ac2dc[PSU_1].force_generic_psu) {
      psyslog("Voltage %d\n", voltage);
      if (voltage < 88) {
         exit_nicely(4,"Voltage low bottom");
      }

      
      if (voltage < 105) {
        if ((vm.ac2dc[PSU_1].voltage_start > 660) || (limit    > apl_105[psu_type])) {
          bot_fix = limit - apl_105[psu_type]; 
          goto ddd2;
        }
      }

      if (voltage < 114) {
        if ((vm.ac2dc[PSU_1].voltage_start > 660) || (limit    > apl_114[psu_type])) {
          bot_fix = limit - apl_114[psu_type]; 
          goto ddd2;            
        }
      }

      if (voltage < 119) {
        if ((vm.ac2dc[PSU_1].voltage_start > 660) || (limit    > apl_119[psu_type])) {
          bot_fix = limit - apl_119[psu_type]; 
          goto ddd2;            
        }
      }

      if (voltage < 200) {
        if ((vm.ac2dc[PSU_1].voltage_start > 660) || (limit    > apl_200[psu_type])) {
          bot_fix = limit - apl_200[psu_type]; 
          goto ddd2;
        }
      }
        

      if (voltage < 210) {
        if ((vm.ac2dc[PSU_1].voltage_start > 660) || (limit   > apl_210[psu_type])) {
          bot_fix =limit - apl_210[psu_type]; 
          goto ddd2;            
        }
      }
    }
    
 ddd2:         
    if (top_fix || bot_fix) {
       update_work_mode(top_fix, bot_fix, true);
    } else {
      unlink("/tmp/mg_custom_mode");
    }
    
  }
 
  vm.ac2dc[PSU_0].vtrim_start = VOLTAGE_TO_VTRIM_MILLI(vm.ac2dc[PSU_0].voltage_start);
  vm.ac2dc[PSU_1].vtrim_start= VOLTAGE_TO_VTRIM_MILLI(vm.ac2dc[PSU_1].voltage_start);  
  vm.vtrim_max = VOLTAGE_TO_VTRIM_MILLI(vm.voltage_max);
  passert(vm.vtrim_max <= VTRIM_MAX);


  // compute VTRIM
  psyslog(
    "vm.max_fan_level: %d, vm.voltage_start: %d/%d, vm.voltage_end: %d vm.vtrim_start: %x/%x, vm.vtrim_end: %x\n"
    ,vm.max_fan_level, 
    vm.ac2dc[PSU_0].voltage_start,vm.ac2dc[PSU_1].voltage_start, 
    vm.voltage_max, 
    vm.ac2dc[PSU_0].vtrim_start,vm.ac2dc[PSU_1].vtrim_start , 
    vm.vtrim_max);
#endif  
}

void configure_mq(uint32_t interval, uint32_t increments, int pause)
{
  //interval = interval * 20;
  uint32_t val = MAKE_MQ_CONFIG(interval, increments, pause);
  write_spi(ADDR_SQUID_MQ_CONFIG, val);
  flush_spi_write();
}


void test_lost_address() {
   // Validate all got address
  if (read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_GOT_ADDR_NOT) != 0) {
    mg_event_x_nonl("ASIC lost address:");
    for (int i = 0; i < ASICS_COUNT; i++) { 
      if (vm.asic[i].asic_present) {
        int reg = read_reg_asic(i, NO_ENGINE,ADDR_VERSION);
        mg_event_x_nonl("[%d %x]",i , reg);
      } else {
        mg_event_x_nonl("[%d NA]",i);
      }
    }
  } else {
    mg_event_x("No reset detected in ASICs");
  }
}
void restart_asics_full(int reason,const char * why) {  
  int err;
  // Close all possible i2c devices
  i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0, &err);   
#ifndef SP2x  
  i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0, &err);    
#endif
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);
  
  if(vm.in_asic_reset != 0) {
    print_stack();
    mg_event_x("Recursive restart_asics (%s)", why);
    passert(0);
  }
  vm.in_asic_reset = 1;

  // Test AC2DC    
  static ASIC asics[ASICS_COUNT];
  int has_chiko = -1;
  for(int i = 0; i < ASICS_COUNT; i++) {
    asics[i] = vm.asic[i];
    ASIC *a = &vm.asic[i];
    if (a->asic_present) {
      update_dc2dc_stats(i);
      if (vm.asic[i].dc2dc.dc_current_16s < 7*16) {
          has_chiko = i;
          mg_event_x("Lazy asic %d",i);
      }
    }
  }

  // We had voltage drop
   /*
  if (has_chiko != -1) {
    if ((time(NULL) - vm.last_chiko_time) < 180) {
      // We had voltage drops one after another
      int ac_id = ASIC_TO_PSU_ID(has_chiko);
      mg_event_x("Consecutive Chiko %d", has_chiko);
      if (vm.ac2dc[ac_id].ac2dc_power_limit > 1200) {
        // We had voltage drops one after another at high wattage
        if (ac_id == PSU_0) {
          update_work_mode(5, 0, false);
        } else {
          update_work_mode(0, 5, false);
        }
      }
      if (vm.ac2dc[ac_id].ac2dc_power_limit > 1100) {
        vm.ac2dc[ac_id].ac2dc_power_limit -= 5;
      }
    }
    vm.last_chiko_time = time(NULL);
  }
   */

  
  vm.consecutive_jobs = 10;
  vm.start_run_time = time(NULL);
  vm.err_restarted++;
  mg_event_x("restart_asics_full (%s)", why);
  
  print_stack();
  psyslog("-------- SOFT RESET 0 -----------\n");  
  
  psyslog("-------- SOFT RESET 1 -----------\n");  
  test_fix_ac2dc_limits();
  psyslog("-------- SOFT RESET 2 -----------\n"); 

  for (int p = 0 ; p < PSU_COUNT ; p++) {
    vm.ac2dc[p].board_cooling_now = 0;
  }
  
  usleep(100000);

  for(int i = 0; i < ASICS_COUNT; i++) {
    ASIC *a = &vm.asic[i];
    a->bist_failed_called = 0;
    a->dc2dc.before_failure_vtrim = (a->dc2dc.vtrim-1);
  }
  psyslog("-------- SOFT RESET 3 -----------\n");  
  // Wait
  usleep(300000);
  discover_good_loops_restart_12v();
  psyslog("-------- SOFT RESET 4 -----------\n");    
  init_asics(ANY_ASIC);
  usleep(700000);
  // Give addresses to devices.
  psyslog("-------- SOFT RESET 5 -----------\n");  
  usleep(100000);
  allocate_addresses_to_devices();
  psyslog("-------- SOFT RESET 6 -----------\n");    
  enable_all_engines_all_asics(1);  
  //enable_all_engines_all_asics(1);
  //passert(read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_VERSION)&0xFF == 0x3c);
  psyslog("-------- SOFT RESET 7 -----------\n");     
  init_asic_clocks(ANY_ASIC);
  thermal_init(ANY_ASIC);
  psyslog("-------- SOFT RESET 8 -----------\n");    
  // TEST
   
  psyslog("-------- SOFT RESET 9 -----------\n");   
  set_nonce_range_in_engines(0xFFFFFFFF);
  psyslog("-------- SOFT RESET 10 -----------\n");     
#if 0
  set_initial_voltage_freq();
#else
  for(int i = 0; i < ASICS_COUNT; i++) {
    vm.asic[i] = asics[i];
    psyslog("ASIC %d present: %d\n", i, vm.asic[i].asic_present);    
    if ((!vm.asic[i].asic_present) ||
         (vm.asic[i].user_disabled)) {
      psyslog("Disable ASIC %d\n", i);
      vm.asic[i].asic_present = 1;
      disable_asic_forever_rt(i,"resisable after reset");
    }
  }
  psyslog("-------- SOFT RESET 10.5 -----------\n");  
  set_initial_voltage_freq_on_restart();
#endif

  psyslog("-------- SOFT RESET 11 -----------\n");     
   while (BROADCAST_READ_ADDR(read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_CONDUCTOR_BUSY))) {
    printf("Waiting conductor buzy\n");
   }  
  //passert(0);
  // First - clear 
  psyslog("-------- SOFT RESET DONE -----------\n");     
  vm.in_asic_reset = 0;
  mg_event_x("Restart ASICS done :)%d", vm.in_asic_reset);
}



void restart_asics_part(const char * why) {
   static int inside = 0;
   if (inside) {
      restart_asics_full(7,why);
      return;
   }
   inside = 1;
   mg_event_x("restart_asics_part (%s)", why);
   // Just relock the PLLs
   for (int x = 0; x < ASICS_COUNT; x++) {
    set_pll(x, vm.asic[x].freq_hw, 1, 0,  "Reset");
   }
   inside = 0;
}


int main(int argc, char *argv[]) {
  printf(RESET);  
  vm.start_run_time = time(NULL);
  int s;
  vm.in_asic_reset = 1;
  srand (time(NULL));
  //enable_reg_debug = 1;
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog("minergate", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  syslog(LOG_NOTICE, "minergate started");
#ifdef REMO_STRESS  
  psyslog( "------------------------\n");
  psyslog( "------------------------\n");
  psyslog( "--RUNNING REMOSTRESS----\n");
  psyslog( "------------------------\n");
  psyslog( "------------------------\n");
  psyslog( "------------------------\n"); 
  mg_event_x("---- RUNNING REMOSTRESS -----");
#endif

  enable_sinal_handler();

  if ((argc > 1) && strcmp(argv[1], "--help") == 0) {
    printf("--testreset = Test asic reset!!!\n");
    return 0;
  }

  struct sockaddr_un address;
  socklen_t address_length = sizeof(address);
  pthread_t main_thread;
  // pthread_t conn_pth;
  load_nvm_ok();
  psyslog("reset_squid\n");
  reset_squid();
  psyslog("init_spi\n");
  init_spi();
  psyslog("ac2dc_init\n");
  int pError;
  vm.fpga_ver = read_spi(ADDR_SQUID_REVISION);
  vm.vtrim_max = VTRIM_MAX;
  i2c_init(&pError);  
  leds_init();
  read_generic_ac2dc();
  ac2dc_init();
  set_light(LIGHT_GREEN, LIGHT_MODE_SLOW_BLINK);
  //update_ac2dc_power_measurments(PSU_1, &vm.ac2dc[PSU_1]);
  //update_ac2dc_power_measurments(PSU_0, &vm.ac2dc[PSU_0]);
  //read_last_voltage();
  read_fet();  
  read_disabled_asics();
  // store voltage
#ifndef SP2x
  FILE* file = fopen ("/tmp/voltage", "w");
  if (file > 0) {
     fprintf (file, "%d/%d", vm.ac2dc[PSU_0].voltage, vm.ac2dc[PSU_1].voltage);
     fclose(file);
  }
#endif

  read_work_mode();
  test_fix_ac2dc_limits();


  vm.temp_mgmt = get_mng_board_temp();
  vm.temp_top = get_top_board_temp();
  vm.temp_bottom = get_bottom_board_temp();

  
  mg_event("Started!");
  int bp = 0;
  for (int p = 0; p < BOARD_COUNT; p++) {
    if (!vm.board_not_present[p]) {
      bp = 1;
    }
    psyslog("BOARD %d PRESENT:%d\n",p, !vm.board_not_present[p]);
  }
  if(!bp) { // at least 1 board must be present
    mg_event("NO BOARDS PRESENT");
    passert(0);
  }
  init_pwm();
  // Enable ALL dc2dc
  dc2dc_init();
  int addr;
  int err;
  psyslog("init_pwm\n");
  //exit(0);
  reset_sw_rt_queue();
  //set_light(LIGHT_YELLOW, LIGHT_MODE_ON);


  // test SPI
  passert(read_spi(ADDR_SQUID_PONG) == 0xDEADBEEF);


  configure_mq(MQ_TIMER_USEC, MQ_INCREMENTS, 0);

  // Find good loops
  // Update vm.good_loops
  // Set ASICS on all disabled loops to asic_ok=0
  discover_good_loops_restart_12v();

  psyslog("enable_good_loops_ok done %d\n", __LINE__);
  // Allocates addresses, sets nonce range.
  // Reset all asics
  init_asics(ANY_ASIC);
  printf("GOT VERSION: 0x%x\n", read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_VERSION)&0xFF);
  passert((read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_VERSION)&0xFF) == 0x46);

  // Give addresses to devices.
  allocate_addresses_to_devices();
  enable_all_engines_all_asics(1);  
  //enable_all_engines_all_asics(1);
  //passert(read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_VERSION)&0xFF == 0x3c);
  
  init_asic_clocks(ANY_ASIC);
  thermal_init(ANY_ASIC);
  // TEST

  printf("Disabling ASICs:\n");
  for (int x = 0; x < ASICS_COUNT; x++) {
    if (vm.asic[x].user_disabled) {
       printf("Disabling ASIC %d:\n", x);      
       disable_asic_forever_rt(x,"User disabled");
    }
  }


  set_nonce_range_in_engines(0xFFFFFFFF);
  set_initial_voltage_freq();
  while (BROADCAST_READ_ADDR(read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_CONDUCTOR_BUSY))) {
   printf("Waiting conductor buzy\n");
  }

  psyslog("Opening socket for cgminer\n");
  // test ASIC read
  // passert(read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_VERSION), "No version found in ASICs");
  init_socket();
  passert(socket_fd > 0);


  if ((argc > 1) && strcmp(argv[1], "--stress") == 0) {
    //struct timeval tv;
    psyslog("Starting stress loop\n");
    int f;
    uint32_t value[50];
    while (1) {
      //start_stopper(&tv);      
      for (int k = 0 ; k < 50 ; k++) {
        push_asic_read(ANY_ASIC, NO_ENGINE, ADDR_VERSION, &value[k]);
      }
      //end_stopper(&tv,"A");      
      squid_wait_asic_reads();
      //end_stopper(&tv,"C");      
      //printf(" %d %x %x %x\n",f++, value[0],value[10],value[20]);      
    }
  }

  vm.in_asic_reset = 0;
  psyslog("Starting HW thread\n");
  s = pthread_create(&main_thread, NULL, squid_regular_state_machine_rt, (void *)NULL);
  passert(s == 0);
  minergate_adapter *adapter = new minergate_adapter;
  passert((int)adapter);

  while ((adapter->connection_fd =
              accept(socket_fd, (struct sockaddr *)&address, &address_length)) >
         -1) {
    // Only 1 thread supportd so far...
    psyslog("New adapter connected %d %p!\n", adapter->connection_fd, adapter);
    s = pthread_create(&adapter->conn_pth, NULL, connection_handler_thread,
                       (void *)adapter);
    passert(s == 0);

    adapter = new minergate_adapter;
    passert((int)adapter);
  }
  psyslog("Err %d:", adapter->connection_fd);
  passert(0);

  close(socket_fd);
  unlink(MINERGATE_SOCKET_FILE_SP30);
  return 0;
}
