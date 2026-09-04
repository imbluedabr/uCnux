#include <drivers/romdisk.h>
#include <kernel/majors.h>
#include <uapi/sys/errno.h>
#include <uapi/sys/fcntl.h>
#include <uapi/sys/disk.h>
#include <uapi/unistd.h>
#include <fs/vfs.h>
#include <lib/stdlib.h>
#include <lib/kmalloc.h>
#include <lib/kprint.h>

#define ALIGN(VAL, AL) ((VAL + (AL - 1)) & -AL)

struct device_driver romdisk_driver = {
    .probe = romdisk_probe,
    .name = "romdisk",
    .bus_accept = BUS_MMIO
};

static const struct dev_ops romdisk_ops = {
    .read = romdisk_read,
    .write = romdisk_write,
    .lseek = romdisk_lseek,
    .ioctl = romdisk_ioctl
};

void romdisk_init()
{
    register_driver(ROMDISK_MAJOR, &romdisk_driver);
}

struct device* romdisk_probe(struct bus_device* parent, const void* descriptor)
{
    const struct mmio_bus_desc* desc = descriptor;
    struct romdisk_device* dev = kzalloc(sizeof(struct romdisk_device));
    if (!dev) return NULL;

    dev->base.ops = (struct dev_ops*) &romdisk_ops;
    dev->conf = *desc;
    uint32_t block_count = desc->size/ROMDISK_BLK_SECSZ;

    kdbg("romdsk: BLK_NSEC=%d, BLK_SECSZ=%d, BLK_SZ=%d\n", block_count, ROMDISK_BLK_SECSZ, desc->size);
    return &dev->base;
}

int romdisk_ioctl(struct file* f, int cmd, void* arg)
{
    struct romdisk_device* romdisk = (struct romdisk_device*) f->i->devfs.dev;
    size_t* s_arg = arg;
    switch(cmd) {
        case IOCTL_BLK_GETSZ:
            *s_arg = romdisk->conf.size;
            break;
        case IOCTL_BLK_GETNSEC:
            *s_arg = romdisk->conf.size/ROMDISK_BLK_SECSZ;
            break;
        case IOCTL_BLK_GETSECSZ:
            *s_arg = ROMDISK_BLK_SECSZ;
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

ssize_t romdisk_read(struct file* f, void* buff, size_t count)
{
    struct romdisk_device* disk = (struct romdisk_device*) f->i->devfs.dev;
    count = ALIGN(count, ROMDISK_BLK_SECSZ);
    if ((f->offset + count) > disk->conf.size) count = disk->conf.size - f->offset;
    memcpy(buff, disk->conf.base + f->offset, count);
    return count;
}

ssize_t romdisk_write(struct file* f, const void* buff, size_t count)
{
    return -ENODEV;
}

off_t romdisk_lseek(struct file* f, off_t offset, int whence)
{
    struct romdisk_device* disk = (struct romdisk_device*) f->i->devfs.dev;
    off_t curr_offset = f->offset;
    if (whence == SEEK_SET) {
        curr_offset = offset;
    } else if (whence == SEEK_CUR) {
        curr_offset += offset;
    } else {
        return  -EINVAL;
    }
    if (curr_offset > disk->conf.size/ROMDISK_BLK_SECSZ)
        return -EINVAL;
    f->offset = curr_offset;
    return curr_offset;
}



