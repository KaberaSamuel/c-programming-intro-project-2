#include "device.h"
#include "colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *get_type_name(int type)
{
    switch (type) {
    case TYPE_TEMPERATURE:
        return "Temperature";
    case TYPE_PRESSURE:
        return "Pressure";
    case TYPE_BATTERY:
        return "Battery";
    default:
        return "Unknown";
    }
}

const char *get_unit(int type)
{
    switch (type) {
    case TYPE_TEMPERATURE:
        return "C";
    case TYPE_PRESSURE:
        return "hPa";
    case TYPE_BATTERY:
        return "V";
    default:
        return "";
    }
}

/* The device type decides which union member is valid. */
float get_reading_as_float(const struct Device *device)
{
    switch (device->type) {
    case TYPE_TEMPERATURE:
        return device->reading.temperature;
    case TYPE_PRESSURE:
        return (float)device->reading.pressure;
    case TYPE_BATTERY:
        return device->reading.voltage;
    default:
        return 0.0f;
    }
}

void set_reading_from_float(struct Device *device, float value)
{
    switch (device->type) {
    case TYPE_TEMPERATURE:
        device->reading.temperature = value;
        break;
    case TYPE_PRESSURE:
        device->reading.pressure = (int)value;
        break;
    case TYPE_BATTERY:
        device->reading.voltage = value;
        break;
    }
}

void format_reading(const struct Device *device, char *text, int text_size)
{
    if (!device->has_reading) {
        snprintf(text, text_size, "no reading");
        return;
    }

    switch (device->type) {
    case TYPE_TEMPERATURE:
        snprintf(text, text_size, "%.1f C", device->reading.temperature);
        break;
    case TYPE_PRESSURE:
        snprintf(text, text_size, "%d hPa", device->reading.pressure);
        break;
    case TYPE_BATTERY:
        snprintf(text, text_size, "%.2f V", device->reading.voltage);
        break;
    default:
        snprintf(text, text_size, "unknown");
        break;
    }
}

void init_device_list(struct DeviceList *list)
{
    list->devices = NULL;
    list->count = 0;
}

struct Device *find_device_by_id(const struct DeviceList *list, int id)
{
    struct Device *device;
    struct Device *end = list->devices + list->count;

    for (device = list->devices; device < end; device++) {
        if (device->id == id) {
            return device;
        }
    }
    return NULL;
}

int register_device(struct DeviceList *list, int id, const char *name, int type)
{
    struct Device *bigger_array;
    struct Device *new_device;

    if (find_device_by_id(list, id) != NULL) {
        return REGISTER_DUPLICATE_ID;
    }

    /* Use a temporary pointer so the old array is kept if realloc fails. */
    bigger_array = realloc(list->devices, (list->count + 1) * sizeof(struct Device));
    if (bigger_array == NULL) {
        return REGISTER_OUT_OF_MEMORY;
    }
    list->devices = bigger_array;

    new_device = &list->devices[list->count];
    memset(new_device, 0, sizeof(struct Device));
    new_device->id = id;
    strncpy(new_device->name, name, DEVICE_NAME_SIZE - 1);
    new_device->type = type;
    new_device->status = STATUS_NORMAL;

    list->count++;
    return REGISTER_OK;
}

/* Returns 1 if the device was removed, 0 if the ID was not found. */
int remove_device(struct DeviceList *list, int id)
{
    struct Device *device_to_remove = find_device_by_id(list, id);
    struct Device *current;
    struct Device *last;
    struct Device *smaller_array;

    if (device_to_remove == NULL) {
        return 0;
    }

    last = list->devices + list->count - 1;
    for (current = device_to_remove; current < last; current++) {
        *current = *(current + 1);
    }
    list->count--;

    if (list->count == 0) {
        free(list->devices);
        list->devices = NULL;
        return 1;
    }

    smaller_array = realloc(list->devices, list->count * sizeof(struct Device));
    if (smaller_array != NULL) {
        list->devices = smaller_array;
    }
    return 1;
}

void display_devices(const struct DeviceList *list)
{
    const struct Device *device;
    const struct Device *end = list->devices + list->count;
    char reading_text[32];

    if (list->count == 0) {
        printf(ANSI_ERROR "No devices registered.\n" ANSI_RESET);
        return;
    }

    printf("\n" ANSI_HEADER "%-5s %-20s %-12s %-12s %-8s" ANSI_RESET "\n",
           "ID", "Name", "Type", "Reading", "Status");
    for (device = list->devices; device < end; device++) {
        int is_alert = device->status == STATUS_ALERT;

        format_reading(device, reading_text, sizeof reading_text);
        printf("%-5d %-20s %-12s %-12s %s%s" ANSI_RESET "\n",
               device->id,
               device->name,
               get_type_name(device->type),
               reading_text,
               is_alert ? ANSI_ALERT_STATUS : ANSI_NORMAL_STATUS,
               is_alert ? "ALERT" : "Normal");
    }
    printf(ANSI_SUCCESS "Total devices: %d\n" ANSI_RESET, list->count);
}

void free_device_list(struct DeviceList *list)
{
    free(list->devices);
    list->devices = NULL;
    list->count = 0;
}
