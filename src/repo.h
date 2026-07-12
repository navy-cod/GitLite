#ifndef REPO_H
#define REPO_H

#include "fs_store.h"
#include "staging_area.h"
#include "btree.h"
#include "commit.h"
#include "queue.h"
#include "tree.h"

#define GLITE_DIR      ".glite"
#define OBJECTS_DIR    ".glite/objects"
#define REFS_DIR       ".glite/refs"
#define HEADS_DIR      ".glite/refs/heads"
#define HEAD_FILE      ".glite/HEAD"

typedef struct {
    FsStore*     store;     
    StagingArea* staging;  
    BTree*       index;    
    char*        HEAD;      
} Repository;

Repository* repo_init(void);

void repo_add(Repository* repo, const char* path);

void repo_commit(Repository* repo, const char* message);

void repo_log(Repository* repo);

void repo_status(Repository* repo);

void repo_diff(Repository* repo, const char* path);

void repo_create_branch(Repository* repo, const char* name);

void repo_switch_branch(Repository* repo, const char* name);

char* repo_find_lca(Repository* repo, const char* hash1, const char* hash2);

void repo_merge(Repository* repo, const char* source_branch);

char* repo_read_branch_ref(Repository* repo, const char* branch_name);

void repo_update_branch_ref(Repository* repo, const char* branch_name, const char* hash);


#endif /* REPO_H */
