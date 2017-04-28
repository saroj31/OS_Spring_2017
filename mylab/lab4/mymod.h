#include <linux/types.h>

//Declaring the fifo_entry
struct fifo_entry{
	__u32 command;
	__u32 value;
};


#define PCI_VENDOR_ID_CCORSI 0x1234
#define PCI_DEVICE_ID_CCORSI_KYOUKO3 0x1113

#define KYUOKO_CONTROL_SIZE 65536

#define FIFO_ENTRIES 1024

//registers
#define DEVICE_RAM 0x0020

#define FIFOSTART 0x1020
#define FIFOEND 0x1024
#define FIFOHEAD 0x4010
#define FIFOTAIL 0x4014

#define FRAMECOLUMNS 0x8000
#define FRAMEROWS 0x8004
#define FRAMEROWPITCH 0x8008
#define FRAMEPIXELFORMAT 0x800C
#define FRAMESTARTADDRESS 0x8010

#define ENCODERWIDTH 0x9000
#define ENCODERHEIGHT 0x9004
#define ENCODEROFFSETX 0x9008
#define ENCODEROFFSETY 0x900C
#define ENCODERFRAME 0x9010

#define CONFIGACCELERATION 0x1010
#define CONFIGMODESET 0x1008

#define DRAWCLEARCOLOR4FBLUE 0x5100
#define DRAWCLEARCOLOR4FGREEN 0x5104
#define DRAWCLEARCOLOR4FRED 0x5108
#define DRAWCLEARCOLOR4FALPHA 0x510C

#define RASTERCLEAR 0x3008
#define RASTERFLUSH 0x3FFC

#define RASTER_PRIMITIVE 0x3000
#define RASTER_EMIT 0x3004

//~~Triangle related Registers
//vertex Coords
#define DRAW_VERTEX_COORD4F_X 0x5000
#define DRAW_VERTEX_COORD4F_Y 0x5004
#define DRAW_VERTEX_COORD4F_Z 0x5008
#define DRAW_VERTEX_COORD4F_W 0x500C
//Color
#define DRAW_VERTEX_COLOR4f_R 0x5018
#define DRAW_VERTEX_COLOR4f_G 0x5014
#define DRAW_VERTEX_COLOR4f_B 0x5010
#define DRAW_VERTEX_COLOR4f_A 0x501C




//ioctl commands
//#define HELLO _IO(0xCC,1)   //For my testing
#define VMODE _IOW(0xCC,0,unsigned long)
#define BIND_DMA _IOW(0xCC,1,unsigned long)
#define START_DMA _IOWR(0xCC,2,unsigned long)
#define FIFO_QUEUE _IOWR(0xCC,3,unsigned long)
#define FIFO_FLUSH _IO(0xCC,4)
#define UBIND_DMA _IOW(0xCC,5,unsigned long)

#define GRAPHICS_ON 1
#define GRAPHICS_OFF 0
