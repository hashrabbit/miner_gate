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

#include "pll.h"
#include "asic.h"
#include "defines.h"
#include "squid.h"
#include "string.h"
#include <sys/time.h>
#include <time.h>

#include "asic_lib.h"
#include "miner_gate.h"

//#include "scaling_manager.h"
#include "spond_debug.h"
#include "dc2dc.h"



pll_frequency_settings pfs[ASIC_FREQ_MAX] = {
FREQ_350,
FREQ_355  ,
FREQ_360  ,
FREQ_365	,
FREQ_370	,
FREQ_375	,
FREQ_380	,
FREQ_385	,
FREQ_390	,
FREQ_395	,
FREQ_400	,
FREQ_405	,
FREQ_410	,
FREQ_415	,
FREQ_420	,
FREQ_425	,
FREQ_430	,
FREQ_435	,
FREQ_440	,
FREQ_445	,
FREQ_450	,
FREQ_455	,
FREQ_460	,
FREQ_465	,
FREQ_470	,
FREQ_475	,
FREQ_480	,
FREQ_485	,
FREQ_490	,
FREQ_495	,
FREQ_500	,
FREQ_505	,
FREQ_510	,
FREQ_515	,
FREQ_520	,
FREQ_525	,
FREQ_530	,
FREQ_535	,
FREQ_540	,
FREQ_545	,
FREQ_550	,
FREQ_555	,
FREQ_560	,
FREQ_565	,
FREQ_570	,
FREQ_575	,
FREQ_580	,
FREQ_585	,
FREQ_590	,
FREQ_595	,
FREQ_600	,
FREQ_605	,
FREQ_610	,
FREQ_615	,
FREQ_620	,
FREQ_625	,
FREQ_630	,
FREQ_635	,
FREQ_640	,
FREQ_645	,
FREQ_650	,
FREQ_655	,
FREQ_660	,
FREQ_665	,
FREQ_670	,
FREQ_675	,
FREQ_680	,
FREQ_685	,
FREQ_690	,
FREQ_695	,
FREQ_700	,
FREQ_705	,
FREQ_700	,
FREQ_715	,
FREQ_720	,
FREQ_725	,
FREQ_730	,
FREQ_735	,
FREQ_740	,
FREQ_745	,
FREQ_750	,
FREQ_755	,
FREQ_760,
FREQ_765,
FREQ_770,
FREQ_775,
FREQ_780,
FREQ_785,
FREQ_790,
FREQ_795,
FREQ_800,
FREQ_805,
FREQ_810,
FREQ_815,
FREQ_820,
FREQ_825,
FREQ_830,
FREQ_835,
FREQ_840,
FREQ_845,
FREQ_850,
FREQ_855,
FREQ_860,
FREQ_865,
FREQ_870,
FREQ_875,
FREQ_880,
FREQ_885,
FREQ_890,
FREQ_895,
FREQ_900,
FREQ_905,
FREQ_910,
FREQ_915,
FREQ_920,
FREQ_925,
FREQ_930,
FREQ_935,
FREQ_940,
FREQ_945,
FREQ_950,
FREQ_955,
FREQ_960,
FREQ_965,
FREQ_970,
FREQ_975,
FREQ_980,
FREQ_985,
FREQ_990,
FREQ_995,
FREQ_1000,
FREQ_1005,
FREQ_1010,
FREQ_1015,
FREQ_1020,
FREQ_1025,
FREQ_1030,
FREQ_1035,
FREQ_1040,
FREQ_1045,
FREQ_1050,
FREQ_1055,
FREQ_1060,
FREQ_1065,
FREQ_1070,
FREQ_1075,
FREQ_1080,
FREQ_1085,
FREQ_1090,
FREQ_1095,
FREQ_1100,
FREQ_1105,
FREQ_1110,
FREQ_1115,
FREQ_1120,
FREQ_1125,
FREQ_1130,
FREQ_1135,
FREQ_1140,
FREQ_1145,
FREQ_1150,
FREQ_1155,
FREQ_1160,
FREQ_1165,
FREQ_1170,
FREQ_1175,
FREQ_1180,
FREQ_1185,
FREQ_1190,
FREQ_1195,
FREQ_1200,
FREQ_1205,
FREQ_1210,
FREQ_1215,
FREQ_1220,
FREQ_1225,
FREQ_1230,
FREQ_1235,
FREQ_1240,
FREQ_1245,
FREQ_1250
};
 



