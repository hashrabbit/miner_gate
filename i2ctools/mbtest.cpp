///
///simple util that reads AC2DC VPD info
///and prints it out.
///


#include <stdio.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
//#include "hammer.h"
#include <spond_debug.h>
#include "squid.h"
#include "i2c.h"
#include "dc2dc.h"
#include "mbtest.h"
#include "mainvpd.h"
#include "hammer_lib.h"
using namespace std;
extern pthread_mutex_t i2c_mutex;

int FIRSTLOOP = TOP_FIRST_LOOP ;
int LASTLOOP = BOTTOM_LAST_LOOP;

int usage(char * app ,int exCode ,const char * errMsg = NULL)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }
    
    printf ("Usage: %s [-top|-bottom|-both]\n\n" , app);

    printf ("       -top : top main board\n");
    printf ("       -bottom : bottom main board\n");
    printf ("       -both : both top & bottom main boards (default)\n");

    if (0 == exCode) // exCode ==0 means - just print usage and get back to app for business. other value - exit with it.
    {
        return 0;
    }
    else 
    {
        exit(exCode);
    }
}


int EnableLoops() {
	int rc = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		int err = 0;
		dc2dc_enable_dc2dc(i, &err);
		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d ENABLE FAIL\n", i);
		} else {
//			fprintf(stdout, "LOOP %2d ENABLE PASS\n", i);
		}
	}
	return rc;
}

int GetLoopCurrent() {
	int rc = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		int err = 0;
		int loopcur = -1;
		uint8_t looptemp = 0;
		loopcur = dc2dc_get_current_16s_of_amper(i, &err , &err,&looptemp ,&err);
		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d CURRENT MEASURE FAIL\n", i);
		} else {
			//fprintf(stdout, "LOOP %2d CURRENT MEASURE PASS -%5d\n", i, loopcur);
		}
	}
	return rc;
}

int GetLoopVoltage(int baseline = 0) {
	int rc = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		int err = 0;
		int loopvoltage = -1;
		loopvoltage = dc2dc_get_voltage(i, &err);
		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d VOLTAGE MEASURE FAIL\n", i);
		} else {
			if (baseline != 0){
				float precision = ((float)loopvoltage/(float)baseline);
				if (precision < 1.02 && precision > 0.98 ){
					//fprintf(stdout, "LOOP %2d SET VOLTAGE PASS [Target:%d , Measure:%d (Ratio=%4.3f)]\n", i, loopvoltage , baseline , precision);
				}
				else
				{
					rc = rc | (1 << i) ; // setting bitmap for error!
					fprintf(stdout, "LOOP %2d SET VOLTAGE FAIL [Target:%d , Measure:%d (Ratio=%4.3f)]\n", i, loopvoltage , baseline , precision);
				}
			}
			else{
				//fprintf(stdout, "LOOP %2d VOLTAGE MEASURE PASS -%5d\n", i, loopvoltage);
			}

		}
	}
	return rc;
}

int GetLoopsTemp(){
	int rc = 0;
	int err = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		err = 0;
		int looptemp = -1;

		looptemp = dc2dc_get_temp(i , &err);

		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d GET TEMP FAIL\n", i);
		} else {
			//fprintf(stdout, "LOOP %2d GET TEMP PASS (%d)\n", i , looptemp);
		}
	}
	return rc;
}

int SetLoopsVtrim(int vt) {
	int rc = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		int err = 0;
		int loopvoltage = -1;
		dc2dc_set_vtrim(i, vt,0, &err);
		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d SET VOLTAGE FAIL\n", i);
		} else {
		//	fprintf(stdout, "LOOP %2d SET VOLTAGE PASS\n", i);
		}
	}
	return rc;
}

