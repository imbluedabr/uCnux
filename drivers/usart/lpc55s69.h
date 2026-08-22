#pragma once
#include <drivers/usart.h>

void usart_lpc55s69_init(struct usart_device* usart, const struct mmio_bus_desc* desc);
int usart_lpc55s69_read(struct file* f, void* buff, size_t count);
int usart_lpc55s69_write(struct file* f, const void* buff, size_t count);

