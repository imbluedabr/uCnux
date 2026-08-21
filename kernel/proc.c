#include <kernel/proc.h>
#include <kernel/interrupt.h>
#include <kernel/time.h>
#include <kernel/device.h>
#include <kernel/panic.h>
#include <kernel/page.h>
#include <lib/stdlib.h>
#include <lib/kmalloc.h>
#include <lib/kprint.h>
#include <fs/vfs.h>
#include <uapi/sys/errno.h>
#include <uapi/signal.h>
#include <stddef.h>

mutex_t proc_acces_lock;
struct proc* proc_free_list;
struct proc* proc_active_list;

uint8_t proc_pid_free_list[PROC_MAX_PROC];
uint8_t proc_pid_free_list_top;

struct proc* proc_run_queue[PROC_MAX_PROC];
uint8_t proc_run_queue_head;
uint8_t proc_run_queue_tail;
uint8_t proc_run_queue_count;

struct proc* current_process;
bool proc_sched_started;

[[gnu::aligned(8)]] stack_t stack_array[PROC_MAX_PROC];
uint8_t stack_free_list[PROC_MAX_PROC];
uint8_t stack_free_list_top;

static int proc_send_sig(pid_t pid, int sig);

inline stack_t* proc_stack_alloc()
{
    if (stack_free_list_top == 0) return NULL;
    return &stack_array[stack_free_list[--stack_free_list_top]];
}

inline void proc_stack_free(stack_t* stack)
{
    if (stack_free_list_top > 4) thread_panic("stack memory leak!");
    int index = stack - stack_array;
    stack_free_list[stack_free_list_top++] = index; 
}

inline pid_t proc_pid_alloc()
{
    if (proc_pid_free_list_top == 0) return 255;
    return proc_pid_free_list[--proc_pid_free_list_top];
}

inline void proc_pid_free(pid_t pid)
{
    if (proc_pid_free_list_top > PROC_MAX_PROC) thread_panic("bruh...");
    proc_pid_free_list[proc_pid_free_list_top++] = pid;
}

inline int proc_enqueue(struct proc* process)
{
    if (proc_run_queue_count == PROC_MAX_PROC) return -1;
    
    proc_run_queue[proc_run_queue_head] = process;
    proc_run_queue_head = (proc_run_queue_head + 1) & (PROC_MAX_PROC - 1);
    proc_run_queue_count++;
    
    return 0;
}

inline struct proc* proc_dequeue()
{
    if (proc_run_queue_count == 0) return NULL;
    
    struct proc* process = proc_run_queue[proc_run_queue_tail];
    proc_run_queue_tail = (proc_run_queue_tail + 1) & (PROC_MAX_PROC - 1);
    proc_run_queue_count--;
    
    return process;
}

inline int proc_fd_alloc(struct proc* p)
{
    for (int i = 0; i < PROC_MAXFILES; i++) {
        if (p->local_fd_table[i] == 255) {
            return i;
        }
    }
    return -EMFILE;
}

inline void proc_fd_free(struct proc* p, int fd)
{
    p->local_fd_table[fd] = 255;
}

inline struct file* proc_fd_get(struct proc* p, int fd)
{
    if (fd == 255) return NULL;
    if (p->local_fd_table[fd] == 255) return NULL;
    struct file* f = &vfs_file_table[p->local_fd_table[fd]];
    if (f->i == NULL) return NULL;
    return f;
}

inline int proc_fd_add(struct proc* p, struct file* f)
{
    int fd = proc_fd_alloc(p);
    if (fd < 0) return fd;
    p->local_fd_table[fd] = f - vfs_file_table;
    return fd;
}

inline int proc_fd_set(struct proc* p, int fd, struct file* f)
{
    int idx = f - vfs_file_table;
    p->local_fd_table[fd] = idx;
    return idx;
}


inline struct proc* proc_get_process(pid_t pid)
{
    mutex_lock(&proc_acces_lock);
    struct proc* current = proc_active_list;
    while(current) {
        if (current->pid == pid) {
            break;
        }
        current = current->next;
    }
    mutex_unlock(&proc_acces_lock);
    return current;
}



inline void proc_free_process(struct proc* p)
{
    mutex_lock(&proc_acces_lock);
    
    struct proc* current = proc_active_list;
    if (current == p) {
        proc_active_list = p->next;
        p->next = proc_free_list;
        proc_free_list = p;
        mutex_unlock(&proc_acces_lock);
        return;
    }

    while(current->next) {
        if (current->next == p) {
            current->next = p->next;
            p->next = proc_free_list;
            proc_free_list = p;
            break;
        }
        current = current->next;
    }

    mutex_unlock(&proc_acces_lock);
}

