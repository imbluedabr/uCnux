#include "mcxa.h"
#include <board/mcxa153/board.h>
#include <kernel/interrupt.h>
#include <lib/kprint.h>
#include <drivers/usart.h>
#include <fs/vfs.h>
#include <uapi/sys/fcntl.h>

struct init_entry {
    volatile uint32_t* clksel_addr;
    uint32_t clksel_bit;
    volatile uint32_t* clkdiv_addr;
    uint32_t clkdiv_bit;
    uint32_t glb_bit0;
    uint32_t glb_bit1;
    uint32_t rst_bit0;
    uint32_t rst_bit1;
    volatile PORT_Type* port_addr;
    uint8_t port_pin_rx;
    uint8_t port_pin_tx;
    uint8_t port_mux;
};

static const volatile void* init_addr_table[] = {
    LPUART0,
    LPUART1,
    LPUART2
};

static const struct init_entry init_table[] = {
    {
        .clksel_addr = &MRCC0->MRCC_LPUART0_CLKSEL,
        .clksel_bit = MRCC_MRCC_LPUART0_CLKSEL_MUX(2),
        .clkdiv_addr = &MRCC0->MRCC_LPUART2_CLKSEL,
        .clkdiv_bit = 0,
        .glb_bit0 = MRCC_MRCC_GLB_CC0_LPUART0(1),
        .glb_bit1 = MRCC_MRCC_GLB_CC0_PORT0(1),
        .rst_bit0 = MRCC_MRCC_GLB_RST0_LPUART0(1),
        .rst_bit1 = MRCC_MRCC_GLB_RST0_PORT0(1),
        .port_addr = PORT0,
        .port_pin_rx = 2,
        .port_pin_tx = 3,
        .port_mux = 2
    },
    {
        .clksel_addr = &MRCC0->MRCC_LPUART1_CLKSEL,
        .clksel_bit = MRCC_MRCC_LPUART1_CLKSEL_MUX(2),
        .clkdiv_addr = &MRCC0->MRCC_LPUART2_CLKSEL,
        .clkdiv_bit = 0,
        .glb_bit0 = MRCC_MRCC_GLB_CC0_LPUART1(1),
        .glb_bit1 = MRCC_MRCC_GLB_CC0_PORT2(1),
        .rst_bit0 = MRCC_MRCC_GLB_RST0_LPUART1(1),
        .rst_bit1 = MRCC_MRCC_GLB_RST0_PORT2(1),
        .port_addr = PORT2,
        .port_pin_rx = 12,
        .port_pin_tx = 13,
        .port_mux = 3
    },
    {
        .clksel_addr = &MRCC0->MRCC_LPUART2_CLKSEL,
        .clksel_bit = MRCC_MRCC_LPUART2_CLKSEL_MUX(2),
        .clkdiv_addr = &MRCC0->MRCC_LPUART2_CLKSEL,
        .clkdiv_bit = 0,
        .glb_bit0 = MRCC_MRCC_GLB_CC0_LPUART2(1),
        .glb_bit1 = MRCC_MRCC_GLB_CC0_PORT1(1),
        .rst_bit0 = MRCC_MRCC_GLB_RST0_LPUART2(1),
        .rst_bit1 = MRCC_MRCC_GLB_RST0_PORT1(1),
        .port_addr = PORT1,
        .port_pin_rx = 4,
        .port_pin_tx = 5,
        .port_mux = 2
    }
};

