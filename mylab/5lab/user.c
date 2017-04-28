#include "mymod.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/types.h>

#define WIDTH 1024
#define HEIGHT 768
#define PIXEL_SIZE 4

struct coord{
   float x,y,z,w;
};

struct color{
   float r,g,b,a;
};

struct vertex_coord_color{
  struct coord coord;
  struct color color;
};

struct u_kyuoko_device {

  unsigned int *u_control_base;
  unsigned int *u_frame_base;
  int fDesc;

}kyouko3;

unsigned int U_READ_REG(unsigned int rgister)
{

	return (*(kyouko3.u_control_base+(rgister>>2)));

}

void U_WRITE_FB(int i,unsigned int color){

	*(kyouko3.u_frame_base + i) = color;
}

//Enter FIFO entry of command and value into the FIFO in Kernel
void Queue_FIFO(unsigned int uiCommand, unsigned int uiValue){

  struct fifo_entry fifo_entry = {uiCommand,uiValue};
  ioctl(kyouko3.fDesc,FIFO_QUEUE,&fifo_entry);

  return;
}

void setVertexBuffer(struct coord ivertCoord){

  Queue_FIFO(DRAW_VERTEX_COORD4F_X,*(unsigned int *)&(ivertCoord.x));
  Queue_FIFO(DRAW_VERTEX_COORD4F_Y,*(unsigned int *)&(ivertCoord.y));
  Queue_FIFO(DRAW_VERTEX_COORD4F_Z,*(unsigned int *)&(ivertCoord.z));
  Queue_FIFO(DRAW_VERTEX_COORD4F_W,*(unsigned int *)&(ivertCoord.w));

  return;

}

void setColorBuffer(struct color ivertColor){

  Queue_FIFO(DRAW_VERTEX_COLOR4f_R,*(unsigned int *)&(ivertColor.r));
  Queue_FIFO(DRAW_VERTEX_COLOR4f_G,*(unsigned int *)&(ivertColor.g));
  Queue_FIFO(DRAW_VERTEX_COLOR4f_B,*(unsigned int *)&(ivertColor.b));
  Queue_FIFO(DRAW_VERTEX_COLOR4f_A,*(unsigned int *)&(ivertColor.a));

  return;
}

int DrawTriangle(){

  // Set the vertices and the color here
  Queue_FIFO(RASTER_PRIMITIVE,2);

 // struct vertex_coord_color stTriangleVert[3];

  struct coord vCoord;
  struct color vColor;


  //1st vertex
  //position
  vCoord.x = -0.5f;
  vCoord.y = -0.5f;
  vCoord.z = 0.0f;
  vCoord.w = 1.0f;
    
  setVertexBuffer(vCoord);
  //color
  vColor.r = 1.0f;
  vColor.g = 0.0f;
  vColor.b = 0.0f;
  vColor.a = 0.0f;

  setColorBuffer(vColor);

  Queue_FIFO(RASTER_EMIT,0);
  //Queue_FIFO(RASTERFLUSH,0x0);

  //2nd vertex
  vCoord.x = 0.5f;
  vCoord.y = 0.0f;
  vCoord.z = 0.0f;
  vCoord.w = 1.0f;

  setVertexBuffer(vCoord);
  //color
  vColor.r = 0.0f;
  vColor.g = 1.0f;
  vColor.b = 0.0f;
  vColor.a = 0.0f;

  setColorBuffer(vColor);

  Queue_FIFO(RASTER_EMIT,0);

  //3rd vertex

  vCoord.x = 0.125f;
  vCoord.y = 0.5f;
  vCoord.z = 0.0f;
  vCoord.w = 1.0f;

  setVertexBuffer(vCoord);
  //color
  vColor.r = 0.0f;
  vColor.g = 0.0f;
  vColor.b = 1.0f;
  vColor.a = 0.0f;

  setColorBuffer(vColor);

  Queue_FIFO(RASTER_EMIT,0);

  Queue_FIFO(RASTER_PRIMITIVE,0);

  Queue_FIFO(RASTERFLUSH,0x0);

  ioctl(kyouko3.fDesc,FIFO_FLUSH,0);

  return 0;
}



int main(void)
{
	int fd,i;
	unsigned int result;
	unsigned int frameSize = WIDTH * HEIGHT * PIXEL_SIZE;


	kyouko3.fDesc = open("/dev/kyouko3",O_RDWR);
	fd = kyouko3.fDesc;
	kyouko3.u_control_base = mmap(0,KYUOKO_CONTROL_SIZE,
					PROT_READ|PROT_WRITE,
					MAP_SHARED,fd,0);

	//~~ get the RAM Size
	printf("%x\n",(kyouko3.u_control_base));
	result = U_READ_REG(DEVICE_RAM);
	printf("Ram size in MB is: %d\n",result);
	//~~
	
	//** setting the graphics mode to VMODE
	kyouko3.u_frame_base = mmap(0,frameSize,PROT_READ|PROT_WRITE,
				      MAP_SHARED, fd, 0x80000000);


	ioctl(fd,VMODE,GRAPHICS_ON);
	
	ioctl(fd, FIFO_FLUSH, 0);
	//**/
	
	//Drawing the triangle ata specific position
	DrawTriangle();
	//for(i = 200*1024; i<201*1024; ++i)
	//	U_WRITE_FB(i,0xff0000);

	//ioctl(fd, FIFO_QUEUE, &command);

	sleep(2);

	ioctl(fd,VMODE,GRAPHICS_OFF);
	close(fd);
	return 0;
}
