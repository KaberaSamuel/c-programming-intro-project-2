#ifndef DEVICE_H
#define DEVICE_H

#define DEVICE_NAME_SIZE 40

enum DeviceType {
    TYPE_TEMPERATURE,
    TYPE_PRESSURE,
    TYPE_BATTERY,
    DEVICE_TYPE_COUNT
};

enum DeviceStatus {
    STATUS_NORMAL,
    STATUS_ALERT
};

enum RegisterResult {
    REGISTER_OK,
    REGISTER_DUPLICATE_ID,
    REGISTER_OUT_OF_MEMORY
};

struct Device {
    int id;
    char name[DEVICE_NAME_SIZE];
    int type;
    union {
        float temperature;
        int pressure;
        float voltage;
    } reading;
    int status;
    int has_reading;
    int is_pending;
    float previous_value;
    int has_previous_value;
};

struct DeviceList {
    struct Device *devices;
    int count;
};

const char *get_type_name(int type);
const char *get_unit(int type);
float get_reading_as_float(const struct Device *device);
void set_reading_from_float(struct Device *device, float value);
void format_reading(const struct Device *device, char *text, int text_size);

void init_device_list(struct DeviceList *list);
struct Device *find_device_by_id(const struct DeviceList *list, int id);
int register_device(struct DeviceList *list, int id, const char *name, int type);
int remove_device(struct DeviceList *list, int id);
void display_devices(const struct DeviceList *list);
void free_device_list(struct DeviceList *list);

#endif
