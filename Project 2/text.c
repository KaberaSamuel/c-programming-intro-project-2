#include "text.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TEXT_GROW 256
#define STAT_INITIAL 32
#define STAT_GROW 2
#define MIN_PARAGRAPH_WORDS 150

char *text_buffer = NULL;
WordStat *word_stats = NULL;
int word_stat_count = 0;
int word_stat_capacity = 0;
int total_word_count = 0;

static const char *default_source_title = "A Tale of Two Cities";
static const char *default_source_author = "Charles Dickens";

const char *source_title = NULL;
const char *source_author = NULL;

static char user_source_title[80];
static char user_source_author[80];

static const char default_paragraph[] =
    "It was the best of times, it was the worst of times, it was the age of wisdom, "
    "it was the age of foolishness, it was the epoch of belief, it was the epoch of "
    "incredulity, it was the season of Light, it was the season of Darkness, it was "
    "the spring of hope, it was the winter of despair, we had everything before us, "
    "we had nothing before us, we were all going direct to Heaven, we were all going "
    "direct the other way in short, the period was so far like the present period, "
    "that some of its noisiest authorities insisted on its being received, for good "
    "or for evil, in the superlative degree of comparison only. There were a king with "
    "a large jaw and a queen with a plain face, on the throne of England; there were "
    "a king with a large jaw and a queen with a fair face, on the throne of France. "
    "In both countries it was clearer than crystal to the lords of the State preserves "
    "of loaves and fishes, that things in general were settled for ever.";

static void trim_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        return 0;
    }
    trim_newline(buf);
    return 1;
}

static int grow_text_buffer(size_t min_capacity)
{
    size_t new_cap = text_buffer == NULL ? TEXT_GROW : strlen(text_buffer) + TEXT_GROW;
    char *block;

    while (new_cap < min_capacity) {
        new_cap += TEXT_GROW;
    }

    block = realloc(text_buffer, new_cap);
    if (block == NULL) {
        printf("Error: could not expand text buffer.\n");
        return 0;
    }
    text_buffer = block;
    return 1;
}

static int grow_word_stats(void)
{
    int new_cap =
        word_stat_capacity == 0 ? STAT_INITIAL : word_stat_capacity * STAT_GROW;
    WordStat *block = realloc(word_stats, (size_t)new_cap * sizeof(WordStat));
    if (block == NULL) {
        printf("Error: could not expand word statistics array.\n");
        return 0;
    }
    word_stats = block;
    word_stat_capacity = new_cap;
    return 1;
}

static void clear_word_stats(void)
{
    free(word_stats);
    word_stats = NULL;
    word_stat_count = 0;
    word_stat_capacity = 0;
    total_word_count = 0;
}

static WordStat *find_word_stat(const char *word)
{
    WordStat *p = word_stats;
    WordStat *end = word_stats + word_stat_count;

    for (; p < end; p++) {
        if (strcmp(p->name, word) == 0) {
            return p;
        }
    }
    return NULL;
}

static int add_word_occurrence(const char *word)
{
    WordStat *entry;

    entry = find_word_stat(word);
    if (entry != NULL) {
        entry->count++;
        return 1;
    }

    if (word_stat_count >= word_stat_capacity && !grow_word_stats()) {
        return 0;
    }

    strcpy(word_stats[word_stat_count].name, word);
    word_stats[word_stat_count].count = 1;
    word_stat_count++;
    return 1;
}

static void lowercase_word(char *word)
{
    char *p = word;
    for (; *p != '\0'; p++) {
        *p = (char)tolower((unsigned char)*p);
    }
}

int has_text(void)
{
    return text_buffer != NULL && text_buffer[0] != '\0';
}

void rebuild_statistics(void)
{
    char *p;
    char *end;
    char token[WORD_NAME_LEN];
    char *write;

    clear_word_stats();

    if (!has_text()) {
        return;
    }

    p = text_buffer;
    end = text_buffer + strlen(text_buffer);

    while (p < end) {
        while (p < end && !isalpha((unsigned char)*p)) {
            p++;
        }
        if (p >= end) {
            break;
        }

        write = token;
        token[WORD_NAME_LEN - 1] = '\0';
        while (p < end && isalpha((unsigned char)*p)) {
            if (write < token + WORD_NAME_LEN - 1) {
                *write++ = (char)tolower((unsigned char)*p);
            }
            p++;
        }
        *write = '\0';

        if (token[0] == '\0') {
            continue;
        }

        total_word_count++;
        if (!add_word_occurrence(token)) {
            printf("Warning: stopped updating statistics (memory error).\n");
            return;
        }
    }
}

static int load_default_text(void)
{
    size_t len = strlen(default_paragraph);
    char *block = malloc(len + 1);

    if (block == NULL) {
        fprintf(stderr, "Fatal: could not allocate text buffer.\n");
        return 0;
    }

    free(text_buffer);
    text_buffer = block;
    strcpy(text_buffer, default_paragraph);
    source_title = default_source_title;
    source_author = default_source_author;
    return 1;
}

