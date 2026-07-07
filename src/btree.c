#include "btree.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static BTreeNode* btree_node_alloc(int is_leaf) {
    BTreeNode* node = calloc(1, sizeof(BTreeNode));
    if (!node) { fprintf(stderr, "btree_node_alloc: calloc failed\n"); exit(1); }
    node->is_leaf = is_leaf;
    node->n       = 0;
    return node;
}

static const char* node_search(BTreeNode* node, const char* key) {
    
    int i = 0;
    while (i < node->n && strcmp(key, node->keys[i]) > 0) i++;

    if (i < node->n && strcmp(key, node->keys[i]) == 0)
        return node->vals[i];   

    if (node->is_leaf) return NULL; 

    return node_search(node->children[i], key);
}

const char* btree_search(BTree* tree, const char* key) {
    if (!tree->root) return NULL;
    return node_search(tree->root, key);
}

static void split_child(BTreeNode* parent, int i) {
    BTreeNode* full  = parent->children[i];
    BTreeNode* right = btree_node_alloc(full->is_leaf);

    right->n = BTREE_T - 1;
    for (int j = 0; j < BTREE_T - 1; j++) {
        right->keys[j] = full->keys[j + BTREE_T];
        right->vals[j] = full->vals[j + BTREE_T];
        full->keys[j + BTREE_T] = NULL;
        full->vals[j + BTREE_T] = NULL;
    }

    if (!full->is_leaf) {
        for (int j = 0; j < BTREE_T; j++) {
            right->children[j] = full->children[j + BTREE_T];
            full->children[j + BTREE_T] = NULL;
        }
    }

    char* median_key = full->keys[BTREE_T - 1];
    char* median_val = full->vals[BTREE_T - 1];
    full->keys[BTREE_T - 1] = NULL;
    full->vals[BTREE_T - 1] = NULL;
    full->n = BTREE_T - 1;

    for (int j = parent->n; j > i; j--) {
        parent->keys[j]         = parent->keys[j - 1];
        parent->vals[j]         = parent->vals[j - 1];
        parent->children[j + 1] = parent->children[j];
    }

    parent->keys[i]         = median_key;
    parent->vals[i]         = median_val;
    parent->children[i + 1] = right;
    parent->n++;
}

static void insert_nonfull(BTreeNode* node, const char* key, const char* value) {
    int i = node->n - 1;

    if (node->is_leaf) {
        while (i >= 0) {
            int cmp = strcmp(key, node->keys[i]);
            if (cmp == 0) {
                free(node->vals[i]);
                node->vals[i] = strdup(value);
                return;
            }
            if (cmp > 0) break;
            node->keys[i + 1] = node->keys[i];
            node->vals[i + 1] = node->vals[i];
            i--;
        }
        node->keys[i + 1] = strdup(key);
        node->vals[i + 1] = strdup(value);
        node->n++;
    } else {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) i--;

        if (i >= 0 && strcmp(key, node->keys[i]) == 0) {
            free(node->vals[i]);
            node->vals[i] = strdup(value);
            return;
        }

        i++;  

        if (node->children[i]->n == 2 * BTREE_T - 1) {
            split_child(node, i);
            if (strcmp(key, node->keys[i]) == 0) {
                free(node->vals[i]);
                node->vals[i] = strdup(value);
                return;
            }
            if (strcmp(key, node->keys[i]) > 0) i++;
        }
        insert_nonfull(node->children[i], key, value);
    }
}

void btree_insert(BTree* tree, const char* key, const char* value) {
    BTreeNode* root = tree->root;

    if (!root) {
        tree->root = btree_node_alloc(1);
        tree->root->keys[0] = strdup(key);
        tree->root->vals[0] = strdup(value);
        tree->root->n       = 1;
        return;
    }

    if (root->n == 2 * BTREE_T - 1) {
        BTreeNode* new_root = btree_node_alloc(0);
        new_root->children[0] = root;
        tree->root = new_root;
        split_child(new_root, 0);
        insert_nonfull(new_root, key, value);
    } else {
        insert_nonfull(root, key, value);
    }
}

static void node_free(BTreeNode* node) {
    if (!node) return;

    if (!node->is_leaf) {
        for (int i = 0; i <= node->n; i++)
            node_free(node->children[i]);
    }

    for (int i = 0; i < node->n; i++) {
        free(node->keys[i]);
        free(node->vals[i]);
    }

    free(node);
}

void btree_free(BTree* tree) {
    if (!tree) return;
    node_free(tree->root);
    free(tree);
}

BTree* btree_init(void) {
    BTree* tree = malloc(sizeof(BTree));
    if (!tree) { fprintf(stderr, "btree_init: malloc failed\n"); exit(1); }
    tree->root = NULL;
    return tree;
}
