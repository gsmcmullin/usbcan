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

/* This file contains parameters for the interface between the firmware
 * and driver.
 */

#ifndef __USBCAN_ICD_H
#define __USBCAN_ICD_H

#define USB_USBCAN_VENDOR_ID	0xCAFE
#define USB_USBCAN_PRODUCT_ID	0xCAFE

/* USB control bRequest values */
#define USBCAN_REQUEST_ON_OFF_BUS	0x00
#define USBCAN_REQUEST_SET_BITTIMING	0x01

struct usbcan_msg {
	u32 id;
	u8 dlc;
	u8 data[8];
} __attribute__((packed));
/* Flags set in usbcan_msg.id */
#define USBCAN_MSG_ID_MASK	0x1FFFFFFFUL
#define USBCAN_MSG_ID_EID	(1UL << 31)
#define USBCAN_MSG_ID_RTR	(1UL << 30)

struct usbcan_bittiming {
	/* Work around libopenstm32 usb problem */
	uint32_t padding;
	u32 brp;
	u8 phase_seg1;
	u8 phase_seg2;
	u8 sjw;
} __attribute__((packed));

#endif

