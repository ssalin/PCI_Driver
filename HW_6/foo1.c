#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main()
{
	int fd;
	ssize_t num_read, num_written;
	char my_read_str[10];
	unsigned short tmp,tmp1; /*to read from driver*/
	unsigned int clr = 0xffffff00; //clr low 4 bts (led0 ctl reg)
	unsigned int on  = 0x0000004e; //00000000000000000000000000001110
	unsigned int off = 0x0000000f; //00000000000000000000000000001111	
	fd = open("/dev/hw_6", O_RDWR);
		num_read = read(fd, &tmp, sizeof(int));//read
                tmp1 = tmp & 0xFF00;
                tmp1 = tmp1 >> 8; //head
                tmp1 = tmp1 & 0xff;
                tmp = (tmp & 0xff)-1; //clear higher bits
		printf("HEAD =  %d, TAIL = %d\n",tmp1 ,tmp);	
	close(fd);
	return 0;
}
