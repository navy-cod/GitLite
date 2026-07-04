#ifndef REPO_H
#define REPO_H

#include "objec_store.h"
#include "staging_area.h"
#include "hashmap.h"
#include "commit.h"

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

#endif
