#pragma once
#include <stdint.h>

typedef struct dt_node {
    const void* desc;
    const struct dt_node* child;
    const struct dt_node* next;
} dt_node_t;

void devtbl_init(const dt_node_t* device_tree);

