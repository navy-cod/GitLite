#include "diff.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char** split_lines(const char* text, size_t* out_count) {
    size_t count = 0;
    for (const char* p = text; *p; p++) {
        if (*p == '\n') count++;
    }
    if (*text && text[strlen(text) - 1] != '\n') count++;

    char** lines = malloc((count + 1) * sizeof(char*));
    if (!lines) {
        fprintf(stderr, "split_lines: malloc failed\n");
        exit(1);
    }

    size_t idx   = 0;
    const char* start = text;

    while (*start) {
        const char* end = strchr(start, '\n');
        size_t len;

        if (end) {
            len = (size_t)(end - start);
        } else {
            len = strlen(start);
        }

        char* line = malloc(len + 1);
        if (!line) { fprintf(stderr, "split_lines: malloc failed\n"); exit(1); }
        memcpy(line, start, len);
        line[len] = '\0';
        lines[idx++] = line;

        if (end) {
            start = end + 1;
        } else {
            break;
        }
    }

    *out_count = idx;
    return lines;
}

static void free_lines(char** lines, size_t count) {
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
}

static int* lcs_build_table(char** a, size_t m, char** b, size_t n) {
    int* dp = calloc((m + 1) * (n + 1), sizeof(int));
    if (!dp) { fprintf(stderr, "lcs_build_table: calloc failed\n"); exit(1); }

    for (size_t i = 1; i <= m; i++) {
        for (size_t j = 1; j <= n; j++) {
            if (strcmp(a[i - 1], b[j - 1]) == 0) {
                dp[i * (n + 1) + j] = dp[(i - 1) * (n + 1) + (j - 1)] + 1;
            } else {
                int from_top  = dp[(i - 1) * (n + 1) + j];
                int from_left = dp[i * (n + 1) + (j - 1)];
                dp[i * (n + 1) + j] =
                    from_top > from_left ? from_top : from_left;
            }
        }
    }

    return dp;
}

static void result_append(DiffResult* r, char prefix, const char* text) {
    r->lines = realloc(r->lines, (r->line_count + 1) * sizeof(char*));
    if (!r->lines) { fprintf(stderr, "result_append: realloc failed\n"); exit(1); }

    size_t len  = strlen(text);
    char*  line = malloc(len + 2);
    if (!line) { fprintf(stderr, "result_append: malloc failed\n"); exit(1); }

    line[0] = prefix;
    memcpy(line + 1, text, len);
    line[len + 1] = '\0';

    r->lines[r->line_count++] = line;

    if (prefix == '+' || prefix == '-') r->has_changes = 1;
}

static void backtrack(int* dp, char** a, char** b, size_t n, long i, long j, DiffResult* result) {
    if (i == 0 && j == 0) return;

    if (i > 0 && j > 0 &&
        strcmp(a[i - 1], b[j - 1]) == 0) {
        backtrack(dp, a, b, n, i - 1, j - 1, result);
        result_append(result, ' ', a[i - 1]);

    } else if (j > 0 && (i == 0 || dp[i * (n + 1) + (j - 1)] >= dp[(i - 1) * (n + 1) + j])) {
        backtrack(dp, a, b, n, i, j - 1, result);
        result_append(result, '+', b[j - 1]);

    } else {
        backtrack(dp, a, b, n, i - 1, j, result);
        result_append(result, '-', a[i - 1]);
    }
}


DiffResult* diff_compute(const char* text_a, const char* text_b) {
    DiffResult* result = malloc(sizeof(DiffResult));
    if (!result) { fprintf(stderr, "diff_compute: malloc failed\n"); exit(1); }
    result->lines       = NULL;
    result->line_count  = 0;
    result->has_changes = 0;

    if (!text_a) text_a = "";
    if (!text_b) text_b = "";

    size_t  m;
    size_t  n;
    char**  a = split_lines(text_a, &m);
    char**  b = split_lines(text_b, &n);

    if (m == 0 && n == 0) {
        free_lines(a, m);
        free_lines(b, n);
        return result;
    }

    int* dp = lcs_build_table(a, m, b, n);

    backtrack(dp, a, b, n, (long)m, (long)n, result);

    free(dp);
    free_lines(a, m);
    free_lines(b, n);

    return result;
}

void diff_print(DiffResult* result, const char* label_a, const char* label_b) {
    if (!result->has_changes) {
        printf("(no changes)\n");
        return;
    }

    printf("--- %s\n", label_a);
    printf("+++ %s\n", label_b);
    printf("@@\n");

    for (size_t i = 0; i < result->line_count; i++) {
        printf("%s\n", result->lines[i]);
    }
}

void diff_result_free(DiffResult* result) {
    if (!result) return;
    for (size_t i = 0; i < result->line_count; i++) {
        free(result->lines[i]);
    }
    free(result->lines);
    free(result);
}
