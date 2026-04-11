#include <stdint.h>

const char *u32_to_hex(uint32_t value) {
    static const char lookup[] = "0123456789abcdef";
    static char buf[9];
    for (int i = 0; i < 8; i++) {
        buf[7 - i] = lookup[value & 0xf];
        value >>= 4;
    }
    buf[8] = '\0';
    return buf;
}

void int_to_hex(char* buff, uint16_t val, uint8_t numc)
{
    static const char lookup[] = "0123456789abcdef";
    for (int i = 0; i < numc; i++) {
        buff[numc - i - 1] = lookup[val & 0xf];
        val >>= 4;
    }
}