int init_text_analyzer(void)
{
    return load_default_text();
}

void cleanup_text_analyzer(void)
{
    free(text_buffer);
    text_buffer = NULL;
    clear_word_stats();
}

void print_program_header(void)
{
    printf("\nDynamic Text Mining and Linguistic Pattern Analyzer\n");
}

void enter_text(void)
{
    char line[512];
    char title[80];
    char author[80];

    for (;;) {
        size_t total_len = 0;

        printf("\nEnter your paragraph below (at least %d words).\n", MIN_PARAGRAPH_WORDS);
        printf("When you are done, enter a blank line.\n");
        printf("A blank paragraph reloads the default text.\n\n");

        free(text_buffer);
        text_buffer = NULL;

        if (!grow_text_buffer(TEXT_GROW)) {
            return;
        }
        text_buffer[0] = '\0';

        while (read_line(line, sizeof line)) {
            if (line[0] == '\0') {
                break;
            }

            {
                size_t line_len = strlen(line);
                size_t need = total_len + line_len + 2;
                if (!grow_text_buffer(need + 1)) {
                    return;
                }
                if (total_len > 0) {
                    strcat(text_buffer, " ");
                    total_len++;
                }
                strcat(text_buffer, line);
                total_len += line_len;
            }
        }

        if (total_len == 0) {
            printf("No text entered. Reloading default paragraph.\n");
            if (!load_default_text()) {
                return;
            }
            break;
        }

        rebuild_statistics();
        if (total_word_count < MIN_PARAGRAPH_WORDS) {
            printf("Paragraph rejected: %d word(s), need at least %d. Paste again.\n",
                   total_word_count, MIN_PARAGRAPH_WORDS);
            continue;
        }

        printf("Source title: ");
        if (!read_line(title, sizeof title) || title[0] == '\0') {
            source_title = "User-provided text";
        } else {
            strncpy(user_source_title, title, sizeof user_source_title - 1);
            user_source_title[sizeof user_source_title - 1] = '\0';
            source_title = user_source_title;
        }

        printf("Author or publication: ");
        if (!read_line(author, sizeof author) || author[0] == '\0') {
            source_author = "Not specified";
        } else {
            strncpy(user_source_author, author, sizeof user_source_author - 1);
            user_source_author[sizeof user_source_author - 1] = '\0';
            source_author = user_source_author;
        }
        break;
    }

    rebuild_statistics();
    printf("Text loaded (%d words, %d distinct).\n", total_word_count, word_stat_count);
}

void count_words(void)
{
    if (!has_text()) {
        printf("No text loaded.\n");
        return;
    }
    printf("Total words: %d\n", total_word_count);
}

void longest_word(void)
{
    WordStat *p;
    WordStat *end;
    WordStat *best = NULL;
    size_t best_len = 0;
    size_t len;

    if (word_stat_count == 0) {
        printf("No words to analyze.\n");
        return;
    }

    p = word_stats;
    end = word_stats + word_stat_count;
    for (; p < end; p++) {
        len = strlen(p->name);
        if (len > best_len) {
            best_len = len;
            best = p;
        }
    }

    printf("Longest word: \"%s\" (length %zu)\n", best->name, best_len);
}

void most_frequent(void)
{
    WordStat *p;
    WordStat *end;
    WordStat *best = word_stats;
    int tie = 0;

    if (word_stat_count == 0) {
        printf("No words to analyze.\n");
        return;
    }

    p = word_stats + 1;
    end = word_stats + word_stat_count;
    for (; p < end; p++) {
        if (p->count > best->count) {
            best = p;
            tie = 0;
        } else if (p->count == best->count) {
            tie = 1;
        }
    }

    if (tie) {
        printf("Most frequent word: \"%s\" (frequency %d; tie with other word(s))\n",
               best->name, best->count);
    } else {
        printf("Most frequent word: \"%s\" (frequency %d)\n", best->name, best->count);
    }
}

void display_word_statistics(void)
{
    WordStat *p;
    WordStat *end;

    if (word_stat_count == 0) {
        printf("No word statistics.\n");
        return;
    }

    printf("\n%-24s %s\n", "Word", "Count");
    printf("--------------------------------\n");

    p = word_stats;
    end = word_stats + word_stat_count;
    for (; p < end; p++) {
        printf("%-24s %d\n", p->name, p->count);
    }
    printf("\n");
}

static int compare_by_name(const WordStat *a, const WordStat *b)
{
    return strcmp(a->name, b->name);
}

static int compare_by_count(const WordStat *a, const WordStat *b)
{
    return a->count - b->count;
}

typedef int (*WordStatCompare)(const WordStat *, const WordStat *);

