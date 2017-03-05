#include<linux/module.h>
#include<linux/version.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<asm/uaccess.h>
#include<asm/io.h>
#include<linux/kdev_t.h>
#include<asm/fcntl.h>
#include<linux/sched.h>
#include<linux/wait.h>
#include<linux/errno.h>
#include<asm/irq.h>
#include<asm/ioctl.h>
#include<linux/string.h>
#include<linux/interrupt.h>
#include<asm/param.h>
#include<linux/fcntl.h>
#include<linux/poll.h>
#include<linux/semaphore.h>
#include<linux/wait.h>
#include<linux/sched.h>
#include"hal.h"                                                                                                                             

MODULE_AUTHOR ("DHIRAJ KUMAR SINHA");
MODULE_DESCRIPTION ("Psuedo Switch DEVICE DRIVER");
MODULE_LICENSE ("GPL");

#define MAJOR_NUMBER 0

int major;
int init_module (void);
void cleanup_module (void);
static int dhiraj_open (struct inode *, struct file *);
static int dhiraj_release (struct inode *, struct file *);
static long dhiraj_ioctl (struct file *, unsigned int,	unsigned long);
#define DRIVER_NAME  "switch"

static struct semaphore sem;

unsigned char myport = 0; // dummy register

#if 0
struct reosource_switch {
  int pin_number;
  int pin_value;
}res_data;
struct reosource_intensity {
  int pin_number;
  int pin_value;
};
struct reosource_sensor {
  int pin_number;
  int pin_value;
};
#endif

struct iot_reosource {
  int pin_number;
  int pin_value;
}iotr;

static struct file_operations fops = {
unlocked_ioctl:dhiraj_ioctl,
               open:dhiraj_open,
               release:dhiraj_release,
};

  int
dhiraj_open (struct inode *inode, struct file *file)
{
  int major_number;
  int minor_number;
  major_number = MAJOR (inode->i_rdev);
  minor_number = MINOR (inode->i_rdev);
  printk ("Open::My major number is: %d and minoe number is %d\n", major_number,minor_number);
  return 0;
}


  static int
dhiraj_release (struct inode *inode, struct file *file)
{
  int major_number;
  int minor_number;
  major_number = MAJOR (inode->i_rdev);
  minor_number = MINOR (inode->i_rdev);
  printk ("Release::My major number is: %d and minoe number is %d\n", major_number,minor_number);
  return 0;
}


  static long
dhiraj_ioctl ( struct file *file, unsigned int cmd,unsigned long arg)
{
  int ret = 0;
  if(down_interruptible(&sem))
  {
    printk(KERN_INFO"\n unable to get ioctl lock\n ");
    return -1;
  }

  if(copy_from_user(&iotr,(struct iot_reosource *)arg,sizeof(iotr)) != 0)
  {
    printk(KERN_INFO"\n copy error\n ");
    up(&sem);
    return -1;
  }

  switch(cmd)
  {
    case IOCTL_SET_FOR_INTENSITY:
      printk(KERN_INFO"\n I am in IOCTL_SET_FOR_INTENSITY \n");
      if(iotr.pin_value == 0)
        myport &= ~(1 << iotr.pin_number);  
      else if(iotr.pin_value == 1)
        myport |= (1 << iotr.pin_number);       

      break;
    case IOCTL_SET_TO_SWITCH:
      printk(KERN_INFO"\n I am in IOCTL_SET_TO_SWITCH\n");
      if(iotr.pin_value == 0)
        myport &= ~(1 << iotr.pin_number);  
      else if(iotr.pin_value == 1)
        myport |= (1 << iotr.pin_number); 

      break;
    case IOCTL_GET_FROM_SWITCH:
      printk(KERN_INFO"\n I am in IOCTL_GET_FROM_SWITCH\n");
      iotr.pin_value = (myport & (1<<iotr.pin_number))>>iotr.pin_number;
      if(copy_to_user((struct iot_reosource *)arg,&iotr,sizeof(iotr)) != 0)
        ret = -1;

      break;
    case IOCTL_GET_FROM_SENSOR:
      printk(KERN_INFO"\n I am in IOCTL_GET_FROM_SENSOR \n");
      iotr.pin_value = myport;
      if(copy_to_user((struct iot_reosource *)arg,&iotr,sizeof(iotr)) != 0)
        ret = -1;
      break;
      printk(KERN_INFO"\n I am in default\n");
    default:
      ret = -1;
      break;
  }
  up(&sem);
  return ret;
}


  int
init_module (void)
{

  major = register_chrdev (MAJOR_NUMBER, DRIVER_NAME, &fops);
  if (major <= 0)
  {
    printk ("<1>\nDriver not  registered successfully.\n\n");
  }
  sema_init(&sem,1);
  return 0;
}

  void
cleanup_module (void)
{
  unregister_chrdev (major, DRIVER_NAME);
  printk ("<1> i am closing this operation\n\n");
}
