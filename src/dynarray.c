#include "dynarray.h"

#include <stdlib.h>
#include <stdio.h>

DynArray* dynarray_init(size_t initial_capacity) {
    DynArray* arr = malloc(sizeof(DynArray));
    if (!arr) {
        fprintf(stderr, "dynarray_init: malloc failed for DynArray struct\n");
        exit(1);
    }

    arr->data = malloc(initial_capacity * sizeof(void*));
    if (!arr->data) {
        fprintf(stderr, "dynarray_init: malloc failed for data buffer\n");
        exit(1);
    }

    arr->size = 0;
    arr->capacity = initial_capacity;
    return arr;
}

void dynarray_push(DynArray* arr, void* item) {                    //is array full?
    if (arr->size == arr->capacity) {
        size_t new_capacity = arr->capacity * 2;
        void** new_data = realloc(arr->data, new_capacity * sizeof(void*));
        if (!new_data) {
            fprintf(stderr, "dynarray_push: realloc failed\n");
            exit(1);
        }
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    arr->data[arr->size] = item;
    arr->size++;
}

void* dynarray_get(DynArray* arr, size_t i) {
    return arr->data[i];
}

void dynarray_free(DynArray* arr) {
    free(arr->data);
    free(arr);
} 