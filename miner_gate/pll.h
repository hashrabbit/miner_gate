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

#ifndef _____PLLLIB_H__B__
#define _____PLLLIB_H__B__

#include <stdint.h>
#include <unistd.h>

#include "nvm.h"

typedef struct {
  uint16_t m_mult;
  uint16_t n_prediv;
  uint16_t p_postdiv;
} pll_frequency_settings;


#define HZ_TO_OFFSET(HZ) ((HZ-350)/5)
#define ASIC_FREQ_MAX   1480
#define ASIC_FREQ_MIN   350



#define	FREQ_350  {94 ,2,2}
#define	FREQ_355  {95 ,2,2} 
#define	FREQ_360  {96 ,2,2}
#define	FREQ_365	{97 ,2,2}
#define	FREQ_370	{99 ,2,2}
#define	FREQ_375	{100,2,2}
#define	FREQ_380	{101,2,2}
#define	FREQ_385	{103,2,2}
#define	FREQ_390	{104,2,2}
#define	FREQ_395	{105,2,2}
#define	FREQ_400	{107,2,2}
#define	FREQ_405	{108,2,2}
#define	FREQ_410	{109,2,2}
#define	FREQ_415	{111,2,2}
#define	FREQ_420	{112,2,2}
#define	FREQ_425	{113,2,2}
#define	FREQ_430	{115,2,2}
#define	FREQ_435	{116,2,2}
#define	FREQ_440	{117,2,2}
#define	FREQ_445	{119,2,2}
#define	FREQ_450	{120,2,2}
#define	FREQ_455	{121,2,2}
#define	FREQ_460	{123,2,2}
#define	FREQ_465	{124,2,2}
#define	FREQ_470	{125,2,2}
#define	FREQ_475	{127,2,2}
#define	FREQ_480	{128,2,2}
#define	FREQ_485	{129,2,2}
#define	FREQ_490	{131,2,2}
#define	FREQ_495	{132,2,2}
#define	FREQ_500	{133,2,2}
#define	FREQ_505	{135,2,2}
#define	FREQ_510	{136,2,2}
#define	FREQ_515	{137,2,2}
#define	FREQ_520	{139,2,2}
#define	FREQ_525	{140,2,2}
#define	FREQ_530	{141,2,2}
#define	FREQ_535	{143,2,2}
#define	FREQ_540	{144,2,2}
#define	FREQ_545	{145,2,2}
#define	FREQ_550	{147,2,2}
#define	FREQ_555	{148,2,2}
#define	FREQ_560	{149,2,2}
#define	FREQ_565	{151,2,2}
#define	FREQ_570	{152,2,2}
#define	FREQ_575	{153,2,2}
#define	FREQ_580	{155,2,2}
#define	FREQ_585	{156,2,2}
#define	FREQ_590	{157,2,2}
#define	FREQ_595	{159,2,2}
#define	FREQ_600	{160,2,2}
#define	FREQ_605	{161,2,2}
#define	FREQ_610	{163,2,2}
#define	FREQ_615	{164,2,2}
#define	FREQ_620	{165,2,2}
#define	FREQ_625	{167,2,2}
#define	FREQ_630	{168,2,2}
#define	FREQ_635	{169,2,2}
#define	FREQ_640	{171,2,2}
#define	FREQ_645	{172,2,2}
#define	FREQ_650	{173,2,2}
#define	FREQ_655	{175,2,2}
#define	FREQ_660	{176,2,2}
#define	FREQ_665	{177,2,2}
#define	FREQ_670	{179,2,2}
#define	FREQ_675	{180,2,2}
#define	FREQ_680	{181,2,2}
#define	FREQ_685	{183,2,2}
#define	FREQ_690	{184,2,2}
#define	FREQ_695	{185,2,2}
#define	FREQ_700	{187,2,2}
#define	FREQ_705	{188,2,2}
#define	FREQ_710	{189,2,2}
#define	FREQ_715	{191,2,2}
#define	FREQ_720	{192,2,2}
#define	FREQ_725	{193,2,2}
#define	FREQ_730	{195,2,2}
#define	FREQ_735	{196,2,2}
#define	FREQ_740	{197,2,2}
#define	FREQ_745	{199,2,2}
#define	FREQ_750	{150,3,1}
#define	FREQ_755	{151,3,1}
#define	FREQ_760	{152,3,1}
#define	FREQ_765	{153,3,1}
#define	FREQ_770	{154,3,1}
#define	FREQ_775	{155,3,1}
#define	FREQ_780	{156,3,1}
#define	FREQ_785	{157,3,1}
#define	FREQ_790	{158,3,1}
#define	FREQ_795	{159,3,1}
#define	FREQ_800	{160,3,1}
#define	FREQ_805	{161,3,1}
#define	FREQ_810	{162,3,1}
#define	FREQ_815	{163,3,1}
#define	FREQ_820	{164,3,1}
#define	FREQ_825	{165,3,1}
#define	FREQ_830	{166,3,1}
#define	FREQ_835	{167,3,1}
#define	FREQ_840	{168,3,1}
#define	FREQ_845	{169,3,1}
#define	FREQ_850	{170,3,1}
#define	FREQ_855	{171,3,1}
#define	FREQ_860	{172,3,1}
#define	FREQ_865	{173,3,1}
#define	FREQ_870	{174,3,1}
#define	FREQ_875	{175,3,1}
#define	FREQ_880	{176,3,1}
#define	FREQ_885	{177,3,1}
#define	FREQ_890	{178,3,1}
#define	FREQ_895	{179,3,1}
#define	FREQ_900	{180,3,1}
#define	FREQ_905	{181,3,1}
#define	FREQ_910	{182,3,1}
#define	FREQ_915	{183,3,1}
#define	FREQ_920	{184,3,1}
#define	FREQ_925	{185,3,1}
#define	FREQ_930	{186,3,1}
#define	FREQ_935	{187,3,1}
#define	FREQ_940	{188,3,1}
#define	FREQ_945	{189,3,1}
#define	FREQ_950	{190,3,1}
#define	FREQ_955	{191,3,1}
#define	FREQ_960	{192,3,1}
#define	FREQ_965	{193,3,1}
#define	FREQ_970	{194,3,1}
#define	FREQ_975	{195,3,1}
#define	FREQ_980	{196,3,1}
#define	FREQ_985	{197,3,1}
#define	FREQ_990	{198,3,1}
#define	FREQ_995	{199,3,1}
#define	FREQ_1000	{200,3,1}
#define	FREQ_1005	{201,3,1}
#define	FREQ_1010	{202,3,1}
#define	FREQ_1015	{203,3,1}
#define	FREQ_1020	{204,3,1}
#define	FREQ_1025	{205,3,1}
#define	FREQ_1030	{206,3,1}
#define	FREQ_1035	{207,3,1}
#define	FREQ_1040	{208,3,1}
#define	FREQ_1045	{209,3,1}
#define	FREQ_1050	{210,3,1}
#define	FREQ_1055	{211,3,1}
#define	FREQ_1060	{212,3,1}
#define	FREQ_1065	{213,3,1}
#define	FREQ_1070	{214,3,1}
#define	FREQ_1075	{215,3,1}
#define	FREQ_1080	{216,3,1}
#define	FREQ_1085	{217,3,1}
#define	FREQ_1090	{218,3,1}
#define	FREQ_1095	{219,3,1}
#define	FREQ_1100	{220,3,1}
#define	FREQ_1105	{221,3,1}
#define	FREQ_1110	{222,3,1}
#define	FREQ_1115	{223,3,1}
#define	FREQ_1120	{224,3,1}
#define	FREQ_1125	{225,3,1}
#define	FREQ_1130	{226,3,1}
#define	FREQ_1135	{227,3,1}
#define	FREQ_1140	{228,3,1}
#define	FREQ_1145	{229,3,1}
#define	FREQ_1150	{230,3,1}
#define	FREQ_1155	{231,3,1}
#define	FREQ_1160	{232,3,1}
#define	FREQ_1165	{233,3,1}
#define	FREQ_1170	{234,3,1}
#define	FREQ_1175	{235,3,1}
#define	FREQ_1180	{236,3,1}
#define	FREQ_1185	{237,3,1}
#define	FREQ_1190	{238,3,1}
#define	FREQ_1195	{239,3,1}
#define	FREQ_1200	{240,3,1}
#define	FREQ_1205	{241,3,1}
#define	FREQ_1210	{242,3,1}
#define	FREQ_1215	{243,3,1}
#define	FREQ_1220	{244,3,1}
#define	FREQ_1225	{245,3,1}
#define	FREQ_1230	{246,3,1}
#define	FREQ_1235	{247,3,1}
#define	FREQ_1240	{248,3,1}
#define	FREQ_1245	{249,3,1}
#define	FREQ_1250	{250,3,1}
#define	FREQ_1255	{251,3,1}
#define	FREQ_1260	{252,3,1}
#define	FREQ_1265	{253,3,1}
#define	FREQ_1270	{254,3,1}
#define	FREQ_1275	{255,3,1}
#define	FREQ_1280	{256,3,1}
#define FREQ_1285	{171,2,1}
#define FREQ_1290	{172,2,1}
#define FREQ_1295	{173,2,1}
#define FREQ_1300	{173,2,1}
#define FREQ_1305	{174,2,1}
#define FREQ_1310	{175,2,1}
#define FREQ_1315	{175,2,1}
#define FREQ_1320	{176,2,1}
#define FREQ_1325	{177,2,1}
#define FREQ_1330	{177,2,1}
#define FREQ_1335	{178,2,1}
#define FREQ_1340	{179,2,1}
#define FREQ_1345	{179,2,1}
#define FREQ_1350	{180,2,1}
#define FREQ_1355	{181,2,1}
#define FREQ_1360	{181,2,1}
#define FREQ_1365	{182,2,1}
#define FREQ_1370	{183,2,1}
#define FREQ_1375	{183,2,1}
#define FREQ_1380	{184,2,1}
#define FREQ_1385	{185,2,1}
#define FREQ_1390	{185,2,1}
#define FREQ_1395	{186,2,1}
#define FREQ_1400	{187,2,1}
#define FREQ_1405	{187,2,1}
#define FREQ_1410	{188,2,1}
#define FREQ_1415	{189,2,1}
#define FREQ_1420	{189,2,1}
#define FREQ_1425	{190,2,1}
#define FREQ_1430	{191,2,1}
#define FREQ_1435	{191,2,1}
#define FREQ_1440	{192,2,1}
#define FREQ_1445	{193,2,1}
#define FREQ_1450	{193,2,1}
#define FREQ_1455	{194,2,1}
#define FREQ_1460	{195,2,1}
#define FREQ_1465	{195,2,1}
#define FREQ_1470	{196,2,1}
#define FREQ_1475	{197,2,1}
#define FREQ_1480	{198,2,1}
#define FREQ_1485	{198,2,1}
#define FREQ_1490	{199,2,1}
#define FREQ_1495	{199,2,1}
#define FREQ_1500 {200,2,1}
















void init_asic_clocks(int addr);
void disable_engines_all_asics(int with_reset);
void disable_engines_asic(int addr,int with_reset);
void enable_engines_asic(int addr, uint32_t engines_mask[7], int with_unreset, int reset_others);
void set_pll(int addr, int freq, int wait_dll_lock, int disale_enable_engines, const char* why);
void set_plls_to_wanted(const char* why);

int enable_good_engines_all_asics_ok_restart_if_error(int with_reset);
void disable_asic_forever_rt_restart_if_error(int addr, int assert_if_none_left, const char* why);
void enable_all_engines_all_asics(int with_reset);
// return -1 if there is a problem
int wait_dll_ready_restart_if_error(int addr, const char* why);

//void set_asic_freq(int addr, int new_freq);

#endif