inline struct proc* proc_alloc_process()
{
    mutex_lock(&proc_acces_lock);
    struct proc* p;
    if (proc_free_list) {
        p = proc_free_list;
        proc_free_list = p->next;
    } else {
        p = kzalloc(sizeof(struct proc));
        if (!p) thread_panic("could not allocate process!");
    }

    p->next = proc_active_list;
    proc_active_list = p;

    mutex_unlock(&proc_acces_lock);
    return p;
}

void proc_init()
{
    proc_free_list = NULL;
    proc_active_list = NULL;
    
    proc_pid_free_list_top = 0;
    for (int i = 0; i < PROC_MAX_PROC; i++) {
        proc_pid_free_list[proc_pid_free_list_top++] = PROC_MAX_PROC - i - 1;
    }

    stack_free_list_top = 0;
    for (int i = 0; i < PROC_MAX_PROC; i++) {
        stack_free_list[stack_free_list_top++] = i;
    }

    proc_run_queue_head = 0;
    proc_run_queue_tail = 0;
    proc_run_queue_count = 0;
    current_process = NULL;
    proc_sched_started = false;
    
    proc_sched_init();
}

int proc_reap(struct proc* p)
{
    if (p->state != PROC_ZOMBIE) return -255;
    inode_free(p->cwd);
    for (int i = 0; i < PROC_MAXFILES; i++) {
        vfs_close(i);
    }
    
    if (p->kernel_mode) {
        proc_stack_free((stack_t*) p->splim);
    } else {
        proc_stack_free((stack_t*) p->save_splim);
    }
    
    if (p->program_base) {
        page_free(p->program_base);
    }
    int exit_code = p->exit_code;
    proc_pid_free(p->pid);
    proc_free_process(p);

    return exit_code;
}

void proc_mark_zombie(struct proc* p, int exit_code)
{
    proc_stop_scheduling();
    if (p->waiting_on) {
        waiter_remove(p->waiting_on, p);
    }

    p->exit_code = exit_code;
    p->state = PROC_ZOMBIE;

    //reparent all the children to pid 1
    struct proc* curr = proc_active_list;
    while(curr) {
        if (curr->ppid == p->pid) {
            curr->ppid = 1;
        }
        curr = curr->next;
    }
    proc_send_sig(p->ppid, SIGCHLD);
}


void proc_kill_process(struct proc* p, int exit_code) {
    if (p->state == PROC_BLOCKED || p->kernel_mode == 0) {
        proc_mark_zombie(p, exit_code);
    } else {
        p->kill_pending = 1;
        p->exit_code = exit_code;
    }
}

static int proc_send_sig(pid_t pid, int sig)
{
    //kdbg("sig: %d to %d from %d\n", sig, pid, current_process->pid);
    if (sig > 31) return -EINVAL;
    if (sig < 0) return -EINVAL;
    
    
    struct proc* p = proc_get_process(pid);
    if (!p) return -ESRCH;
    
    int action = 0;
    if (p->sigmask & (1 << sig)) {
        p->exit_code = sig;
        switch(sig) {
            case SIGCHLD: //sigchld wakes up the parent of a child process
            case SIGCONT:
                if (p->state == PROC_BLOCKED) {
                    action = 1;
                }
                break;
            case SIGSTOP:
            case SIGTSTP:
            case SIGTTIN:
            case SIGTTOU:
                if (p->state == PROC_READY) {
                    action = 2;
                }
                break;
            default:
                if (p->state != PROC_ZOMBIE) {
                    action = 3;
                }
        }

    }
    
    if (action == 1) {
        proc_unblock_process(p);
    } else if (action == 2) {
        proc_block(p);
    } else if (action == 3) {
        proc_kill_process(p, 128 + sig);
    } else {
        proc_schedule();
    }

    return 0;
}

int proc_kill(pid_t pid, int sig)
{
    mutex_lock(&proc_acces_lock);

    uint8_t proc_group[PROC_MAX_PROC];
    int proc_count = 0;
    int status = -ESRCH;

    if (pid < 0) {
        struct proc* p = proc_active_list;
        while(p) {
            if (p->pgrp == -pid) {
                proc_group[proc_count++] = p->pid;
            }
            p = p->next;
        }
    } else {
        proc_group[proc_count++] = pid;
    }

    for (int i = 0; i < proc_count; i++) {
        status = proc_send_sig(proc_group[i], sig);
        if (i == 0 && status < 0) goto exit;
    }

exit:
    mutex_unlock(&proc_acces_lock);
    return status;
}



