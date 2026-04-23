#pragma once
#include "types.h"
#include "../limits.h"

#define FS_MODE_DIR 0
#define FS_MODE_FILE 2
#define FS_MODE_MOUNT 1
#define FS_MODE_DEV 3
#define FS_IS_A_FILE(MODE) (MODE & 0b10)

struct stat {
    uint32_t size;
    time_t time;
    uint8_t mode;
    dev_t dev;
    ino_t d_ino;
    char name[FS_INAME_LEN + 1];
};

