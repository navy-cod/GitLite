#include "staging_area.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//simple FNV-1a hasher for blob content
static void compute_blob_hash(const char* content, size_t size, char* out_hash) {
    unsigned int hash = 2166136261u;
    const unsigned char* p = (const unsigned char*)content;
    for (size_t i = 0; i < size; i++) {
        hash ^= p[i];
        hash *= 16777619u;
    }
    snprintf(out_hash, 9, "%08x", hash);
}

        //read an entire file in a heap-allocated buffer
static char* read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "glite: cannot open '%s'\n", path);
        return NULL;
    }
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

StagingArea* staging_area_init(void) {
    StagingArea* sa = malloc(sizeof(StagingArea));
    if (!sa) { fprintf(stderr, "staging_area_init: malloc failed\n"); exit(1); }

    sa->root = tree_node_create_dir("");
    sa->store = object_store_init();
    return sa;
}

int staging_area_add(StagingArea* sa, const char* path) {
    size_t file_size;
    char* content = read_file(path, &file_size);
    if (!content) return -1;

    char blob_hash[9];      //hash content and store as blob
    compute_blob_hash(content, file_size, blob_hash);

    if (!object_store_get(sa->store, blob_hash)) {
        Blob* blob = blob_create(content, file_size);
        object_store_put(sa->store, blob_hash, blob);
    }
    free(content);

        /* Walk / build tree along the path */
    char* path_copy = strdup(path);
    char* token[64];
    int token_count = 0;

    char* tok = strtok(path_copy, "/");
    while (tok != NULL && token_count < 64) {
        token[token_count++] = tok;
        tok = strtok(NULL, "/");
    }

    if (token_count == 0) {
        fprintf(stderr, "glite: invalid path '%s'\n", path);
        free(path_copy);
        return -1;
    }

    TreeNode* current = sa->root;   /* Navigate from root, creating directory as needed */
    for (int i = 0; i < token_count - 1; i++) {
        TreeNode* child = tree_node_find_child(current, token[i]);
        if (!child) {
            child = tree_node_create_dir(token[i]);
            tree_node_add_child(current, child);
        }
        current = child;
    }

                /* Add or update the file node */
    const char* filename = token[token_count - 1];
    TreeNode* existing = tree_node_find_child(current, filename);

    if (existing) {
        free(existing->blob_hash);
        existing->blob_hash = strdup(blob_hash);
        printf("updated: %s\n", path);
    } else {
        TreeNode* file_node = tree_node_create_file(filename, blob_hash);
        tree_node_add_child(current, file_node);
        printf("staged: %s\n", path);
    }

    free(path_copy);
    return 0;
}

void staging_area_list(StagingArea* sa) {
    printf("Staged files:\n");
    TreeNode* child = sa->root->children;
    if (!child) {
        printf("  (nothing staged)\n");
        return;
    }
    while (child != NULL) {
        tree_node_print(child, 1);
        child = child->next;
    }
}

void staging_area_free(StagingArea* sa) {
    tree_node_free(sa->root);
    object_store_free(sa->store);
    free(sa);
}
