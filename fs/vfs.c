#include <fs/vfs.h>
#include <uapi/sys/errno.h>
#include <uapi/sys/fcntl.h>
#include <uapi/unistd.h>
#include <fs/devfs.h>
#include <fs/ucfs.h>
#include <kernel/interrupt.h>
#include <kernel/lock.h>
#include <kernel/proc.h>
#include <kernel/panic.h>
#include <lib/stdlib.h>
#include <lib/kmalloc.h>
#include <lib/kprint.h>

static struct inode* free_list;
static struct inode* cache_list;
static uint8_t cache_list_size;
static uint8_t free_list_size;

struct mount mount_table[VFS_MAXMOUNTS];

//you must hold this lock when you use an inode
mutex_t vfs_cache_lock;

#define CACHE_FLUSH_THRESHOLD 16

/* The inode cache works by first keeping a free list of free inodes. Then when an inode is allocated its removed from the free list and added to the cache list.
 * When an inodes refcount reaches 0 we keep it inside the cache list so its cached, then when the cache gets too big we flush it and only then will the inodes return to the original free list.
*/

struct inode* inode_alloc()
{
    mutex_lock(&vfs_cache_lock);
    struct inode* i;
    if (!free_list) {
        i = kmalloc(sizeof(struct inode));
    } else {
        i = free_list;
        free_list = i->next;
        free_list_size--;
    }
    if (!i) {
        mutex_unlock(&vfs_cache_lock);
        return NULL;
    }

    i->next = cache_list;
    cache_list = i;
    cache_list_size++;
    i->refcount = 1;

    mutex_unlock(&vfs_cache_lock);
    return i;
}

void inode_free(struct inode* i)
{
    if (!i) {
        return;
    }
    mutex_lock(&vfs_cache_lock);
    
    if (i->refcount > 1) {
        i->refcount--;
        mutex_unlock(&vfs_cache_lock);
        return;
    }
    
    if (cache_list_size > CACHE_FLUSH_THRESHOLD) {
        //TODO: implement eviction policy
    }
    mutex_unlock(&vfs_cache_lock);
}

void inode_stat()
{
    mutex_lock(&vfs_cache_lock);
    
    kdbg("vfs: cache_list_size=%d\n", cache_list_size);
    struct inode* curr = cache_list;
    while (curr) {
        kdbg("ino=%d, fsid=%d, perm=%o\n", FS_GET_INO(curr->ino), FS_GET_FSID(curr->ino), (uint32_t) curr->perm.mode);
        curr = curr->next;
    }
    
    mutex_unlock(&vfs_cache_lock);
}

#define DENTRY_CACHE_LEN 16
struct dentry dentry_cache[DENTRY_CACHE_LEN];
uint8_t dentry_cache_index;
mutex_t vfs_dentry_lock;

struct dentry* dentry_alloc()
{
    struct dentry* d = &dentry_cache[dentry_cache_index];
    dentry_cache_index = (dentry_cache_index + 1) & (DENTRY_CACHE_LEN-1);
    return d;
}

int vfs_get_fsid()
{
    static int fsid = 0;
    return fsid++;
}

const struct file_ops* fs_table[4] = {
    &ucfs_file_ops,
    &devfs_file_ops
};

const char* fs_name_table[4] = {
    "ucfs",
    "devfs"
};

struct file vfs_file_table[VFS_MAXFILES];
uint8_t file_table_free[VFS_MAXFILES];
uint8_t file_table_free_top;

mutex_t vfs_file_lock;

struct file* file_alloc()
{
    mutex_lock(&vfs_file_lock);
    if (file_table_free_top == 0) {
        return NULL;
    }
    struct file* f = &vfs_file_table[file_table_free[--file_table_free_top]];
    f->refcount = 1;
    mutex_unlock(&vfs_file_lock);
    return f;
}

void file_free(struct file* f)
{
    mutex_lock(&vfs_file_lock);
    if (f->refcount > 1) {
        f->refcount--;
        mutex_unlock(&vfs_file_lock);
        return;
    }
    f->refcount = 0;
    inode_free(f->i);
    
    if (file_table_free_top == VFS_MAXFILES)
    {   //very bad, means double free
        thread_panic("vfs: memory leaked in file_free()");
    }

    file_table_free[file_table_free_top++] = f - vfs_file_table;
    mutex_unlock(&vfs_file_lock);
}

