#pragma once
#include <stddef.h>
#include <stdint.h>
#include <uapi/sys/types.h>
#include <kernel/proc.h>

//maximum of io_request's that can be allocated at one time
#define IO_REQUEST_MAX_MEMUSAGE 8

struct io_request {
    void* buffer;
    uint32_t offset : 24;
    uint8_t op      : 1; //op: read = 0, write = 1
    uint8_t status  : 1; //status: pending = 0, done = 1
    uint8_t res0    : 6; //reserved
    uint32_t count  : 24;
    pid_t waiter    : 8; //process waiting for this to complete
    struct io_request* next_free; //this is used to keep a free list of io_request objects to speed up allocation
};

struct io_request* io_request_create();
void io_request_destroy(struct io_request* req);

struct device;

struct device_driver {
    struct device* (*create)(uint8_t* minor, void* descriptor);
    int (*destroy)(struct device* dev);
    int (*ioctl)(struct device* dev, int op, void* arg);
    int (*readb)(struct device* dev);
    int (*writeb)(struct device* dev, char val);
    void (*update)(struct device* dev);
    struct device* instances; //linked list of device instances
    uint8_t instance_count;
};

#define DEVICE_IOQUEUE_LEN 8
#define DEVICE_IOQUEUE_MSK (DEVICE_IOQUEUE_LEN - 1)
struct device {
    struct device_driver* driver;
    struct device* next; //next device in the linked list of devices
    struct io_request* io_queue[DEVICE_IOQUEUE_LEN];
    uint8_t io_queue_head;
    uint8_t io_queue_tail;
    uint8_t impl; //for selecting backends
    uint8_t flags; //driver specific flags
};

#define DRIVER_TABLE_LEN 16
extern struct device_driver* driver_table[DRIVER_TABLE_LEN];

void device_init();
struct device* device_create(dev_t* devno, uint8_t major, void* desc);
int device_request_io(struct device* dev, struct io_request* req); //enqueue an io request in the io queue of a device
struct io_request* device_dequeue_request(struct device* dev); //dequeue the top request from the queue
struct io_request* device_peek_request(struct device* dev); //get the top request without removing it from the queue
struct device* device_lookup(dev_t devno);
void device_global_update();


