#include "mymod.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <asm/pgtable.h>
#include <asm/mman.h>


//macro declarations
#define NUM_BUFF 8

MODULE_LICENSE("Proprietary");   //macro defined in module.h giving the license details
MODULE_AUTHOR("Saroj Dash");   //src:module.h to give author of the module

//device Table
struct pci_device_id kyouko3_dev_ids[] = {

	{PCI_DEVICE(PCI_VENDOR_ID_CCORSI,PCI_DEVICE_ID_CCORSI_KYOUKO3)},
	{0}
};


//for the fifo
struct fifo{
	u64 p_base;
	struct fifo_entry *k_base; //k_base will be an array of fifo_entry
	u32 head;
	u32 tail;
};

//Structure of dma buffer
struct dma_buffer{
	u64 phy_base;
	u32 kv_base;
	u32 *v_base;
	unsigned int count;
}dma_buffs[NUM_BUFF]; //8 buffers declared

//Structure that holds all info of kyouko3 device
struct kyouko3{
	u32 k_control_base;
	u32 k_ram_card_base;
	struct pci_dev *pciDevice;
	u32 *kv_control_base;
	u32 *kv_ram_card_base;
	struct fifo fifo;
	_Bool bGraphics_on;
	_Bool bdma_mapped;
	u32 fill;
	u32 drain;

}kyouko3;

unsigned int K_READ_REG(unsigned int reg)
{
	unsigned int value;
	//printk(KERN_ALERT "K_READ_REG %x\n",kyouko3.kv_control_base);
	udelay(1);
	rmb();
	value = *(kyouko3.kv_control_base+(reg>>2));
	return value;
}

void K_WRITE_REG(unsigned int reg,unsigned int value)
{
	udelay(1);
	*(kyouko3.kv_control_base + (reg>>2)) = value;
}



//Basic operation of FIFO_WRITE
void FIFO_WRITE(unsigned int reg,unsigned int value) {

	kyouko3.fifo.k_base[kyouko3.fifo.head].command = reg;
	kyouko3.fifo.k_base[kyouko3.fifo.head].value = value;
	kyouko3.fifo.head+=1;

	if( kyouko3.fifo.head > FIFO_ENTRIES) kyouko3.fifo.head=0;

}



//Probes the device driver where we fill up the device driver details.
int kyouko3_probe(struct pci_dev *pci_device,const struct pci_device_id *pci_id)
{
	kyouko3.pciDevice = pci_device;
	kyouko3.k_control_base = pci_resource_start(pci_device,1);
	kyouko3.k_ram_card_base = pci_resource_start(pci_device,2);

	//pci_device->irq;

	if(pci_enable_device(pci_device))
	   printk(KERN_ALERT "pci_device enable device failed\n");

	pci_set_master(pci_device);
	printk(KERN_ALERT "Kyuoko3 probe is being called\n");
	return 0;
}

void kyouko3_remove(struct pci_dev *pci_device){

	pci_disable_device(pci_device);
	printk(KERN_ALERT "kyouko3_remove Call recieved\n");
	return;
}


//File operation API .open implemented. The FIFO details are filled up here.
int kyouko3fops_open(struct inode *inode,struct file *fp){
 
  unsigned int ram_size;

  kyouko3.kv_control_base = ioremap(kyouko3.k_control_base,KYUOKO_CONTROL_SIZE);

 
  ram_size = K_READ_REG(DEVICE_RAM);

  ram_size *= 1024*1024;
  //printk(KERN_ALERT "Kyuoko_open Ram_size = %d\n\n",ram_size );
  kyouko3.kv_ram_card_base = ioremap(kyouko3.k_ram_card_base,ram_size);

  //allocate memory for FIFO BUFFER
  kyouko3.fifo.k_base = pci_alloc_consistent(	kyouko3.pciDevice,
						8192u,
						&kyouko3.fifo.p_base);
  K_WRITE_REG(FIFOSTART,kyouko3.fifo.p_base);
  K_WRITE_REG(FIFOEND, kyouko3.fifo.p_base + 8192u);

  kyouko3.fifo.head = 0;
  kyouko3.fifo.tail = 0;

  kyouko3.bGraphics_on = 0;
  kyouko3.bdma_mapped = 0;


  printk(KERN_ALERT "Kyouko_Kernel open\n");
  return 0;

}

//ioctl function to flush the commands
void fifo_flush(void)
{
	K_WRITE_REG(FIFOHEAD,kyouko3.fifo.head);
	
	while(kyouko3.fifo.tail != kyouko3.fifo.head){
		kyouko3.fifo.tail = K_READ_REG(FIFOTAIL);
		schedule();
	}
	return;
}

