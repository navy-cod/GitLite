#include "commit.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static unsigned int fnv1a(const char* data) {
    unsigned int hash = 2166136261u;
    const unsigned char* p = (const unsigned char*)data;
    while (*p) {
        hash ^= *p++;
        hash *= 16777619u;
    }
    return hash;
}

static void compute_commit_hash(Commit* c, char* out_hash) {
    const char* parent = c->parent_hash ? c->parent_hash : "null";

    size_t len = strlen(parent) + strlen(c->tree_hash) + strlen(c->message) + strlen(c->timestamp) + 1;

    char* combined = malloc(len);
    if (!combined) { fprintf(stderr, "compute_commit_hash: malloc failed\n"); exit(1); }

    strcpy(combined, parent);
    strcat(combined, c->tree_hash);
    strcat(combined, c->message);
    strcat(combined, c->timestamp);

    snprintf(out_hash, 9, "%08x", fnv1a(combined));
    free(combined);
}

Commit* commit_create(const char* parent_hash, const char* tree_hash, const char* message, const char* timestamp) {
    Commit* c = malloc(sizeof(Commit));
    if (!c) { fprintf(stderr, "commit_create: malloc failed\n"); exit(1); }

    c->parent_hash = parent_hash ? strdup(parent_hash) : NULL;
    c->tree_hash = strdup(tree_hash);
    c->message = strdup(message);
    c->timestamp = strdup(timestamp);

    char hash_buf[9];
    compute_commit_hash(c, hash_buf);
    c->hash = strdup(hash_buf);

    return c;
}

char* commit_serialize(Commit* c) {
    const char* parent = c->parent_hash ? c->parent_hash : "null";
    size_t len = strlen("hash:") + strlen(c->hash) + 1
               + strlen("parent:") + strlen(parent) + 1
               + strlen("tree:") + strlen(c->tree_hash) + 1
               + strlen("message:") + strlen(c->message) + 1
               + strlen("timestamp:") + strlen(c->timestamp) + 1
               + 1;
    char* buf = malloc(len);
    if (!buf) { fprintf(stderr, "commit_serialize: malloc failed\n"); exit(1); }
    
    snprintf(buf, len, "hash:%s\nparent:%s\ntree:%s\nmessage:%s\ntimestamp:%s\n",
             c->hash, parent, c->tree_hash, c->message, c->timestamp);
    return buf;
}

Commit* commit_deserialize(const char* data) {
    char* copy = strdup(data);
    char* saveptr;

    char* hash = NULL;
    char* parent = NULL;
    char* tree = NULL;
    char* message = NULL;
    char* timestamp = NULL;

    char* line = strtok_r(copy, "\n", &saveptr);
    while (line) {
        char* colon = strchr(line, ':');
        if (!colon) { line = strtok_r(NULL, "\n", &saveptr); continue; }
        *colon = '\0';
        char* key = line;
        char* val = colon + 1;

        if (strcmp(key, "hash") == 0) hash = strdup(val);
        else if (strcmp(key, "parent") == 0) parent = strdup(val);
        else if (strcmp(key, "tree") == 0) tree = strdup(val);
        else if (strcmp(key, "message") == 0) message = strdup(val);
        else if (strcmp(key, "timestamp") == 0) timestamp = strdup(val);

        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(copy);

    if (!hash || !tree || !message || !timestamp) {
        fprintf(stderr, "commit_deserialize: malformed data\n");
        free(hash); free(parent); free(tree); free(message); free(timestamp);
        return NULL;
    }

    Commit* c = malloc(sizeof(Commit));
    if (!c) { fprintf(stderr, "commit_deserialize: malloc failed\n"); exit(1); }

    c->hash = hash;
    c->parent_hash = (parent && strcmp(parent, "null") == 0) ? NULL : parent;
    if (parent && strcmp(parent, "null") == 0) free(parent);
    c->tree_hash = tree;
    c->message = message;
    c->timestamp = timestamp;

    return c;
}

void commit_free(Commit* c) {
    if (!c) return;
    free(c->hash);
    free(c->parent_hash);
    free(c->tree_hash);
    free(c->message);
    free(c->timestamp);
    free(c);
}