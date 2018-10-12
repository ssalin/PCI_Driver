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
	unsigned int tmp; /*to read from driver*/
	unsigned int clr = 0xffffff00; //clr low 4 bts (led0 ctl reg)
	unsigned int on  = 0x0000004e; //00000000000000000000000000001110
	unsigned int off = 0x0000000f; //00000000000000000000000000001111	
	fd = open("/dev/hw_3_driver_0", O_RDWR);
	
	/*turn LED0 on*/
		num_read = read(fd, &tmp, sizeof(int));//read
		printf("THE LED CTRL REG CONTAINS %x\n",tmp);
		tmp = tmp & clr;//modify
		tmp = tmp | on;
		num_written = write(fd, &tmp, sizeof(int));//write
		printf("LED0 should be ON, wrote %x\n", tmp);
	/*wait 2 sec*/
		sleep(2); //sleep 2 sec
	/*read back, display*/
		num_read = read(fd, &tmp, sizeof(int)); //read
		printf("THE LED control register contains %x\n", tmp);
	/*turn off LED*/
		tmp = tmp & clr;//modify //tmp should still be set from rd
		tmp = tmp | off;
		num_written = write(fd, &tmp, sizeof(int));//write
		printf("LED0 should be OFF\n");
	
	close(fd);
	return 0;
}
