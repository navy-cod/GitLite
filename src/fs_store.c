#include "fs_store.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>   
#include <errno.h>

static int mkdir_p(const char* path) {
    int ret = mkdir(path, 0755);
    if (ret == -1 && errno == EEXIST) return 0;
    return ret;
}


static void hash_to_path(const char* base_dir, const char* hash, char* out_dir, size_t dir_size, char* out_file, size_t file_size) {
    char prefix[3] = { hash[0], hash[1], '\0' };
    const char* suffix = hash + 2;

    snprintf(out_dir,  dir_size,  "%s/objects/%s",    base_dir, prefix);
    snprintf(out_file, file_size, "%s/objects/%s/%s", base_dir, prefix, suffix);
}

FsStore* fs_store_init(const char* base_dir) {
    FsStore* store = malloc(sizeof(FsStore));
    if (!store) { fprintf(stderr, "fs_store_init: malloc failed\n"); exit(1); }
    store->base_dir = strdup(base_dir);
    return store;
}

int fs_store_put(FsStore* store, const char* hash, const char* data, size_t size) {
    char dir_path[512];
    char file_path[512];
    hash_to_path(store->base_dir, hash, dir_path,  sizeof(dir_path), file_path, sizeof(file_path));

    if (mkdir_p(dir_path) != 0) {
        fprintf(stderr, "fs_store_put: cannot create dir '%s': %s\n",
                dir_path, strerror(errno));
        return -1;
    }

    struct stat st;
    if (stat(file_path, &st) == 0) return 0;

    FILE* f = fopen(file_path, "wb");
    if (!f) {
        fprintf(stderr, "fs_store_put: cannot open '%s': %s\n",
                file_path, strerror(errno));
        return -1;
    }

    size_t written = fwrite(data, 1, size, f);
    fclose(f);

    if (written != size) {
        fprintf(stderr, "fs_store_put: incomplete write to '%s'\n", file_path);
        return -1;
    }

    return 0;
}

char* fs_store_get(FsStore* store, const char* hash, size_t* out_size) {
    char dir_path[512];
    char file_path[512];
    hash_to_path(store->base_dir, hash,
                 dir_path,  sizeof(dir_path),
                 file_path, sizeof(file_path));

    FILE* f = fopen(file_path, "rb");
    if (!f) return NULL;  

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) { fclose(f); return NULL; }

    char* buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, (size_t)size, f);
    buf[size] = '\0';
    fclose(f);

    *out_size = (size_t)size;
    return buf;
}

int fs_store_exists(FsStore* store, const char* hash) {
    char dir_path[512];
    char file_path[512];
    hash_to_path(store->base_dir, hash, dir_path,  sizeof(dir_path), file_path, sizeof(file_path));

    struct stat st;
    return stat(file_path, &st) == 0;
}

void fs_store_free(FsStore* store) {
    if (!store) return;
    free(store->base_dir);
    free(store);
}
