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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <libopencm3/usb/usbd.h>

static char *get_dev_unique_id(char *s);

const struct usb_device_descriptor dev = {
        .bLength = USB_DT_DEVICE_SIZE,
        .bDescriptorType = USB_DT_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = 0xFF,
        .bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = 64,
        .idVendor = 0xCAFE,
        .idProduct = 0xCAFE,
        .bcdDevice = 0x0200,
        .iManufacturer = 1,
        .iProduct = 2,
        .iSerialNumber = 3,
        .bNumConfigurations = 1,
};

const struct usb_interface_descriptor iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = 0xFF,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,
};

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &iface,
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static char serial_no[25];

const char *usb_strings[] = {
	"x",
	"Black Sphere Technologies",
	"USB-CAN Adapter",
	serial_no
};

static int simple_control_callback(struct usb_setup_data *req, u8 **buf, 
		u16 *len, void (**complete)(struct usb_setup_data *req))
{
	(void)buf;
	(void)len;
	(void)complete;

	if(req->bmRequestType != 0x40) 
		return 0; /* Only accept vendor request */
	
	if(req->wValue & 1)
		gpio_set(GPIOA, GPIO1);
	else
		gpio_clear(GPIOA, GPIO1);

	return 1;
}

int main(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_OTGFSEN);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);

	/* LED output */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, 
			GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);

	get_dev_unique_id(serial_no);

	usbd_init(&stm32f107_usb_driver, &dev, &config, usb_strings);
	usbd_register_control_callback(
				USB_REQ_TYPE_VENDOR, 
				USB_REQ_TYPE_TYPE,
				simple_control_callback);

	while (1) 
		usbd_poll();
}

static char *get_dev_unique_id(char *s) 
{
        volatile uint8_t *unique_id = (volatile uint8_t *)0x1FFFF7E8;
        int i;

        /* Fetch serial number from chip's unique ID */
        for(i = 0; i < 24; i+=2) {
                s[i] = ((*unique_id >> 4) & 0xF) + '0';
                s[i+1] = (*unique_id++ & 0xF) + '0';
        }
        for(i = 0; i < 24; i++) 
                if(s[i] > '9') 
                        s[i] += 'A' - '9' - 1;

	return s;
}

