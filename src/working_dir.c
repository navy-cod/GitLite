#include "working_dir.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>   
#include <dirent.h>     

static int should_skip(const char* name) {
    return strcmp(name, ".")      == 0 ||
           strcmp(name, "..")     == 0 ||
           strcmp(name, ".git")   == 0 ||
           strcmp(name, ".glite") == 0 ||
           strcmp(name, "build")  == 0;
}

char* working_dir_read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
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

static void scan_recursive(const char* dir_path, TreeNode* node) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "working_dir_scan: cannot open '%s'\n", dir_path);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (should_skip(entry->d_name)) continue;

        size_t full_len = strlen(dir_path) + 1 + strlen(entry->d_name) + 1;
        char*  full_path = malloc(full_len);
        if (!full_path) {
            fprintf(stderr, "scan_recursive: malloc failed\n");
            exit(1);
        }
        snprintf(full_path, full_len, "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) {
            free(full_path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            TreeNode* child = tree_node_create_dir(entry->d_name);
            scan_recursive(full_path, child);
            tree_node_add_child(node, child);
        } else {
            TreeNode* child = tree_node_create_file(entry->d_name, "");
            tree_node_add_child(node, child);
        }

        free(full_path);
    }

    closedir(dir);
}

TreeNode* working_dir_scan(const char* root_path) {
    TreeNode* root = tree_node_create_dir("");
    scan_recursive(root_path, root);
    return root;
}

static void status_recursive(TreeNode* work_node, TreeNode* committed_node, const char* repo_root, const char* path_prefix) {
    TreeNode* wc = work_node ? work_node->children : NULL;
    while (wc != NULL) {
        size_t prefix_len  = strlen(path_prefix) + strlen(wc->name) + 2;
        char*  entry_path  = malloc(prefix_len);
        if (!entry_path) { fprintf(stderr, "status_recursive: malloc failed\n"); exit(1); }
        snprintf(entry_path, prefix_len, "%s%s", path_prefix, wc->name);

        TreeNode* cc = committed_node ? tree_node_find_child(committed_node, wc->name) : NULL;

        if (wc->is_dir) {
            size_t dir_path_len = strlen(entry_path) + 2;
            char*  dir_path     = malloc(dir_path_len);
            if (!dir_path) { fprintf(stderr, "status_recursive: malloc failed\n"); exit(1); }
            snprintf(dir_path, dir_path_len, "%s/", entry_path);

            status_recursive(wc, cc, repo_root, dir_path);
            free(dir_path);
        } else {
            if (!cc) {
                printf("  new file:  %s\n", entry_path);
            } else {
                size_t disk_path_len = strlen(repo_root) + 1 + strlen(entry_path) + 1;
                char*  disk_path = malloc(disk_path_len);
                if (!disk_path) { fprintf(stderr, "status_recursive: malloc failed\n"); exit(1); }
                snprintf(disk_path, disk_path_len, "%s/%s", repo_root, entry_path);

                size_t work_size;
                char*  work_content = working_dir_read_file(disk_path, &work_size);
                free(disk_path);

                if (work_content) {
                    unsigned int hash = 2166136261u;
                    const unsigned char* p = (const unsigned char*)work_content;
                    for (size_t i = 0; i < work_size; i++) {
                        hash ^= p[i];
                        hash *= 16777619u;
                    }
                    char work_hash[9];
                    snprintf(work_hash, sizeof(work_hash), "%08x", hash);

                    if (strcmp(work_hash, cc->blob_hash) != 0) { printf("  modified:  %s\n", entry_path); }
                    free(work_content);
                }
            }
        }

        free(entry_path);
        wc = wc->next;
    }

    TreeNode* cc = committed_node ? committed_node->children : NULL;
    while (cc != NULL) {
        if (!work_node || !tree_node_find_child(work_node, cc->name)) {
            size_t prefix_len = strlen(path_prefix) + strlen(cc->name) + 2;
            char*  entry_path = malloc(prefix_len);
            if (!entry_path) { fprintf(stderr, "status_recursive: malloc failed\n"); exit(1); }
            snprintf(entry_path, prefix_len, "%s%s", path_prefix, cc->name);
            printf("  deleted:   %s\n", entry_path);
            free(entry_path);
        }
        cc = cc->next;
    }
}

void working_dir_status(TreeNode* work_root, TreeNode* committed_root, const char* repo_root) {
    printf("Changes in working directory:\n");
    status_recursive(work_root, committed_root, repo_root, "");
}

void working_dir_diff_file(const char* filepath, const char* committed_content, const char* disk_path) {
    size_t work_size;
    char*  work_content = working_dir_read_file(disk_path, &work_size);

    if (!work_content) { fprintf(stderr, "diff: cannot read '%s'\n", disk_path); return; }

    DiffResult* result = diff_compute( committed_content ? committed_content : "", work_content);

    size_t label_len = strlen(filepath) + 16;
    char*  label_a   = malloc(label_len);
    char*  label_b   = malloc(label_len);
    if (!label_a || !label_b) {
        fprintf(stderr, "working_dir_diff_file: malloc failed\n");
        exit(1);
    }
    snprintf(label_a, label_len, "%s (committed)", filepath);
    snprintf(label_b, label_len, "%s (working)",   filepath);

    diff_print(result, label_a, label_b);

    free(label_a);
    free(label_b);
    free(work_content);
    diff_result_free(result);
}

