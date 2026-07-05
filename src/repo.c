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

    if (!object_store_get(repo->store, tree_hash)){
    char* tree_str = tree_serialize(repo->staging->root);
    Blob* tree_blob = blob_create(tree_str, strlen(tree_str));
    free(tree_str);
    object_store_put(repo->store, tree_hash, tree_blob);
    }

    char* parent_hash = (char*)hashmap_get(repo->branches, repo->HEAD);

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    Commit* c = commit_create(parent_hash, tree_hash, message, timestamp);
    if (!object_store_get(repo->store, c->hash)) {
        char* commit_str = commit_serialize(c);
        Blob* commit_blob = blob_create(commit_str, strlen(commit_str));
        free(commit_str);
        object_store_put(repo->store, c->hash, commit_blob);
    }
    
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

void repo_create_branch(Repository* repo, const char* name) {
    if (hashmap_get(repo->branches, name)) {
        fprintf(stderr, "glite: branch '%s' already exists\n", name);
        return;
    }

    char* current_hash = (char*)hashmap_get(repo->branches, repo->HEAD);
    char* new_hash = current_hash ? strdup(current_hash) : NULL;

    hashmap_put(repo->branches, name, new_hash);
    printf("Branch '%s' created.\n", name);
}

void repo_switch_branch(Repository* repo, const char* name) {
    if (!hashmap_get(repo->branches, name) && hashmap_get(repo->branches, name) == NULL) {
        /*use direct key-existence check via the entry walk instead*/
    }

    int found = 0;
    for (size_t i = 0; i < repo->branches->capacity && !found; i++) {
        Entry* e = (Entry*)dynarray_get(repo->branches->buckets, i);
        while (e != NULL) {
            if (strcmp(e->key, name) == 0) { found = 1; break; }
            e = e->next;
        }
    }
    if (!found) {
        fprintf(stderr, "glite: branch '%s' does not exist\n", name);
        return;
    }

    free(repo->HEAD);
    repo->HEAD = strdup(name);
    printf("Switched to branch '%s'.\n", name);
}

typedef struct {
    char* hash;
    int color;
} BFSEntry;

char* repo_find_lca(Repository* repo, const char* hash1, const char* hash2) {
    if (!hash1 || !hash2) return NULL;

    HashMap* visited = hashmap_init(64);
    Queue* queue = queue_init();

    BFSEntry* e1 = malloc(sizeof(BFSEntry));
    BFSEntry* e2 = malloc(sizeof(BFSEntry));
    if (!e1 || !e2) { fprintf(stderr, "repo_find_lca: malloc failed\n"); exit(1); }

    e1->hash = strdup(hash1);
    e1->color =0;
    e2->hash = strdup(hash2);
    e2->color =1;

    queue_enqueue(queue, e1);
    queue_enqueue(queue, e2);

    char* lca = NULL;

    while (!queue_is_empty(queue) && lca == NULL) {
        BFSEntry* entry = (BFSEntry*)queue_dequeue(queue);
        char* hash = entry->hash;
        int color = entry->color;
        free(entry);

        char* seen_color = (char*)hashmap_get(visited, hash);

        if (seen_color != NULL) {
            int seen_as = (seen_color[0] == '0') ? 0 : 1;
            if (seen_as != color) { lca = strdup(hash); }
        }
        free(hash);
        continue;
    

        hashmap_put(visited, hash, color == 0 ? "0" : "1");

        Blob* blob = object_store_get(repo->store, hash);
        if (blob) {
            Commit* c = commit_deserialize(blob->content);
            if (c && c->parent_hash) {
                BFSEntry* parent_entry = malloc(sizeof(BFSEntry));
                if (!parent_entry) { fprintf(stderr, "repo_find_lca: malloc failed\n"); exit(1); }

            parent_entry->hash = strdup(c->parent_hash);
            parent_entry->color = color;
            queue_enqueue(queue, parent_entry);
        }
        commit_free(c);
        }
        free(hash);
    }

    while (!queue_is_empty(queue)) {
        BFSEntry* leftover = (BFSEntry*)queue_dequeue(queue);
        free(leftover->hash);
        free(leftover);
    }

    queue_free(queue);
    hashmap_free(visited);

    return lca;
}

void repo_merge(Repository* repo, const char* source_branch) {
    int source_exists = 0;
    for (size_t i = 0; i < repo->branches->capacity; i++) {
        Entry* e = (Entry*)dynarray_get(repo->branches->buckets, i);
        while (e) {
            if (strcmp(e->key, source_branch) == 0) { source_exists = 1; break; }
            e = e->next;
        }
        if (source_exists) break;
    }

    if (!source_exists) {
        fprintf(stderr, "glite merge: branch '%s' does not exist\n", source_branch);
        return;
    }

    if (strcmp(repo->HEAD, source_branch) ==0) {
        fprintf(stderr, "glite merge: cannot merge a branch into itself\n");
        return;
    }

    char* current_hash = (char*)hashmap_get(repo->branches, repo->HEAD);
    char* source_hash = (char*)hashmap_get(repo->branches, source_branch);

    if (!current_hash) {
        char* new_tip = source_hash ? strdup(source_hash) : NULL;
        char* old_val = (char*)hashmap_get(repo->branches, repo->HEAD);
        free(old_val);
        hashmap_put(repo->branches, repo->HEAD, new_tip);
        printf("Fast-forward: '%s' is now at %.8s\n", repo->HEAD, source_hash ? source_hash : "empty" );
        return;
    }

    char* lca_hash = repo_find_lca(repo, current_hash, source_hash);
    if (lca_hash) {
        printf("Merge Base (LCA): %.8s\n", lca_hash);
    } else {
        printf("No common ancestor found - proceeding with full union merge.\n");
    }

    TreeNode* tree_b = NULL;
    if (source_hash) {
        Blob* src_commit_blob = object_store_get(repo->store, source_hash);
        if (src_commit_blob) {
            Commit* src_commit = commit_deserialize(src_commit_blob->content);
            if (src_commit) {
                Blob* tree_blob = object_store_get(repo->store, src_commit->tree_hash);
                if (tree_blob) {
                    tree_b = tree_node_create_dir("root");
                    TreeNode* marker = tree_node_create_file(source_branch, src_commit->tree_hash);
                    tree_node_add_child(tree_b, marker);
                }
                commit_free(src_commit);
            }
        }
    }

    TreeNode* merged = tree_merge_union(repo->staging->root, tree_b ? tree_b : repo->staging->root);
    if (tree_b) tree_node_free(tree_b);
    tree_node_free(repo->staging->root);
    repo->staging->root = merged;

    char merge_msg[256];
    snprintf(merge_msg, sizeof(merge_msg), "Merge branch '%s' into '%s'", source_branch, repo->HEAD);
    repo_commit(repo, merge_msg);

    free(lca_hash);
}

