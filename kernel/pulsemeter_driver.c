// SPDX-License-Identifier: GPL-2.0
/*
 * pulsemeter_driver.c - Minimal Linux character device driver that
 * simulates a smart electricity meter's pulse output in kernel space.
 *
 * A real S0-interface meter toggles a GPIO line on every pulse, serviced
 * from a hardware interrupt handler. This driver substitutes a kernel
 * timer for that interrupt source (no real GPIO/IRQ hardware is available
 * in this build/test environment), but everything else -- device
 * registration, interrupt-context-safe synchronization, and the
 * userspace-facing read() interface -- is written the way a real
 * interrupt-driven pulse-counter driver would be.
 *
 * Userspace interface:
 *   open("/dev/pulsemeter", O_RDONLY)
 *   read(fd, buf, size) -> ASCII decimal pulse count accumulated since the
 *                          last read, newline-terminated, then resets the
 *                          counter to 0 (a "read-and-clear" register, the
 *                          same convention many real pulse-counting
 *                          peripherals use).
 *
 * Build:   make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
 * Load:    sudo insmod pulsemeter_driver.ko
 * Unload:  sudo rmmod pulsemeter_driver
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/err.h>

#define DEVICE_NAME       "pulsemeter"
#define CLASS_NAME        "pulsemeter_class"
#define PULSE_INTERVAL_MS 500

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ayon Dalai");
MODULE_DESCRIPTION("Simulated smart-meter pulse counter character device driver");
MODULE_VERSION("1.0");

static dev_t dev_number;
static struct cdev pulsemeter_cdev;
static struct class *pulsemeter_class;
static struct device *pulsemeter_device;

static struct timer_list pulse_timer;
static spinlock_t counter_lock;
static unsigned long pulse_count; /* protected by counter_lock */

/*
 * Fires every PULSE_INTERVAL_MS, standing in for a real hardware pulse
 * interrupt. Runs in softirq (timer) context, so it must never sleep --
 * exactly why this uses a spinlock (with IRQ save/restore) rather than a
 * mutex to protect the shared counter.
 */
static void pulse_timer_callback(struct timer_list *t)
{
    unsigned long flags;

    spin_lock_irqsave(&counter_lock, flags);
    pulse_count++;
    spin_unlock_irqrestore(&counter_lock, flags);

    mod_timer(&pulse_timer, jiffies + msecs_to_jiffies(PULSE_INTERVAL_MS));
}

static int pulsemeter_open(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    return 0;
}

static int pulsemeter_release(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    return 0;
}

/*
 * Read-and-clear: returns the pulse count accumulated since the previous
 * read, then resets it to 0.
 */
static ssize_t pulsemeter_read(struct file *file, char __user *buf,
                                size_t count, loff_t *offset)
{
    char kbuf[32];
    int len;
    unsigned long flags;
    unsigned long snapshot;

    (void)file;

    if (*offset > 0) {
        /* EOF on a second read() within the same open(), so shell tools
         * like `cat` terminate cleanly instead of looping forever. */
        return 0;
    }

    spin_lock_irqsave(&counter_lock, flags);
    snapshot = pulse_count;
    pulse_count = 0;
    spin_unlock_irqrestore(&counter_lock, flags);

    len = scnprintf(kbuf, sizeof(kbuf), "%lu\n", snapshot);

    if ((size_t)len > count) {
        return -EINVAL;
    }
    if (copy_to_user(buf, kbuf, len)) {
        return -EFAULT;
    }

    *offset += len;
    return len;
}

static const struct file_operations pulsemeter_fops = {
    .owner   = THIS_MODULE,
    .open    = pulsemeter_open,
    .release = pulsemeter_release,
    .read    = pulsemeter_read,
};

static int __init pulsemeter_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("pulsemeter: failed to allocate device number\n");
        return ret;
    }

    cdev_init(&pulsemeter_cdev, &pulsemeter_fops);
    pulsemeter_cdev.owner = THIS_MODULE;

    ret = cdev_add(&pulsemeter_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("pulsemeter: failed to add cdev\n");
        goto fail_cdev;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    pulsemeter_class = class_create(CLASS_NAME);
#else
    pulsemeter_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(pulsemeter_class)) {
        pr_err("pulsemeter: failed to create device class\n");
        ret = PTR_ERR(pulsemeter_class);
        goto fail_class;
    }

    pulsemeter_device = device_create(pulsemeter_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(pulsemeter_device)) {
        pr_err("pulsemeter: failed to create device node\n");
        ret = PTR_ERR(pulsemeter_device);
        goto fail_device;
    }

    spin_lock_init(&counter_lock);
    pulse_count = 0;

    timer_setup(&pulse_timer, pulse_timer_callback, 0);
    mod_timer(&pulse_timer, jiffies + msecs_to_jiffies(PULSE_INTERVAL_MS));

    pr_info("pulsemeter: loaded, device node /dev/%s ready (major=%d minor=%d)\n",
            DEVICE_NAME, MAJOR(dev_number), MINOR(dev_number));
    return 0;

fail_device:
    class_destroy(pulsemeter_class);
fail_class:
    cdev_del(&pulsemeter_cdev);
fail_cdev:
    unregister_chrdev_region(dev_number, 1);
    return ret;
}

static void __exit pulsemeter_exit(void)
{
    del_timer_sync(&pulse_timer);
    device_destroy(pulsemeter_class, dev_number);
    class_destroy(pulsemeter_class);
    cdev_del(&pulsemeter_cdev);
    unregister_chrdev_region(dev_number, 1);
    pr_info("pulsemeter: unloaded\n");
}

module_init(pulsemeter_init);
module_exit(pulsemeter_exit);
