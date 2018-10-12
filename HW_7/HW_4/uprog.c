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
	int usr_num;
	int flag = 1;
	fd = open("/dev/hw_4", O_RDWR);
	
	while(flag)
	{
		printf("Enter a new blink rate: ");
		scanf("%d",&usr_num);
		write(fd, &usr_num, sizeof(int));
		printf("\n set blink rate to %d\n",usr_num);
		printf("would you like to enter a new blink rate?\n");
		printf("Enter 1 for yes zero for no: ");
		scanf("%d", &flag);
		printf("You entered %d\n" , flag);
	}
	read(fd, &usr_num, sizeof(int));
	printf("the blink rate is currently set to:%d\n", usr_num);
	close(fd);
	return 0;
}
