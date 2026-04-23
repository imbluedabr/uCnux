#pragma once

#include <uapi/sys/types.h>
#include <uapi/sys/stat.h>
#include <uapi/limits.h>
#include <kernel/device.h>
#include <stdint.h>

struct superblock;
struct file;
struct dentry;

#define FS_GET_INO_OFF(ID) (ID << 16)

struct permissions {
    uid_t user;
    gid_t group;
    mode_t mode;
};

struct devfs_inode {
    dev_t devno;
};

struct fatfs_inode {
    uint8_t fat_index;
};

struct inode {
    struct superblock* fs; //this is the filesystem that the inode is part of
    char name[FS_INAME_LEN + 1]; //the name of the file
    uint8_t refcount; //the amount of references exist to this inode, this includes file descriptors and dentries
    ino_t dir; //the parent directory of this inode
    ino_t file; //the inum of this inode
    size_t size;
    struct permissions perm;
    union {
        struct devfs_inode devfs;
        struct fatfs_inode fatfs;
    };
};

//file operations
struct file_ops {
    //file descriptor ops
    int8_t (*read)(struct file* f, char* buff, int count);
    int8_t (*write)(struct file* f, const char* buff, int count);
    int (*fstat)(struct file* f, struct stat* statbuff);
    int (*lseek)(struct file* f, off_t offset, int whence);
    int (*ftruncate)(struct file* f, off_t lenght);

    //inode operations
    int (*mount)(struct inode* mountpoint, dev_t devno, const char* args);
    int (*umount)(struct superblock* fs);
    
    int (*mkdir)(struct inode* dir, const char* name);
    int (*rmdir)(struct inode* dir);
    int (*unlink)(struct inode* target);

    struct permissions (*getperm)(struct inode* target);
    int (*setperm)(struct inode* target, struct permissions perm);

    //filesystem lookup functions
    struct inode* (*lookupn)(struct inode* dir, const char* name); //lookup an inode in a dir
    struct inode* (*lookupi)(struct superblock* fs, ino_t ino); //lookup an inode using an inode number

};

struct superblock {
    struct file_ops* fops;
};

struct file {
    struct inode* file;
    uint32_t offset : 24;
    uint8_t flags : 8;
};


