

LIBC = ../larpc
TOOLCHAIN = arm-none-eabi
KERNEL_HEADERS= ../../include/uapi
INSTALL_DIR = ../../staging/bin

include $(LIBC)/libc.mk

CFLAGS += -Os
LDFLAGS += -Os

CC = $(TOOLCHAIN)-gcc
SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)

%.o: %.c
	$(TOOLCHAIN)-gcc $(CFLAGS) -c $<

larpbox: $(OBJS)
	$(TOOLCHAIN)-gcc $(LDFLAGS) $(OBJS) -o larpbox.elf
	$(TOOLCHAIN)-objcopy -O binary larpbox.elf larpbox

all: clean install

install: larpbox
	mv larpbox $(INSTALL_DIR)

clean:
	rm -f *.o *.elf larpbox

