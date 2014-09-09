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

///
///simple util that reads AC2DC VPD info
///and prints it out.
///

//#define drpintf printf
#include <stdio.h>


#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
//#include "hammer.h"
#include <spond_debug.h>
#include "asic.h"
#include "squid.h"
#include "i2c.h"
#include "ac2dc.h"
#include "dc2dc.h"


using namespace std;
//pthread_mutex_t network_hw_mutex = PTHREAD_MUTEX_INITIALIZER;

int usage(char * app ,int exCode ,const char * errMsg = NULL)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }
    
    printf ("Usage: %s [-q] [-a] [-p] [-m] [-r] [-s] [-t] [-top|-bottom|-both]\n\n" , app);

    printf ("       -q : quiet mode, values only w/o headers\n"); 
    printf ("       -a : print all VPD params as one string\n"); 
    printf ("       -p : print part number\n"); 
    printf ("       -m : print model IDl\n"); 
    printf ("       -r : print revision number\n"); 
    printf ("       -s : print serial number\n");
    printf ("       -t : print print PSU TYPE (EMERSON1200,MURATA)\n");
    printf ("       -top : top PSU\n");
    printf ("       -bottom : bottom PSU (default)\n");
    printf ("       -both : both top & bottom PSUs\n");

    if (0 == exCode) // exCode ==0 means - just print usage and get back to app for business. other value - exit with it.
    {
        return 0;
    }
    else 
    {
        exit(exCode);
    }
}