void init_asic_clocks(int addr) {
  write_reg_asic(addr, NO_ENGINE,ADDR_GLOBAL_HASH_RESETN,1);
  write_reg_asic(addr, NO_ENGINE,ADDR_GLOBAL_CLK_EN,1);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_0,0);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_1,0);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_2,0);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_3,0);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_4,0);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_5,0);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_6,0);
  
#ifdef ASSAFS_EXPERIMENTAL_PLL
  write_reg_asic(addr, ANY_ENGINE, ADDR_CONTROL_SET1, BIT_ADDR_CONTROL_USE_1XCK);
#endif
  write_reg_asic(addr, ANY_ENGINE, ADDR_CONTROL_SET1, BIT_ADDR_CONTROL_FIFO_RESET_N);
  write_reg_asic(addr, ANY_ENGINE, ADDR_CONTROL_SET0, BIT_ADDR_CONTROL_IDLE_WHEN_FIFO_EMPTY);

  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_0,ENABLED_ENGINES_MASK);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_1,ENABLED_ENGINES_MASK);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_2,ENABLED_ENGINES_MASK);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_3,ENABLED_ENGINES_MASK);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_4,ENABLED_ENGINES_MASK);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_5,ENABLED_ENGINES_MASK);
  write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_6,ENABLED_ENGINES_MASK);
}


void enable_all_engines_all_asics(int with_reset) {
  for (int h = 0; h < ASICS_COUNT ; h++) {
    if ( vm.asic[h].asic_present) {
      vm.asic[h].engines_down = 0;
    }
  }

  if (with_reset) {
     write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_0,ENABLED_ENGINES_MASK);
     write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_1,ENABLED_ENGINES_MASK);
     write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_2,ENABLED_ENGINES_MASK);
     write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_3,ENABLED_ENGINES_MASK);
     write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_4,ENABLED_ENGINES_MASK);
     write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_5,ENABLED_ENGINES_MASK);
     write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_6,ENABLED_ENGINES_MASK);
  }
#ifdef ASSAFS_EXPERIMENTAL_PLL
     write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_CONTROL_SET1, BIT_ADDR_CONTROL_USE_1XCK);
#endif
   write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_CONTROL_SET1, BIT_ADDR_CONTROL_FIFO_RESET_N);
   write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_WIN_LEADING_0, vm.cur_leading_zeroes);   

   write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_0,ENABLED_ENGINES_MASK);
   write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_1,ENABLED_ENGINES_MASK);
   write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_2,ENABLED_ENGINES_MASK);
   write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_3,ENABLED_ENGINES_MASK);
   write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_4,ENABLED_ENGINES_MASK);
   write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_5,ENABLED_ENGINES_MASK);
   write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_6,ENABLED_ENGINES_MASK);
  flush_spi_write();
}


// When changing PLL
void disable_engines_all_asics(int with_reset) {
  psyslog("DISABLE ALL ENGINES\n");
  for (int h = 0; h < ASICS_COUNT ; h++) {
     vm.asic[h].engines_down = 1;
  }

  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_0,0);
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_1,0);
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_2,0);
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_3,0);
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_4,0);
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_5,0);
  write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_ENABLE_ENGINES_6,0);

  if (with_reset) {
    write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_0,0);
    write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_1,0);
    write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_2,0);
    write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_3,0);
    write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_4,0);
    write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_5,0);
    write_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_6,0);
  }
  flush_spi_write();
}


