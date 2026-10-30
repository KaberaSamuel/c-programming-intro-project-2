#include "colors.h"
#include "device.h"
#include "monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define INPUT_SIZE 100

/* Windows consoles need this switched on to show ANSI colors. */
static void enable_colors(void)
{
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (GetConsoleMode(console, &mode)) {
        SetConsoleMode(console, mode | 0x0004);
    }
#endif
}

/* Shows the prompt, reads one line (typed text is colored). Returns 0 on end of input. */
static int read_line(const char *prompt, char *line, int size)
{
    int got_line;

    printf(ANSI_PROMPT "%s" ANSI_INPUT, prompt);
    got_line = fgets(line, size, stdin) != NULL;
    printf(ANSI_RESET);

    if (got_line) {
        line[strcspn(line, "\n")] = '\0';
    }
    return got_line;
}

/* Asks until the user types a whole number. Returns 0 on end of input. */
static int read_int(const char *prompt, int *value)
{
    char line[INPUT_SIZE];
    char extra;

    for (;;) {
        if (!read_line(prompt, line, sizeof line)) {
            return 0;
        }
        if (sscanf(line, "%d %c", value, &extra) == 1) {
            return 1;
        }
        printf(ANSI_ERROR "Invalid input. Please type a whole number.\n" ANSI_RESET);
    }
}

static void print_menu(void)
{
    printf(ANSI_HEADER " IoT Event Monitoring Engine " ANSI_RESET "\n");
    printf(ANSI_MENU);
    printf("1. Register a new device\n");
    printf("2. Display all registered devices\n");
    printf("3. Remove a device by ID\n");
    printf("4. Generate simulated readings\n");
    printf("5. Process readings through callbacks\n");
    printf("6. Display detected alerts\n");
    printf("7. Generate monitoring summary\n");
    printf("8. Exit\n");
    printf(ANSI_RESET);
}

static void register_device_from_input(struct DeviceList *list)
{
    int id;
    int type_choice;
    char name[INPUT_SIZE];
    int result;

    if (!read_int("Device ID (positive number): ", &id)) {
        return;
    }
    if (id <= 0) {
        printf(ANSI_ERROR "ID must be a positive number.\n" ANSI_RESET);
        return;
    }
    if (find_device_by_id(list, id) != NULL) {
        printf(ANSI_ERROR "A device with ID %d already exists.\n" ANSI_RESET, id);
        return;
    }

    if (!read_line("Device name: ", name, sizeof name)) {
        return;
    }
    if (name[0] == '\0' || strlen(name) >= DEVICE_NAME_SIZE) {
        printf(ANSI_ERROR "Name must be 1 to %d characters.\n" ANSI_RESET,
               DEVICE_NAME_SIZE - 1);
        return;
    }

    printf(ANSI_MENU "Device types:\n");
    printf("  1. Temperature sensor\n");
    printf("  2. Pressure sensor\n");
    printf("  3. Battery voltage monitor\n" ANSI_RESET);
    if (!read_int("Type (1-3): ", &type_choice)) {
        return;
    }
    if (type_choice < 1 || type_choice > DEVICE_TYPE_COUNT) {
        printf(ANSI_ERROR "Invalid device type.\n" ANSI_RESET);
        return;
    }

    result = register_device(list, id, name, type_choice - 1);
    if (result == REGISTER_OK) {
        printf(ANSI_SUCCESS "Device %d (%s) registered.\n" ANSI_RESET, id, name);
    } else if (result == REGISTER_DUPLICATE_ID) {
        printf(ANSI_ERROR "A device with ID %d already exists.\n" ANSI_RESET, id);
    } else {
        printf(ANSI_ERROR "Out of memory: device was not registered.\n" ANSI_RESET);
    }
}

static void remove_device_from_input(struct DeviceList *list)
{
    int id;

    if (!read_int("ID of the device to remove: ", &id)) {
        return;
    }
    if (remove_device(list, id)) {
        printf(ANSI_SUCCESS "Device %d removed.\n" ANSI_RESET, id);
    } else {
        printf(ANSI_ERROR "No device with ID %d.\n" ANSI_RESET, id);
    }
}

int main(void)
{
    struct DeviceList device_list;
    int choice;

    enable_colors();
    init_device_list(&device_list);
    seed_random_generator();

    for (;;) {
        printf("\n");
        print_menu();
        if (!read_int("Choice: ", &choice)) {
            break;
        }

        if (choice == 8) {
            break;
        }

        switch (choice) {
        case 1:
            register_device_from_input(&device_list);
            break;
        case 2:
            display_devices(&device_list);
            break;
        case 3:
            remove_device_from_input(&device_list);
            break;
        case 4:
            generate_all_readings(&device_list);
            break;
        case 5:
            process_all_readings(&device_list);
            break;
        case 6:
            display_alerts();
            break;
        case 7:
            display_summary(&device_list);
            break;
        default:
            printf(ANSI_ERROR "Invalid menu choice.\n" ANSI_RESET);
            break;
        }
    }

    free_device_list(&device_list);
    free_monitor();
    printf(ANSI_SUCCESS "Goodbye.\n" ANSI_RESET);
    return EXIT_SUCCESS;
}
