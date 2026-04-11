#include <stddef.h>
#include <stdint.h>

typedef uint8_t dev_t;

#define MKDEV(MAJOR, MINOR) ((MAJOR << 4) | MINOR)
#define MAJOR(DEVNO) (DEVNO >> 4)
#define MINOR(DEVNO) (DEVNO & 0xF)

struct io_request {
    void* buffer;
    int offset;
    int count;
    uint8_t op; //operation: read = 0, write = 1
    uint8_t status; //status: pending = 0, done = 1
};

struct device;

struct device_driver {
    struct device* (*create)(int8_t* minor, void* descriptor);
    int (*destroy)(struct device* dev);
    int (*ioctl)(struct device* dev, int op, void* arg);
    int (*readb)(struct device* dev);
    int (*writeb)(struct device* dev, char val);
    void (*update)();
};

#define DEVICE_IOQUEUE_LEN 8
struct device {
    struct device_driver* driver;
    struct io_request* io_queue[DEVICE_IOQUEUE_LEN];
    uint8_t io_queue_head;
    uint8_t io_queue_tail;
};

#define DRIVER_TABLE_LEN 16
extern struct device_driver* driver_table[DRIVER_TABLE_LEN];


