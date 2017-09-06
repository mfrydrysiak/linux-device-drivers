#include <linux/init.h>
#include <linux/fs.h>           /* operations on file */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>

#define MFCDEV_AUTHOR   "Marek Frydrysiak <marek.frydrysiak@gmail.com>"
#define MFCDEV_DESC     "A simple, char device driver"

#define MFCDEV_MINOR    0
#define MFCDEV_DEVNUM   1

static dev_t    mf_dev;         /* structure for major and minor numbers */
struct cdev     mf_chardev;  

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

struct file_operations mf_cdev_fops = {
    .owner      = THIS_MODULE,
    .open       = mf_cdev_open,
    .release    = mf_cdev_release
};

static int __init mf_cdev_init_function(void)
{
    int err;

    err = alloc_chrdev_region(&mf_dev, MFCDEV_MINOR, MFCDEV_DEVNUM, "mf_cdev");
    if (err < 0) {
        printk(KERN_ALERT "Could not register char device (ERR=%d).\n", err);
        goto init_failed;
    }
    printk(KERN_INFO "mf_cdev: activated! major: %d, minor: %d\n",
                                            MAJOR(mf_dev), MINOR(mf_dev));   

    cdev_init(&mf_chardev, &mf_cdev_fops); 
    err = cdev_add(&mf_chardev, mf_dev, MFCDEV_DEVNUM);
    if (err < 0) {
        printk(KERN_ALERT "Could not add char device (ERR=%d).\n", err);
        goto init_cdev_add_failed;
    }    
    printk(KERN_INFO "mf_cdev: successfully added char device!\n");

    return 0; /* on success */

    /* if something fails */
init_cdev_add_failed:
    unregister_chrdev_region(mf_dev, MFCDEV_DEVNUM);
init_failed: 
    return err;
}

static void __exit mf_cdev_cleanup_function(void)
{
    cdev_del(&mf_chardev);
    unregister_chrdev_region(mf_dev, MFCDEV_DEVNUM);
    printk(KERN_INFO "mf_cdev: deactivated!\n");
}

module_init(mf_cdev_init_function);
module_exit(mf_cdev_cleanup_function);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR(MFCDEV_AUTHOR);
MODULE_DESCRIPTION(MFCDEV_DESC);
