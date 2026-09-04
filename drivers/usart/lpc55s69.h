#pragma once
#include <drivers/usart.h>

void usart_lpc55s69_init(struct usart_device* usart, const struct mmio_bus_desc* desc);
ssize_t usart_lpc55s69_read(struct device* dev, void* buff, size_t count);
ssize_t usart_lpc55s69_write(struct device* dev, const void* buff, size_t count);

