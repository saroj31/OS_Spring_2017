#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <asm/pgtable.h>

#define PCI_VENDOR_ID_CCORSI 0x1234
#define PCI_DEVICE_ID_CCORSI_KYOUKO3 0x1113
#define KYUOKO_CONTROL_SIZE 65536
#define DEVICE_RAM 0x0020

MODULE_LICENSE("Proprietary");   //macro defined in module.h giving the license details
MODULE_AUTHOR("Team07");   //src:module.h to give author of the module

//device Table
struct pci_device_id kyuoko3_dev_ids[] = {

	{PCI_DEVICE(PCI_VENDOR_ID_CCORSI,PCI_DEVICE_ID_CCORSI_KYOUKO3)},
	{0}
};

struct kyuoko3{
	u32 k_control_base;
	u32 k_ram_card_base;
	struct pci_dev *pciDevice;
	u32 *kv_control_base;
	u32 *kv_ram_card_base;

}kyuoko3;

int kyuoko3_probe(struct pci_dev *pci_device,const struct pci_device_id *pci_id)
{
	kyuoko3.pciDevice = pci_device;
	kyuoko3.k_control_base = pci_resource_start(pci_device,1);
	kyuoko3.k_ram_card_base = pci_resource_start(pci_device,2);
	
	//pci_device->irq;
	
	if(pci_enable_device(pci_device))
	   printk(KERN_ALERT "pci_device enable device error");

	pci_set_master(pci_device);
	printk(KERN_ALERT "Kyuoko3 probe is being called\n");
	return 0;
}

void kyuoko3_remove(struct pci_dev *pci_device){

	
	pci_disable_device(pci_device);
	printk(KERN_ALERT "kyuoko3_remove Call recieved\n");
	return;
}

struct pci_driver kyuoko3_pci_drv = {
	.name = "Kyuoko3_My_Device",
	.id_table = kyuoko3_dev_ids,
	.probe = kyuoko3_probe,
	.remove = kyuoko3_remove

};

unsigned int K_READ_REG(unsigned int reg)
{
	unsigned int value;
	//udelay(1);
	//printk(KERN_ALERT "K_READ_REG %x\n",kyuoko3.kv_control_base);
	rmb();
	value = *(kyuoko3.kv_control_base+(reg>>2));
	return value;
}


int kyouko3_open(struct inode *inode,struct file *fp){
   
  kyuoko3.kv_control_base = ioremap(kyuoko3.k_control_base,KYUOKO_CONTROL_SIZE);
  
  unsigned int ram_size = K_READ_REG(DEVICE_RAM);
  
  ram_size*=1024*1024;
  printk(KERN_ALERT "Kyuoko_open Ram_size = %d\n\n",ram_size ); 
  kyuoko3.kv_ram_card_base = ioremap(kyuoko3.k_ram_card_base,ram_size);   
  printk(KERN_ALERT "Kyouko_Kernel open\n");
  return 0;

}


int kyouko3_release(struct inode *inode, struct file *fp){

     iounmap(kyuoko3.kv_control_base);
     iounmap(kyuoko3.kv_ram_card_base);
     
     printk(KERN_ALERT "kyuoko3_release: BUUH BYE\n");
     return 0;
}

int kyuoko3_mmap(struct file *fp,struct vm_area_struct *vma){
	
	int ret=0;
	
	ret = io_remap_pfn_range( vma,vma->vm_start,
				  kyuoko3.k_control_base>>PAGE_SHIFT,
			   	  vma->vm_end - vma->vm_start,
			   	  vma->vm_page_prot);


	printk(KERN_ALERT "kyuoko3_mmap: called \n");

	return ret;
	
	
}


struct file_operations kyouko3_fops = {

	//file_operations structure dclared in fs.h and defined here to make useof it
	.open = kyouko3_open,
	.release = kyouko3_release,
	.mmap = kyuoko3_mmap,
	.owner = THIS_MODULE
};

struct cdev myDevice;

int my_init_function(void)
{

	cdev_init(&myDevice,&kyouko3_fops);
	cdev_add(&myDevice,MKDEV(500,127),1);
	
        pci_register_driver(&kyuoko3_pci_drv);
	printk(KERN_ALERT "init function from kernel\n");
	return 0;
}

module_init(my_init_function);

void my_exit_function(void)
{
	
	pci_unregister_driver(&kyuoko3_pci_drv);
	cdev_del(&myDevice);
	printk(KERN_ALERT "Exit Function Kernel\n");
	return;
}

module_exit(my_exit_function);



