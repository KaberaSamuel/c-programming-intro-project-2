#include "monitor.h"
#include "colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Operating limits: a reading outside these is an alert. */
#define TEMPERATURE_MIN_LIMIT 10.0f
#define TEMPERATURE_MAX_LIMIT 80.0f
#define PRESSURE_MIN_LIMIT 950.0f
#define PRESSURE_MAX_LIMIT 1050.0f
#define BATTERY_MIN_LIMIT 3.3f
#define BATTERY_MAX_LIMIT 4.2f

/* Ranges used to generate readings. They are wider than the limits on purpose. */
#define TEMPERATURE_GENERATE_MIN 0.0f
#define TEMPERATURE_GENERATE_MAX 95.0f
#define PRESSURE_GENERATE_MIN 920
#define PRESSURE_GENERATE_MAX 1080
#define BATTERY_GENERATE_MIN 3.2f
#define BATTERY_GENERATE_MAX 4.3f

/* Sensor drift limits: the biggest change allowed between two readings. */
#define TEMPERATURE_DRIFT_LIMIT 60.0f
#define PRESSURE_DRIFT_LIMIT 110.0f
#define BATTERY_DRIFT_LIMIT 0.8f

#define ALERT_GROW_STEP 10
#define EXPECTED_TEXT_SIZE 60
#define REASON_TEXT_SIZE 80

struct Alert {
    int device_id;
    char device_name[DEVICE_NAME_SIZE];
    int device_type;
    char reading_text[32];
    char expected[EXPECTED_TEXT_SIZE];
    char reason[REASON_TEXT_SIZE];
    const char *callback_name;
};

static struct Alert *alert_log = NULL;
static int alert_count = 0;
static int alert_capacity = 0;

static int total_readings = 0;
static int normal_readings = 0;
static int abnormal_readings = 0;
static int alerts_per_type[DEVICE_TYPE_COUNT] = {0};

static int drift_checks = 0;
static int drift_alerts = 0;
static int drift_skipped = 0;

static void temperature_monitor(struct Device *device);
static void pressure_monitor(struct Device *device);
static void battery_monitor(struct Device *device);
static void drift_monitor(struct Device *device);

/* One callback per device type, picked by the device type. */
static const MonitorCallback type_callbacks[DEVICE_TYPE_COUNT] = {
    temperature_monitor,
    pressure_monitor,
    battery_monitor,
};

static const float drift_limits[DEVICE_TYPE_COUNT] = {
    TEMPERATURE_DRIFT_LIMIT,
    PRESSURE_DRIFT_LIMIT,
    BATTERY_DRIFT_LIMIT,
};

void seed_random_generator(void)
{
    srand((unsigned int)time(NULL));
}

static float random_float_between(float minimum, float maximum)
{
    float fraction = (float)rand() / (float)RAND_MAX;
    return minimum + fraction * (maximum - minimum);
}

static int random_int_between(int minimum, int maximum)
{
    return minimum + rand() % (maximum - minimum + 1);
}

static void generate_reading(struct Device *device)
{
    switch (device->type) {
    case TYPE_TEMPERATURE:
        device->reading.temperature =
            random_float_between(TEMPERATURE_GENERATE_MIN, TEMPERATURE_GENERATE_MAX);
        break;
    case TYPE_PRESSURE:
        device->reading.pressure =
            random_int_between(PRESSURE_GENERATE_MIN, PRESSURE_GENERATE_MAX);
        break;
    case TYPE_BATTERY:
        device->reading.voltage =
            random_float_between(BATTERY_GENERATE_MIN, BATTERY_GENERATE_MAX);
        break;
    default:
        return;
    }
    device->has_reading = 1;
    device->is_pending = 1;
}

