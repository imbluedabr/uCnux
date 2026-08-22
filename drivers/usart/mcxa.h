#pragma once
#include <drivers/usart.h>

void usart_mcxa_init(struct usart_device* usart, const struct mmio_bus_desc* desc);
ssize_t usart_mcxa_read(struct file* f, void* buff, size_t count);
ssize_t usart_mcxa_write(struct file* f, const void* buff, size_t count);

