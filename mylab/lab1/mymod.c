#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("Proprietary");   //macro defined in module.h giving the license details
MODULE_AUTHOR("Mickey Mouse");   //src:module.h to give author of the module

//Routing the file operation open when we try to open the device driver through the mknod created for the kyouko3 driver 
int kyouko3_open(struct inode *inode,struct file *fp){
  //this function is called when we do the system call open( )
   printk(KERN_ALERT "Kyouko_Kernel open\n");
   return 0;

}


//this callback function is defined in the fs.h file the sugnature matches with the one declared in the header 
int kyouko3_release(struct inode *inode, struct file *fp){

     //is invoked when we call the close system call of UNIX
     printk(KERN_ALERT "kyouko_Kernel released\n");
     return 0;
}

struct file_operations kyouko3_fops = {

	//file_operations structure dclared in fs.h and defined here to make useof it
	.open= kyouko3_open,
	.release= kyouko3_release,
	.owner= THIS_MODULE
};

struct cdev myDevice;

int my_init_function(void)
{

	cdev_init(&myDevice,&kyouko3_fops);
	cdev_add(&myDevice,MKDEV(500,127),1);
	
	printk(KERN_ALERT "init function from kernel\n");
	return 0;
}

module_init(my_init_function);

void my_exit_function(void)
{
	cdev_del(&myDevice);
	printk(KERN_ALERT "Exit Function Kernel\n");
	return;
}

module_exit(my_exit_function);



