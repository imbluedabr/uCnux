#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <unistd.h>
#include <syscalls.h>
#include "svcall.h"

int stat(const char* pathname, struct stat* statbuf) {
    int fd = open(pathname, O_RDONLY);
    if (fd < 0) return -ENOENT;
    int status =  fstat(fd, statbuf);
    close(fd);
    return status;
}

[[gnu::naked]] int fstat(int fd, struct stat* statbuf) {
    SVCALL(SYS_FSTAT);
}

[[gnu::naked]] int mkdir(const char* pathname, mode_t mode) {
    SVCALL(SYS_MKDIR);
}

[[gnu::naked]] int mknod(const char* pathname, mode_t mode, dev_t dev) {
    SVCALL(SYS_MKNOD);
}

[[gnu::naked]] mode_t umask(mode_t mask) {
    SVCALL(SYS_UMASK);
}

