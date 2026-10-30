#include "text.h"

#include <stdio.h>
#include <stdlib.h>

static void print_menu(void)
{
    printf("\n--- Menu ---\n");
    printf(" 1. Enter or replace paragraph text\n");
    printf(" 2. Count total words\n");
    printf(" 3. Find longest word\n");
    printf(" 4. Find most frequent word\n");
    printf(" 5. Display word statistics\n");
    printf(" 6. Sort word statistics\n");
    printf(" 7. Search for a word\n");
    printf(" 8. Run custom analysis (Lexical Diversity Score)\n");
    printf(" 9. Generate text summary\n");
    printf("10. Memory address inspection\n");
    printf("11. Exit\n");
    printf("Choice: ");
}

int main(void)
{
    static AnalysisHandler handlers[] = {
        enter_text,
        count_words,
        longest_word,
        most_frequent,
        display_word_statistics,
        sort_word_statistics,
        search_word,
        custom_analysis,
        generate_text_summary,
        print_memory_inspection,
    };
    const int handler_count = (int)(sizeof handlers / sizeof handlers[0]);
    int choice;

    if (!init_text_analyzer()) {
        return EXIT_FAILURE;
    }

    print_program_header();
    rebuild_statistics();

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

        if (choice == 11) {
            break;
        }
        if (choice < 1 || choice > handler_count) {
            printf("Invalid menu choice.\n");
            continue;
        }

        handlers[choice - 1]();
    }

    cleanup_text_analyzer();
    printf("Goodbye.\n");
    return EXIT_SUCCESS;
}