//Sets Graphics Mode to OFF
void off_GraphicsMode(void)
{

	fifo_flush();

	K_WRITE_REG(CONFIGACCELERATION,0x80000000);

	K_WRITE_REG(CONFIGMODESET,0);

	msleep(10);

	kyouko3.bGraphics_on = 0;
	return;
}

//Releases the Driver and its allocated resources
int kyouko3fops_release(struct inode *inode, struct file *fp){

     off_GraphicsMode();
     pci_free_consistent(kyouko3.pciDevice,8192u,
			 kyouko3.fifo.k_base,kyouko3.fifo.p_base);

     iounmap(kyouko3.kv_control_base);
     iounmap(kyouko3.kv_ram_card_base);

     printk(KERN_ALERT "kyouko3_release: BUUH BYE\n");
	     return 0;
}


//API for mapping of data
int kyouko3fops_mmap(struct file *fp,struct vm_area_struct *vma){

	int ret=0;
	
	int offset = vma->vm_pgoff << PAGE_SHIFT;

	switch( offset ){

	    case 0:
		printk(KERN_ALERT "Mapped the Control base\n");
		ret = io_remap_pfn_range( vma,vma->vm_start,
					  kyouko3.k_control_base>>PAGE_SHIFT,
				   	  vma->vm_end - vma->vm_start,
				   	  vma->vm_page_prot);
		break;

	     case 0x80000000:
		printk(KERN_ALERT "Mapped the Ram base\n");
		ret = io_remap_pfn_range( vma,vma->vm_start,
					  kyouko3.k_ram_card_base>>PAGE_SHIFT,
					  vma->vm_end - vma->vm_start,
					  vma->vm_page_prot);
					  
		 //add a case for dma allocation of memory
		break;
	      default:
		ret = 1;
		printk(KERN_ALERT "Default call mmap\n");
		break;


	}
	printk(KERN_ALERT "kyouko3_mmap: called \n");
	return ret;
}


struct pci_driver kyouko3_pci_drv = {
	.name = "Kyuoko3_My_Device",
	.id_table = kyouko3_dev_ids,
	.probe = kyouko3_probe,
	.remove = kyouko3_remove

};


void on_GraphicsMode(void)
{
	float zero = 0.0f, blue = 0.2f;

	K_WRITE_REG(FRAMECOLUMNS,1024);
	K_WRITE_REG(FRAMEROWS,768);
	K_WRITE_REG(FRAMEROWPITCH,1024*4);
	K_WRITE_REG(FRAMEPIXELFORMAT, 0xf888);
	K_WRITE_REG(FRAMESTARTADDRESS,0);


	K_WRITE_REG(CONFIGACCELERATION,0x40000000);

	K_WRITE_REG(ENCODERWIDTH,1024);
	K_WRITE_REG(ENCODERHEIGHT,768);
	K_WRITE_REG(ENCODEROFFSETX,0);
	K_WRITE_REG(ENCODEROFFSETY,0);
	K_WRITE_REG(ENCODERFRAME,0);

	K_WRITE_REG(CONFIGMODESET,0);

	msleep(10);

	FIFO_WRITE(DRAWCLEARCOLOR4FRED,*(unsigned int*)(&zero));
	FIFO_WRITE(DRAWCLEARCOLOR4FGREEN,*(unsigned int*)(&zero));
	FIFO_WRITE(DRAWCLEARCOLOR4FBLUE,*(unsigned int*)(&blue));
	FIFO_WRITE(DRAWCLEARCOLOR4FALPHA,*(unsigned int*)(&zero));

	FIFO_WRITE(RASTERCLEAR, 0x03);
	FIFO_WRITE(RASTERFLUSH, 0x0);

	fifo_flush();

	kyouko3.bGraphics_on = 1;

	return;
}



irqreturn_t dma_intr_handler(int i, void *addr, struct pt_regs *reg){
	
	irqreturn_t ret;
	
	return ret;
	
}
//ioctl methods
void fifo_vmode(unsigned int iG_Mode)
{
	if( GRAPHICS_OFF == iG_Mode )
		off_GraphicsMode();
	else
		on_GraphicsMode();
	return;
}


//ioctl function FIFO_QUEUE
long fifo_queue(unsigned int cmd, unsigned long arg){

	long ret;
	struct fifo_entry fifo_entry;
	ret = copy_from_user(	&fifo_entry,
				(struct fifo_entry*)arg,
				sizeof(struct fifo_entry));
	FIFO_WRITE(fifo_entry.command,fifo_entry.value);
	return ret;
}

