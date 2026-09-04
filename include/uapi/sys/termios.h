#pragma once
#include <stdint.h>

struct termios {
    uint8_t c_cflag;
    uint8_t o_flag;
    uint8_t c_lflag;
};
#define ONLRET (1 << 10)  //treat \n as \r\n
#define CBAUD 0xF       //set the baud rate
#define CSIZE 0x300     //frame size, options: 5, 6, 7 ,8
#define ICANON (1 << 0)
#define ECHO (1 << 1)

#define IOCTL_TTY_SETMODE 0
#define IOCTL_TTY_GETMODE 1
#define IOCTL_TTY_SETFGGRP 2


