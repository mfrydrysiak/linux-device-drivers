#include <linux/init.h>
#include <linux/fs.h>           /* operations on file */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define MFCDEV_AUTHOR   "Marek Frydrysiak <marek.frydrysiak@gmail.com>"
#define MFCDEV_DESC     "A simple, char device driver"

#define MFDEVNODE_NAME  "mfchar"
#define MFCDEV_NAME     "mf_cdev"
#define MFCDEV_MINOR    0
#define MFCDEV_DEVNUM   1

static DEFINE_MUTEX(mfchar_mutex);

#define MAX_BUF_SIZE    128
static unsigned char driver_rw_buf[MAX_BUF_SIZE];
static size_t buf_copy_size;

static dev_t mf_dev;            /* structure for major and minor numbers */
static struct cdev mfchar;
static struct class * mfchar_class = NULL;
static struct device * mfchar_device = NULL;

struct mf_cdev_struct {
    unsigned char buffer[MAX_BUF_SIZE];
    size_t buf_copy_size;
};

/**
 * \brief Call by open syscall
 */
static int mf_cdev_open(struct inode *inodp, struct file *filp)
{
    struct mf_cdev_struct *mf;

    mf = kzalloc(sizeof(*mf), GFP_KERNEL);

    if (mf) {
        filp->private_data = mf;
        printk(KERN_INFO "%s: device opened\n", MFCDEV_NAME);
    }

	//if (!mutex_trylock(&mfchar_mutex)) {
	//	printk(KERN_INFO "%s: device is busy\n", MFCDEV_NAME);
	//	return -EBUSY;
	//}
	
	return  mf ? 0 : -ENOMEM;
}

/**
 * \brief Call by close syscall
 *        Note: if this char device file structure is shared,
 *              then the release function won't be called until
 *              the last instance releases (closes) it!
 */
static int mf_cdev_release(struct inode *inodp, struct file *filp)
{
    struct mf_cdev_struct *mf = filp->private_data;
    filp->private_data = NULL;
    kfree(mf);

	printk(KERN_INFO "%s: device closed\n", MFCDEV_NAME);
//	mutex_unlock(&mfchar_mutex);
	return 0;
}

static ssize_t mf_cdev_write(struct file *filp, const char __user *buf,
						size_t count, loff_t *ppos)
{
    struct mf_cdev_struct *mf = filp->private_data;

    mf->buf_copy_size = count;

	/* Check user-space copy size request */
	if (mf->buf_copy_size > MAX_BUF_SIZE)
		mf->buf_copy_size = MAX_BUF_SIZE;
	if (copy_from_user(mf->buffer, buf, mf->buf_copy_size))
		return -EFAULT;

	printk(KERN_INFO "%s: number of chars received = %ld\n",
						MFCDEV_NAME, mf->buf_copy_size);
	printk(KERN_INFO "%s: received from user-space: %s\n",
						MFCDEV_NAME, mf->buffer);
	return buf_copy_size;
}

static ssize_t mf_cdev_read(struct file *filp, char __user *buf, size_t count,
						loff_t *ppos)
{
    struct mf_cdev_struct *mf = filp->private_data;

	/* Check user-space copy size request */
	if (count > mf->buf_copy_size)
		count = mf->buf_copy_size;
	if (copy_to_user(buf, mf->buffer, count))
		return -EFAULT;

	mf->buf_copy_size = 0;

	printk(KERN_INFO "%s: copied %ld chars to user-space\n",
						MFCDEV_NAME, count);
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

	ret = alloc_chrdev_region(&mf_dev, MFCDEV_MINOR, MFCDEV_DEVNUM,
								MFDEVNODE_NAME);
	if (ret < 0) {
		printk(KERN_ALERT "%s: could not register char "
				  "device (ERR=%d)\n", MFCDEV_NAME, ret);
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
		printk(KERN_ALERT "%s: could not add char device (ERR=%d)\n",
							MFCDEV_NAME, ret);
		goto init_cdev_add_failed;
	}
	printk(KERN_INFO "%s: successfully added char device\n", MFCDEV_NAME);

	mfchar_class = class_create(mfchar.owner, MFDEVNODE_NAME);
	if (IS_ERR(mfchar_class)) {
		printk(KERN_ALERT "%s: could not create device class\n",
							MFCDEV_NAME);
		ret = PTR_ERR(mfchar_class);
		goto init_cdev_add_failed;
	}
	printk(KERN_INFO "%s: registered device class\n", MFCDEV_NAME);

	/* create a device and register it with sysfs */
	mfchar_device = device_create(mfchar_class, NULL, mf_dev, NULL,
								MFDEVNODE_NAME);
	if (IS_ERR(mfchar_device)) {
		printk(KERN_ALERT "%s: could not create device\n", MFCDEV_NAME);
		ret = PTR_ERR(mfchar_device);
		goto init_device_create_failed;
	}
	printk(KERN_INFO "%s: created device and registered with sysfs\n",
							MFCDEV_NAME);

	mutex_init(&mfchar_mutex);

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
	mutex_destroy(&mfchar_mutex);
	device_destroy(mfchar_class, mf_dev);
	class_destroy(mfchar_class);
	cdev_del(&mfchar);
	unregister_chrdev_region(mf_dev, MFCDEV_DEVNUM);
	printk(KERN_INFO "%s: deactivated!\n", MFCDEV_NAME);
}

module_init(mf_cdev_init_function);
module_exit(mf_cdev_cleanup_function);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR(MFCDEV_AUTHOR);
MODULE_DESCRIPTION(MFCDEV_DESC);
