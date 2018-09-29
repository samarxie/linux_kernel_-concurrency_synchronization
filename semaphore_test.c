#include <linux/init.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/semaphore.h>

static dev_t devid;
static struct class *cls = NULL;
static struct cdev mydev;

//static DEFINE_SEMAPHORE(mysema);
static struct semaphore mysema;

static int my_open(struct inode *inode, struct file *file)
{
    int i;
    while(down_interruptible(&mysema) != 0);
    for(i = 0; i < 10; i++) {
        printk("semaphore test:%d\n", i);
        ssleep(1);
    }

    up(&mysema);
    printk("open success!\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    printk("close success!\n");
    return 0;
}

//定义文件操作
static struct file_operations myfops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
};

static void hello_cleanup(void)
{
    cdev_del(&mydev);
    device_destroy(cls, devid);
    class_destroy(cls);
    unregister_chrdev_region(devid, 1);
}

static __init int hello_init(void)
{
    int result;

    sema_init(&mysema, 1);

    //动态注册设备号
    if(( result = alloc_chrdev_region(&devid, 0, 1, "samar-alloc-dev") ) != 0) {
        printk("register dev id error:%d\n", result);
        goto err;
    } else {
        printk("register dev id success!\n");
    }
    //动态创建设备节点
    cls = class_create(THIS_MODULE, "samar-class");
    if(IS_ERR(cls)) {
        printk("create class error!\n");
        goto err;
    }

    if(device_create(cls, NULL, devid, "", "hello%d", 0) == NULL) {
        printk("create device error!\n");
        goto err;
    }
    //字符设备注册
    mydev.owner = THIS_MODULE;      //必要的成员初始化
    mydev.ops = &myfops;
    cdev_init(&mydev, &myfops);
    //添加一个设备
    result = cdev_add(&mydev, devid, 1);
    if(result != 0) {
        printk("add cdev error!\n");
        goto err;
    }

    printk(KERN_ALERT "hello init success!\n");
    return 0;
err:
    hello_cleanup();
    return -1;
}

static __exit void hello_exit(void)
{
    hello_cleanup();
    printk("helloworld exit!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("samarxie");
MODULE_DESCRIPTION("For semaphore use method");