void disable_engines_asic(int addr, int with_reset) {
  //passert(vm.asic[addr].asic_present);
  vm.asic[addr].engines_down = 1;
  write_reg_asic(addr, NO_ENGINE, ADDR_ENABLE_ENGINES_0,0);
  write_reg_asic(addr, NO_ENGINE, ADDR_ENABLE_ENGINES_1,0);
  write_reg_asic(addr, NO_ENGINE, ADDR_ENABLE_ENGINES_2,0);
  write_reg_asic(addr, NO_ENGINE, ADDR_ENABLE_ENGINES_3,0);
  write_reg_asic(addr, NO_ENGINE, ADDR_ENABLE_ENGINES_4,0);
  write_reg_asic(addr, NO_ENGINE, ADDR_ENABLE_ENGINES_5,0);
  write_reg_asic(addr, NO_ENGINE, ADDR_ENABLE_ENGINES_6,0);
  if (with_reset) {
    write_reg_asic(addr, NO_ENGINE, ADDR_SERIAL_RESETN_ENGINES_0,0);
    write_reg_asic(addr, NO_ENGINE, ADDR_SERIAL_RESETN_ENGINES_1,0);
    write_reg_asic(addr, NO_ENGINE, ADDR_SERIAL_RESETN_ENGINES_2,0);
    write_reg_asic(addr, NO_ENGINE, ADDR_SERIAL_RESETN_ENGINES_3,0);
    write_reg_asic(addr, NO_ENGINE, ADDR_SERIAL_RESETN_ENGINES_4,0);
    write_reg_asic(addr, NO_ENGINE, ADDR_SERIAL_RESETN_ENGINES_5,0);
    write_reg_asic(addr, NO_ENGINE, ADDR_SERIAL_RESETN_ENGINES_6,0);
  }
  flush_spi_write();
}


void enable_engines_asic(int addr, uint32_t engines_mask[7], int with_unreset, int reset_others) {
  vm.asic[addr].engines_down = 0;

  if (with_unreset) {
    write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_0,engines_mask[0]);
    write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_1,engines_mask[1]);
    write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_2,engines_mask[2]);
    write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_3,engines_mask[3]);
    write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_4,engines_mask[4]);
    write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_5,engines_mask[5]);
    write_reg_asic(addr, NO_ENGINE,ADDR_SERIAL_RESETN_ENGINES_6,engines_mask[6]);
  }
#ifdef ASSAFS_EXPERIMENTAL_PLL
  write_reg_asic(ANY_ASIC, ANY_ENGINE, ADDR_CONTROL_SET1, BIT_ADDR_CONTROL_USE_1XCK);
#endif
  write_reg_asic(addr, ANY_ENGINE, ADDR_CONTROL_SET1, BIT_ADDR_CONTROL_FIFO_RESET_N);
  write_reg_asic(addr, ANY_ENGINE, ADDR_WIN_LEADING_0, vm.cur_leading_zeroes);


  write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_0,engines_mask[0]);
  write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_1,engines_mask[1]);
  write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_2,engines_mask[2]);
  write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_3,engines_mask[3]);
  write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_4,engines_mask[4]);
  write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_5,engines_mask[5]);
  write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_6,engines_mask[6]);

  if (reset_others) { // TODO - Error?
    write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_0,~engines_mask[0]);
    write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_1,~engines_mask[1]);
    write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_2,~engines_mask[2]);
    write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_3,~engines_mask[3]);
    write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_4,~engines_mask[4]);
    write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_5,~engines_mask[5]);
    write_reg_asic(addr, NO_ENGINE,ADDR_ENABLE_ENGINES_6,~engines_mask[6]);
  }


  flush_spi_write();

}



