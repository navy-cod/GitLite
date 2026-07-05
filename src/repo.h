#ifndef REPO_H
#define REPO_H

#include "objec_store.h"
#include "staging_area.h"
#include "hashmap.h"
#include "commit.h"
#include "queue.h"

typedef struct {
    ObjectStore* store;
    StagingArea* staging;
    HashMap* branches;
    char* HEAD;
} Repository;

Repository* repo_init(void);

void repo_commit(Repository* repo, const char* message);

void repo_log(Repository* repo);

void repo_free(Repository* repo);

void repo_create_branch(Repository* repo, const char* name);

void repo_switch_branch(Repository* repo, const char* name);

char* repo_find_lca(Repository* repo, const char* hash1, const char* hash2);

void repo_merge(Repository* repo, const char* source_branch);

// void repo_list_branches(Repository* repo);

#endif
