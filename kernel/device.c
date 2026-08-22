#include <kernel/device.h>
#include <uapi/sys/errno.h>

struct device_driver* driver_table[DRIVER_TABLE_LEN];

void device_init()
{
    for (int i = 0; i < DRIVER_TABLE_LEN; i++) {
        driver_table[i] = NULL;
    }
}

int bus_add_driver(struct bus_driver* drv, uint8_t major)
{
    if (drv->registered_drivers_count > BUS_MAX_DRIVERS) return -EAGAIN;
    drv->registered_drivers[drv->registered_drivers_count++] = major;
    return 0;
}

int register_driver(uint8_t major, struct device_driver* drv)
{
    driver_table[major] = drv;
    for (int i = 0; i < DRIVER_TABLE_LEN; i++) {
        struct device_driver* bus = driver_table[i];
        if (bus) {
            if (bus->bus_type == drv->bus_accept) {
                if (bus_add_driver((struct bus_driver*) bus, major) < 0)
                    return -EAGAIN;
            }
        }
    }
    return 0;
}

//TODO: add spinlock to probe 
struct device* device_probe(uint8_t major, struct bus_device* parent, const void* desc)
{
    struct device_driver* drv = driver_table[major];
    if (!drv) return NULL;

    struct device* dev = drv->probe(parent, desc);
    if (!dev) return NULL;
    dev->minor = drv->instance_count++;
    dev->next = drv->instances;
    drv->instances = dev;
    return dev;
}

struct device* device_lookup(dev_t devno)
{
    struct device_driver* drv = driver_table[MAJOR(devno)];
    for (struct device* dev = drv->instances; dev; dev = dev->next) {
        if (dev->minor == MINOR(devno)) {
            return dev;
        }
    }
    return NULL;
}