struct file_ops* get_filesystem(const char* name)
{
    for (int i = 0; i < 4; i++) {
        if (strncmp(fs_name_table[i], name, 4) == 0) {
            return (struct file_ops*) fs_table[i];
        }
    }
    return NULL;
}

const char* get_last_name(const char* path)
{
    if (*path == '/') path++;
    const char* word = path;
    while (*path) {
        if (*path++ == '/') {
            word = path;
        }
    }
    return word;
}


void vfs_init()
{
    free_list = NULL;
    cache_list = NULL;
    dentry_cache_index = 0;
    for (int i = 0; i < VFS_MAXFILES; i++) {
        file_table_free[file_table_free_top++] = i;
    }
}

//lookup an inode using a name and parent directory inode
static inline ino_t vfs_read_dentry(struct inode* dir, const char* name)
{
    if (strncmp(name, ".", FS_INAME_LEN) == 0) {
        return dir->ino;
    }
    
    if (strncmp(name, "..", FS_INAME_LEN) == 0) {
        for (int i = 0; i < VFS_MAXMOUNTS; i++) {
            struct mount* m = &mount_table[i];
            if (m->root == dir) {
                if (m->mountpoint) {
                    dir = m->mountpoint;
                }
            }
        }
    }

    for (int i = 0; i < DENTRY_CACHE_LEN; i++) {
        struct dentry* d = &dentry_cache[i];

        if (strncmp(d->name, name, FS_INAME_LEN) == 0 && d->parent_ino == dir->ino) {
            return d->ino;
        }
    }
    ino_t new_ino = dir->fs->fops->lookup(dir, name);
    if (new_ino < 0) return new_ino;
    mutex_lock(&vfs_dentry_lock);
    struct dentry* d = dentry_alloc();
    d->ino = new_ino;
    d->parent_ino = dir->ino;
    strlcpy(d->name, name, FS_INAME_LEN);
    mutex_unlock(&vfs_dentry_lock);

    return new_ino;
}

static inline struct inode* vfs_read_inode(struct filesystem* fs, ino_t ino)
{
    struct inode* current = cache_list;
    mutex_lock(&vfs_cache_lock);

    while (current) {
        if (current->ino == ino) {
            current->refcount++;
            mutex_unlock(&vfs_cache_lock);
            return current;
        }
        current = current->next;
    }
    mutex_unlock(&vfs_cache_lock);
    
    return fs->fops->read_i(fs, ino);
}

static inline struct inode* check_mount(struct inode* node) {

    mutex_lock(&vfs_cache_lock);

    if (FS_GET_FTYPE(node->perm) == S_IFMNT) {
        for (int i = 0; i < VFS_MAXMOUNTS; i++) {
            struct mount* m = &mount_table[i];
            if (m->mountpoint == node) {
                struct inode* root = m->root;
                mutex_unlock(&vfs_cache_lock);
                inode_free(node);
                root->refcount++;
                return root;
            }
        }
        thread_panic("mount without mountpoint!\n");
    }
    mutex_unlock(&vfs_cache_lock);
    return node;
}

static struct inode* vfs_walk_rel(struct inode* base, const char* path, int mode) {
    int path_idx = 0;
    char path_cpy[FS_PATH_LEN+1];
    char* current_word = path_cpy;
    
    if (!base) {
        thread_panic("vfs: cwd not set!");
    }
    strlcpy(path_cpy + path_idx, path + path_idx, FS_PATH_LEN);

    while (path[path_idx]) {
        if (path[path_idx] == '/') {
            path_cpy[path_idx] = '\0';
            if (*current_word != '\0') {
                ino_t ino = vfs_read_dentry(base, current_word);
                if (ino < 0) goto error;
                struct inode* next = vfs_read_inode(base->fs, ino);
                if (!next) goto error;
                inode_free(base);
                base = check_mount(next);
            }
            current_word = path_cpy + path_idx + 1;
        }
        path_idx++;
    }

    if (mode != VFS_LOOKUP_BASE && *current_word) {
        ino_t ino = vfs_read_dentry(base, current_word);
        if (ino < 0) goto error;
        struct inode* next = vfs_read_inode(base->fs, ino);
        if (!next) goto error;
        inode_free(base);
        base = check_mount(next);
    }

    return base;
error:
    inode_free(base);
    return NULL;
}

