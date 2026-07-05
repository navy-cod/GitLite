#include "queue.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Queue* queue_init(void) {
    Queue* q = malloc(sizeof(Queue));
    if (!q) { fprintf(stderr, "queue_init: malloc failed\n"); exit(1); }

    q->capacity = 16;
    q->data = malloc(q->capacity * sizeof(void*));
    if (!q->data) { fprintf(stderr, "queue_init: malloc data failed\n"); exit(1); }

    q->head = 0;
    q->size = 0;
    return q;
}

void queue_enqueue(Queue* q, void* item) {
    if (q->size == q->capacity) {
        void** new_data = realloc(q->data, q->capacity * sizeof(void*));
        if (!new_data) { fprintf(stderr, "queue_enqueue: realloc failed\n"); exit(1); }
    }
    q->data[q->size++] = item;
}

void* queue_dequeue(Queue* q) {
    if (q->head >= q->size) return NULL;
    return q->data[q->head++];
}

int queue_is_empty(Queue* q) {
    return q->head >= q->size;
}

void queue_free(Queue* q) {
    free(q->data);
    free(q);
}