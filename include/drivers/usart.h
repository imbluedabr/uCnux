#pragma once
#include <kernel/device.h>

#define USART_MCXA 0
#define USART_LPC55S69 1

extern const int usart_baud_rates[11];

#define USART_TX_FIFO_LEN 16
#define USART_TX_FIFO_MSK (USART_TX_FIFO_LEN - 1)
#define USART_RX_FIFO_LEN 16
#define USART_RX_FIFO_MSK (USART_RX_FIFO_LEN - 1)

struct usart_device {
    struct device base;
    void* usart_base;
    char tx_fifo[USART_TX_FIFO_LEN];
    char rx_fifo[USART_RX_FIFO_LEN];
    uint8_t tx_head;
    uint8_t tx_tail;
    uint8_t rx_head;
    uint8_t rx_tail;
};

void usart_init();
struct device* usart_probe(struct bus_device* parent, const void* descriptor);
ssize_t usart_read(struct file* f, void* buff, size_t count);
ssize_t usart_write(struct file* f, const void* buff, size_t count);

struct usart_impl {
    void (*init)(struct usart_device* usart, const struct mmio_bus_desc* desc);
    ssize_t (*read)(struct file* f, void* buff, size_t count);
    ssize_t (*write)(struct file* f, const void* buff, size_t count);
};

