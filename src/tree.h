#ifndef TREE_H
#define TREE_H

typedef struct TreeNode {
    char* name;
    int is_dir;
    char* blob_hash;
    struct TreeNode* children;
    struct TreeNode* next;
} TreeNode;

TreeNode* tree_node_create_file(const char* name, const char* blob_hash);
TreeNode* tree_node_create_dir(const char* name);

void tree_node_add_child(TreeNode* parent, TreeNode* child);

TreeNode* tree_node_find_child(TreeNode* parent, const char* name);

void tree_node_print(TreeNode* node, int depth);

void tree_node_free(TreeNode* node);


#endif