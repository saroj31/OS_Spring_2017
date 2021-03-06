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

struct kyouko3{
	u32 k_control_base;
	u32 k_ram_card_base;
	struct pci_dev *pciDevice;
	u32 *kv_control_base;
	u32 *kv_ram_card_base;
	struct fifo fifo;
	_Bool bGraphics_on;
	_Bool bdma_mapped;

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


void fifo_flush(void)
{
	K_WRITE_REG(FIFOHEAD,kyouko3.fifo.head);
	
	while(kyouko3.fifo.tail != kyouko3.fifo.head){
		kyouko3.fifo.tail = K_READ_REG(FIFOTAIL);
		schedule();
	}
	return;
}


void off_GraphicsMode(void)
{

	//fifo_flush();

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

	switch( vma->vm_pgoff << PAGE_SHIFT){

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


void FIFO_WRITE(unsigned int reg,unsigned int value) {

	kyouko3.fifo.k_base[kyouko3.fifo.head].command = reg;
	kyouko3.fifo.k_base[kyouko3.fifo.head].value = value;
	kyouko3.fifo.head+=1;

	if( kyouko3.fifo.head > FIFO_ENTRIES) kyouko3.fifo.head=0;

}

long fifo_queue(unsigned int cmd, unsigned long arg){

	long ret;
	struct fifo_entry fifo_entry;
	ret = copy_from_user(	&fifo_entry,
				(struct fifo_entry*)arg,
				sizeof(struct fifo_entry));
	FIFO_WRITE(fifo_entry.command,fifo_entry.value);
	return ret;
}


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


}


void fifo_vmode(unsigned int iG_Mode)
{
	if( GRAPHICS_OFF == iG_Mode )
		off_GraphicsMode();
	else
		on_GraphicsMode();
	return;
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
