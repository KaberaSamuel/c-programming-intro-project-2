#ifndef STUDENT_H
#define STUDENT_H

#define NAME_LEN 50
#define COURSE_LEN 50
#define PASS_GRADE 50

typedef struct {
    int id;
    char name[NAME_LEN];
    char course[COURSE_LEN];
    float grade;
} Student;

typedef void (*MenuHandler)(void);

extern Student *students;
extern int student_count;
extern int student_capacity;

int init_students(void);
void cleanup_students(void);
void add_student(void);
void display_students(void);
void sort_students(void);
void calculate_average(void);
void find_top_student(void);
void search_student(void);
void remove_student(void);
void custom_analysis(void);
void generate_summary_report(void);

#endif
