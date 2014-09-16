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
#include "mainvpd.h"

using namespace std;
extern pthread_mutex_t i2c_mutex;

#ifdef SP2x
	  uint8_t I2C_DC2DC_SWITCH = I2C_DC2DC_SWITCH_GROUP0;
#else
	  uint8_t I2C_DC2DC_SWITCH = I2C_DC2DC_SWITCH_GROUP1;
#endif

void resetI2CSwitches(){
	int err;
	i2c_write(I2C_DC2DC_SWITCH, PRIMARY_I2C_SWITCH_DEAULT , &err);
	usleep(2000);

	i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT , &err);
	usleep(2000);
}

inline int fix_max_cap(int val, int max){
	if (val < 0)
		return 0;
	else if (val > max)
		return max;
	else
		return val;
}


int usage(char * app ,int exCode ,const char * errMsg = NULL)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }
    
    printf ("Usage: %s [-top|-bottom][[-q] [-a] [-p] [-s] [-r]] [-v vpd]\n\n" , app);

    printf ("       -top : top main board\n");
    printf ("       -bottom : bottom main board (default)\n");
    printf ("        default mode is read\n");
    printf ("       -q : quiet mode, values only w/o headers\n"); 
    printf ("       -a : print all VPD params as one string\n"); 
    printf ("       -p : print part number\n"); 
    printf ("       -r : print revision number\n"); 
    printf ("       -s : print serial number\n"); 
    printf ("       -v : vpd value to write\n");
    if (0 == exCode) // exCode ==0 means - just print usage and get back to app for business. other value - exit with it.
    {
        return 0;
    }
    else 
    {
        exit(exCode);
    }
}

int setI2CSwitches(int tob){

	int err = 0;
	int I2C_MY_MAIN_BOARD_PIN = (tob == TOP_BOARD)?PRIMARY_I2C_SWITCH_BOARD0_MAIN_PIN : PRIMARY_I2C_SWITCH_BOARD1_MAIN_PIN;
	i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN | PRIMARY_I2C_SWITCH_DEAULT , &err);

	if (err==0){
		usleep(2000);
		i2c_write(I2C_DC2DC_SWITCH,  PRIMARY_I2C_SWITCH_DEAULT , &err);
	}
	else{
		fprintf(stderr,"failed calling i2c set 0x70 %x\n", I2C_MY_MAIN_BOARD_PIN | PRIMARY_I2C_SWITCH_DEAULT);
		return err;
	}

	if (err==0){
		usleep(2000);
		i2c_write(I2C_DC2DC_SWITCH,  MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);
		usleep(2000);
	}
	else{
		fprintf(stderr,"failed calling i2c set 0x71 %x\n", PRIMARY_I2C_SWITCH_DEAULT);
		return err;
	}

	return err;
}

int mainboard_set_vpd(  int topOrBottom, const char * vpd){

	int rc = 0;
	int err=0;
	bool firstTimeBurn = false;

	pthread_mutex_lock(&i2c_mutex);

	err = setI2CSwitches(topOrBottom);

	int vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);

	if (vpdrev == 0xFF){
			firstTimeBurn = true;
			i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
			usleep(2000);
			for (int a = 1 ; a < MAIN_BOARD_VPD_EEPROM_SIZE ; a++){
				i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, a , 0 , &err);
				usleep(2000);
			}
	}

	for (int b = 0 ; b < (MAIN_BOARD_VPD_RESERVED_ADDR_START-MAIN_BOARD_VPD_SERIAL_ADDR_START ); b++)
	{
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, b+MAIN_BOARD_VPD_SERIAL_ADDR_START  , vpd[b] , &err);
		usleep(2000);
		if(err != 0)
			break;
	}

	if (err!=0)
	{
		rc = err;
	}

	resetI2CSwitches();

	pthread_mutex_unlock(&i2c_mutex);

	return rc;

}

int readMain_I2C_eeprom (char * vpd_str , int topOrBottom , int startAddress , int length){
	int rc = 0;
	int err=0;

	pthread_mutex_lock(&i2c_mutex);

	err = setI2CSwitches(topOrBottom);

	if (err==0){
		for (int i = 0; i < length; i++) {
			vpd_str[i] = (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, (unsigned char)(startAddress+i), &err);
			if (err!= 0)
				break;
		}
	}

	if (err!=0)
	{
		rc = err;
	}

	resetI2CSwitches();

	pthread_mutex_unlock(&i2c_mutex);

	return rc;
}

