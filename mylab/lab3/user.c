#include "mymod.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#define WIDTH 1024
#define HEIGHT 768
#define PIXEL_SIZE 4

struct u_kyuoko_device {

  unsigned int *u_control_base;
  unsigned int *u_frame_base;
	
}kyouko3;

unsigned int U_READ_REG(unsigned int rgister)
{
	
	return (*(kyouko3.u_control_base+(rgister>>2)));
	
}

void U_WRITE_FB(int i,unsigned int color){

	*(kyouko3.u_frame_base + i) = color;
}

int main(void)
{
	int fd,i;
	unsigned int result;
	unsigned int frameSize = WIDTH * HEIGHT * PIXEL_SIZE;
	
	unsigned long command;
	command = FIFO_QUEUE;
	command = command << 32;
	command += 0x3;
	
	fd = open("/dev/kyouko3",O_RDWR);
	kyouko3.u_control_base = mmap(0,KYUOKO_CONTROL_SIZE,
					PROT_READ|PROT_WRITE,
					MAP_SHARED,fd,0);
	
	printf("%x\n",(kyouko3.u_control_base));
	result = U_READ_REG(DEVICE_RAM);
	printf("Ram size in MB is: %d\n",result);
	

	
	kyouko3.u_frame_base = mmap(0,frameSize,PROT_READ|PROT_WRITE,
				      MAP_SHARED, fd, 0x80000000);

	
	ioctl(fd,VMODE,GRAPHICS_ON);

	for(i = 200*1024; i<201*1024; ++i)
		U_WRITE_FB(i,0xff0000);

	ioctl(fd, FIFO_QUEUE, &command);
	ioctl(fd, FIFO_FLUSH, 0);

	sleep(5);

	ioctl(fd,VMODE,GRAPHICS_OFF);
	close(fd);
	return 0;
}

