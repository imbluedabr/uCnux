#include <stdint.h>

#define VECTOR_TABLE_SIZE 96
extern uint32_t vector_table[VECTOR_TABLE_SIZE];
extern uint32_t __Vectors[VECTOR_TABLE_SIZE];


//initialize the ram interrupt vector table
void interrupt_init();

int register_interrupt(int ivec, void* handler_struct, void (*handler)(void));

void set_interrupt_priority(int ivec, int priority);

int get_current_interrupt();

void* get_current_handler_struct();