//ioctl bind_dma
int bind_dma(unsigned long *oUsrAddr){
	
	int ret = 0;
	int i = 0;
	unsigned long addr;
	
    //FILE *fp;  //Keeping it NULL
	
	//Allocation of memory for 8 buffers
	//1. Memory allocation for the Buffer and set oUsrAddr
	for(i =0; i < 8; i++){
		kyouko3.fill = i;
		//We get Physical address and Kernel virtual address
		dma_buffs[i].kv_base = pci_alloc_consistent( kyouko3.pciDevice,
						NUM_BUFF*124*1024,
						&dma_buffs[i].phy_base);
						
		
		//offset is other than 0 and 0x8000 00000				
		addr = vm_mmap(	0, 0,
				NUM_BUFF*124*1024, PROT_READ | PROT_WRITE,
				MAP_SHARED, 0x1 ); 
		
		
	}
	
	ret = pci_enable_device(kyouko3.pciDevice);
	if(ret)
		printk(KERN_ALERT "kyouko3 device could not be enabled\n");
	
	
	//2. Configure the Interrupt handler by using request_irq
	
	ret = request_irq(	kyouko3.pciDevice->irq,
						(irq_handler_t)&dma_intr_handler,
						IRQF_SHARED,
						"dma_intr_handler",
						&kyouko3);
	if(ret){
		pci_disable_device(kyouko3.pciDevice);
		printk(KERN_ALERT "request_irq failed\n");
		return -EIO;
	}
	//3. Set INTERRUPT_SET register to value 0x02
	K_WRITE_REG(INTERRUPT_SET,0x02);
	
	//assign oUsrAddr with the address of first index buffer
	oUsrAddr = copy_to_user(	*oUsrAddr,dma_buffs[0].kv_base,
								sizeof(dma_buffs[0].kv_base));
	
	return ret;
}


int ubind_dma(){
	
	int ret = 0;
	int i = 0;
	//Free the Buffers that is allocated.
	for( i =0; i<NUM_BUFF; i++){
		pci_free_consistent(kyouko3.pciDevice,124*1024,
			 dma_buffs[i].kv_base,dma_buffs[i].phy_base);
	}
	return ret;
	
}

void initiate_transfer(unsigned int iCnt){
	
	/*Here we will transfer the iCnt number of bytes of commands to the 
	buffer. This will increment the fill counter of the buffer queue.*/
	
	
	return;
	
}


int start_dma(unsigned int iCnt){
	
	int ret = 0;
	//Do initiate transfer
	
	/*Also assign interrupt handler here, this shall increment the drain
	 * index in the buffer queue.
	*/
	
	return 0;
}

long kyouko3fops_ioctl(struct file *fp,unsigned int cmd, unsigned long arg){

	long ret = 0;

	switch(cmd){

	case FIFO_QUEUE:
		ret = fifo_queue(cmd, arg); //Function to be defined
		break;

	case FIFO_FLUSH:
		fifo_flush();    //Function to be defined
		break;

	case VMODE:
		fifo_vmode(arg);     //Function to handle subcommands
				    //GRAPHICS_OFF and GRAPHICS_ON
		break;
		
	case BIND_DMA:
		bind_dma(&arg);
		break;
	
	case UBIND_DMA:
		ubind_dma();
		break;
	
	case START_DMA:
		break;

	default:
		printk(KERN_ALERT "IOCTL Default call\n");
		break;

	}
	return ret;
}

struct file_operations kyouko3_fops = {

//file_operations structure dclared in fs.h and defined here to make useof it
	.open = kyouko3fops_open,
	.release = kyouko3fops_release,
	.mmap = kyouko3fops_mmap,
	.owner = THIS_MODULE,
	.unlocked_ioctl = kyouko3fops_ioctl
};

struct cdev myDevice;

int my_init_function(void)
{
	int rc =0;
	cdev_init(&myDevice,&kyouko3_fops);
	cdev_add(&myDevice,MKDEV(500,127),1);

	rc = pci_register_driver(&kyouko3_pci_drv);
	printk(KERN_ALERT "init function from kernel\n");
	return rc;
}

module_init(my_init_function);

void my_exit_function(void)
{

	pci_unregister_driver(&kyouko3_pci_drv);
	cdev_del(&myDevice);
	printk(KERN_ALERT "Exit Function Kernel\n");
	return;
}

module_exit(my_exit_function);
