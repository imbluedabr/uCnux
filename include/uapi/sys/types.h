#pragma once
#include <stdint.h>


#define MAJOR(DEVNO) (DEVNO >> 8)
#define MINOR(DEVNO) (DEVNO & 0xFF)

#define MKDEV(MAJOR_NO, MINOR_NO) ((MAJOR_NO << 8) | (MINOR_NO & 0xFF))

typedef int ino_t; //this is a unique identifier for an inode, this would be like the cluster + offset in fat
typedef uint32_t off_t;
typedef int32_t ssize_t;
typedef uint16_t dev_t;
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
typedef uint8_t nlink_t;
typedef uint16_t mode_t;
typedef uint16_t blksize_t;
typedef uint16_t blkcnt_t;
typedef uint8_t fsid_t;
typedef uint32_t sigset_t;

