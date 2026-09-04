#include "lpc55s69.h"
#include <kernel/board.h>

#define CALC_BAUD(BAUD) (12000000/(16*BAUD) - 1)

void usart_lpc55s69_init(struct usart_device* usart, const struct mmio_bus_desc* desc)
{
    //enable ioconn module
    SYSCON->AHBCLKCTRLX[0] |= SYSCON_AHBCLKCTRL0_IOCON(1);

    //enable flexcomm 0
    SYSCON->AHBCLKCTRLX[1] |= SYSCON_AHBCLKCTRL1_FC0(1);
    SYSCON->PRESETCTRLCLR[1] = SYSCON_PRESETCTRL1_FC0_RST(1);
    SYSCON->FCCLKSELX[0] = SYSCON_FCCLKSEL0_SEL(2); //select 12 MHz clock

    //set the pin function
    IOCON->PIO[0][24] |= IOCON_PIO_FUNC(1); //pin 24 is rx
    IOCON->PIO[0][25] |= IOCON_PIO_FUNC(1); //pin 25 is tx
    
    //setup flexcom0 as a usart
    FLEXCOMM0->PSELID = FLEXCOMM_PSELID_PERSEL(1) | FLEXCOMM_PSELID_LOCK(1);
    

    //setup usart
    USART0->CFG = USART_CFG_ENABLE(1) | USART_CFG_DATALEN(1); //8 data bits, no parity, 1 stop bit
    USART0->BRG = CALC_BAUD(usart_baud_rates[115200]);
    USART0->OSR = USART_OSR_OSRVAL(15); //16x oversampling
    USART0->FIFOCFG = USART_FIFOCFG_ENABLETX(1) | USART_FIFOCFG_ENABLERX(1) | USART_FIFOCFG_EMPTYTX(1) | USART_FIFOCFG_EMPTYRX(1); //enable fifo's and reset the tx/rx fifo
}

static inline int readb(struct usart_device* usart)
{
    if (USART0->FIFOSTAT & USART_FIFOSTAT_RXNOTEMPTY_MASK) {
        char c = USART0->FIFORD;
        if (c == 'h') return -1;
        return c;
    }
    return -1;
}

static inline int writeb(struct usart_device* usart, uint8_t val)
{
    if (!(USART0->FIFOSTAT & USART_FIFOSTAT_TXNOTFULL_MASK)) return -1;
    USART0->FIFOWR = val;
    return 0;
}

ssize_t usart_lpc55s69_read(struct device* dev, void* buff, size_t count)
{
    struct usart_device* usart = (struct usart_device*) dev;
    uint32_t i;
    for (i = 0; i < count; i++) {
        int c;
        if ((c = readb(usart)) < 0) {
            break;
        }
        ((uint8_t*) buff)[i] = c;
    }
    if (!i) return -EAGAIN;
    return i;
}

ssize_t usart_lpc55s69_write(struct device* dev, const void* buff, size_t count)
{
    struct usart_device* usart = (struct usart_device*) dev;
    uint32_t i;
    for (i = 0; i < count; i++) {
        if ((writeb(usart, ((uint8_t*) buff)[i])) < 0) {
            break;
        }
    }
    if (!i) return -EAGAIN;
    return i;   
}

