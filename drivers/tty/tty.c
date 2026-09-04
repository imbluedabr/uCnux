#include <drivers/tty.h>
#include <kernel/majors.h>
#include <kernel/proc.h>
#include <lib/kmalloc.h>
#include <lib/kprint.h>
#include <uapi/sys/errno.h>
#include <uapi/signal.h>
#include <uapi/sys/fcntl.h>
#include <stddef.h>

static const struct dev_ops tty_ops = {
    .read = tty_read,
    .write = tty_write,
    .ioctl = tty_ioctl,
    .open = tty_open,
    .release = tty_release
};

//tty driver
struct device_driver tty_driver = {
    .probe = tty_probe,
    .name = "tty"
};

void tty_init() {
    register_driver(TTY_MAJOR, &tty_driver);
}

struct device* tty_create(dev_t reader, dev_t writer, struct termios* mode)
{
    struct tty_device* tty = (struct tty_device*) device_probe(TTY_MAJOR, NULL, NULL);
    if (!tty) return NULL;

    tty->reader = device_lookup(reader);
    tty->writer = device_lookup(writer);
    tty->mode = *mode;

    return &tty->base;
}

//create a tty instance
struct device* tty_probe(struct bus_device* parent, const void* descriptor)
{
    struct tty_device* tty = kzalloc(sizeof(struct tty_device));
    if (!tty) {
        return NULL;
    }
    tty->base.ops = (struct dev_ops*) &tty_ops;
    
    return &tty->base;
}

int tty_ioctl(struct file* f, int cmd, void* arg) {
    struct tty_device* tty = (struct tty_device*) f->i->devfs.dev;
   
    switch(cmd) {
        case IOCTL_TTY_SETMODE:
            tty->mode = *((struct termios*) arg);
            break;
        case IOCTL_TTY_GETMODE:
            *((struct termios*) arg) = tty->mode;
            break;
        case IOCTL_TTY_SETFGGRP:
            tty->fg_pgrp = *((pid_t*) arg);
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

//helper function
static inline void writeb(struct tty_device* tty, char c)
{
    if (tty->mode.o_flag & ONLRET && c == '\n') {
        while(tty->writer->ops->ll_write(tty->writer, "\r", 1));
    }
    while(tty->writer->ops->ll_write(tty->writer, &c, 1) < 0);
}

ssize_t tty_read(struct file* f, void* buff, size_t count)
{
    struct tty_device* tty = (struct tty_device*) f->i->devfs.dev;
    struct device* reader = tty->reader;
    struct device* writer = tty->writer;
    if (!writer) return -ENODEV;
    if (!writer->ops->ll_write) return -ENODEV;
    if (!reader) return -ENODEV;
    if (!reader->ops->ll_read) return -ENODEV;

    uint8_t* cbuff = buff;
    uint32_t i = 0;
    while (i < count) {
        char c;
        while (reader->ops->ll_read(reader, &c, 1) < 0);
        
        writeb(tty, c);
        if (c == '\r' || c == '\n') {
            cbuff[i++] = '\n';
            break;
        } else if (c == '\b') {
            writeb(tty, ' ');
            writeb(tty, '\b');
            cbuff[--i] = ' ';
        } else if (c == 0x04) {
            break;
        } else {
            cbuff[i++] = c;
        }
    }

    return i;
}

ssize_t tty_write(struct file* f, const void* buff, size_t count)
{
    struct tty_device* tty = (struct tty_device*) f->i->devfs.dev;
    struct device* writer = tty->writer;
    if (!writer) return -ENODEV;
    if (!writer->ops->ll_write) return -ENODEV;

    uint32_t i;
    
    //horrible code, like what am i even doing here, just kys atp
    while(i < count) {
        if (((uint8_t*) buff)[i] == '\n' && tty->mode.o_flag & ONLRET) {
            while (writer->ops->ll_write(writer, "\r", 1) < 0);
        }

        while (writer->ops->ll_write(writer, buff + i, 1) < 0) {
            if (f->flags & O_NONBLOCK) goto end;
        }
        i++;
    }
end:
    if (i == 0) return -EAGAIN;
    return i;
}

int tty_open(struct device* dev, struct file* f)
{
    
}

int tty_release(struct device* dev, struct file* f)
{
    
}

