///
///simple util that reads AC2DC VPD info
///and prints it out.
///


#include "mainvpd.h"



int usage(char * app ,int exCode ,const char * errMsg)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }

    printf ("Usage: %s [-top|-bottom][[-q] [-a] [-p] [-s] [-r] [-f]] [-v vpd] [-vf fet]\n\n" , app);

    printf ("       -top : top main board\n");
    printf ("       -bottom : bottom main board (default)\n");
    printf ("        default mode is read\n");
    printf ("       -q : quiet mode, values only w/o headers\n");
    printf ("       -a : print all VPD params as one string\n");
    printf ("       -p : print part number\n");
    printf ("       -r : print revision number\n");
    printf ("       -s : print serial number\n");
    printf ("		-f : print FET type and code\n");
    printf ("       -v : vpd value to write\n");
    printf ("       -vf : fet value to write (integer)\n");
    if (0 == exCode) // exCode ==0 means - just print usage and get back to app for business. other value - exit with it.
    {
        return 0;
    }
    else
    {
        exit(exCode);
    }
}

int usage(char * app ,int exCode)
{
     return usage(app ,exCode ,NULL) ;
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
	bool print_fet = false;
	bool write = false;
	bool writefet = false;
	int fet_value  = -1;
	char vpd_str[32];
	char fet_str[16];
	int  vpd_rev = 0;
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
	else if ( 0 == strcmp(argv[i],"-f"))
	  print_fet = true;
	else if ( 0 == strcmp(argv[i],"-vf")){
	  writefet = true;
	  i++;
	  if (sscanf( argv[i], "%d", &fet_value ) < 0){
		  badParm = true;
	  }
	}
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

	if ((write||writefet) && (print_fet | print_ser | print_rev | print_pnr | print_all )){
		badParm = true;
	}


	// if no print spec was given (all are false, then set all sub fields (except for all)
	if (!(write||writefet) &&  false == (print_fet || print_all || print_pnr || print_rev || print_ser) ){
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

	if (writefet){
		rc = set_fet(topOrBottom , fet_value);
		if (rc == 0)
		{
			 get_fet_str(topOrBottom , fet_str);
			 printf("%s FET %s (%d)\n",h_board[topOrBottom],fet_str,fet_value);
		}
		else{
			return rc;
		}
	}

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
		 if (print_fet)
		 {
			 fet_value = get_fet(topOrBottom);
			 if (quiet){
				 printf("%d\n",fet_value);
			 }
			 else{
				 get_fet_str(topOrBottom , fet_str);
				 printf("%s FET %s (%d)\n",h_board[topOrBottom],fet_str,fet_value);
			 }
		 }
	 }
	 return rc;
}
