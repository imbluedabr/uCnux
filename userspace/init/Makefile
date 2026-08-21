

LIBC = ../larpc
TOOLCHAIN = arm-none-eabi
KERNEL_HEADERS= ../../include/uapi
INSTALL_DIR = ../../staging/bin

include $(LIBC)/libc.mk

CFLAGS += -Os
LDFLAGS += -Os

$(CC) = $(TOOLCHAIN)-gcc

init.o: init.c
	$(TOOLCHAIN)-gcc $(CFLAGS) -c init.c

init: init.o
	$(TOOLCHAIN)-gcc $(LDFLAGS) init.o -o init.elf
	$(TOOLCHAIN)-objcopy -O binary init.elf init

all: clean install

install: init
	mv init $(INSTALL_DIR)

clean:
	rm -f *.o *.elf

