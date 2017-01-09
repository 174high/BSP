#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "chrdev_ioctl.h"

int main(int argc, const char *argv[])
{
	int fd;
	int buf[64];

	bzero(buf,64);

	fd = open("/dev/chrdev_driver1",O_RDWR);
	printf("> fd = %d",fd);
	if ( -1 == fd )
	{
		perror("fail to open");
		return -1;
	}

	while(1)
	{
		puts(">Please input the command(A,B,C,D,E,R,W,Q)!\n>");
		switch(getchar())
		{
		case 'a':
		case 'A':
			printf(">start the A...\n");
			ioctl(fd,A_START);
			break;
		case 'b':
		case 'B':
			printf(">start the B...\n");
			ioctl(fd,B_START);
			break;
		case 'c':
		case 'C':
			printf(">change the value...\n");
			ioctl(fd,B_STOP);
			break;
		case 'd':
		case 'D':
			printf(">A close B...\n");
			ioctl(fd,A_CLOSE_B);
			break;
		case 'e':
		case 'E':
			printf(">close A and B...\n");
			ioctl(fd,CLOSE_AB);
			break;
		case 'r':
		case 'R':
			puts(">read the character device...");
			read(fd,buf,64);
			break;
		case 'w':
		case 'W':
			puts(">write the character device...");
			write(fd,buf,64);
			break;
		case 'q':
		case 'Q':
			puts(">Quit the driver test application...");
			goto quit;
			break;
		default:
			puts(">The command of input is wrong...");
			break;
		}
	}

quit:
	close(fd);

	return 0;
}
