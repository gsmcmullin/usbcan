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
#include <linux/netdevice.h>
#include <linux/can.h>
#include <linux/can/dev.h>

MODULE_AUTHOR("Gareth McMullin <gareth@blacksphere.co.nz>");
MODULE_DESCRIPTION("SocketCAN driver Black Sphere USB-CAN Adapter");
MODULE_LICENSE("GPL v2");

#define USB_USBCAN_VENDOR_ID	0xCAFE
#define USB_USBCAN_PRODUCT_ID	0xCAFE

#define ECHO_SKB_MAX		10

/* table of devices that work with this driver */
static struct usb_device_id usbcan_table[] = {
	{USB_DEVICE(USB_USBCAN_VENDOR_ID, USB_USBCAN_PRODUCT_ID)},
	{} /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, usbcan_table);

struct usbcan {
	struct can_priv can;

	struct usb_device *udev;
	struct net_device *netdev;
};

static int usbcan_open(struct net_device *netdev)
{
	struct usbcan *dev = netdev_priv(netdev);

	printk(KERN_INFO "%s\n", __func__);
	usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			0, 0x40, 1, 0, NULL, 0, 100);
	netif_start_queue(netdev);

	return 0;
}

static int usbcan_close(struct net_device *netdev)
{
	struct usbcan *dev = netdev_priv(netdev);

	printk(KERN_INFO "%s\n", __func__);
	usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			0, 0x40, 0, 0, NULL, 0, 100);
	netif_stop_queue(netdev);

	return 0;
}

static netdev_tx_t usbcan_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	printk(KERN_INFO "%s\n", __func__);
	netdev->trans_start = jiffies;

	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}

static const struct net_device_ops usbcan_netdev_ops = {
	.ndo_open = usbcan_open,
	.ndo_stop = usbcan_close,
	.ndo_start_xmit = usbcan_start_xmit,
};

static int usbcan_probe(struct usb_interface *intf,
				const struct usb_device_id *id)
{
	struct net_device *netdev;
	struct usbcan *dev;

	printk(KERN_INFO "%s\n", __func__);

	netdev = alloc_candev(sizeof(struct usbcan), ECHO_SKB_MAX);
	if(!netdev) {
		dev_err(&intf->dev, "%s: Couldn't allocate candev!\n", __func__);
		return -ENOMEM;
	}

	dev = netdev_priv(netdev);

	dev->netdev = netdev;
	dev->udev = interface_to_usbdev(intf);

	usb_set_intfdata(intf, dev);
	SET_NETDEV_DEV(netdev, &intf->dev);

	netdev->netdev_ops = &usbcan_netdev_ops;
	/* TODO: populate netdev */

	register_netdev(netdev);

	return 0;
}

static void usbcan_disconnect(struct usb_interface *intf)
{
	struct usbcan *dev = usb_get_intfdata(intf);

	printk(KERN_INFO "%s\n", __func__);

	usb_set_intfdata(intf, NULL);

	if (dev) {
		unregister_netdev(dev->netdev);
		free_candev(dev->netdev);
	}
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