int formatBadLoopsString ( int badloops ,  char * formatted){
	int index = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		if (1 == ((badloops >> i ) & 0x00000001)){ // loop i is bad
			sprintf(formatted+index  , "%2d, ",i+1);
			index += 4;
		}
	}
	// terminate string (allow reuse)
	formatted[index] = '\0';

	// better terminate without the ", " in the end.
	if (index > 1){
		formatted[index-2] = '\0';
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int err;


	bool callUsage = false;
	bool badParm = false;

	int topOrBottom = -1; // default willbe TOP, start with -1, to rule out ambiguity.

	char badloopsformattedstring[100];

	for (int i = 1 ; i < argc ; i++){
	if ( 0 == strcmp(argv[i],"-h"))
	  callUsage = true;

	else if ( 0 == strcmp(argv[i],"-both")){
	  if (topOrBottom == -1 || topOrBottom == BOTH_MAIN_BOARDS)
		  topOrBottom = BOTH_MAIN_BOARDS;
	  else
		  badParm = true;
	}
	else if ( 0 == strcmp(argv[i],"-top")){
	  if (topOrBottom == -1 || topOrBottom == TOP_BOARD)
		  topOrBottom = TOP_BOARD;
	  else
		  badParm = true;
	}
	else if ( 0 == strcmp(argv[i],"-bottom")){
	  if (topOrBottom == -1 || topOrBottom == BOTTOM_BOARD)
		  topOrBottom = BOTTOM_BOARD;
	  else
		  badParm = true;
	}
	else
	  badParm = true;
	}

	if(badParm)
	{
		usage(argv[0],1,"Bad arguments");
	}

	// applying default as top.
	if (-1 == topOrBottom)
	  topOrBottom = BOTH_MAIN_BOARDS;

	switch (topOrBottom){
		case TOP_BOARD:
			FIRSTLOOP = TOP_FIRST_LOOP;
			LASTLOOP = TOP_LAST_LOOP;
			break;
		case BOTTOM_BOARD:
			FIRSTLOOP = BOTTOM_FIRST_LOOP;
			LASTLOOP = BOTTOM_LAST_LOOP;
			break;
		case BOTH_MAIN_BOARDS:
			FIRSTLOOP = TOP_FIRST_LOOP;
			LASTLOOP = BOTTOM_LAST_LOOP;
			break;
	}



	if (callUsage)
	return usage(argv[0] , 0);


	i2c_init(&err);

	dc2dc_init();

	int failedloops;
	printf ("+============  ENABLE  LOOPS  =================\n");
	failedloops =  EnableLoops();
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL ENABLE LOOPS - %d groups failed (%s)\n" , count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS ENABLE LOOPS\n");
	printf ("-----------------------------------------------\n\n");

	printf ("+============  MEASURE CURRENT =================\n");
	failedloops = GetLoopCurrent();
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL MEASURE CURRENT - %d groups failed (%s)\n" , count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS MEASURE CURRENT\n");
	printf ("-----------------------------------------------\n\n");

	printf ("+============  MEASURE VOLTAGE =================\n");
	failedloops = GetLoopVoltage();
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL MEASURE VOLTAGE - %d groups failed (%s)\n" , count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS MEASURE VOLTAGE\n");
	printf ("-----------------------------------------------\n\n");


	printf ("+============  SET VOLTAGE %d ===============\n", VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN,0));
	failedloops = SetLoopsVtrim(VTRIM_MIN);
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d groups failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN,0), count_bits(failedloops) , badloopsformattedstring);
	}else{
		usleep(200000);
		failedloops = GetLoopVoltage(VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN,0));
		if (failedloops > 0){
			formatBadLoopsString(failedloops , badloopsformattedstring);
			printf ("TEST-FAIL SET VOLTAGE %d - %d groups failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN,0), count_bits(failedloops) , badloopsformattedstring);
		}
		else
			printf ("TEST-PASS SET VOLTAGE %d\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN,0));
	}
	printf ("-----------------------------------------------\n\n");


	printf ("+============  SET VOLTAGE %d ===============\n", VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH,0));
	failedloops = SetLoopsVtrim(VTRIM_HIGH);
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d groups failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH,0), count_bits(failedloops) , badloopsformattedstring);
	}else{
		usleep(200000);
		failedloops = GetLoopVoltage(VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH,0));
		if (failedloops > 0){
			formatBadLoopsString(failedloops , badloopsformattedstring);
			printf ("TEST-FAIL SET VOLTAGE %d - %d groups failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH,0), count_bits(failedloops) , badloopsformattedstring);
		}
		else
			printf ("TEST-PASS SET VOLTAGE %d\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH,0));
	}
	printf ("-----------------------------------------------\n\n");

	printf ("+============  SET VOLTAGE %d ===============\n", VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM,0));
	failedloops = SetLoopsVtrim(VTRIM_MEDIUM);
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d groups failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM,0), count_bits(failedloops) , badloopsformattedstring);
	}else{
		usleep(200000);
		failedloops = GetLoopVoltage(VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM,0));
		if (failedloops > 0){
			formatBadLoopsString(failedloops , badloopsformattedstring);
			printf ("TEST-FAIL SET VOLTAGE %d - %d groups failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM,0), count_bits(failedloops) , badloopsformattedstring);
		}
		else
			printf ("TEST-PASS SET VOLTAGE %d\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM,0));
	}
	printf ("-----------------------------------------------\n\n");

//	printf ("+============  GET DC2DC TEMP ===============\n", VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM,0));
//	failedloops = GetLoopsTemp();
//	if (failedloops > 0){
//		formatBadLoopsString(failedloops , badloopsformattedstring);
//		printf ("TEST-FAIL GET DC2DC TEMP - %d groups failed (%s)\n" , count_bits(failedloops) , badloopsformattedstring);
//	}
//	else
//		printf ("TEST-PASS GET DC2DC TEMP\n");
//	printf ("-----------------------------------------------\n\n");


//	if (do_bist_ok_rt(0)){
//		printf ("TEST-PASS ASICs SELF TEST\n");
//	}else{
//		printf ("TEST-FAIL ASICs SELF TEST\n");
//	}


	return rc;
}
