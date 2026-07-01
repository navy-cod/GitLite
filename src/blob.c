#include "blob.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Blob* blob_create(const char* data, size_t size) {
    Blob* blob = malloc(sizeof(Blob));
    if (!blob) { fprintf(stderr, "blob_create: malloc failed\n"); exit(1); }

    blob->content = malloc(size + 1);
    if (!blob->content) { fprintf(stderr, "blob_create: malloc failed for content\n"); exit(1); }

    memcpy(blob->content, data, size);
    blob->content[size] = '\0';
    blob->size = size;
    return(blob);
}

void blob_free(Blob* blob) {
    free(blob->content);
    free(blob);
}
