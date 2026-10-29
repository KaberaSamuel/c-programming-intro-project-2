#include "student.h"

#include <stdio.h>
#include <stdlib.h>

static void print_menu(void)
{
    printf("\n--- Menu ---\n");
    printf(" 1. Add a new student record\n");
    printf(" 2. Display all student records\n");
    printf(" 3. Sort students (grade, name, or ID)\n");
    printf(" 4. Calculate average grade\n");
    printf(" 5. Identify top-performing student\n");
    printf(" 6. Search by ID or name\n");
    printf(" 7. Remove a student record\n");
    printf(" 8. Run custom analysis (Academic Risk Index)\n");
    printf(" 9. Display summary report\n");
    printf("10. Exit\n");
    printf("Choice: ");
}

int main(void)
{
    static MenuHandler handlers[] = {
        add_student,
        display_students,
        sort_students,
        calculate_average,
        find_top_student,
        search_student,
        remove_student,
        custom_analysis,
        generate_summary_report,
    };
    const int handler_count = (int)(sizeof handlers / sizeof handlers[0]);
    int choice;

    if (!init_students()) {
        return EXIT_FAILURE;
    }

    for (;;) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            int ch;
            printf("Invalid input.\n");
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
            continue;
        }
        while (getchar() != '\n') {
        }

        if (choice == 10) {
            break;
        }
        if (choice < 1 || choice > handler_count) {
            printf("Invalid menu choice.\n");
            continue;
        }

        handlers[choice - 1]();
    }

    cleanup_students();
    printf("Goodbye.\n");
    return EXIT_SUCCESS;
}
