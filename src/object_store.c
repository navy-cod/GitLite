#include "objec_store.h"

#include <stdlib.h>
#include <stdio.h>

ObjectStore* object_store_init(void) {
    ObjectStore* os = malloc(sizeof(ObjectStore));
    if (!os) { fprintf(stderr, "object_store_init: malloc failed\n"); exit(1); }
    os->store = hashmap_init(64);
    return os;
}

void object_store_put(ObjectStore* os, const char* hash, Blob* blob) {
    hashmap_put(os->store, hash, blob);
}

Blob* object_store_get(ObjectStore* os, const char* hash) {
    return (Blob*)hashmap_get(os->store, hash);
}

void object_store_free(ObjectStore* os) {
    for (size_t i = 0; i < os->store->capacity; i++) {
        Entry* curr = (Entry*)dynarray_get(os->store->buckets, i);
        while (curr != NULL) {
            blob_free((Blob*)curr->value);
            curr = curr->next;
        }
    }
    hashmap_free(os->store);
    free(os);
}
