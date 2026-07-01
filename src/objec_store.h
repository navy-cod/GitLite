#ifndef OBJECT_STORE_H
#define OBJECT_STORE_H

#include "hashmap.h"
#include "blob.h"

typedef struct {
    HashMap* store;
} ObjectStore;

ObjectStore* object_store_init(void);

void object_store_put(ObjectStore* os, const char* hash, Blob* blob);

Blob* object_store_get(ObjectStore* os, const char* hash);

void object_store_free(ObjectStore* os);


#endif