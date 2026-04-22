#include <kernel/syscall.h>
#include <kernel/interrupt.h>
#include <kernel/proc.h>
#include <board/board.h>
#include <arch/armv8-m/proc.h>
#include <arch/armv8-m/syscall.h>
#include <uapi/syscalls.h>
#include <lib/kprint.h>
#include <stddef.h>

typedef void (*syscall_t)(struct exception_frame*);

syscall_t table[SYSCALL_COUNT] = {

};

/*  Syscalls:
 *      so syscalls have to be preemptible but i dont want to deal with nested interrupt preemption,
 *      so i modify the pc in the stack frame to point to the syscall function and switch to privileged mode.
 *      this way the process will basicly switch to kernel mode and execute the syscall code which now can be preempted since its running in thread mode.
 *      now when the syscall returns it calls svc which returns from a syscall and returns to the user process.
 *      and then we only need one kernel worker for pending functions like driver update functions or other small stuff.
*/

void syscall_init()
{
    register_interrupt(SVCall_IRQn, NULL, &svc_handler);
    NVIC_SetPriority(SVCall_IRQn, 7);
}


void syscall_thread_main()
{
    struct context_frame* frame = (struct context_frame*) current_process->save_psp;
    uint8_t svcno = *((uint8_t*) frame->base_frame.pc - 2);
    
    uint8_t devno = frame->base_frame.caller_regs[0];
    char* data = (char*) frame->base_frame.caller_regs[1];
    int size = frame->base_frame.caller_regs[2];
    
    struct device* dev = device_lookup(devno);
    struct io_request req = {
        .buffer = data,
        .offset = 0,
        .op = 1,
        .status = 0,
        .count = size,
        .waiter = current_process->pid
    };
    
    device_request_io(dev, &req);
    
    proc_block();
    
}





