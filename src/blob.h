#ifndef BLOB_H
#define BLOB_H

#include<stddef.h>

typedef struct {
    char* content;
    size_t size;
} Blob;

Blob* blob_create(const char* data, size_t size);

void blob_free(Blob* blob);


#endif