#include "main.h"

#include <unistd.h>
#include <sys/utsname.h>
#include <sys/spawn.h>
#include <sys/dir.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


static char data_buff[256];


int builtin_cat(int argc, char** argv)
{
    int index = 1;

    do {
        int fd = STDIN_FILENO;
        if (argc > 1) {
            fd = open(argv[index++], O_RDONLY);
        }

        if (fd < 0) {
            puts("cat: no such file or directory\n");
            return -1;
        }
        int count = 0;
        while((count = read(fd, &data_buff, sizeof(data_buff))) > 0) {
            write(STDOUT_FILENO, data_buff, count);
            memset(data_buff, 0, sizeof(data_buff));
        }
        close(fd);
    } while(argv[index]);
    return 0;
}


