#ifndef COMMIT_H
#define COMMIT_H

typedef struct Commit {
    char* hash;
    char* parent_hash;
    char* tree_hash;
    char* message;
    char* timestamp;
} Commit;

Commit* commit_create(const char* parent_hash, const char* tree_hash, const char* message, const char* timestamp);

char* commit_serialize(Commit* c);

Commit* commit_deserialize(const char* data);

void commit_free(Commit* c);

#endif