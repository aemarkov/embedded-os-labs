#include "linux/err.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define DEVICE_NAME     "rpi_led%d"
#define CLASS_NAME      "rpi_led_class"
#define MAX_MINOR_CNT   3

struct rpi_device_data {
    dev_t devno;
    struct cdev cdev;
};

static int base_devno;
static struct class* rpi_class = NULL;
static struct rpi_device_data rpi_devices[MAX_MINOR_CNT] = {0};


static int rpi_open(struct inode *inode, struct file *file)
{
    pr_info("open\n");
    return 0;
}

static int rpi_release(struct inode *inode, struct file *file)
{
    pr_info("close\n");
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = rpi_open,
    .release = rpi_release,
};

static int rpi_create_dev(int minor, struct rpi_device_data *data)
{
    int ret;
    int major;
    struct device *device;

    major = MAJOR(base_devno);
    data->devno = MKDEV(major, minor);

    pr_info("Create device %d.%d\n", major, minor);

    cdev_init(&data->cdev, &fops);
    pr_info("cdev_add\n");
    ret = cdev_add(&data->cdev, data->devno, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        goto err;
    }

    pr_info("device_create\n");
    device = device_create(rpi_class, NULL, data->devno, NULL, DEVICE_NAME, minor);
    if (IS_ERR(device)) {
        pr_err("Failed to create device\n");
        goto del_cdev;
    }

    return 0;

del_cdev:
    pr_info("cdev_del\n");
    cdev_del(&data->cdev);
err:
    return -1;
}

static int __init rpi_led_init(void)
{
    int ret;
    int major, minor;

    pr_info("Driver loaded\n");

    ret = alloc_chrdev_region(&base_devno, 0, MAX_MINOR_CNT, DEVICE_NAME);
    if (ret != 0) {
        pr_err("Unable to allocate chrdev region: %d\n", ret);
        goto err;
    }

    pr_info("class_create\n");
    rpi_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(rpi_class)) {
        pr_err("Failed to create  device class\n");
        goto del_region;
    }

    major = MAJOR(base_devno);

    for (minor = 0; minor < MAX_MINOR_CNT; minor++) {
        ret = rpi_create_dev(minor, &rpi_devices[minor]);
        if (ret < 0) {
            goto del_class;
        }
    }

    pr_info("Devices created\n");

    return 0;

del_class:
   class_destroy(rpi_class);
del_region:
   unregister_chrdev_region(base_devno, MAX_MINOR_CNT);
err:
    return -1;
}

static void __exit rpi_led_exit(void)
{
    int minor;
    pr_info("Deinitialization...\n");

    for (minor = 0; minor < MAX_MINOR_CNT; minor++) {
        pr_info("Deleting device %d.%d\n",
                MAJOR(rpi_devices[minor].devno), MINOR(rpi_devices[minor].devno));

        if (MAJOR(rpi_devices[minor].devno) != 0) {
            pr_info("device_destroy\n");
            device_destroy(rpi_class, rpi_devices[minor].devno);

            pr_info("cdev_del\n");
            cdev_del(&rpi_devices[minor].cdev);

            rpi_devices[minor].devno = MKDEV(0, 0);
        }
    }

    pr_info("class_destroy\n");
    class_destroy(rpi_class);
    rpi_class = NULL;

    pr_info("unregister_chrdev_region\n");
    unregister_chrdev_region(base_devno, MAX_MINOR_CNT);
    base_devno = MKDEV(0, 0);

    pr_info("Driver unloaded\n");
}

module_init(rpi_led_init);
module_exit(rpi_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Markov Alexey <markovalex95@gmail.com>");

MODULE_DESCRIPTION("Simple test driver for Raspberry Pi");