static void bubble_sort_stats(WordStatCompare compare)
{
    int i;
    int j;
    WordStat temp;

    for (i = 0; i < word_stat_count - 1; i++) {
        for (j = 0; j < word_stat_count - 1 - i; j++) {
            if (compare(&word_stats[j], &word_stats[j + 1]) > 0) {
                temp = word_stats[j];
                word_stats[j] = word_stats[j + 1];
                word_stats[j + 1] = temp;
            }
        }
    }
}

void sort_word_statistics(void)
{
    int choice;
    char line[32];
    int value;
    char extra;

    if (word_stat_count == 0) {
        printf("No statistics to sort.\n");
        return;
    }

    printf("Sort by: 1=Alphabetical  2=Frequency\n");
    printf("Choice: ");
    if (!read_line(line, sizeof line)) {
        return;
    }
    if (sscanf(line, "%d%c", &value, &extra) != 1 || value < 1 || value > 2) {
        printf("Invalid choice.\n");
        return;
    }
    choice = value;

    if (choice == 1) {
        bubble_sort_stats(compare_by_name);
    } else {
        bubble_sort_stats(compare_by_count);
    }
    printf("Word statistics sorted.\n");
}

void search_word(void)
{
    char query[WORD_NAME_LEN];
    WordStat *entry;

    if (word_stat_count == 0) {
        printf("No words to search.\n");
        return;
    }

    printf("Enter word to search: ");
    if (!read_line(query, sizeof query) || query[0] == '\0') {
        printf("Empty search term.\n");
        return;
    }
    lowercase_word(query);

    entry = find_word_stat(query);
    if (entry == NULL) {
        printf("Word \"%s\" not found (frequency 0).\n", query);
    } else {
        printf("Word \"%s\" appears %d time(s).\n", entry->name, entry->count);
    }
}

static float lexical_diversity_score(void)
{
    if (total_word_count == 0) {
        return 0.0f;
    }
    return ((float)word_stat_count / (float)total_word_count) * 100.0f;
}

void custom_analysis(void)
{
    float score;

    if (total_word_count == 0) {
        printf("No text to analyze.\n");
        return;
    }

    score = lexical_diversity_score();
    printf("\nLexical Diversity Score\n");
    printf("Formula: (distinct words / total words) * 100\n");
    printf("Distinct words: %d\n", word_stat_count);
    printf("Total words: %d\n", total_word_count);
    printf("Score: %.2f%%\n", score);
    printf("Higher scores mean more unique vocabulary relative to text length.\n\n");
}

static float average_word_length(void)
{
    WordStat *p = word_stats;
    WordStat *end = word_stats + word_stat_count;
    int letters = 0;

    if (total_word_count == 0) {
        return 0.0f;
    }

    for (; p < end; p++) {
        letters += (int)strlen(p->name) * p->count;
    }
    return (float)letters / (float)total_word_count;
}

void generate_text_summary(void)
{
    WordStat *p;
    WordStat *end;
    WordStat *longest = NULL;
    WordStat *frequent = NULL;
    size_t longest_len = 0;

    if (total_word_count == 0) {
        printf("No summary available.\n");
        return;
    }

    p = word_stats;
    end = word_stats + word_stat_count;
    frequent = word_stats;
    for (; p < end; p++) {
        size_t len = strlen(p->name);
        if (len > longest_len) {
            longest_len = len;
            longest = p;
        }
        if (p->count > frequent->count) {
            frequent = p;
        }
    }

    printf("\n--- Text Summary ---\n");
    printf("Total words: %d\n", total_word_count);
    printf("Distinct words: %d\n", word_stat_count);
    printf("Average word length: %.2f characters\n", average_word_length());
    if (longest != NULL) {
        printf("Longest word: \"%s\" (length %zu)\n", longest->name, longest_len);
    }
    if (frequent != NULL) {
        printf("Most frequent word: \"%s\" (frequency %d)\n", frequent->name, frequent->count);
    }
    printf("Lexical Diversity Score: %.2f%%\n", lexical_diversity_score());
    printf("--------------------\n\n");
}

void print_memory_inspection(void)
{
    WordStat *sample = NULL;

    printf("\nMemory Inspection\n");
    printf("--------------------------\n");

    if (text_buffer != NULL) {
        printf("Text buffer address: %p\n", (void *)text_buffer);
        printf("  Holds the paragraph characters loaded for analysis.\n");
    } else {
        printf("Text buffer address: (null)\n");
    }

    if (word_stats != NULL) {
        printf("Word statistics array: %p\n", (void *)word_stats);
        printf("  Base of the dynamic array of distinct words and counts.\n");
    } else {
        printf("Word statistics array: (null)\n");
    }

    if (word_stat_count > 0) {
        sample = word_stats;
        printf("Selected word-stat entry: %p\n", (void *)sample);
        printf("  First entry \"%s\" with count %d.\n", sample->name, sample->count);
    } else {
        printf("Selected word-stat entry: (none)\n");
    }
    printf("\n");
}
