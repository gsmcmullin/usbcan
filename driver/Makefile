KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

obj-m += usbcan.o

modules modules_install clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@

