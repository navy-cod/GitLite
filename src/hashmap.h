#ifndef HASHMAP_H
#define HASHMAP_H

#include "dynarray.h"
#include <stddef.h>

typedef struct Entry {
    char* key;
    void* value;
    struct Entry* next;
} Entry;

typedef struct {
    DynArray* buckets;
    size_t capacity;
    size_t size;
} HashMap;

HashMap* hashmap_init(size_t bucket_count);
void hashmap_put(HashMap* map, const char* key, void* value);
void* hashmap_get(HashMap* map, const char* key);
void hashmap_free(HashMap* map);


#endif