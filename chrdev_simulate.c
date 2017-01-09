#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include "chrdev_ioctl.h"

#define DEVNO_MAR 300
#define DEVNO_MIN 0
#define MEM_SIZE  128

struct class * chrdev_cls = NULL;
struct device * chrdev_dev = NULL;
struct task_struct * pth_a = NULL;
struct task_struct * pth_b = NULL;
struct timer_list chrdev_timer;
struct workqueue_struct * wqueue;
struct delayed_work chrdev_queue;
wait_queue_head_t head_timer;


dev_t devno;
unsigned int thr_a_flag = 0; 
unsigned int com_flag = 0;
char *chrdev_mem = NULL;

void thread_a_timer(unsigned long data)
{
	//printk(">timer handler function...\n");
	mod_timer(&chrdev_timer,jiffies+2*HZ);
	thr_a_flag = 1;
	wake_up(&head_timer);
	if ( 3 == com_flag )
	{
		queue_delayed_work(wqueue,&chrdev_queue,2*HZ);
			*chrdev_mem = 'T';
			com_flag = 0;

	}
	//	printk(">write date:%ld",data);
}

void thread_b_queue(unsigned long data)
{
	//printk(">work_queue function...\n");
	printk(">Thread_B:\n%s",chrdev_mem);
	//	strcat(chrdev_mem,"change...");
	queue_delayed_work(wqueue,&chrdev_queue,2*HZ);
}

int thread_a(void * data)
{
//	printk(">do thread_a...\n");

	do
	{
//	printk(">com_flag = %d\n",com_flag);
		if ( 3 == com_flag )
			*chrdev_mem = 'T';
		printk(">Thread_A:\n%s\n",chrdev_mem);
		//printk(">Thread_A:\n%c",*chrdev_mem);
		wait_event(head_timer,thr_a_flag != 0);
		thr_a_flag = 0;
		com_flag = 0;
	}while(!kthread_should_stop());

//	printk(">exit thread_a...\n");

	return 0;
}

int thread_b(void * data)
{
	//	printk(">do thread_b...\n");

	//	do
	//	{
	//		printk(">Thread_B:\n%s",chrdev_mem);
	queue_delayed_work(wqueue,&chrdev_queue,2*HZ);
	//	}while(!kthread_should_stop());

	//	printk(">exit thread_b...\n");

	return 0;
}

int chrdev_open(struct inode * inode, struct file *file)
{
	printk(">>>chrdev_open...\n");

	return 0;
}

ssize_t chrdev_read(struct file * file, char __user * buf, size_t count, loff_t * offp)
{
	int ret=0;
	printk(">>>chrdev_read...\n");

	if( count > 128 )
		count =128;
	if( count < 0 )
		return -ENOMEM;
	printk("my_buf:%s\n",chrdev_mem);
	ret = copy_to_user(buf,chrdev_mem,count);
	if ( ret != 0 )
		return -EINVAL;

	return count;
}

ssize_t chrdev_write(struct file * file, const char __user * buf, size_t count, loff_t * offp)
{
	int ret=0;
	printk(">>>chrdev_write...\n");

	if( count > 96 )
		count = 96;
	if( count < 0 )
		return -ENOMEM;
	ret = copy_from_user(chrdev_mem+32,buf,count);
	if ( ret != 0 )
		return -EINVAL;

	printk("my_buf:%s\n",chrdev_mem+32);

	return count;
}

