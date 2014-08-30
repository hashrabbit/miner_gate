#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int fd;
    unsigned char c;

    int start = -1;
    int end = -1;
    int ind = 0;



	if (argc < 4)
	{
		fprintf(stderr,"Not Enought parms %d\n",argc);
		return 1;
	}


    sscanf(argv[2],"%d",&start);
    sscanf(argv[3],"%d",&end);

	if (start == -1 || end == -1)
	{
		fprintf(stderr,"parm 2-start (%s),3-end (%s) should be ins\n",argv[2],argv[3]);
		return 2;
	}

	if (start > end)
	{
		fprintf(stderr,"parm 2-start (%s), should be smaller than 3-end (%s)\n",argv[2],argv[3]);
		return 3;
	}


	     unsigned char * buff = new unsigned char[2 + end - start];
	
    /* needs error checking */
    fd = open(argv[1], O_RDONLY);

	if (fd <  0)
	{
		fprintf(stderr,"Failed to open file %s)\n",argv[1]);
		return 4;
	}
	
	int skval = lseek(fd,0,SEEK_END) ;	

//	printf("end file offset %d \n",skval);


	if (skval < start ){
		fprintf(stderr,"file offset %d invalid (maybe bigger than file?)\n",start);
		return 5;
	}

    
	if (skval < end ){
		fprintf(stderr,"file offset %d invalid (maybe bigger than file?)\n",end);
		return 6;
	}

	lseek(fd,start,SEEK_SET) ;	

	if (read(fd,buff,(1 + end - start)) < 0)
	{
		fprintf(stderr,"read error\n");
		return 7;
	}

//    for (ind = start ; ind <= end ; ind++){
//	read(fd, &c, sizeof(c));
  //  	printf("%d: <0x%x> - %c\n", ind ,c , c);
//	buff[ind-start] = c;
	    
//    }

	buff[1 + end - start] = 0;
	printf("%s\n",buff);

    	return 0;
}
