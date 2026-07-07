#ifndef FS_STORE_H
#define FS_STORE_H

#include <stddef.h>

typedef struct {
    char* base_dir;
} FsStore;

FsStore* fs_store_init(const char* base_dir);

int fs_store_put(FsStore* store, const char* hash, const char* data, size_t size);

char* fs_store_get(FsStore* store, const char* hash, size_t* out_size);

int fs_store_exists(FsStore* store, const char* hash);

void fs_store_free(FsStore* store);

#endif