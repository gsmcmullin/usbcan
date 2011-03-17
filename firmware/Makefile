CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc

CFLAGS = -g3 -O0 \
	-std=gnu99 -Wall -Wextra \
	-mcpu=cortex-m3 -mthumb 

LDFLAGS = -mthumb -march=armv7 -mfix-cortex-m3-ldrd -msoft-float \
	-lopencm3_stm32 -lc -lnosys -nostartfiles -Wl,-T,usbcan.ld

SRC = usbcan.c
OBJ = $(SRC:.c=.o)

usbcan:	$(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	-rm -f *.o usbcan

.PHONY: clean