void generate_all_readings(struct DeviceList *list)
{
    struct Device *device;
    struct Device *end = list->devices + list->count;
    char reading_text[32];

    if (list->count == 0) {
        printf(ANSI_ERROR "No devices registered. Register a device first.\n" ANSI_RESET);
        return;
    }

    for (device = list->devices; device < end; device++) {
        generate_reading(device);
        format_reading(device, reading_text, sizeof reading_text);
        printf(ANSI_INFO "Generated: ID %d (%s, %s) = %s\n" ANSI_RESET,
               device->id, device->name, get_type_name(device->type), reading_text);
    }
}

static void add_alert(const struct Device *device, const char *expected,
                      const char *reason, const char *callback_name)
{
    struct Alert *new_alert;

    if (alert_count == alert_capacity) {
        struct Alert *bigger_log =
            realloc(alert_log, (alert_capacity + ALERT_GROW_STEP) * sizeof(struct Alert));
        if (bigger_log == NULL) {
            printf(ANSI_ERROR "Out of memory: alert could not be saved.\n" ANSI_RESET);
            return;
        }
        alert_log = bigger_log;
        alert_capacity += ALERT_GROW_STEP;
    }

    new_alert = &alert_log[alert_count];
    new_alert->device_id = device->id;
    strcpy(new_alert->device_name, device->name);
    new_alert->device_type = device->type;
    format_reading(device, new_alert->reading_text, sizeof new_alert->reading_text);
    snprintf(new_alert->expected, EXPECTED_TEXT_SIZE, "%s", expected);
    snprintf(new_alert->reason, REASON_TEXT_SIZE, "%s", reason);
    new_alert->callback_name = callback_name;

    alert_count++;
    alerts_per_type[device->type]++;
}

/* Shared by the three type callbacks: compare a value with its limits. */
static void check_limits(struct Device *device, float value, float minimum,
                         float maximum, const char *callback_name)
{
    char expected[EXPECTED_TEXT_SIZE];
    const char *reason = NULL;

    if (value < minimum) {
        reason = "Reading is below the minimum limit";
    } else if (value > maximum) {
        reason = "Reading is above the maximum limit";
    }

    if (reason == NULL) {
        return;
    }

    snprintf(expected, sizeof expected, "%g to %g %s", minimum, maximum,
             get_unit(device->type));
    device->status = STATUS_ALERT;
    add_alert(device, expected, reason, callback_name);
}

static void temperature_monitor(struct Device *device)
{
    float temperature = device->reading.temperature;
    check_limits(device, temperature, TEMPERATURE_MIN_LIMIT, TEMPERATURE_MAX_LIMIT,
                 "temperature_monitor");
}

static void pressure_monitor(struct Device *device)
{
    int pressure = device->reading.pressure;
    check_limits(device, (float)pressure, PRESSURE_MIN_LIMIT, PRESSURE_MAX_LIMIT,
                 "pressure_monitor");
}

static void battery_monitor(struct Device *device)
{
    float voltage = device->reading.voltage;
    check_limits(device, voltage, BATTERY_MIN_LIMIT, BATTERY_MAX_LIMIT,
                 "battery_monitor");
}

/*
 * Custom callback: Sensor Drift Detector.
 * drift = |current reading - previous reading of the same device|.
 * If drift is bigger than the limit for the device type, raise an alert.
 * The first reading of a device has nothing to compare with, so it is skipped.
 */
static void drift_monitor(struct Device *device)
{
    float current_value = get_reading_as_float(device);
    float drift;
    float drift_limit = drift_limits[device->type];
    char expected[EXPECTED_TEXT_SIZE];
    char reason[REASON_TEXT_SIZE];

    if (!device->has_previous_value) {
        drift_skipped++;
        return;
    }

    drift_checks++;
    drift = current_value - device->previous_value;
    if (drift < 0) {
        drift = -drift;
    }

    if (drift <= drift_limit) {
        return;
    }

    drift_alerts++;
    device->status = STATUS_ALERT;
    snprintf(expected, sizeof expected, "change <= %g %s between readings",
             drift_limit, get_unit(device->type));
    snprintf(reason, sizeof reason, "Sensor drift: changed by %.2f %s since the last reading",
             drift, get_unit(device->type));
    add_alert(device, expected, reason, "drift_monitor");
}

