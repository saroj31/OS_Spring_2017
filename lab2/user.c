#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/mman.h>
#include <linux/types.h>

#define KYUOKO_CONTROL_SIZE 65536
#define DEVICE_RAM 0x0020

struct u_kyuoko_device {

  unsigned int *u_control_base;
	
}kyuoko3;

unsigned int U_READ_REG(unsigned int rgister)
{
	
	return (*(kyuoko3.u_control_base+(rgister>>2)));
	
}

int main()
{
	int fd;
	int result;
	
	//printf("Hello from main\n");
	
	fd = open("/dev/kyouko3",O_RDWR);
	kyuoko3.u_control_base = mmap(0,KYUOKO_CONTROL_SIZE,
					PROT_READ|PROT_WRITE,
					MAP_SHARED,fd,0);
	
	printf("%x\n",(kyuoko3.u_control_base));
	result = U_READ_REG(DEVICE_RAM);
	printf("Ram size in MB is: %d\n",result);
	close(fd);
	return 0;
}

