#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int fd;
    unsigned char c;

    int start = -1;
    int end = -1;
    int ind = 0;
    unsigned char * wstr;


	if (argc < 4)
	{
		fprintf(stderr,"Not Enought parms %d\n",argc);
		return 1;
	}

	wstr = (unsigned char *)argv[3];
	int len = strlen (argv[3]);


    sscanf(argv[2],"%d",&start);
    //sscanf(argv[3],"%d",&end);

	if (start == -1 )
	{
		fprintf(stderr,"parm 2-start (%s) should be int\n",argv[2]);
		return 2;
	}



	
    /* needs error checking */
    	fd = open(argv[1], O_RDWR);

	if (fd <  0)
	{
		fprintf(stderr,"Failed to open file %s)\n",argv[1]);
		return 4;
	}
	
	int skval = lseek(fd,0,SEEK_END) ;	



	if (skval < start ){
		fprintf(stderr,"file offset %d invalid (maybe bigger than file?)\n",start);
		return 5;
	}

    
	if (skval < start + len)
	{
		fprintf(stderr,"file offset %d invalid (maybe bigger than file?)\n",start + len);
		return 6;
	}

	lseek(fd,start,SEEK_SET) ;	
	
	if (write(fd,wstr,len) < 0)
	{
		fprintf(stderr,"write error\n");
		return 7;
	}
	return 0;
}
