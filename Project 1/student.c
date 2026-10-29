#include "student.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Student list lives on the heap so it can grow and shrink at runtime. */
#define INITIAL_CAPACITY 12
#define GROW_FACTOR 2

Student *students = NULL;
int student_count = 0;
int student_capacity = 0;

/* Double capacity when the array is full */
static int grow_array(void)
{
    int new_capacity = student_capacity == 0 ? INITIAL_CAPACITY : student_capacity * GROW_FACTOR;
    Student *new_block = realloc(students, (size_t)new_capacity * sizeof(Student));
    if (new_block == NULL) {
        printf("Error: memory allocation failed.\n");
        return 0;
    }
    students = new_block;
    student_capacity = new_capacity;
    return 1;
}

/* Shrink allocation when few records use a large buffer. */
static int shrink_array_if_needed(void)
{
    if (student_count == 0) {
        free(students);
        students = NULL;
        student_capacity = 0;
        return 1;
    }
    if (student_count < student_capacity / 4 && student_capacity > INITIAL_CAPACITY) {
        int new_capacity = student_capacity / 2;
        if (new_capacity < INITIAL_CAPACITY) {
            new_capacity = INITIAL_CAPACITY;
        }
        Student *new_block = realloc(students, (size_t)new_capacity * sizeof(Student));
        if (new_block == NULL) {
            return 1;
        }
        students = new_block;
        student_capacity = new_capacity;
    }
    return 1;
}

/* Walk with pointers instead of only array indexes*/
static int id_exists(int id)
{
    Student *p = students;
    Student *end = students + student_count;
    for (; p < end; p++) {
        if (p->id == id) {
            return 1;
        }
    }
    return 0;
}

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

/* Loop until the user gives a valid integer in range. */
static int read_int_in_range(const char *prompt, int min_val, int max_val, int *out)
{
    char line[64];
    int value;
    char extra;

    while (1) {
        printf("%s", prompt);
        if (!read_line(line, sizeof line)) {
            return 0;
        }
        if (sscanf(line, "%d%c", &value, &extra) != 1) {
            printf("Invalid input. Enter a whole number.\n");
            continue;
        }
        if (value < min_val || value > max_val) {
            printf("Value must be between %d and %d.\n", min_val, max_val);
            continue;
        }
        *out = value;
        return 1;
    }
}

/* Valid grades are 0 through 100. */
static int read_grade(float *out)
{
    char line[64];
    float value;
    char extra;

    while (1) {
        printf("Enter grade (0-100): ");
        if (!read_line(line, sizeof line)) {
            return 0;
        }
        if (sscanf(line, "%f%c", &value, &extra) != 1) {
            printf("Invalid grade.\n");
            continue;
        }
        if (value < 0.0f || value > 100.0f) {
            printf("Grade must be between 0 and 100.\n");
            continue;
        }
        *out = value;
        return 1;
    }
}

