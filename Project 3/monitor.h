#ifndef MONITOR_H
#define MONITOR_H

#include "device.h"

typedef void (*MonitorCallback)(struct Device *);

void seed_random_generator(void);
void generate_all_readings(struct DeviceList *list);
void process_device(struct Device *device, MonitorCallback callback);
void process_all_readings(struct DeviceList *list);
void display_alerts(void);
void display_summary(const struct DeviceList *list);
void free_monitor(void);

#endif
