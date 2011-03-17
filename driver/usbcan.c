/*
 * This file is part of the usbcan project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>

MODULE_AUTHOR("Gareth McMullin <gareth@blacksphere.co.nz>");
MODULE_DESCRIPTION("SocketCAN driver Black Sphere USB-CAN Adapter");
MODULE_LICENSE("GPL v2");

#define USB_USBCAN_VENDOR_ID	0xCAFE
#define USB_USBCAN_PRODUCT_ID	0xCAFE

/* table of devices that work with this driver */
static struct usb_device_id usbcan_table[] = {
	{USB_DEVICE(USB_USBCAN_VENDOR_ID, USB_USBCAN_PRODUCT_ID)},
	{} /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, usbcan_table);

static int usbcan_probe(struct usb_interface *intf,
				const struct usb_device_id *id)
{
	printk(KERN_INFO "%s\n", __func__);

	return 0;
}

static void usbcan_disconnect(struct usb_interface *intf)
{
	printk(KERN_INFO "%s\n", __func__);
}

static struct usb_driver usbcan_usb_driver = {
	.name = "usbcan",
	.id_table = usbcan_table,
	.probe = usbcan_probe,
	.disconnect = usbcan_disconnect,
};

static int __init usbcan_init(void)
{
	int err;

	printk(KERN_INFO "USB-CAN driver loaded\n");

	err = usb_register(&usbcan_usb_driver);

	if (err) {
		err("usb_register failed. Error number %d\n", err);
		return err;
	}

	return 0;
}

static void __exit usbcan_exit(void)
{
	usb_deregister(&usbcan_usb_driver);

	printk(KERN_INFO "USB-CAN driver unloaded\n");
}

module_init(usbcan_init);
module_exit(usbcan_exit);
