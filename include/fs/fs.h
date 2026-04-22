#pragma once

#include <uapi/vfs.h>
#include <kernel/device.h>
#include <stdint.h>

struct superblock;
struct file;
struct dentry;

#define FS_GET_INO_OFF(ID) (ID << 16)

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
    uint8_t mode; //currently only stores filetype, but i will later expand it will actual permissions and ownership
    ino_t dir; //the parent directory of this inode
    ino_t file; //the inum of this inode
    uint32_t size;
    union {
        struct devfs_inode devfs;
        struct fatfs_inode fatfs;
    };
};

#define INODE_TABLE_SIZE 32
extern struct inode inode_table[INODE_TABLE_SIZE];
extern uint8_t inode_free_list_size;

//file operations
struct file_ops {
    int8_t (*read)(struct file* f, char* buff, int count);
    int8_t (*write)(struct file* f, const char* buff, int count);
    int (*fstat)(struct file* f, struct stat* statbuff); //these are instant (i hope)
    int (*lseek)(struct file* f, uint32_t offset, int whence);

    int (*mount)(struct inode* mountpoint, dev_t devno, const char* args);
    int (*umount)(struct superblock* fs);
    
    int (*mkdir)();
    int (*rmdir)();
    int (*unlink)();

    struct inode* (*lookupn)(struct inode* dir, const char* name); //lookup an inode in a dir
    struct inode* (*lookupi)(struct inode* dir, ino_t ino); //lookup an inode using an inode number

};

struct superblock {
    struct file_ops* fops;
};

struct file {
    struct inode* file;
    uint32_t offset : 24;
    uint8_t flags : 8;
};