static int lpuart_init(volatile void* addr)
{
    int instance = 0;
    for (int i = 0; i < 3; i++) {
        if (init_addr_table[i] == addr) {
            i = instance;
            break;
        }
    }
    if (!instance) return -1;
    const struct init_entry* data = &init_table[instance];
    
    *data->clksel_addr = data->clksel_bit;
    *data->clkdiv_addr = data->clkdiv_bit;

    MRCC0->MRCC_GLB_CC0_SET = data->glb_bit0;
    MRCC0->MRCC_GLB_CC0_SET = data->glb_bit1;
    
    MRCC0->MRCC_GLB_RST0_SET = data->rst_bit0;
    MRCC0->MRCC_GLB_RST0_SET = data->rst_bit1;

    data->port_addr->PCR[data->port_pin_rx] = PORT_PCR_LK(1) | PORT_PCR_MUX(data->port_mux) | PORT_PCR_IBE(1);
    data->port_addr->PCR[data->port_pin_tx] = PORT_PCR_LK(1) | PORT_PCR_MUX(data->port_mux);

    return 0;
}

void usart_mcxa_interrupt()
{
    struct usart_device* usart = get_current_handler_struct();

    volatile LPUART_Type* lpuart = usart->usart_base;
    
    if (lpuart->STAT & LPUART_STAT_RDRF_MASK) {
        char c = lpuart->DATA;
        //lpuart->DATA = 'B';
        uint8_t tmp = usart->rx_head;
        usart->rx_fifo[tmp] = c;
        usart->rx_head = (tmp + 1) & USART_RX_FIFO_MSK;
        return;
    }
    
    if (usart->tx_tail != usart->tx_head) {
        uint8_t tmp = (usart->tx_tail + 1) & USART_TX_FIFO_MSK;
        usart->tx_tail = tmp;
        lpuart->DATA = usart->tx_fifo[tmp];
    } else {
        lpuart->CTRL &= ~LPUART_CTRL_TIE_MASK;
    }
}

void usart_mcxa_init(struct usart_device* usart, const struct mmio_bus_desc* desc)
{
    volatile LPUART_Type* lpuart = desc->base;
    lpuart_init(lpuart);
    register_interrupt(desc->irq, usart, usart_mcxa_interrupt);
    NVIC_SetPriority(desc->irq, 3);
    NVIC_ClearPendingIRQ(desc->irq);
    NVIC_EnableIRQ(desc->irq);

    lpuart->BAUD = LPUART_BAUD_OSR(0b01111) | LPUART_BAUD_SBR(CLK_FRO_48MHZ / (usart_baud_rates[9]*16));
    lpuart->CTRL |= LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK | LPUART_CTRL_RIE_MASK;
}

static inline int readb(struct usart_device* usart)
{
    __disable_irq();
    if (usart->rx_tail == usart->rx_head) {
        __enable_irq();
        return -1;
    }
    uint8_t tmp = usart->rx_tail;
    usart->rx_tail = (tmp + 1) & USART_RX_FIFO_MSK;
    __enable_irq();
    //kputc('A');
    return usart->rx_fifo[tmp];
}


ssize_t usart_mcxa_read(struct file* f, void* buff, size_t count)
{
    struct usart_device* usart = (struct usart_device*) f->i->devfs.dev;
    uint32_t i;
    for (i = 0; i < count; i++) {
        int c;
        while ((c = readb(usart)) < 0) {
            if (f->flags & O_NONBLOCK) return i;
        }
        ((uint8_t*) buff)[i] = c;
    }

    return i;
}


static inline int writeb(struct usart_device* usart, uint8_t val)
{
    __disable_irq();
    uint8_t tmp = (usart->tx_head + 1) & USART_TX_FIFO_MSK;
    if (tmp == usart->tx_tail) {
        __enable_irq();
        return -1;
    }
    usart->tx_head = tmp;
    usart->tx_fifo[tmp] = val;
    volatile LPUART_Type* lpuart = usart->usart_base;
    lpuart->CTRL |= LPUART_CTRL_TIE_MASK;
    __enable_irq();
    return 0;
}

ssize_t usart_mcxa_write(struct file* f, const void* buff, size_t count)
{
    struct usart_device* usart = (struct usart_device*) f->i->devfs.dev;
    uint32_t i;
    for (i = 0; i < count; i++) {
        while ((writeb(usart, ((uint8_t*) buff)[i])) < 0) {
            if (f->flags & O_NONBLOCK) return i;
        }
    }

    return i;   
}


