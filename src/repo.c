#include "repo.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static void get_timestamp(char* buf, size_t bufsize) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S", t);
}

Repository* repo_init(void) {
    Repository* repo = malloc(sizeof(Repository));
    if (!repo) { fprintf(stderr, "repo_init: malloc failed\n"); exit(1); }

    repo->store = object_store_init();
    repo->staging = staging_area_init();
    repo->branches = hashmap_init(16);
    repo->HEAD = strdup("master");

    hashmap_put(repo->branches, "master", NULL);

    return repo;
}

void repo_commit(Repository* repo, const char* message) {
    char tree_hash[9];
    tree_compute_hash(repo->staging->root, tree_hash);

    char* tree_str = tree_serialize(repo->staging->root);
    Blob* tree_blob = blob_create(tree_str, strlen(tree_str));
    free(tree_str);
    object_store_put(repo->store, tree_hash, tree_blob);

    char* parent_hash = (char*)hashmap_get(repo->branches, repo->HEAD);

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    Commit* c = commit_create(parent_hash, tree_hash, message, timestamp);
    char* commit_str = commit_serialize(c);
    Blob* commit_blob = blob_create(commit_str, strlen(commit_str));
    free(commit_str);
    object_store_put(repo->store, c->hash, commit_blob);

    char* old_hash = (char*)hashmap_get(repo->branches, repo->HEAD);
    free(old_hash);
    hashmap_put(repo->branches, repo->HEAD, strdup(c->hash));

    staging_area_free(repo->staging);
    repo->staging = staging_area_init();

    commit_free(c);
}

void repo_log(Repository* repo) {
    char* current_hash = (char*)hashmap_get(repo->branches, repo->HEAD);
    if (!current_hash) {
        printf("No commits yet on branch '%s'.\n", repo->HEAD);
        return;
    }

    char* hash = current_hash;

    while (hash != NULL) {
        Blob* blob = object_store_get(repo->store, hash);
        if (!blob) {
            fprintf(stderr, "repo_log: commit '%s' not found in store\n", hash);
            break;
        }

        Commit* c = commit_deserialize(blob->content);
        if (!c) break;

        printf("commit: %s\n", c->hash);
        printf("Date: %s\n", c->timestamp);
        printf("   %s\n\n", c->message);

        char* next_hash = c->parent_hash ? strdup(c->parent_hash) : NULL;
        commit_free(c);

        if (hash != current_hash) {
            free(hash);
        }
        hash = next_hash;
    }
}

void repo_free(Repository* repo) {
    for (size_t i = 0; i < repo->branches->capacity; i++) {
        Entry* curr = (Entry*)dynarray_get(repo->branches->buckets, i);
        while (curr != NULL) {
            free(curr->value);
            curr = curr->next;
        }
    }

    hashmap_free(repo->branches);
    object_store_free(repo->store);
    staging_area_free(repo->staging);
    free(repo->HEAD);
    free(repo);
}