int enable_good_engines_all_asics_ok(int with_reset) {
    int i = 0;
    int reg;
    int killed_pll=0;
    while ((reg = read_reg_asic(ANY_ASIC, NO_ENGINE,ADDR_INTR_BC_PLL_NOT_READY)) != 0) {
      if (i++ > 100) {
        psyslog(RED "PLL %x stuck, killing ASIC Y\n" RESET, reg);
        //return 0;
        int addr = BROADCAST_READ_ADDR(reg);
        disable_asic_forever_rt(addr, "can't enable");
        killed_pll++;
      }
      usleep(10);
    }
   //printf("Enabling engines from NVM:\n");
   if (killed_pll) {
     passert(test_serial(-2));
   }

   enable_all_engines_all_asics(with_reset);

   for (int h = 0; h < ASICS_COUNT ; h++) {
     vm.asic[h].engines_down = 1;
     if (vm.loop[h/ASICS_PER_LOOP].enabled_loop &&
         !vm.asic[h].asic_present) {
        disable_engines_asic(h, with_reset);
     }

     if (vm.asic[h].asic_present) {
      vm.asic[h].engines_down = 0;
      if ((vm.asic[h].not_brocken_engines[0] != ENABLED_ENGINES_MASK) ||
          (vm.asic[h].not_brocken_engines[1] != ENABLED_ENGINES_MASK) ||
          (vm.asic[h].not_brocken_engines[2] != ENABLED_ENGINES_MASK) ||
          (vm.asic[h].not_brocken_engines[3] != ENABLED_ENGINES_MASK) ||
          (vm.asic[h].not_brocken_engines[4] != ENABLED_ENGINES_MASK) ||
          (vm.asic[h].not_brocken_engines[5] != ENABLED_ENGINES_MASK) ||
          (vm.asic[h].not_brocken_engines[6] != 0x1))
        {
          enable_engines_asic(h, vm.asic[h].not_brocken_engines, 0, 0);
        }
     }
   }
   return 1;
}


int wait_dll_ready(int a_addr,const char* why) {
   int dll;
   int i = 0;
   do {
     i++;
     dll = read_reg_asic(a_addr, NO_ENGINE, ADDR_INTR_BC_PLL_NOT_READY);
   } while ((dll != 0) && (i < 3000));

   if (dll != 0) {
      int addr = BROADCAST_READ_ADDR(dll);
      psyslog(RED "Error::: PLL stuck:%x asic(%d/%d) LOOP:%d (%s), %d\n" RESET,dll, a_addr,addr,addr/ASICS_PER_LOOP,why,i);      
      write_reg_asic(addr, NO_ENGINE, ADDR_PLL_ENABLE, 0x0);
      write_reg_asic(addr, NO_ENGINE, ADDR_PLL_ENABLE, 0x1);
      usleep(10000);
      dll = read_reg_asic(addr, NO_ENGINE, ADDR_INTR_BC_PLL_NOT_READY);
      if (dll != 0) {
        psyslog(RED "Discovered stuck PLL %x\n" RESET, dll);
        if (addr >= 0 && addr < ASICS_COUNT) {
          disable_asic_forever_rt(a_addr,"Stuck PLL A");
          // Try again          
          return -1;
        } else {
          print_chiko(1);
          vm.err_stuck_pll++;
          test_lost_address();
          restart_asics_full(8,"stuck PLL B");
          return 0;
        }
        //
      } else {
        psyslog(RED "PLL %d looks ok now.\n" RESET, addr);
      }
   }
   return 0;
}


