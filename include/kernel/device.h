#pragma once
#include <stddef.h>
#include <stdint.h>
#include <uapi/sys/types.h>

struct file;
struct device;
struct bus_device;

enum bus_types : uint8_t {
    BUS_NONE,
    BUS_MMIO,
    BUS_I2C,
    BUS_SPI,
    BUS_UART
};

//bus device descriptors, contains info about the location and type of the device on the bus
struct mmio_bus_desc {
    void* base;
    uint16_t vendor_id;
    uint8_t major;
    uint8_t irq;
};

struct i2c_bus_desc {
    uint8_t major;
    uint8_t address;
};

struct device_driver {
    struct device* (*probe)(struct bus_device* parent, const void* descriptor);
    int (*remove)(struct device* dev);
    const char* name;
    struct device* instances; //linked list of device instances
    uint8_t instance_count;
    enum bus_types bus_accept; //bus type this driver attaches to
    enum bus_types bus_type; //bus type of this driver implements(if any)
};

struct dev_ops {
    ssize_t (*read)(struct file* f, void* buff, size_t count);
    ssize_t (*write)(struct file* f, const void* buff, size_t count);
    off_t (*lseek)(struct file* f, off_t offset, int whence);
    int (*ioctl)(struct file* f, int cmd, void* arg);
    int (*open)(struct device* dev, struct file* f);
    int (*release)(struct device* dev, struct file* f);
};

struct device {
    struct device_driver* driver;
    struct dev_ops* ops;
    struct device* next; //next device in the linked list of devices
    struct bus_device* parent;   
    uint8_t minor; //minor number of this instance
};

#define BUS_MAX_DRIVERS 16
struct bus_driver {
    struct device_driver base;
    uint8_t registered_drivers[BUS_MAX_DRIVERS];
    int registered_drivers_count;
};

struct bus_ops {
    int (*enumerate)(struct bus_device* bus);
    struct device* (*probe)(struct bus_device* bus, const void* desc);

    //i2c
    int (*i2c_tx)(struct bus_device* bus, uint8_t addr, uint8_t* buff, int count);
    int (*i2c_rx)(struct bus_device* bus, uint8_t addr, uint8_t* buff, int count);

    //uart
    int (*uart_tx)(struct bus_device* bus, uint8_t* buff);
    int (*uart_rx)(struct bus_device* bus, uint8_t* buff);
    
    //gpio
    int (*gpio_mode_set)(struct bus_device* bus, int mask);
    int (*gpio_mode_clr)(struct bus_device* bus, int mask);
    int (*gpio_port_set)(struct bus_device* bus, int mask);
    int (*gpio_port_clr)(struct bus_device* bus, int mask);
    int (*gpio_port_get)(struct bus_device* bus, int* buff);
};

struct bus_device {
    struct device base;
    struct bus_ops* bus_ops;
};

#define DRIVER_TABLE_LEN 16
extern struct device_driver* driver_table[DRIVER_TABLE_LEN];

void device_init();
int register_driver(uint8_t major, struct device_driver* drv);
int bus_add_driver(struct bus_driver* drv, uint8_t major);
struct device* device_probe(uint8_t major, struct bus_device* parent, const void* desc);
struct device* device_lookup(dev_t devno);

