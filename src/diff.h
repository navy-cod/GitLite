#ifndef DIFF_H
#define DIFF_H

#include <stddef.h>

typedef struct {
    char** lines;
    size_t line_count;
    int    has_changes;
} DiffResult;

DiffResult* diff_compute(const char* text_a, const char* text_b);

void diff_print(DiffResult* result, const char* label_a, const char* label_b);

void diff_result_free(DiffResult* result);

#endif 
