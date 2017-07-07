/* Simple USB driver for magic eye */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/usb.h>
#include <linux/slab.h> // kmalloc

#define VENDOR_USB_ID 0x03EB // Atmel
#define DEVICE_USB_ID 0x21FE // RS232 converter AT90S2313
#define DEVICE_VERSION 2     // v0.02
const char* cAvrDevString = "AVR309:USB to UART protocol converter (simple)";
const int cSetOutDataPort = 5;

struct usb_magiceye {
    struct usb_device *	udev;
    unsigned char pwm1;
    unsigned char pwm_min;
    unsigned char pwm_max;
};

// Do AVR309 vendor device requests (USB_DIR_IN | USB_TYPE_VENDOR)
// (The AVR device is using control IN endpoint) (request: FNCNumberDoSetOutDataPort)
// request number cSetOutDataPort
static bool set_out_data_port(struct usb_device *udev, int value)
{
    int retval;

    printk(KERN_INFO "set_out_data_port(%d)\n", value);

    retval = usb_control_msg(udev, // *dev
        usb_sndctrlpipe(udev, 0), // pipe
        cSetOutDataPort, // request
        USB_DIR_IN | USB_TYPE_VENDOR, // requesttype
        value, // value
        0, // index
        0, // data
        0, // size
        2 * HZ); // timeout

    if (retval < 0)
        printk(KERN_INFO "set_out_data_port failed (%d)", retval);

    return (retval >= 0);
}

static ssize_t show_pwm1(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev);	
    struct usb_magiceye *magiceye = usb_get_intfdata(intf);

    printk(KERN_INFO "show_pwm1\n");
    return sprintf(buf, "%d\n", magiceye->pwm1);
}

static ssize_t set_pwm1(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev);	
    struct usb_magiceye *magiceye = usb_get_intfdata(intf);
    int value = simple_strtoul(buf, NULL, 10);

    magiceye->pwm1 = value;

    if (value < 0)
    {
        value = -value;
        if (value > 127) value = 127;
    }
    else
    {
        if (value > 127) value = 127;
        value += 0x80;
    }

    if (!set_out_data_port(magiceye->udev, value))
        return -EIO;

    return count;
}
static DEVICE_ATTR(pwm1, S_IWUSR | S_IWGRP | S_IRUGO, show_pwm1, set_pwm1);

static ssize_t show_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev);	
    struct usb_magiceye *magiceye = usb_get_intfdata(intf);

    printk(KERN_INFO "show_status\n");
    return sprintf(buf, "%d %d\n", magiceye->pwm_min, magiceye->pwm_max);
}

static ssize_t start(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev);
    struct usb_magiceye *magiceye = usb_get_intfdata(intf);
    int pwm_min;
    int pwm_max;
    int retval;

    retval = sscanf(buf, "%d %d", &pwm_min, &pwm_max);
    if (retval == 0)
    {
        pwm_min = 0;
        pwm_max = 100;
    }
    else if (retval != 2)
    {
        return -EIO;
    }

    magiceye->pwm_min = pwm_min;
    magiceye->pwm_max = pwm_max;

    printk(KERN_INFO "start (%d, %d)\n", pwm_min, pwm_max);

    if (pwm_min < 0) pwm_min = 0;
    else if (pwm_min > 127) pwm_min = 127;

    if (pwm_max < 0) pwm_max = 0;
    else if (pwm_max > 127) pwm_max = 127;

    return count;
}
static DEVICE_ATTR(start, S_IWUSR | S_IWGRP | S_IRUGO, show_status, start);

static int magiceye_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(interface);
    struct usb_magiceye *dev = NULL;
    unsigned char configuration;
    int retval;

    if ((udev == NULL) || (udev->product == NULL) || (strcmp(cAvrDevString, udev->product) != 0))
       return -ENODEV;

    printk(KERN_INFO "magiceye found!\n");

    retval = usb_control_msg(udev, // *dev
        usb_rcvctrlpipe(udev, 0), // pipe
        USB_REQ_GET_CONFIGURATION, // request
        USB_DIR_IN | USB_TYPE_STANDARD, // requesttype
        0, // value
        0, // index
        &configuration, // data
        1, // size
        2 * HZ); // timeout

    if (retval < 0)
    {
        printk(KERN_INFO "get_configuration failed (%d)", retval);
        return retval;
    }

    if (configuration != 1)
    {
        printk(KERN_INFO "incorrect configuration %d", configuration);
        return -1;
    }

    dev = kmalloc(sizeof(struct usb_magiceye), GFP_KERNEL);
    if (dev == NULL) {
        dev_err(&interface->dev, "Out of memory\n");
        return -ENOMEM;
    }
    memset(dev, 0x00, sizeof (*dev));

    dev->udev = usb_get_dev(udev);

    usb_set_intfdata(interface, dev);

    device_create_file(&interface->dev, &dev_attr_pwm1);
    device_create_file(&interface->dev, &dev_attr_start);

    dev_info(&interface->dev, "USB MagicEye device now attached\n");

    return 0;
}

static void magiceye_disconnect(struct usb_interface *interface)
{
    struct usb_magiceye *dev;

    dev = usb_get_intfdata (interface);
    usb_set_intfdata (interface, NULL);

    device_remove_file(&interface->dev, &dev_attr_pwm1);
    device_remove_file(&interface->dev, &dev_attr_start);

    usb_put_dev(dev->udev);

    kfree(dev);

    printk(KERN_INFO "Magic eye i/f %d now disconnected\n", interface->cur_altsetting->desc.bInterfaceNumber);
}

static struct usb_device_id magiceye_table[] =
{
    { USB_DEVICE_VER(VENDOR_USB_ID, DEVICE_USB_ID, DEVICE_VERSION, DEVICE_VERSION) },
    {} /* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, magiceye_table);

static struct usb_driver magiceye_driver =
{
    .name = "magiceye_driver",
    .probe = magiceye_probe,
    .disconnect = magiceye_disconnect,
    .id_table = magiceye_table,
};

static int __init magiceye_init(void)
{
    return usb_register(&magiceye_driver);
}

static void __exit magiceye_exit(void)
{
    usb_deregister(&magiceye_driver);
}

module_init(magiceye_init);
module_exit(magiceye_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JDJ Keijzer");
MODULE_DESCRIPTION("Magic eye Driver");
