ROOT ?= $(PWD)

BUILD = $(ROOT)/build

CONFIG ?= .config

-include $(CONFIG)

SETTINGS_FILE = $(ROOT)/include/kernel/settings.h

$(shell echo "#define ROOTFS_DEVNO (($(CONFIG_ROOTFS_DEV_MAJ) << 4) | $(CONFIG_ROOTFS_DEV_MIN))" > $(SETTINGS_FILE))
$(shell echo "#define ROOTFS_TYPE \"$(CONFIG_ROOTFS_TYPE)\"" >> $(SETTINGS_FILE))
$(shell echo "#define INIT_PATH \"$(CONFIG_INIT_PATH)\"" >> $(SETTINGS_FILE))
$(shell echo "#define INIT_CONSOLE_DEVNO $(CONFIG_INIT_CONSOLE_DEVNO)" >> $(SETTINGS_FILE))
$(shell echo "#define BOARD_TYPE $(CONFIG_BOARD_TYPE)" >> $(SETTINGS_FILE))


ifeq ($(CONFIG_BOARD_MCXA153), y)
ARCH = armv8-m
BOARD = mcxa153
TOOLCHAIN = arm-none-eabi
ARCH_CFLAGS = -mcpu=cortex-m33 -mthumb -std=gnu11 -DCPU_MCXA153VFM
ARCH_LDFLAGS =
$(shell echo "#define ARCH_MCXA153" >> $(SETTINGS_FILE))
endif

# the filesystems
FS_SELECT = fs/fs.c

ifeq ($(CONFIG_FS_FATFS), y)
FS_SELECT += fs/fatfs.c
$(shell echo "#define FS_FATFS" >> $(SETTINGS_FILE))
endif

DEV_SELECT = 

ifeq ($(CONFIG_TTY_DRIVER), y)
DEV_SELECT += drivers/tty/tty.c
$(shell echo "#define TTY_DRIVER" >> $(SETTINGS_FILE))
endif

ifeq ($(CONFIG_USART_DRIVER), y)
DEV_SELECT += drivers/usart/usart.c
$(shell echo "#define USART_DRIVER" >> $(SETTINGS_FILE))
endif

ifeq ($(CONFIG_USART_DRIVER_MCXA), y)
DEV_SELECT += drivers/usart/mcxa.c
$(shell echo "#define USART_DRIVER_MCXA" >> $(SETTINGS_FILE))
endif

ifeq ($(CONFIG_ST7920_DRIVER), y)
DEV_SELECT += drivers/st7920/st7920.c
$(shell echo "#define ST7920_DRIVER" >> $(SETTINGS_FILE))
endif


#source files
CSRCS = $(FS_SELECT) \
		$(DEV_SELECT) \
	   $(wildcard kernel/*.c) \
	   $(wildcard arch/$(ARCH)/*.c) \
	   $(wildcard lib/*.c) \
	   $(wildcard board/$(BOARD)/*.c)

ASRCS = $(wildcard arch/$(ARCH)/*.S) \
		$(wildcard board/$(BOARD)/*.S)

#include options
INCL ?= -I$(ROOT)/include

#linker file
LNKF = $(ROOT)/board/$(BOARD)/linker.ld

# link path options
LNKP ?=

MN_FILE ?= kernel.elf

# IF debugging stack faliures:
# make CONFIG_DEBUG=y

CFLAGS = $(ARCH_CFLAGS) -ffreestanding -Wall -Wextra -Wno-unused-parameter  $(INCL)
ASFLAGS = $(CFLAGS)
LDFLAGS = $(ARCH_LDFLAGS) -nostdlib -nostartfiles -static


ifeq ($(CONFIG_DEBUG), y)
	CFLAGS += -D__DEBUG__ -g -fverbose-asm
endif

CFLAGS += -Os
LDFLAGS += -Os
ASFLAGS += -fno-lto

ifeq ($(CONFIG_LTO), y)
CFLAGS += -flto
LDFLAGS += -flto
endif

OBJS = $(patsubst %.c,$(BUILD)/%.o,$(CSRCS)) $(patsubst %.S,$(BUILD)/%.o,$(ASRCS))
CC = $(TOOLCHAIN)-gcc 
OBJCOPY = $(TOOLCHAIN)-objcopy
READELF = $(TOOLCHAIN)-readelf

.PHONY: userspace install

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@


$(MN_FILE): $(OBJS)
	$(CC) $(LDFLAGS) -T $(LNKF) $(OBJS) -o $@

image: $(MN_FILE)
	$(OBJCOPY) -O binary $(MN_FILE) image.bin

dump:
	$(TOOLCHAIN)-objdump -d -M no-aliases $(MN_FILE) >> dump.s.dump
	$(TOOLCHAIN)-objdump -S -d -M no-aliases $(MN_FILE) >> verbose_dump.s.dump

clean:
	rm -r $(BUILD)


