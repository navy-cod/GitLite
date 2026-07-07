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

static void buf_append(char** buf, size_t* pos, size_t* cap, const char* src, size_t len) {
    while (*pos + len >= *cap) {
        *cap = (*cap == 0) ? 64 : *cap * 2;
        char* new_buf = realloc(*buf, *cap);
        if (!new_buf) { fprintf(stderr, "buf_append: realloc failed\n"); exit(1); 
        }
        *buf = new_buf;
    }
    memcpy(*buf + *pos, src, len);
    *pos += len;
    (*buf)[*pos] = '\0';  
}

static void buff_append_str(char** buf, size_t* pos, size_t* cap, const char* str) {
    buf_append(buf, pos, cap, str, strlen(str));
}

static void serialize_node(TreeNode* node, char** buf, size_t* pos, size_t* cap) {
    if (!node) return;

    if (node->is_dir) {
        buff_append_str(buf, pos, cap, "(dir:"); 
        buff_append_str(buf, pos, cap, node->name);
    
        TreeNode* child = node->children;
        while (child != NULL) {
            serialize_node(child, buf, pos, cap);
            child = child->next;
        }
        buff_append_str(buf, pos, cap, ")");
    } else {
        buff_append_str(buf, pos, cap, "(file:");
        buff_append_str(buf, pos, cap, node->name);
        buff_append_str(buf, pos, cap, ":");
        buff_append_str(buf, pos, cap, node->blob_hash ? node->blob_hash : "00000000");
        buff_append_str(buf, pos, cap, ")");
    }
}

char* tree_serialize(TreeNode* node) {
    char* buf = NULL;
    size_t pos = 0;
    size_t cap = 0;

    serialize_node(node, &buf, &pos, &cap);
    return buf;
}

void tree_compute_hash(TreeNode* node, char* out_hash) {
    char* serialized = tree_serialize(node);

    unsigned int hash = 2166136261u;
    const unsigned char* p = (const unsigned char*)serialized;
    while (p && *p) {
        hash ^= *p;
        hash *= 16777619u;
        p++;
    }

    snprintf(out_hash, 9, "%08x", hash);
    free(serialized);
}

static TreeNode* merge_dir(TreeNode* a, TreeNode* b);

static TreeNode* copy_subtree(TreeNode* src) {
    if (!src) return NULL;

    TreeNode* copy;
    if (src->is_dir) {
        copy = tree_node_create_dir(src->name);
        TreeNode* child = src->children;

        TreeNode* reversed = NULL;
        while (child != NULL) {
            TreeNode* child_copy = copy_subtree(child);
            child_copy->next = reversed;
            reversed = child_copy;
            child = child->next;
        }
        TreeNode* c = reversed;
        while (c != NULL) {
            TreeNode* next = c->next;
            c->next = NULL;
            tree_node_add_child(copy, c);
            c = next;
        }
    } else {
        copy = tree_node_create_file(src->name, src->blob_hash);
    }
    return copy;
}

static TreeNode* merge_dir(TreeNode* a, TreeNode* b) {
    const char* name = a ? a->name : b->name;
    TreeNode* result = tree_node_create_dir(name);

    TreeNode* ca = a ? a->children : NULL;
    while (ca != NULL) {
        TreeNode* cb = b ? tree_node_find_child(b, ca->name) : NULL;

        if (!cb) {
            tree_node_add_child(result, copy_subtree(ca));
        } else if (ca->is_dir && cb->is_dir) {
            if (strcmp(ca->blob_hash, cb->blob_hash) == 0) {
                tree_node_add_child(result, copy_subtree(ca));
            } else {
                tree_node_add_child(result, copy_subtree(ca));

                size_t conflict_len = strlen("CONFLICT:") + strlen(cb->name) + 1;
                char* conflict_name = malloc(conflict_len);
                if (!conflict_name) {
                    fprintf(stderr, "merge_dir: malloc failed");
                    exit(1);
                }
                snprintf(conflict_name, conflict_len, "CONFLICT:%s", cb->name);

                TreeNode* conflict_node = tree_node_create_file(conflict_name, cb->blob_hash);
                free(conflict_name);

                tree_node_add_child(result, conflict_node);
            }
        } else {
            tree_node_add_child(result, copy_subtree(ca));

            size_t conflict_len = strlen("CONFLICT:") + strlen(cb->name) + 1;
            char* conflict_name = malloc(conflict_len);
            if (!conflict_name) {
                fprintf(stderr, "merge_dir: malloc failed\n");
                exit(1);
            }
            snprintf(conflict_name, conflict_len, "CONFLICT:%s", cb->name);

            TreeNode* conflict_node = copy_subtree(cb);
            free(conflict_node->name);
            conflict_node->name = strdup(conflict_name);
            free(conflict_name);

            tree_node_add_child(result, conflict_node);
        }
        
        ca = ca->next;
    }

    TreeNode* cb = b ? b->children : NULL;
    while (cb != NULL) {
        if (!a || tree_node_find_child(a, cb->name)) {
            tree_node_add_child(result, copy_subtree(cb));
        }
        cb = cb->next;
    }

    return result;
}

TreeNode* tree_merge_union(TreeNode* a, TreeNode* b) {
    return merge_dir(a, b);
}

static char* parse_token(const char** input, char stop_a, char stop_b) {
    const char* start = *input;
    while (**input && **input != stop_a && **input != stop_b) {
        (*input)++;
    }
    size_t len = (size_t)(*input - start);
    char* tok = malloc(len + 1);
    if (!tok) { fprintf(stderr, "parse_token: malloc failed\n"); exit(1); }
    memcpy(tok, start, len);
    tok[len] = '\0';
    return tok;
}

TreeNode* tree_deserialize(const char** input) {
    if (!input || !*input || **input != '(') return NULL;
    (*input)++;

    char* type = parse_token(input, ':', ')');

    if (**input != ':') {
        free(type);
        return NULL;
    }
    (*input)++;

    char* name = parse_token(input, ':', ')');

    TreeNode* node = NULL;

    if (strcmp(type, "dir") == 0) {
        node = tree_node_create_dir(name);

        TreeNode* children_head = NULL;
        while (*input && **input == '(') {
            TreeNode* child = tree_deserialize(input);
            if (!child) break;
            child->next = children_head;
            children_head = child;
        }

        TreeNode* reversed = NULL;
        while (children_head) {
            TreeNode* next = children_head->next;
            children_head->next = reversed;
            reversed = children_head;
            children_head = next;
        }

        while (reversed) {
            TreeNode* next = reversed->next;
            reversed->next = NULL;
            tree_node_add_child(node, reversed);
            reversed = next;
        }

    } else if (strcmp(type, "file") == 0) {
        if (**input == ':') (*input)++;
        char* hash = parse_token(input, ')', ':');
        node = tree_node_create_file(name, hash);
        free(hash);
    }

    if (*input && **input == ')') (*input)++;

    free(type);
    free(name);
    return node;
}