void set_pll(int addr, int freq, int wait_dll_lock, int disable_enable_engines,  const char* why) {
  passert(vm.asic[addr].asic_present);
  passert(vm.asic[addr].dc2dc.dc2dc_present);  
  DBG(DBG_SCALING, CYAN "ASIC %d: %dHz -> %dHz - (%s)\n" RESET, 
    addr, 
    (vm.asic[addr].freq_hw), 
    (freq),
     why);

  passert(!(!wait_dll_lock && disable_enable_engines));

  
  if (disable_enable_engines) {
    disable_engines_asic(addr, 0);
  }

  
  passert(freq <= ASIC_FREQ_MAX);
  passert(freq >= ASIC_FREQ_MIN);
  passert(vm.asic[addr].engines_down);
  pll_frequency_settings *ppfs;
  ppfs = &pfs[HZ_TO_OFFSET(freq)];
  uint32_t pll_config = 0;
  uint32_t M = ppfs->m_mult;
  uint32_t P = ppfs->p_postdiv;
  uint32_t N = ppfs->n_prediv;
  pll_config = (M - 1) & 0xFF;
  pll_config |= ((N - 1) & 0x1F) << 8;
  pll_config |= ((P - 1) & 0x1F) << 13;
  pll_config |= 0x100000;
  //printf("Pll %d %x->%x\n",addr,vm.asic[addr].freq_hw, freq);
  write_reg_asic(addr, NO_ENGINE, ADDR_PLL_CONFIG, pll_config);

  write_reg_asic(addr, NO_ENGINE, ADDR_PLL_ENABLE, 0x0);
  write_reg_asic(addr, NO_ENGINE, ADDR_PLL_ENABLE, 0x1);

  flush_spi_write();
  vm.asic[addr].freq_hw = freq;

  if (wait_dll_lock) {
    wait_dll_ready(addr, why);
    while (wait_dll_ready(addr, why) != 0) {
       psyslog("DLL STUCK, TRYING AGAIN c5432\n");
    }    
  }

  if (disable_enable_engines) {
    enable_engines_asic(addr,vm.asic[addr].not_brocken_engines, 0,0);
  }   
}

void disable_asic_forever_rt(int addr, const char* why) {
  psyslog("Called disable ASIC reset in_reset:%d\n", vm.in_asic_reset);
  if (!vm.asic[addr].asic_present) {
    return;
  }

  // After X minutes - restart miner
  if (!vm.in_asic_reset) {
    mg_event_x("Run time failed crutial %d", addr);
    vm.err_runtime_disable++;  
    test_lost_address();
    restart_asics_full(9,"disable asic while running");
    return;
  }

  
  memset(vm.asic[addr].not_brocken_engines, 0, sizeof(vm.asic[addr].not_brocken_engines));
  vm.asic[addr].asic_present = 0;
  //vm.asic[addr].dc2dc.dc2dc_present = 0; 
  if (why) {
    vm.asic[addr].why_disabled = why;
  }
  int err;
  write_reg_asic(addr, NO_ENGINE,ADDR_GLOBAL_HASH_RESETN,0);
  write_reg_asic(addr, NO_ENGINE,ADDR_GLOBAL_CLK_EN,0);
  write_reg_asic(addr, NO_ENGINE,ADDR_INTR_MASK,0xFFFF);
  write_reg_asic(addr, NO_ENGINE,ADDR_DEBUG_CONTROL,BIT_ADDR_DEBUG_DISABLE_TRANSMIT);
  flush_spi_write();
  mg_event_x("Asic disable %d: %s, left:%d",addr,vm.asic[addr].why_disabled, vm.asic_count);

  for (int i = 0; i < ENGINE_BITMASCS; i++) {
     vm.asic[addr].not_brocken_engines[i] = 0;
  }
  vm.asic[addr].not_brocken_engines[ENGINE_BITMASCS-1] = 0;  
  disable_engines_asic(addr, 1);

/*
  if (!dc2dc_is_removed(addr)) {
    int err;
    //dc2dc_disable_dc2dc(addr,&err);
  }
  */
  
  if (vm.asic[addr].cooling_down) {
    vm.ac2dc[ASIC_TO_BOARD_ID(addr)].board_cooling_now--;
  }
  // dc2dc_disable_dc2dc(addr,&err);
  vm.asic_count--;
  psyslog("Disabing ASIC forever %d (0x%x) from loop %d (%s), count %d\n", addr, addr, addr/ASICS_PER_LOOP, why, vm.asic_count);
  if (vm.asic_count == 0) {
    passert(0);
  }
}
