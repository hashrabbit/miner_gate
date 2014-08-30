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

int usage(char * app ,int exCode ,const char * errMsg = NULL)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }
    
    printf ("Usage: %s [-top|-bottom][-q] [-a] [-p] [-s] [-r]\n\n" , app);

    printf ("       -top : top main board (default)\n");
    printf ("       -bottom : bottom main board\n");
    printf ("       -q : quiet mode, values only w/o headers\n"); 
    printf ("       -a : print all VPD params as one string\n"); 
    printf ("       -p : print part number\n"); 
    printf ("       -r : print revision number\n"); 
    printf ("       -s : print serial number\n"); 

    if (0 == exCode) // exCode ==0 means - just print usage and get back to app for business. other value - exit with it.
    {
        return 0;
    }
    else 
    {
        exit(exCode);
    }
}


int foo(int topOrBottom ) {
	 int err;
	 i2c_init();
	 uint8_t chr;

	 int I2C_MY_MAIN_BOARD_PIN = (topOrBottom == TOP_BOARD)?PRIMARY_I2C_SWITCH_TOP_MAIN_PIN : PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN;
	 i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN, &err);
	 i2c_write(I2C_DC2DC_SWITCH_GROUP1 , MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);

	 //const char buff[45]{};

	 //const unsigned char * buff = "1234567890123456789012345678901234";

	 //my_i2c_set_address(MAIN_BOARD_I2C_EEPROM_DEV_ADDR,&err);


	 //__i2c_write_block_data(MAIN_BOARD_VPD_ADDR_START,43,buff );

	 for (int i = 0; i < MAIN_BOARD_VPD_ADDR_END ; i++) {

//		 i2c_waddr_write_byte( MAIN_BOARD_I2C_EEPROM_DEV_ADDR ,(uint16_t)(0x00FF & i) , 65 );

		 i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR,i,0x65,&err );
		 usleep(200);
//		 i2c_write_word(MAIN_BOARD_I2C_EEPROM_DEV_ADDR,i,0x0000,&err );
		 usleep(3000);
	 }

  i2c_write(PRIMARY_I2C_SWITCH, 0x00);
   pthread_mutex_unlock(&i2c_mutex);

	return 0;
}

int bar(int topOrBottom ) {
	 int err;
	 i2c_init();
	 uint8_t chr;
	 uint16_t wchr;
	 int I2C_MY_MAIN_BOARD_PIN = (topOrBottom == TOP_BOARD)?PRIMARY_I2C_SWITCH_TOP_MAIN_PIN : PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN;
	 i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN, &err);
	 i2c_write(I2C_DC2DC_SWITCH_GROUP1 , MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);

	 for (int i = 0; i < MAIN_BOARD_VPD_ADDR_END ; i++) {
		 //chr = i2c_waddr_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR , (uint16_t)(0x00FF & i), &err);
		 chr = i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR ,  i, &err);
		 printf(" 0x%X - 0x%X (%c)\n" , i , chr , chr);
		 usleep(3000);

//		 wchr = i2c_read_word(MAIN_BOARD_I2C_EEPROM_DEV_ADDR ,  i, &err);
//		 printf(" 0x%X - 0x%X (%c)\n" , i , chr , chr);
//		 usleep(3000);

	 }

   i2c_write(PRIMARY_I2C_SWITCH, 0x00);
    pthread_mutex_unlock(&i2c_mutex);

	return 0;
}

int mainboard_get_vpd(int topOrBottom ,  mainboard_vpd_info_t *pVpd) {
	int rc = 0;
	int err = 0;

	if (NULL == pVpd) {
		psyslog("call ac2dc_get_vpd performed without allocating sturcture first\n");
		return 1;
	}

  	 pthread_mutex_lock(&i2c_mutex);

	 i2c_init();

	 int I2C_MY_MAIN_BOARD_PIN = (topOrBottom == TOP_BOARD)?PRIMARY_I2C_SWITCH_TOP_MAIN_PIN : PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN;

	 i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN, &err);

	 if (err) {
		 fprintf(stderr, "Failed writing to I2C address 0x%X (err %d)",PRIMARY_I2C_SWITCH, err);

		 pthread_mutex_unlock(&i2c_mutex);
		 return err;
	 }


	 i2c_write(I2C_DC2DC_SWITCH_GROUP1 , MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);

	 if (err) {
		 fprintf(stderr, "Failed writing to I2C address 0x%X (err %d)",I2C_DC2DC_SWITCH_GROUP1, err);
		 pthread_mutex_unlock(&i2c_mutex);
		 return err;
	 }

	 for (int i = 0; i < MAIN_BOARD_VPD_PNR_ADDR_LENGTH ; i++) {
		 pVpd->pnr[i] = i2c_waddr_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR , (uint16_t)(0x00FF & (MAIN_BOARD_VPD_PNR_ADDR_START + i)), &err);
		 if (err)
			 goto get_eeprom_err;
	 }

	  for (int i = 0; i < MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH; i++) {
		pVpd->revision[i] = i2c_waddr_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR , (uint16_t)(0x00FF & (MAIN_BOARD_VPD_PNRREV_ADDR_START + i)), &err);
		if (err)
		  goto get_eeprom_err;
	  }

	  for (int i = 0; i < MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH; i++) {
		pVpd->serial[i] = i2c_waddr_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR , (uint16_t)(0x00FF & (MAIN_BOARD_VPD_SERIAL_ADDR_START + i)), &err);
		if (err)
		  goto get_eeprom_err;
	  }


get_eeprom_err:

  if (err) {
    fprintf(stderr, RED            "Failed reading AC2DC eeprom (err %d)\n" RESET, err);
    rc =  err;
  }
    i2c_write(PRIMARY_I2C_SWITCH, 0x00);
     pthread_mutex_unlock(&i2c_mutex);

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

	////////////////////////////////////////////
	if (argc == 2){
		if ( 0 == strcmp(argv[1],"1")){
			foo(topOrBottom);
			return 0;
		}

		else if ( 0 == strcmp(argv[1],"2")){
			bar(topOrBottom);
			return 0;
		}

		else {
			fprintf(stderr , "special mode - either 1[write] or 2[read]\n");
			return 126;
		}

	}
	else{
		fprintf(stderr , "special mode - either 1[write] or 2[read]\n");
		return 127;
	}
	////////////////////////////////////////////

	// if no print spec was given (all are false, then set all sub fields (except for all)
	if ( false == (print_all || print_pnr || print_rev || print_ser) ){
	print_pnr = true;
	print_rev = true;
	print_ser = true;
	}

	if(badParm)
	{
	usage(argv[0],1,"Bad arguments");
	}

	// applying default as top.
	if (-1 == topOrBottom)
	  topOrBottom = TOP_BOARD;

	if (callUsage)
	return usage(argv[0] , 0);

	 mainboard_vpd_info_t vpd = {}; // allocte, and initializero


	rc  = mainboard_get_vpd(topOrBottom, &vpd);

	if (0 == rc)
	{
		if (print_all)
		  printf("%s%s%s%s%s\n",quiet?"":h_board[topOrBottom],quiet?"":h_all,vpd.pnr,vpd.serial,vpd.revision);
		if (print_pnr)
		  printf("%s%s%s\n",quiet?"":h_board[topOrBottom],quiet?"":h_pnr,vpd.pnr);
		if (print_rev)
		  printf("%s%s%s\n",quiet?"":h_board[topOrBottom],quiet?"":h_rev,vpd.revision);
		if (print_ser)
		  printf("%s%s%s\n",quiet?"":h_board[topOrBottom],quiet?"":h_ser,vpd.serial);
	}
	return rc;
}