struct inode* vfs_walk_path(const char* path, int mode)
{

    struct inode* current_dir;
    
    //absolute path
    if (*path == '/') {
        current_dir = mount_table[0].root;
        path++;
    } else {
        mutex_lock(&vfs_cache_lock);
        current_dir = current_process->cwd;
        current_process->cwd->refcount++;
        mutex_unlock(&vfs_cache_lock);
    }
    return vfs_walk_rel(current_dir, path, mode);
}

int vfs_mount_root(dev_t devno, const char* filesystemtype, int mountflags)
{
    struct file_ops* fops = get_filesystem(filesystemtype);
    if (!fops) return -ENODEV;

    int status = fops->mount(&mount_table[0], devno, mountflags);
    return status;
}

int vfs_mount_dev(const char* path, const char* filesystemtype, int mountflags)
{
    struct file_ops* fops = get_filesystem(filesystemtype);
    if (!fops) return -ENODEV;

    struct inode* node = vfs_walk_path(path, 0);
    if (!node) return -ENOENT;

    if (!(node->perm.mode & S_IFDIR)) {
        inode_free(node);
        return -ENOTDIR;
    }

    for (int i = 0; i < VFS_MAXMOUNTS; i++) {
        struct mount* m = &mount_table[i];
        if (m->root == NULL) {
            int status = fops->mount(m, 0, mountflags);
            if (status < 0) {
                inode_free(node);
                return status;
            }
            m->mountpoint = node;
            FS_SET_FTYPE(node->perm, S_IFMNT);
            return 0;
        }
    }
    inode_free(node);
    return -ENOMEM;
}

int check_access(cred_t credentials, struct permissions* perm, int mode)
{
    return 1;
}

ssize_t vfs_read(int fd, void* buffer, size_t count)
{
    ssize_t n;
    struct file* f = proc_fd_get(current_process, fd);
    if (!f || FS_GET_FDMODE(f->flags) != O_WRONLY) {
        return -EBADF;
    }

    if (f->i->perm.mode & S_IFDIR) {
        return -EISDIR;
    }

    n = f->i->fs->fops->read(f, buffer, count);

    mutex_lock(&vfs_file_lock);
    if (n > 0) f->offset += n;
    mutex_unlock(&vfs_file_lock);
    return n;
}

ssize_t vfs_write(int fd, const void* buffer, size_t count)
{
    struct file* f = proc_fd_get(current_process, fd);
    if (!f || FS_GET_FDMODE(f->flags) == O_RDONLY) {
        return -EBADF;
    }

    if (f->i->perm.mode & S_IFDIR) {
        return -EISDIR;
    }

    ssize_t n = f->i->fs->fops->write(f, buffer, count);
    mutex_lock(&vfs_file_lock);
    if (n > 0) f->offset += n;
    mutex_unlock(&vfs_file_lock);
    return n;
}

int vfs_open(const char* path, int flags)
{
    struct inode* i = vfs_walk_path(path, 0);
    if (!i) {
        return -ENOENT;
    }

    struct file* f = file_alloc();
    if (!f) {
        return -ENFILE;
    }

    f->flags = flags;
    f->i = i;
    f->offset = 0;
    int fd = proc_fd_add(current_process, f);

    return fd;
}

int vfs_close(int fd)
{
    struct file* f = proc_fd_get(current_process, fd);
    if (!f) return -EBADF;
    file_free(f);
    proc_fd_free(current_process, fd); 
    return 0;
}

int vfs_fstat(int fd, struct stat *statbuf)
{
    struct file* f = proc_fd_get(current_process, fd);
    if (!f) return -EBADF;
    
    return f->i->fs->fops->fstat(f, statbuf);
}

off_t vfs_lseek(int fd, off_t offset, int whence)
{
    struct file* f = proc_fd_get(current_process, fd);
    if (!f) return -EBADF;
    
    mutex_lock(&vfs_file_lock);
    off_t result;
    if (f->i->fs->fops->lseek) {
        result = f->i->fs->fops->lseek(f, offset, whence);
    } else {
        if (whence == SEEK_SET) {
            f->offset = offset;
        } else if (whence == SEEK_CUR) {
            f->offset += offset;
        } else if (whence == SEEK_END) {
            f->offset = f->i->size;
        }
        result = f->offset;
    }
    mutex_unlock(&vfs_file_lock);
    return result;
}

