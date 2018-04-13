#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <linux/poll.h>
#include <linux/interrupt.h> 
#include <linux/gpio.h>

#define DEVICE_NAME			"GIOP_park"
#define GIOP_park_MAJOR 	232

static unsigned int flag = 0;

static volatile int pinval;



static DECLARE_TASKLET(GIOP_taskllet, GIOP_do_tasklet, 0);
static DECLARE_WAIT_QUEUE_HEAD(GIOP_waitq);
static spinlock_t GIOP_lcok;

spin_lock_init(GIOP_lcok);


static void GIOP_do_tasklet(unsigned long)
{   
	pinval = gpio_get_value(GPIO_PF0);
	
    wake_up_interruptible(&GIOP_waitq); 
}

static ssize_t GIOP_park_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	pinval = gpio_get_value(GPIO_PF0);
	if (pinval){
   		wait_event_interruptible(GIOP_waitq, pinval);

   		copy_to_user(buff, &pinval, 1);
	}
    return 1;	
}

static irqreturn_t GIOP_interript(int irq, void *dev_id)
{
	tasklet_schedule(&GIOP_do_tasklet);
    
    return IRQ_RETVAL(IRQ_HANDLED);
}


static int GIOP_park_open(struct inode *inode, struct file *file)
{
	int ret;

	spinlock(&GIOP_lcok);
	if (flag){
		spin_unlock(&GIOP_lcok);
		return -EBUSY;
	}
	flag++;
	spin_unlock(&GIOP_lcok);
	
	ret = request_irq(IRQ_EINT0, GIOP_interript, IRQF_TRIGGER_HIGH,
				"GPIO_test", NULL);
	if (ret){
		GTP_ERROR("tpd request_irq IRQ LINE NOT AVAILABLE!.");
		return -1;
	}
	
	ret = gpio_request_one(GPIO_PF0, GPIOF_IN,
				 "GIOP_test");
	if (ret < 0) {
		GTP_ERROR("Unable to request gpio\n");
		return -1;
	}
	return 0;
}

static int GIOP_park_release(struct inode *inode, struct file *file)
{
	spinlock(&GIOP_lcok);
	flag--;
	spin_unlock(&GIOP_lcok);
	gpio_free(GPIO_PF0);
	free_irq(IRQ_EINT0, GIOP_interript);

	return 0;
}


static struct file_operations GIOP_park_fops = {
    .owner   =  THIS_MODULE,
    .open    =  GIOP_park_open,     
	.read	 =	GIOP_park_read,	   
	.release =  GIOP_park_release,
};


int major;
static int GIOP_park_init(void)
{
	int ret;

	ret = register_chrdev(GIOP_park_MAJOR, DEVICE_NAME, &GIOP_park_fops);
	if (ret < 0)
	{
		printk(DEVICE_NAME "can't register major number\n");
		return ret;
	}

	printk(DEVICE_NAME "initialized\n");
	return 0;
}

static void GIOP_park_exit(void)
{
	unregister_chrdev(GIOP_park_MAJOR, DEVICE_NAME);
}


module_init(GIOP_park_init);

module_exit(GIOP_park_exit);

MODULE_LICENSE("GPL");
"# test" 
