#include <kernel/devtbl.h>
#include <kernel/device.h>
#include <lib/kprint.h>

void list_devices(int major)
{
    struct device_driver* drv = driver_table[major];
    if (!drv) return;

    struct device* current = drv->instances;
    for (int i = 0; i < drv->instance_count; i++) {
        if (!current)  {
            kerr("dev: %s: device list corrupted\n", drv->name);
            break;
        }
        kinfo("dev: discovered %s%d with devno (%d,%d)\n", drv->name, current->minor, major, current->minor);
        current = current->next;
    }
}

void walk_dt(struct bus_device* parent, const dt_node_t* node)
{
    struct device* dev;
    if (!parent) { //the first layer is always an 
        const struct mmio_desc* desc = node->desc;
        dev = device_probe(desc->major, NULL, desc);
    } else {
        dev = parent->bus_ops->probe(parent, node->desc);
    }
    if (!dev) return;

    for (const dt_node_t* child = node->child; child; child = child->next) {
        walk_dt((struct bus_device*) dev, child);
    }
}

void devtbl_init(const dt_node_t* device_tree)
{
    for (const dt_node_t* child = device_tree->child; child; child = child->next) {
        walk_dt(NULL, child);
    }

    for (int i = 0; i < DRIVER_TABLE_LEN; i++) {
        list_devices(i);
    }
}


