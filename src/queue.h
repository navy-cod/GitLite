#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

typedef struct {
    void** data;
    size_t head;
    size_t size;
    size_t capacity;
} Queue;

Queue* queue_init(void);
void queue_enqueue(Queue* q, void* item);
void* queue_dequeue(Queue* q);
int queue_is_empty(Queue* q);
void queue_free(Queue* q);

#endif
