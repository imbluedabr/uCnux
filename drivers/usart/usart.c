#include <drivers/usart.h>
#include <lib/kmalloc.h>
#include <lib/kprint.h>
#include <kernel/settings.h>
#include <kernel/majors.h>
#include <kernel/proc.h>
#include "mcxa.h"
#include "lpc55s69.h"

const int usart_baud_rates[11] = {
    110,
    300,
    1200,
    2400,
    4800,
    9600,
    19200,
    28800,
    38400,
    57600,
    115200
};


static const struct usart_impl impl[2] = {
    {
        .init = usart_mcxa_init,
        .read = usart_mcxa_read,
        .write = usart_mcxa_write
    },
    {
        .init = usart_lpc55s69_init,
        .read = usart_lpc55s69_read,
        .write = usart_lpc55s69_write
    }
};

static struct dev_ops usart_ops = {
};

struct device_driver usart_driver = {
    .probe = usart_probe,
    .name = "usart",
    .bus_accept = BUS_MMIO
};

void usart_init()
{
    register_driver(USART_MAJOR, &usart_driver);
}

struct device* usart_probe(struct bus_device* parent, const void* descriptor)
{
    struct usart_device* dev = kzalloc(sizeof(struct usart_device));
    if (!dev) return NULL;
    const struct mmio_bus_desc* desc = descriptor;
    dev->usart_base = desc->base;
    
    impl[desc->vendor_id].init(dev, desc);
    usart_ops.read = impl[desc->vendor_id].read;
    usart_ops.write = impl[desc->vendor_id].write;
    
    return &dev->base;
}


