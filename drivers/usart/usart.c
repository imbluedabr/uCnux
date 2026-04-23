#include <drivers/usart.h>
#include <kernel/alloc.h>
#include <kernel/settings.h>
#include <kernel/majors.h>
#include "mcxa.h"

//build table of available backends
static const struct usart_impl impl_table[] = {
#ifdef USART_DRIVER_MCXA
    [0] = {
        .init = &usart_mcxa_init,
        .readb = &usart_mcxa_readb,
        .writeb = &usart_mcxa_writeb,
        .ioctl = &usart_mcxa_ioctl,
        .destroy = &usart_mcxa_destroy
    }
#endif
};

struct device_driver usart_driver = {
    .create = &usart_create,
    .destroy = &usart_destroy,
    .ioctl = &usart_ioctl,
    .readb = &usart_readb,
    .writeb = &usart_writeb,
    .update = &usart_update,
    .instances = NULL,
    .instance_count = 0
};

void usart_init()
{
    driver_table[USART_MAJOR] = &usart_driver;
}

struct device* usart_create(uint8_t* minor, void* desc)
{
    struct usart_device* usart = kzalloc(sizeof(struct usart_device));
    if (usart == NULL) {
        return NULL;
    }

    usart->base.driver = &usart_driver;
    usart->base.next = usart_driver.instances; //append the new instance to the linked list
    usart_driver.instances = &usart->base;

    //update instance count and set minor value
    uint8_t tmp = usart_driver.instance_count + 1;
    if (tmp > 15) {
        return NULL; //max instance count reached
    }
    usart_driver.instance_count = tmp;
    *minor = tmp;
    struct usart_desc* d = desc;
    usart->base.impl = d->type;
    usart->usart_base = d->base;
    impl_table[d->type].init(usart, d);

    return &usart->base;
}

int usart_destroy(struct device* dev)
{
    return -1;
}

int usart_ioctl(struct device* dev, int op, void* arg)
{
    struct usart_device* usart = (struct usart_device*) dev;
    return impl_table[usart->base.impl].ioctl(usart, op, arg);
}

int usart_readb(struct device* dev)
{
    struct usart_device* usart = (struct usart_device*) dev;
    return impl_table[usart->base.impl].readb(usart);
}

int usart_writeb(struct device* dev, char val)
{
    struct usart_device* usart = (struct usart_device*) dev;
    return impl_table[usart->base.impl].writeb(usart, val);
}

void usart_update(struct device* dev)
{
    struct usart_device* usart = (struct usart_device*) dev;
    struct io_request* req = device_peek_request(dev);
    if (!req) {
        return;
    }
    uint32_t bytes_transfered = usart->bytes_transfered;
    if (req->op) {
        if (usart_writeb(dev, ((char*)req->buffer)[bytes_transfered]) < 0) {
            return;
        };
    } else {
        int tmp  = usart_readb(dev);
        if (tmp < 0) {
            return;
        }
        ((char*)req->buffer)[bytes_transfered] = tmp;
    }

    bytes_transfered++;
    if (bytes_transfered >= req->count) {
        req->count = bytes_transfered;
        req->status = 1;
        proc_unblock_process(req->waiter);
        device_dequeue_request(dev);
        usart->bytes_transfered = 0;
    } else {
        usart->bytes_transfered = bytes_transfered;
    }
}



