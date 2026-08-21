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

#define LS_ALL (1 << 0)
#define LS_LIST (1 << 1)

int builtin_ls(int argc, char** argv)
{
    int dir;
    int options = 0;
    const char* path = NULL;
    
    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        if (strcmp(arg, "-a") == 0) {
            options |= LS_ALL;
        } else if (strcmp(arg, "-l") == 0) {
            options |= LS_LIST;
        } else if (strcmp(arg, "-la") == 0) {
            options |= LS_LIST | LS_ALL;
        } else if (path == NULL) {
            path = arg;
        } else {
            puts("ls: invalid argument\n");
            return -1;
        }
    }
    if (path == NULL) path = ".";

    dir = open(path, O_RDONLY);

    if (dir < 0) {
        printf("ls: %s: no such file or directory\n", path);
        return -1;
    }

    struct dirent dirbuff;
    while (readdir(dir, &dirbuff, 1) == 1) {
        if (!(options & LS_ALL) && (strcmp(dirbuff.d_name, ".") == 0 || strcmp(dirbuff.d_name, "..") == 0)) continue;
        if (options & LS_LIST) {
            struct stat statbuff = {0};
            char newpath[32] = {0};
            snprintf(newpath, 32, "%s/%s", path, dirbuff.d_name);
            stat(newpath, &statbuff);
            printf("%o %d %d:%d\t%d\t%s\n", statbuff.st_mode, statbuff.st_nlink, statbuff.st_uid, statbuff.st_gid, statbuff.st_size, dirbuff.d_name);
        } else {
            printf("%s ", dirbuff.d_name);
        }
    }
    putc('\n');

    close(dir);
    return 0;
}

