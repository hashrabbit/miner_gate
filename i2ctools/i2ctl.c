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
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
//#include "hammer.h"
#include <spond_debug.h>
#include "squid.h"
#include "i2c.h"


using namespace std;
pthread_mutex_t network_hw_mutex = PTHREAD_MUTEX_INITIALIZER;


#define ACTION_READ_BYTE   0
#define ACTION_READ_WORD   1
#define ACTION_WRITE_BYTE  2
#define ACTION_WRITE_WORD  3


int usage(char * app ,int exCode ,const char * errMsg = NULL)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }
    
    printf ("Usage: %s action address [command value]\n\n" , app);    
    printf ("       action - rb|rw|wb|ww (read byte|read word|write byte|write word)\n"); 
    printf ("       address - in hexa\n"); 
    printf ("       command - [optoinal] in hexa \n"); 
    printf ("       value - for write in hexa \n"); 
    printf ("\n Note - if two values are given in a write action,\n        the second one is assumed to be value (and command omitted)\n");

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
     int test_mode = 0;
     int init_mode = 0;
     int i2c_action = -1;
     int bad_arguments = 0;
     int there_is_command = 0; // change to 1 if there is an expected command in the passed args.

     uint16_t w_val = 0;
     uint8_t b_val = 0;
     uint8_t b_command = 0; // also serve as default value for w/o command calls.
     uint8_t addr = 0;
     int rc = 0;



    if (argc < 2)
    {
        usage(argv[0],2); 
    }

    if ( argc == 2 && ( (strcmp(argv[1],"-h") == 0) ||
                        (strcmp(argv[1],"-?") == 0) ||
                        (strcmp(argv[1],"--help") == 0) ) )
    {
        usage(argv[0],0); 
        return 0;
    }

    if (strcmp(argv[1],"rb") == 0)
    {
        i2c_action = ACTION_READ_BYTE;
    } 
    else if (strcmp(argv[1],"rw") == 0) 
    {
        i2c_action = ACTION_READ_WORD;
    }
    else if (strcmp(argv[1],"wb") == 0) 
    {
        i2c_action = ACTION_WRITE_BYTE;
    }
    else if (strcmp(argv[1],"ww") == 0) 
    {
        i2c_action = ACTION_WRITE_WORD;
    }else
    {
        bad_arguments = 1;
        char msg[32];
        sprintf(msg , "action %s is unknown." , argv[1]);
        usage(argv[0] , 3 , (const char *)msg);
    }

    // on read we expect either argc==3,4 (0 , action , address [command])
    // on write we expect either argc==4,5 (0 , action , address [,command] , value)

    int maxexparg = 5; // maximum expected argc (for a write, with command)
    
    if (i2c_action == ACTION_READ_BYTE || i2c_action == ACTION_READ_WORD)
        maxexparg --;

    there_is_command = 0;   
    if (argc == maxexparg)
    {
        there_is_command = 1;
    }else if (argc != maxexparg-1)
    {
        usage(argv[0] , 4 , "Bad number of arguments passed");
    }
    

// Read parameters from command line into internal vars.
    errno = 0;
    rc = sscanf(argv[2] , "%x" , &addr);
    if (errno != 0 || rc == 0)
    {
        usage(argv[0] , errno , "Failed in conversion into addr");
    }

    if (there_is_command){
        rc = sscanf(argv[3] , "%x" , &b_command);
        if (errno != 0 || rc == 0)
        {
            usage(argv[0] , errno , "Failed in conversion into command");
        }
    }else
        b_command = 0;

    if ( ACTION_WRITE_BYTE == i2c_action ||ACTION_WRITE_WORD == i2c_action){
        rc = sscanf(argv[argc-1] , "%x" , (ACTION_WRITE_WORD == i2c_action) ? &w_val : &b_val);
        if (errno != 0 || rc == 0)
        {
            usage(argv[0] , errno , "Failed in conversion into value");
        }
    }

    
//    printf("Parsed args: Address Command Value are:0x%X (%d) , 0x%X (%d) , 0x%X (%d) \n" , addr , addr , b_command , b_command ,
//            (ACTION_WRITE_WORD == i2c_action) ? w_val : b_val ,
//            (ACTION_WRITE_WORD == i2c_action) ? w_val : b_val);    
    
    
   
    ///////////////////    ALL params are checked and read - let's call the actual command! ////////////////////////////    


    i2c_init();


    
    switch (i2c_action){
        case ACTION_READ_BYTE:
            b_val = i2c_read_byte(addr,b_command);
            printf("0x%X\n",b_val);
            break;
        case ACTION_READ_WORD:
            w_val = i2c_read_word(addr,b_command);
            printf("0x%X\n",w_val);
            break;
        case ACTION_WRITE_BYTE:
            i2c_write_byte(addr,b_command,b_val);
            break;
        case ACTION_WRITE_WORD:
            i2c_write_word(addr,b_command,w_val);
            break;
    }
    return 0;
}
