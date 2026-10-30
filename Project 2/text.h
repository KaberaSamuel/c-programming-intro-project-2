#ifndef TEXT_H
#define TEXT_H

#define WORD_NAME_LEN 40
#define STUDENT_ID "12345678"

typedef struct {
    char name[WORD_NAME_LEN];
    int count;
} WordStat;

typedef void (*AnalysisHandler)(void);

extern char *text_buffer;
extern WordStat *word_stats;
extern int word_stat_count;
extern int word_stat_capacity;
extern int total_word_count;
extern const char *source_title;
extern const char *source_author;

int init_text_analyzer(void);
void cleanup_text_analyzer(void);
void enter_text(void);
void rebuild_statistics(void);
int has_text(void);

void count_words(void);
void longest_word(void);
void most_frequent(void);
void display_word_statistics(void);
void sort_word_statistics(void);
void search_word(void);
void custom_analysis(void);
void generate_text_summary(void);
void print_memory_inspection(void);
void print_program_header(void);

#endif
