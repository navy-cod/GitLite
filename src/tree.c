#include "tree.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

TreeNode* tree_node_create_file(const char* name, const char* blob_hash) {
    TreeNode* node = malloc(sizeof(TreeNode));
    if (!node) { fprintf(stderr, "tree_node_create_file: malloc failed\n"); exit(1); }

    node->name = strdup(name);
    node->is_dir = 0;
    node->blob_hash = strdup(blob_hash);
    node->children = NULL;
    node->next = NULL;
    return node;
}

TreeNode* tree_node_create_dir(const char* name) {
    TreeNode* node = malloc(sizeof(TreeNode));
    if (!node) { fprintf(stderr, "tree_node_create_dir: malloc failed\n"); exit(1); }

    node->name = strdup(name);
    node->is_dir = 1;
    node->blob_hash = NULL;
    node->children = NULL;
    node->next = NULL;
    return node;
}

void tree_node_add_child(TreeNode* parent, TreeNode* child) {
    child->next = parent->children;
    parent->children = child;
}

TreeNode* tree_node_find_child(TreeNode* parent, const char* name) {
      /*walk sibling linked list looking for matching name */
    TreeNode* curr = parent->children;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void tree_node_print(TreeNode* node, int depth) {
    if (!node) return;
    //print indentation: 2 spaces per depth level
    for (int i = 0; i < depth; i++) printf("  ");

    if (node->is_dir) {
        printf("[dir] %s/\n", node->name);
        TreeNode* child = node->children;
        while (child != NULL) {
            tree_node_print(child, depth + 1);
            child = child->next;
        }
    } else {
        printf("[file] %s (hash: %.8s)\n", node->name, node->blob_hash ? node->blob_hash : "none");
    }
}

void tree_node_free(TreeNode* node) {
    if (!node) return;

    TreeNode* child = node->children;
    while (child != NULL) {
        TreeNode* next_child = child->next;
        tree_node_free(child);
        child = next_child;
    }

    free(node->name);
    free(node->blob_hash);

    free(node);
}