int main(int argc, char *argv[])
{
     int rc = 0;
     int rctemp=0;
     AC2DC ac2dc[2];

  bool callUsage = false;
  bool badParm = false;
  bool quiet = false;
  bool print_all = false;
  bool print_pnr = false;
  bool print_mod = false;
  bool print_rev = false;
  bool print_ser = false;
  bool print_type = false;

  bool showTop = false;
  bool showBot = false;

  const char * h_all = "AC2DC VPD: ";
  const char * h_pnr = "AC2DC PNR: ";
  const char * h_mod = "AC2DC MOD: ";
  const char * h_rev = "AC2DC REV: ";
  const char * h_ser = "AC2DC SER: ";
  const char * h_type = "AC2DC TYPE: ";

  
  for (int i = 1 ; i < argc ; i++){
    if ( 0 == strcmp(argv[i],"-h"))
      callUsage = true;
    else if ( 0 == strcmp(argv[i],"-q"))
      quiet = true;
    else if ( 0 == strcmp(argv[i],"-a"))
      print_all = true;
    else if ( 0 == strcmp(argv[i],"-p"))
      print_pnr = true;
    else if ( 0 == strcmp(argv[i],"-m"))
      print_mod = true;
    else if ( 0 == strcmp(argv[i],"-r"))
      print_rev = true;
    else if ( 0 == strcmp(argv[i],"-s"))
       print_ser = true;
    else if ( 0 == strcmp(argv[i],"-t"))
       print_type = true;
    else if ( 0 == strcmp(argv[i],"-top"))
        showTop = true;
     else if ( 0 == strcmp(argv[i],"-bottom"))
       showBot = true;
     else if ( 0 == strcmp(argv[i],"-both")){
    	 showBot = true;
    	 showTop = true;
     }
      else
      badParm = true;

  }

  // if no print spec was given (all are false, then set all sub fields (except for all)
  if ( false == (print_type || print_all || print_pnr || print_mod || print_rev || print_ser) ){
	  print_type = true;
	  print_pnr = true;
    print_mod = true;
    print_rev = true;
    print_ser = true;
  }

  // default is showBottom
  if (! ( showTop || showBot))
	  showBot = true;

  if(badParm)
  {
    usage(argv[0],1,"Bad arguments");
  }  

  if (callUsage)
    return usage(argv[0] , 0);

     i2c_init();

     ac2dc_init2(ac2dc);

     /*
     if (ac2dc[PSU_BOTTOM].ac2dc_type == AC2DC_TYPE_NONE)
     {
    	  printf("NO BOTTOM PSU\n");
     }

     if (ac2dc[PSU_TOP].ac2dc_type == AC2DC_TYPE_NONE)
     {
         	  printf("NO TOP PSU\n");
     }
      */

     ac2dc_vpd_info_t vpd = {}; // allocte, and initializero
     if (showTop){
         rc += rctemp  = ac2dc_get_vpd(&vpd , 0 , &ac2dc[0] );
         if (0 == rctemp)
         {

           if (! quiet)
        	   printf("TOP    PSU:\n==========\n");
           if (print_all)
             printf("%s%s%s%s\n",quiet?"":h_all,vpd.pnr,vpd.model,vpd.serial);

           if (print_type)
           {
        	   char psu_type[32]={};
        	 switch (ac2dc[0].ac2dc_type){
        	 case AC2DC_TYPE_UNKNOWN:
        		 sprintf(psu_type,"%s","NO_PSU");
        		 break;
        	 case AC2DC_TYPE_MURATA_NEW:
        		 sprintf(psu_type,"%s","MURATA_NEW");
        		 break;
        	 case AC2DC_TYPE_MURATA_OLD:
        		 sprintf(psu_type,"%s","MURATA_OLD");
        		 break;
        	 case AC2DC_TYPE_EMERSON_1_2:
        		 sprintf(psu_type,"%s","EMERSON1200");
        		 break;
        	 default:
        		 sprintf(psu_type,"%s","UNKNOWN_PSU");
        		 break;
        	 }

        	 printf("%s%s\n",quiet?"":h_type,psu_type);
           }


           if (print_pnr)
             printf("%s%s\n",quiet?"":h_pnr,vpd.pnr);
           if (print_mod)
             printf("%s%s\n",quiet?"":h_mod,vpd.model);
           if (print_rev)
             printf("%s%s\n",quiet?"":h_rev,vpd.revision);
           if (print_ser)
             printf("%s%s\n",quiet?"":h_ser,vpd.serial);
         }
         else
         {
        	  printf("TOP    PSU:\n==========\nFailed getting VPD\n");
         }

     }

     if (showBot){
         rc += rctemp = ac2dc_get_vpd(&vpd , 1 , &ac2dc[1] );
         if (0 == rctemp)
         {
           if (! quiet)
          	   printf("BOTTOM PSU:\n==========\n");

           if (print_all)
             printf("%s%s%s%s\n",quiet?"":h_all,vpd.pnr,vpd.model,vpd.serial);

           if (print_type)
           {
        	   char psu_type[32]={};
        	 switch (ac2dc[1].ac2dc_type){
        	 case AC2DC_TYPE_UNKNOWN:
        		 sprintf(psu_type,"%s","NO_PSU");
        		 break;
        	 case AC2DC_TYPE_MURATA_NEW:
        		 sprintf(psu_type,"%s","MURATA_NEW");
        		 break;
        	 case AC2DC_TYPE_MURATA_OLD:
        		 sprintf(psu_type,"%s","MURATA_OLD");
        		 break;
        	 case AC2DC_TYPE_EMERSON_1_2:
        		 sprintf(psu_type,"%s","EMERSON1200");
        		 break;
        	 default:
        		 sprintf(psu_type,"%s","UNKNOWN_PSU");
        		 break;
        	 }

        	 printf("%s%s\n",quiet?"":h_type,psu_type);
           }



           if (print_pnr)
             printf("%s%s\n",quiet?"":h_pnr,vpd.pnr);
           if (print_mod)
             printf("%s%s\n",quiet?"":h_mod,vpd.model);
           if (print_rev)
             printf("%s%s\n",quiet?"":h_rev,vpd.revision);
           if (print_ser)
             printf("%s%s\n",quiet?"":h_ser,vpd.serial);
         }
         else
         {
        	  printf("BOTTOM PSU:\n==========\nFailed getting VPD\n");
         }

     }

  return rc;
}
