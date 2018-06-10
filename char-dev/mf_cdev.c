#include <linux/init.h>
#include <linux/fs.h>           /* operations on file */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define MFCDEV_AUTHOR   "Marek Frydrysiak <marek.frydrysiak@gmail.com>"
#define MFCDEV_DESC     "A simple, char device driver"

#define MFCDEV_NAME     "mfchar"
#define MFCDEV_MINOR    0
#define MFCDEV_DEVNUM   1

#define MAX_BUF_SIZE    128
static unsigned char driver_rw_buf[MAX_BUF_SIZE];
static size_t buf_copy_size;

static dev_t mf_dev;            /* structure for major and minor numbers */
static struct cdev mfchar;
static struct class * mfchar_class = NULL;
static struct device * mfchar_device = NULL;

/**
 * \brief Call by open syscall
 */
static int mf_cdev_open(struct inode *inodp, struct file *filp)
{
	printk(KERN_INFO "mf_cdev: I was opened! :).\n");
	/* TBA */
	return 0;
}

/**
 * \brief Call by close syscall
 *        Note: if this char device file structure is shared,
 *              then the release function won't be called until
 *              the last instance releases (closes) it!
 */
static int mf_cdev_release(struct inode *inodp, struct file *filp)
{
	printk(KERN_INFO "mf_cdev: I was closed! :(.\n");
	/* TBA */
	return 0;
}

static ssize_t mf_cdev_write(struct file *filp, const char __user *buf,
						size_t count, loff_t *ppos)
{
	buf_copy_size = count;

	/* Check user-space copy size request */
	if (buf_copy_size > MAX_BUF_SIZE)
		buf_copy_size = MAX_BUF_SIZE;
	if (copy_from_user(driver_rw_buf, buf, buf_copy_size))
		return -EFAULT;

	printk(KERN_INFO "mf_cdev: number of chars received = %ld\n", buf_copy_size);
	printk(KERN_INFO "mf_cdev: received from user-space: %s\n", driver_rw_buf);
	return buf_copy_size;
}

static ssize_t mf_cdev_read(struct file *filp, char __user *buf, size_t count,
						loff_t *ppos)
{
	/* Check user-space copy size request */
	if (count > buf_copy_size)
		count = buf_copy_size;
	if (copy_to_user(buf, driver_rw_buf, count))
		return -EFAULT;

	buf_copy_size = 0;

	printk(KERN_INFO "mf_cdev: copied %ld chars to user-space\n", count);
	return count;
}

static const struct file_operations mf_cdev_fops = {
	.owner      = THIS_MODULE,
	.open       = mf_cdev_open,
	.release    = mf_cdev_release,
	.write      = mf_cdev_write,
	.read       = mf_cdev_read
};

static int __init mf_cdev_init_function(void)
{
	int ret;

	ret = alloc_chrdev_region(&mf_dev, MFCDEV_MINOR, MFCDEV_DEVNUM, MFCDEV_NAME);
	if (ret < 0) {
		printk(KERN_ALERT "Could not register char device (ERR=%d)\n", ret);
		goto init_failed;
	}
	printk(KERN_INFO "mf_cdev: activated; major: %d, minor: %d\n",
	       MAJOR(mf_dev), MINOR(mf_dev));

	/* there will be only one standalone cdev */
	cdev_init(&mfchar, &mf_cdev_fops);
	mfchar.owner = THIS_MODULE;
	mfchar.ops = &mf_cdev_fops;
	ret = cdev_add(&mfchar, mf_dev, 1);
	if (ret < 0) {
		printk(KERN_ALERT "mf_cdev: could not add char device (ERR=%d)\n",
		       ret);
		goto init_cdev_add_failed;
	}
	printk(KERN_INFO "mf_cdev: successfully added char device\n");

	mfchar_class = class_create(mfchar.owner, MFCDEV_NAME);
	if (IS_ERR(mfchar_class)) {
		printk(KERN_ALERT "mf_cdev: could not create device class\n");
		ret = PTR_ERR(mfchar_class);
		goto init_cdev_add_failed;
	}
	printk(KERN_INFO "mf_cdev: registered device class\n");

	/* create a device and register it with sysfs */
	mfchar_device = device_create(mfchar_class, NULL, mf_dev, NULL, MFCDEV_NAME);
	if (IS_ERR(mfchar_device)) {
		printk(KERN_ALERT "mf_cdev: could not create device\n");
		ret = PTR_ERR(mfchar_device);
		goto init_device_create_failed;
	}
	printk(KERN_INFO "mf_cdev: created device and registered with sysfs\n");

	return 0; /* on success */

	/* if something fails */
init_device_create_failed:
	class_destroy(mfchar_class);
init_cdev_add_failed:
	unregister_chrdev_region(mf_dev, MFCDEV_DEVNUM);
init_failed:
	return ret;
}

static void __exit mf_cdev_cleanup_function(void)
{
	device_destroy(mfchar_class, mf_dev);
	class_destroy(mfchar_class);
	cdev_del(&mfchar);
	unregister_chrdev_region(mf_dev, MFCDEV_DEVNUM);
	printk(KERN_INFO "mf_cdev: deactivated!\n");
}

module_init(mf_cdev_init_function);
module_exit(mf_cdev_cleanup_function);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR(MFCDEV_AUTHOR);
MODULE_DESCRIPTION(MFCDEV_DESC);