int vfs_ioctl(int fd, int cmd, void* arg)
{
    struct file* f = proc_fd_get(current_process, fd);
    if (!f) return -EBADF;

    struct inode* i = f->i;
    if (FS_GET_FTYPE(i->perm) != S_IFDEV) return -ENODEV;
    if (!i->devfs.dev->ops->ioctl) return -ENOTTY;
    return i->devfs.dev->ops->ioctl(f, cmd, arg);
}

int vfs_access(const char* path, int mode)
{
    return -ENOSYS;
}

int vfs_select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout)
{
    return -ENOSYS;
}

int vfs_fcntl(int fd, int op, int arg)
{
    struct file* f = proc_fd_get(current_process, fd);
    if (!f) return -EBADF;

    switch(op) {
        case F_DUPFD:
            mutex_lock(&vfs_file_lock);
            f->refcount++;
            mutex_unlock(&vfs_file_lock);
            return proc_fd_add(current_process, f);
        default:
            return -EINVAL;
    }
}

int vfs_ftruncate(int fd, off_t lenght)
{
    return 0;
}

int vfs_fchdir(int fd)
{
    struct file* newcwd = proc_fd_get(current_process, fd);
    if (FS_GET_FTYPE(newcwd->i->perm) != S_IFDIR) return -ENOTDIR;
    inode_free(current_process->cwd);
    mutex_lock(&vfs_cache_lock);
    newcwd->i->refcount++;
    current_process->cwd = newcwd->i;
    mutex_unlock(&vfs_cache_lock);
    return 0;
}

ssize_t vfs_readdir(int fd, struct dirent* buf, size_t count)
{
    struct file* f = proc_fd_get(current_process, fd);
    if (!f || !(f->flags & O_RDONLY)) {
        return -EBADF;
    }

    //check if it is a directory or nah
    if (!(f->i->perm.mode & S_IFDIR)) {
        return -ENOTDIR;
    }
    if (f->offset == 0 && count > 0) {
        buf[0].d_ino = f->i->ino;
        buf[0].d_namelen = 2;
        buf[0].d_offset = 0;
        strlcpy(buf[0].d_name, ".", FS_INAME_LEN);
        f->offset++;
        if (--count == 0) return 1;
    }

    ssize_t n = f->i->fs->fops->readdir(f, buf, count);
    mutex_lock(&vfs_file_lock);
    if (n > 0) f->offset += n;
    mutex_unlock(&vfs_file_lock);

    return n;
}

int vfs_openat(int dirfd, const char* pathname, int flags)
{
    struct file* dir = proc_fd_get(current_process, dirfd);
    if (!dir) return -EBADF;
    
    struct inode* i = vfs_walk_rel(dir->i, pathname, 0);
    if (!i) {
        return -ENOENT;
    }

    struct file* f = file_alloc();
    if (!f) {
        return -ENFILE;
    }

    f->flags = flags;
    f->i = i;
    f->offset = 0;
    int fd = proc_fd_add(current_process, f);

    return fd;
}

int vfs_mkdir(const char* path, mode_t mode)
{

}

int vfs_rmdir(const char* path)
{

}

int vfs_link(const char* oldpath, const char* newpath)
{

}

int vfs_unlink(const char* path)
{

}

int vfs_mknod(const char* path, mode_t mode, dev_t devno)
{
    struct inode* i = vfs_walk_path(path, VFS_LOOKUP_BASE);
    if (!i) return -ENOENT;
    

    int rt = i->fs->fops->mknod(i->fs, get_last_name(path), FS_MAKE_PERM(0, 0, mode), devno);
    inode_free(i);
    return rt;
}

int vfs_fstatfs(int fd, struct statfs* buf)
{
    struct file* f = proc_fd_get(current_process, fd);
    if (!f) return -EBADF;

    struct filesystem* fs = f->i->fs;
    
    buf->f_bsize = fs->block_size;
    buf->f_blocks = fs->block_count;
    buf->f_bfree = fs->block_count - fs->block_used;

    return 0;
}

int vfs_fchmod(int fd, mode_t mode)
{

}

int vfs_fchown(int fd, uid_t owner, gid_t group)
{

}

int vfs_sync()
{

}

int vfs_mount(const char* source, const char* target, const char* filesystemtype, int mountflags)
{

}

int vfs_umount(const char* target)
{

}