int mainboard_get_vpd(int topOrBottom ,  mainboard_vpd_info_t *pVpd) {
	int rc = 0;
	int err = 0;

	char vpd_str[32];

	if (pVpd == NULL ) {
		fprintf(stderr,"call mainboard_get_vpd performed without allocating sturcture first\n");
		return 1;
	}

	err = readMain_I2C_eeprom(vpd_str , topOrBottom , 0 , 32);

	strncpy(pVpd->pnr, vpd_str + MAIN_BOARD_VPD_PNR_ADDR_START, MAIN_BOARD_VPD_PNR_ADDR_LENGTH );
	pVpd->pnr[MAIN_BOARD_VPD_PNR_ADDR_LENGTH] = '\0';

	strncpy(pVpd->revision, vpd_str + MAIN_BOARD_VPD_PNRREV_ADDR_START, MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH );
	pVpd->revision[MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH] = '\0';

	strncpy(pVpd->serial, vpd_str + MAIN_BOARD_VPD_SERIAL_ADDR_START, MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH );
	pVpd->serial[MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH] = '\0';

	if (err) {
		fprintf(stderr, RED            "Failed reading %s Main Board VPD (err %d)\n" RESET,(topOrBottom==TOP_BOARD)?"TOP":"BOTTOM", err);
		rc =  err;
	}
	return rc;
}

int main(int argc, char *argv[])
{
	int rc = 0;


	bool callUsage = false;
	bool badParm = false;
	bool quiet = false;
	bool print_all = false;
	bool print_pnr = false;
	bool print_rev = false;
	bool print_ser = false;
	bool write = false;
	char vpd_str[32];
	int topOrBottom = -1; // default willbe TOP, start with -1, to rule out ambiguity.

	const char * h_board[] = {"TOP MAIN BOARD ","BOTTOM MAIN BOARD "};
	const char * h_all = "VPD: ";
	const char * h_pnr = "PNR: ";
	const char * h_rev = "REV: ";
	const char * h_ser = "SER: ";



	for (int i = 1 ; i < argc ; i++){
	if ( 0 == strcmp(argv[i],"-h"))
	  callUsage = true;
	else if ( 0 == strcmp(argv[i],"-q"))
	  quiet = true;
	else if ( 0 == strcmp(argv[i],"-a"))
	  print_all = true;
	else if ( 0 == strcmp(argv[i],"-p"))
	  print_pnr = true;
	else if ( 0 == strcmp(argv[i],"-r"))
	  print_rev = true;
	else if ( 0 == strcmp(argv[i],"-s"))
	  print_ser = true;
	else if ( 0 == strcmp(argv[i],"-v")){
	  write = true;
	  i++;
	  strncpy(vpd_str,argv[i],sizeof(vpd_str));
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

	if (write && (print_ser | print_rev | print_pnr | print_all )){
		badParm = true;
	}


	// if no print spec was given (all are false, then set all sub fields (except for all)
	if (!write &&  false == (print_all || print_pnr || print_rev || print_ser) ){
	print_pnr = true;
	print_rev = true;
	print_ser = true;
	}

	if(badParm)
	{
		usage(argv[0],1,"Bad arguments");
	}

	// applying default as BOTTOM.
	if (-1 == topOrBottom)
	  topOrBottom = BOTTOM_BOARD;

	if (callUsage)
	return usage(argv[0] , 0);

	int err = 0;

	i2c_init(&err);

	if (err != 0)
	{
		fprintf(stderr,"FAILED to init i2c bus\n");
		return 4;
	}


	mainboard_vpd_info_t vpd = {}; // allocte, and initializero


	 if (write){
		 rc  = mainboard_set_vpd(topOrBottom, vpd_str );

		 if (0 == rc)
		 {
				 printf("%s%s\n",quiet?"":h_board[topOrBottom],vpd_str);
		 }

	 }

	 else{

		 rc  = mainboard_get_vpd(topOrBottom ,&vpd   );

		 if (0 == rc)
		 {
			 if (print_all)
				 printf("%s%s%s%s%s\n",quiet?"":h_board[topOrBottom],quiet?"":h_all,vpd.serial,vpd.pnr,vpd.revision);
			 if (print_pnr)
				 printf("%s%s%s\n",quiet?"":h_board[topOrBottom],quiet?"":h_pnr,vpd.pnr);
			 if (print_rev)
				 printf("%s%s%s\n",quiet?"":h_board[topOrBottom],quiet?"":h_rev,vpd.revision);
			 if (print_ser)
				 printf("%s%s%s\n",quiet?"":h_board[topOrBottom],quiet?"":h_ser,vpd.serial);
		 }
	 }
	 return rc;
}