/* Starting dataset (12 students). */
static void load_initial_dataset(void)
{
    static const Student seed[] = {
        {1001, "Alice Nguyen", "Programming", 88.5f},
        {1002, "Ben Carter", "Programming", 72.0f},
        {1003, "Carla Ortiz", "Calculus", 91.0f},
        {1004, "David Kim", "Calculus", 64.5f},
        {1005, "Elena Rossi", "Physics", 79.0f},
        {1006, "Frank Miller", "Physics", 58.0f},
        {1007, "Grace Lee", "Programming", 95.5f},
        {1008, "Hassan Ali", "English", 83.0f},
        {1009, "Ivy Brooks", "English", 47.0f},
        {1010, "James Wu", "Calculus", 76.5f},
        {1011, "Kira Patel", "Physics", 69.0f},
        {1012, "Leo Santos", "Programming", 54.0f},
    };
    size_t n = sizeof seed / sizeof seed[0];
    size_t i;

    student_capacity = INITIAL_CAPACITY;
    students = malloc((size_t)student_capacity * sizeof(Student));
    if (students == NULL) {
        fprintf(stderr, "Fatal: could not allocate initial student array.\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < n; i++) {
        students[i] = seed[i];
    }
    student_count = (int)n;
}

int init_students(void)
{
    load_initial_dataset();
    return 1;
}

void cleanup_students(void)
{
    /* Free everything before the program exits. */
    free(students);
    students = NULL;
    student_count = 0;
    student_capacity = 0;
}

void add_student(void)
{
    Student record;
    int id;

    if (student_count >= student_capacity && !grow_array()) {
        return;
    }

    if (!read_int_in_range("Enter student ID: ", 1, 999999, &id)) {
        return;
    }

    if (id_exists(id)) {
        printf("Error: duplicate student ID %d.\n", id);
        return;
    }

    record.id = id;
    printf("Enter full name: ");
    if (!read_line(record.name, sizeof record.name)) {
        return;
    }
    if (record.name[0] == '\0') {
        printf("Name cannot be empty.\n");
        return;
    }

    printf("Enter course name: ");
    if (!read_line(record.course, sizeof record.course)) {
        return;
    }
    if (record.course[0] == '\0') {
        printf("Course cannot be empty.\n");
        return;
    }

    if (!read_grade(&record.grade)) {
        return;
    }

    students[student_count] = record;
    student_count++;
    printf("Student added successfully.\n");
}

/* Print the table using pointer traversal from start to end. */
void display_students(void)
{
    Student *p;
    Student *end;

    if (student_count == 0) {
        printf("No student records.\n");
        return;
    }

    printf("\n%-8s %-22s %-18s %-8s\n", "ID", "Name", "Course", "Grade");
    printf("----------------------------------------------------------------\n");

    p = students;
    end = students + student_count;
    for (; p < end; p++) {
        printf("%-8d %-22s %-18s %-8.1f\n", p->id, p->name, p->course, p->grade);
    }
    printf("\n");
}

/* Comparators for sort */
static int compare_by_id(const Student *a, const Student *b)
{
    return a->id - b->id;
}

static int compare_by_grade(const Student *a, const Student *b)
{
    if (a->grade < b->grade) {
        return -1;
    }
    if (a->grade > b->grade) {
        return 1;
    }
    return 0;
}

static int compare_by_name(const Student *a, const Student *b)
{
    return strcmp(a->name, b->name);
}

typedef int (*StudentCompare)(const Student *, const Student *);

/* One sort loop */
static void bubble_sort(StudentCompare compare)
{
    int i;
    int j;
    Student temp;

    for (i = 0; i < student_count - 1; i++) {
        for (j = 0; j < student_count - 1 - i; j++) {
            if (compare(&students[j], &students[j + 1]) > 0) {
                temp = students[j];
                students[j] = students[j + 1];
                students[j + 1] = temp;
            }
        }
    }
}

void sort_students(void)
{
    int choice;

    if (student_count == 0) {
        printf("No records to sort.\n");
        return;
    }

    printf("Sort by: 1=Grade  2=Name  3=ID\n");
    if (!read_int_in_range("Choice: ", 1, 3, &choice)) {
        return;
    }

    if (choice == 1) {
        bubble_sort(compare_by_grade);
    } else if (choice == 2) {
        bubble_sort(compare_by_name);
    } else {
        bubble_sort(compare_by_id);
    }
    printf("Records sorted.\n");
}

void calculate_average(void)
{
    Student *p;
    Student *end;
    float sum = 0.0f;

    if (student_count == 0) {
        printf("No records.\n");
        return;
    }

    /* Sum grades with pointers */
    p = students;
    end = students + student_count;
    for (; p < end; p++) {
        sum += p->grade;
    }

    printf("Average grade: %.2f\n", sum / (float)student_count);
}

/* Track the best grade with a pointer into the array. */
void find_top_student(void)
{
    Student *p;
    Student *end;
    Student *best;

    if (student_count == 0) {
        printf("No records.\n");
        return;
    }

    best = students;
    p = students + 1;
    end = students + student_count;
    for (; p < end; p++) {
        if (p->grade > best->grade) {
            best = p;
        }
    }

    printf("Top student: %s (ID %d) - grade %.1f in %s\n",
           best->name, best->id, best->grade, best->course);
}

void search_student(void)
{
    int mode;
    char name[NAME_LEN];
    int id;
    Student *p;
    Student *end;
    int found = 0;

    if (student_count == 0) {
        printf("No records.\n");
        return;
    }

    printf("Search by: 1=ID  2=Name\n");
    if (!read_int_in_range("Choice: ", 1, 2, &mode)) {
        return;
    }

    if (mode == 1) {
        if (!read_int_in_range("Enter ID: ", 1, 999999, &id)) {
            return;
        }
        p = students;
        end = students + student_count;
        for (; p < end; p++) {
            if (p->id == id) {
                printf("Found: %s, course %s, grade %.1f\n", p->name, p->course, p->grade);
                found = 1;
                break;
            }
        }
    } else {
        printf("Enter name (exact match): ");
        if (!read_line(name, sizeof name)) {
            return;
        }
        p = students;
        end = students + student_count;
        /* Name search can match more than one record. */
        for (; p < end; p++) {
            if (strcmp(p->name, name) == 0) {
                printf("Found: ID %d, course %s, grade %.1f\n", p->id, p->course, p->grade);
                found = 1;
            }
        }
    }

    if (!found) {
        printf("No matching student.\n");
    }
}

void remove_student(void)
{
    int id;
    int i;
    int index = -1;

    if (student_count == 0) {
        printf("No records.\n");
        return;
    }

    if (!read_int_in_range("Enter ID to remove: ", 1, 999999, &id)) {
        return;
    }

    for (i = 0; i < student_count; i++) {
        if (students[i].id == id) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        printf("Student ID not found.\n");
        return;
    }

    /* Shift left to close the gap */
    for (i = index; i < student_count - 1; i++) {
        students[i] = students[i + 1];
    }
    student_count--;
    shrink_array_if_needed();
    printf("Student removed.\n");
}

/* Custom analysis: risk index = 100 - grade; below 60 flagged AT RISK. */
#define RISK_THRESHOLD 60.0f

void custom_analysis(void)
{
    Student *p;
    Student *end;
    int at_risk = 0;

    if (student_count == 0) {
        printf("No records.\n");
        return;
    }

    printf("\nAcademic Risk Index (risk = %.0f - grade; threshold grade < %.0f)\n",
           100.0f, RISK_THRESHOLD);
    printf("%-8s %-22s %-8s %-12s\n", "ID", "Name", "Grade", "Risk Index");
    printf("------------------------------------------------------\n");

    p = students;
    end = students + student_count;
    for (; p < end; p++) {
        float risk = 100.0f - p->grade;
        printf("%-8d %-22s %-8.1f %-12.1f", p->id, p->name, p->grade, risk);
        if (p->grade < RISK_THRESHOLD) {
            printf("  AT RISK");
            at_risk++;
        }
        printf("\n");
    }
    printf("\nStudents below threshold: %d\n", at_risk);
}

static float average_grade_pointer(void)
{
    Student *p = students;
    Student *end = students + student_count;
    float sum = 0.0f;

    for (; p < end; p++) {
        sum += p->grade;
    }
    return sum / (float)student_count;
}

void generate_summary_report(void)
{
    Student *p;
    Student *end;
    float highest;
    float lowest;
    int passing = 0;
    int below = 0;
    int at_risk = 0;

    if (student_count == 0) {
        printf("No records for summary.\n");
        return;
    }

    highest = students[0].grade;
    lowest = students[0].grade;
    p = students;
    end = students + student_count;
    for (; p < end; p++) {
        if (p->grade > highest) {
            highest = p->grade;
        }
        if (p->grade < lowest) {
            lowest = p->grade;
        }
        if (p->grade >= PASS_GRADE) {
            passing++;
        } else {
            below++;
        }
        if (p->grade < RISK_THRESHOLD) {
            at_risk++;
        }
    }

    printf("\n--- Summary Report ---\n");
    printf("Total records: %d\n", student_count);
    printf("Average grade: %.2f\n", average_grade_pointer());
    printf("Highest grade: %.1f\n", highest);
    printf("Lowest grade: %.1f\n", lowest);
    printf("Pass threshold: %d\n", PASS_GRADE);
    printf("At or above pass: %d\n", passing);
    printf("Below pass: %d\n", below);
    printf("Custom (risk threshold %.0f): %d at-risk students\n", RISK_THRESHOLD, at_risk);
    printf("----------------------\n\n");
}
