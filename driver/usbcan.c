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

#include "../icd.h"

MODULE_AUTHOR("Gareth McMullin <gareth@blacksphere.co.nz>");
MODULE_DESCRIPTION("SocketCAN driver Black Sphere USB-CAN Adapter");
MODULE_LICENSE("GPL v2");

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

static void usbcan_read_bulk_callback(struct urb *urb)
{
	struct usbcan *dev = urb->context;
	struct usbcan_msg *msg = urb->transfer_buffer;
	struct sk_buff *skb;
	struct can_frame *cf;
	int retval;

	printk(KERN_INFO "%s\n", __func__);

	/* Construct socket buffer and push up the stack */
	skb = alloc_can_skb(dev->netdev, &cf);
	cf->can_id = le32_to_cpu(msg->id);
	cf->can_dlc = msg->dlc;
	memcpy(cf->data, msg->data, 8);
	netif_rx(skb);

	/* Resubmit URB */
	usb_fill_bulk_urb(urb, dev->udev, usb_rcvbulkpipe(dev->udev, 1),
			  urb->transfer_buffer, 64,
			  usbcan_read_bulk_callback, dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval == -ENODEV)
		netif_device_detach(dev->netdev);
}

static int usbcan_open(struct net_device *netdev)
{
	struct usbcan *dev = netdev_priv(netdev);
	int retval;
	struct urb *urb = NULL;
	u8 *buf = NULL;

	printk(KERN_INFO "%s\n", __func__);
	open_candev(netdev);

	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			USBCAN_REQUEST_ON_OFF_BUS, 
			USB_TYPE_VENDOR | USB_RECIP_INTERFACE, 
			1, 0, NULL, 0, 100);

	if(retval < 0)
		return -EINVAL;

	/* create a URB, and a buffer for it */
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		dev_err(netdev->dev.parent,
			"No memory left for URBs\n");
		return -ENOMEM;
	}

	buf = usb_alloc_coherent(dev->udev, 64, GFP_KERNEL,
			       &urb->transfer_dma);
	if (!buf) {
		dev_err(netdev->dev.parent,
			"No memory left for USB buffer\n");
		usb_free_urb(urb);
		return -ENOMEM;
	}

	usb_fill_bulk_urb(urb, dev->udev, usb_rcvbulkpipe(dev->udev, 1),
			  buf, 64,
			  usbcan_read_bulk_callback, dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if (retval) {
		if (retval == -ENODEV)
			netif_device_detach(dev->netdev);

		usb_free_coherent(dev->udev, 64, buf,
				urb->transfer_dma);
		return retval;
	}

	netif_start_queue(netdev);

	return 0;
}

static int usbcan_close(struct net_device *netdev)
{
	struct usbcan *dev = netdev_priv(netdev);

	printk(KERN_INFO "%s\n", __func__);
	close_candev(netdev);

	usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			USBCAN_REQUEST_ON_OFF_BUS, 
			USB_TYPE_VENDOR | USB_RECIP_INTERFACE, 
			0, 0, NULL, 0, 100);
	netif_stop_queue(netdev);

	return 0;
}

static netdev_tx_t usbcan_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct usbcan *dev = netdev_priv(netdev);
	struct can_frame *cf = (struct can_frame *)skb->data;
	struct usbcan_msg msg;

	printk(KERN_INFO "%s\n", __func__);
	netdev->trans_start = jiffies;

	msg.id = cpu_to_le32(cf->can_id);
	msg.dlc = cf->can_dlc;
	memcpy(msg.data, cf->data, sizeof(msg.data));

	usb_bulk_msg(dev->udev,
			usb_sndbulkpipe(dev->udev, 1),
			&msg,
			sizeof(msg),
			NULL, HZ*10);

	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}

static const struct net_device_ops usbcan_netdev_ops = {
	.ndo_open = usbcan_open,
	.ndo_stop = usbcan_close,
	.ndo_start_xmit = usbcan_start_xmit,
};

static struct can_bittiming_const usbcan_bittiming_const = {
	.name = "usbcan",
	.tseg1_min = 1,
	.tseg1_max = 16,
	.tseg2_min = 1,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 1,
	.brp_max = 1024,
	.brp_inc = 1,
};

static int usbcan_set_bittiming(struct net_device *netdev)
{
	struct usbcan *dev = netdev_priv(netdev);
	struct can_bittiming *bt = &dev->can.bittiming;
	struct usbcan_bittiming ubt;

	printk(KERN_INFO "%s\n", __func__);

	ubt.brp = bt->brp;
	ubt.phase_seg1 = bt->phase_seg1 - 1;
	ubt.phase_seg2 = bt->phase_seg2 - 1;
	ubt.sjw = bt->sjw - 1;
	printk(KERN_INFO "%d %d %d %d\n", ubt.brp, ubt.phase_seg1,
			ubt.phase_seg2, ubt.sjw);
	usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			USBCAN_REQUEST_SET_BITTIMING, 
			USB_TYPE_VENDOR | USB_RECIP_INTERFACE, 
			0, 0, &ubt, sizeof(ubt), 100);

	return 0;
}

static int usbcan_set_mode(struct net_device *netdev, enum can_mode mode)
{
	struct usbcan *dev = netdev_priv(netdev);

	printk(KERN_INFO "%s\n", __func__);

	switch (mode) {
	case CAN_MODE_START:
		/* FIXME: What is this supposed to do? */

		if (netif_queue_stopped(netdev))
			netif_wake_queue(netdev);
		break;

	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

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
	dev->can.clock.freq = 72000000UL;
	dev->can.bittiming_const = &usbcan_bittiming_const;
	dev->can.do_set_bittiming = usbcan_set_bittiming;
	dev->can.do_set_mode = usbcan_set_mode;

	register_candev(netdev);

	return 0;
}

static void usbcan_disconnect(struct usb_interface *intf)
{
	struct usbcan *dev = usb_get_intfdata(intf);

	printk(KERN_INFO "%s\n", __func__);

	/* FIXME: Wait for any urb-less requests to finish. */
	usb_set_intfdata(intf, NULL);

	if (dev) {
		unregister_candev(dev->netdev);
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

