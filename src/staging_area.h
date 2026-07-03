#ifndef STAGING_AREA_H
#define STAGING_AREA_H

#include "tree.h"
#include "objec_store.h"

typedef struct {
    TreeNode* root;
    ObjectStore* store;
} StagingArea;

StagingArea* staging_area_int(void);

int staging_area_add(StagingArea* sa, const char* path);

void staging_area_list(StagingArea* sa);

void staging_area_free(StagingArea* sa);

#endif
