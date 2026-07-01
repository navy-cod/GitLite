#include "hashmap.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FNV_OFFSET_BASIS 2166136261u
#define FNV_PRIME 16777619u

static unsigned int fnv1a_hash(const char* key) {
    unsigned int hash = FNV_OFFSET_BASIS;
    
    const unsigned char* p = (const unsigned char*)key;

    while (*p) {
        hash ^= *p;
        hash *= FNV_PRIME;
        p++;
    }
    return hash;
}

HashMap* hash_map_init(size_t bucket_count) {
    HashMap* map = malloc(sizeof(HashMap));
    if (!map) { fprintf(stderr, "hashmap_init: malloc failed"); }

    map->capacity = bucket_count;
    map->size = 0;
    map->buckets = dynarray_init(bucket_count);

    for (size_t i = 0; i < bucket_count; i++) {
        dynarray_push(map->buckets, NULL);
    }

    return map;
}

void hashmap_put(HashMap* map, const char* key, void* value) {
    unsigned int hash = fnv1a_hash(key);
    size_t index = hash % map->capacity;

    Entry* head = (Entry*)dynarray_get(map->buckets, index);
    Entry* curr = head;

    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            curr->value = value;
            return;
        }
        curr = curr->next;
    }

    Entry* new_entry = malloc(sizeof(Entry));
    if (!new_entry) { fprintf(stderr, "hashmap_put: malloc failed\n"); exit(1); }

    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->next = head;

    map->buckets->data[index] = new_entry;
    map->size++;
}

void* hashmap_get(HashMap* map, const char* key) {
    unsigned int hash = fnv1a_hash(key);
    size_t index = hash % map->capacity;

    Entry* curr = (Entry*)dynarray_get(map->buckets, index);
    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            return curr->value;
        }
        curr = curr->next;
    }
    return NULL;
}

void hashmap_free(HashMap* map) {
    for (size_t i = 0; i < map->capacity; i++) {
        Entry* curr = (Entry*)dynarray_get(map->buckets, i);
        while (curr != NULL) {
            Entry* next = curr->next;
            free(curr);
            curr = next;
        }
    }
    dynarray_free(map->buckets);
    free(map);
}