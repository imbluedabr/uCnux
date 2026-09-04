#pragma once
#include <kernel/device.h>
#include <uapi/sys/types.h>

struct romdisk_device {
    struct device base;
    struct mmio_bus_desc conf;
};

#define ROMDISK_BLK_SECSZ 512
void romdisk_init();
struct device* romdisk_probe(struct bus_device* parent, const void* descriptor);
ssize_t romdisk_read(struct file* f, void* buff, size_t count);
ssize_t romdisk_write(struct file* f, const void* buff, size_t count);
off_t romdisk_lseek(struct file* f, off_t offset, int whence);
int romdisk_ioctl(struct file* f, int cmd, void* arg);


