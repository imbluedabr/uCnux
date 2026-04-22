#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t dev_t;

#define MAJOR(DEVNO) (DEVNO >> 4)
#define MINOR(DEVNO) (DEVNO & 0xF)

#define MKDEV(MAJOR_NO, MINOR_NO) ((MAJOR_NO << 4) | (MINOR_NO & 0xF))

#define IOCTL_RESET 1

struct termios {
    uint16_t c_cflag;
    uint16_t c_lflag;
};
#define CNLRET (1 << 10)  //treat \n as \r\n
#define CBAUD 0xF       //set the baud rate
#define CSIZE 0x300     //frame size, options: 5, 6, 7 ,8
#define ICANON (1 << 0)
#define ECHO (1 << 1)

#define IOCTL_TTY_CLEAR 4
#define IOCTL_TTY_SETMODE 5
#define IOCTL_TTY_GETMODE 6
