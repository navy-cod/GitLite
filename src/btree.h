#ifndef BTREE_H
#define BTREE_H

#include <stddef.h>

#define BTREE_T 2

typedef struct BTreeNode {
    int n; 
    char* keys[2 * BTREE_T - 1];
    char* vals[2 * BTREE_T - 1];
    struct BTreeNode* children[2 * BTREE_T];
    int is_leaf;
} BTreeNode;

typedef struct {
    BTreeNode* root;
} BTree;

BTree* btree_init(void);

void btree_insert(BTree* tree, const char* key, const char* value);

const char* btree_search(BTree* tree, const char* key);

void btree_free(BTree* tree);

#endif