#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <stddef.h>

typedef struct {
    void** data;
    size_t size;
    size_t capacity;
} DynArray;

DynArray* dynarray_init(size_t initial_capacity);
void dynarray_push(DynArray* arr, void* item);
void* dynarray_get(DynArray* arr, size_t i);
void dynarray_free(DynArray* arr);

#endif 