void process_device(struct Device *device, MonitorCallback callback)
{
    callback(device);
}

static void process_reading(struct Device *device)
{
    char reading_text[32];

    device->status = STATUS_NORMAL;
    process_device(device, type_callbacks[device->type]);
    process_device(device, drift_monitor);

    total_readings++;
    if (device->status == STATUS_ALERT) {
        abnormal_readings++;
    } else {
        normal_readings++;
    }

    format_reading(device, reading_text, sizeof reading_text);
    printf(ANSI_HEADER " Reading %d " ANSI_RESET " ID %d | %s | %s | %s | %s%s" ANSI_RESET "\n",
           total_readings, device->id, device->name, get_type_name(device->type),
           reading_text,
           device->status == STATUS_ALERT ? ANSI_ALERT_STATUS : ANSI_NORMAL_STATUS,
           device->status == STATUS_ALERT ? "ALERT" : "Normal");

    device->previous_value = get_reading_as_float(device);
    device->has_previous_value = 1;
    device->is_pending = 0;
}

void process_all_readings(struct DeviceList *list)
{
    struct Device *device;
    struct Device *end = list->devices + list->count;
    int processed_count = 0;

    for (device = list->devices; device < end; device++) {
        if (device->is_pending) {
            process_reading(device);
            processed_count++;
        }
    }

    if (processed_count == 0) {
        printf(ANSI_ERROR "No new readings to process. Generate readings first.\n" ANSI_RESET);
    }
}

void display_alerts(void)
{
    int index;

    if (alert_count == 0) {
        printf(ANSI_SUCCESS "No alerts detected.\n" ANSI_RESET);
        return;
    }

    for (index = 0; index < alert_count; index++) {
        const struct Alert *alert = &alert_log[index];
        printf("\n" ANSI_ALERT_HEADER " Alert %d " ANSI_RESET "\n", index + 1);
        printf("  Device:   ID %d, %s (%s)\n", alert->device_id, alert->device_name,
               get_type_name(alert->device_type));
        printf("  Reading:  " ANSI_ALERT_STATUS "%s" ANSI_RESET "\n", alert->reading_text);
        printf("  Expected: " ANSI_SUCCESS "%s" ANSI_RESET "\n", alert->expected);
        printf("  Reason:   " ANSI_PROMPT "%s" ANSI_RESET "\n", alert->reason);
        printf("  Callback: " ANSI_INFO "%s" ANSI_RESET "\n", alert->callback_name);
    }
}

void display_summary(const struct DeviceList *list)
{
    int type;

    printf("\n" ANSI_HEADER " Monitoring Summary " ANSI_RESET "\n");
    printf("Registered devices: %d\n", list->count);
    printf("Readings processed: %d\n", total_readings);
    printf("Normal readings:    " ANSI_NORMAL_STATUS "%d" ANSI_RESET "\n", normal_readings);
    printf("Abnormal readings:  " ANSI_ALERT_STATUS "%d" ANSI_RESET "\n", abnormal_readings);

    printf(ANSI_MENU "Alerts per device type:\n" ANSI_RESET);
    for (type = 0; type < DEVICE_TYPE_COUNT; type++) {
        printf("  %-12s %d\n", get_type_name(type), alerts_per_type[type]);
    }

    printf(ANSI_MENU "Custom callback (drift_monitor):\n" ANSI_RESET);
    printf("  Readings compared:        %d\n", drift_checks);
    printf("  Drift alerts:             %d\n", drift_alerts);
    printf("  First readings skipped:   %d\n", drift_skipped);
}

void free_monitor(void)
{
    free(alert_log);
    alert_log = NULL;
    alert_count = 0;
    alert_capacity = 0;
}
