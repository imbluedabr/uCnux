#include <kernel/alloc.h>
#include <stddef.h>

static char* block_base;
struct block block_array[BLOCK_ARRAY_LEN];

uint8_t unused_blocks[BLOCK_ARRAY_LEN];
uint8_t unused_blocks_top;

struct block* get_block()
{
    if (unused_blocks_top == 0) {
        return NULL;
    }
    return &block_array[unused_blocks[--unused_blocks_top]];
}

int get_block_idx(struct block* b)
{
    return b - block_array;
}

struct block* get_block_addr(int idx)
{
    return &block_array[idx];
}

void put_block(struct block* b)
{
    uint8_t idx = b - block_array;
    unused_blocks[unused_blocks_top++] = idx;
}

void init_heap(void* base, int size)
{
    block_base = base;
    block_array[0].size = size;
    block_array[0].occupied = false;
    block_array[0].next = BLOCK_NIL;
    block_array[0].prev = BLOCK_NIL;
    unused_blocks_top = 0;
    for (int i = 1; i < BLOCK_ARRAY_LEN; i++) {
        unused_blocks[unused_blocks_top++] = i;
    }
}

void* kmalloc(int size)
{
    struct block* current = get_block_addr(0);
    char* base = block_base;
    do {
        if (current->size > size && !current->occupied) {
            struct block* new = get_block();
            if (new == NULL) {
                return NULL;
            }

            new->prev = get_block_idx(current);
            new->next = current->next;
            if (current->next != BLOCK_NIL) {
                struct block* new_next = get_block_addr(current->next);
                new_next->prev = get_block_idx(new);
            }
            new->occupied = false;
            new->size = current->size - size;
            current->size = size;
            current->occupied = true;
            current->next = get_block_idx(new);
            return base;
        }
        if (current->size == size && !current->occupied) {
            current->occupied = true;
            return base;
        }
        if (current->next == BLOCK_NIL) {
            return NULL;
        }
        base += current->size;
        current = get_block_addr(current->next);
    } while(1);
}

void try_coalesce(struct block* b)
{
    if (b->next != BLOCK_NIL) {
        struct block* next = get_block_addr(b->next);

        if (next->occupied == false) {
            b->size += next->size;
            b->next = next->next;
            if (b->next != BLOCK_NIL) {
                struct block* new_next = get_block_addr(b->next);
                new_next->prev = get_block_idx(b);
            }
            put_block(next);
        }
    }

    if (b->prev != BLOCK_NIL) {
        struct block* prev = get_block_addr(b->prev);

        if (prev->occupied == false) {
            prev->next = b->next;
            prev->size += b->size;
            if (b->next != BLOCK_NIL) {
                struct block* new_next = get_block_addr(b->next);
                new_next->prev = b->prev;
            }
            put_block(b);
        }
    }

}

void kfree(void* ptr)
{
    struct block* current = get_block_addr(0);
    char* base = block_base;
    do {
        
        if (base == ptr) {
            current->occupied = false;
            try_coalesce(current);
            return;
        }

        if (current->next == BLOCK_NIL) {
            return;
        }
        base += current->size;
        current = get_block_addr(current->next);
    } while(1);
}