long chrdev_ioctl(struct file * file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	printk(">>>chrdev_ioctl...\n");

	switch(cmd)
	{
	case A_START:
		printk(">A_START...\n");
		wake_up_process(pth_a);
		break;
	case B_START:
		printk(">B_START...\n");
		wake_up_process(pth_b);
		break;
	case B_STOP:
		printk(">B_STOP...\n");
		cancel_delayed_work(&chrdev_queue);
		flush_work(wqueue);
		com_flag = 3;
		break;
	case A_CLOSE_B:
		printk(">A_CLOSE_B...\n");
		break;
	case CLOSE_AB:
		printk(">CLOSE_AB...\n");
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
int chrdev_close(struct inode * inode, struct file * file)
{
	printk(">>>chrdev_close...");
	return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = chrdev_open,
	.read = chrdev_read,
	.write = chrdev_write,
	.unlocked_ioctl = chrdev_ioctl,
	.release = chrdev_close,
};

int chrdev_init(void)
{
	int ret = 0;

	printk(">>>chrdev_init...\n");

	devno = MKDEV(DEVNO_MAR,DEVNO_MIN);

	//1、申请，注册设备号，初始化cdev结构体
	if ( 0 != register_chrdev(DEVNO_MAR,"char_driver",&fops))
	{
		printk(">[error]:fail to register the character device!!!\n");
		ret = -ENOMEM;
		goto err1;
	}

	//2、申请设备空间
	chrdev_mem = kmalloc(MEM_SIZE,GFP_KERNEL);
	if ( NULL == chrdev_mem )
	{
		printk(">[error]:fail to alloc the memory!!!\n");
		ret = -ENOMEM;
		goto err2;
	}
	//	printk(">the pointer is :%p\n",chrdev_mem);
	memset(chrdev_mem,0,MEM_SIZE);
	strcpy(chrdev_mem,"virtual character device\n>a");

	printk(">device message:\n%s",chrdev_mem);


	//3、创建设备类
	chrdev_cls = class_create(THIS_MODULE,"simulation");
	//chrdev_cls = ERR_PTR(-ENOMEM); 	//测试ERR_PTR,PTR_ERR,IS_ERR函数功能
	if ( IS_ERR(chrdev_cls) ) //判断chrdev_cls是否为无效指针
	{
		printk(">[error]:fail to create class!!!\n");
		ret = PTR_ERR(chrdev_cls); //将无效指针转换为错误码
		goto err3;
	}

	//4、创建设备节点
	chrdev_dev = device_create(chrdev_cls,NULL, devno,NULL,"chrdev_driver1");
	if ( IS_ERR(chrdev_dev) ) //判断chrdev_cls是否为无效指针
	{
		printk(">[error]:fail to create device node !!!\n");
		ret = PTR_ERR(chrdev_dev); //将无效指针转换为错误码
		goto err4;
	}


	//5、创建线程A与线程B
	pth_a = kthread_create(thread_a,NULL,"Thread_A");
	if ( IS_ERR(pth_a) )
	{
		printk(">[error]:Fail to create Thread_A!!!\n");
		ret = PTR_ERR(pth_a);
		goto err4;
	}
	pth_b = kthread_create(thread_b,NULL,"Thread_B");
	if ( IS_ERR(pth_b) )
	{
		printk(">[error]:Fail to create Thread_B!!!\n");
		ret = PTR_ERR(pth_b);
		goto err4;
	}

	//6、定时器的初始化
	init_timer(&chrdev_timer);
	chrdev_timer.function = thread_a_timer;
	chrdev_timer.expires = jiffies + 2*HZ;
	chrdev_timer.data = 20160805;
	add_timer(&chrdev_timer);

	//7、工作队列初始化
	wqueue = create_workqueue("work_queue");
	INIT_DELAYED_WORK(&chrdev_queue,thread_b_queue);
	//8、等待队列头初始化
	init_waitqueue_head(&head_timer);


	return ret;

err4:
	device_destroy(chrdev_cls, devno);
err3:
	class_destroy(chrdev_cls);
err2:
err1:
	unregister_chrdev(DEVNO_MAR,"char_driver");
	return ret;
}

void chrdev_exit(void)
{
	printk(">>>chrdev_exit...\n");

	//	kthread_stop(pth_b);
	kthread_stop(pth_a);
	cancel_delayed_work(&chrdev_queue);
	flush_work(wqueue);
	destroy_workqueue(wqueue);
	del_timer(&chrdev_timer);
	device_destroy(chrdev_cls, devno);
	class_destroy(chrdev_cls);
	kfree(chrdev_mem);
	unregister_chrdev(DEVNO_MAR,"char_driver");
}

module_init(chrdev_init);
module_exit(chrdev_exit);

MODULE_LICENSE("Dual BSD/GPL");
