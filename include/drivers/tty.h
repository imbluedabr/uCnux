#pragma once
#include <kernel/device.h>
#include <kernel/lock.h>
#include <uapi/sys/termios.h>

struct tty_device {
    struct device base;
    struct device* reader;
    struct device* writer;
    struct termios mode;
    uint8_t status_flags;
    uint8_t fg_pgrp;
};

void tty_init();
struct device* tty_create(dev_t reader, dev_t writer, struct termios* mode);
struct device* tty_probe(struct bus_device* bus, const void* descriptor);
int tty_ioctl(struct file* f, int cmd, void* arg);
ssize_t tty_read(struct file* f, void* buff, size_t count);
ssize_t tty_write(struct file* f, const void* buff, size_t count);
int tty_open(struct device* dev, struct file* f);
int tty_release(struct device* dev, struct file* f);

