#ifndef WORKING_DIR_H
#define WORKING_DIR_H

#include "tree.h"
#include "diff.h"

TreeNode* working_dir_scan(const char* root_path);


char* working_dir_read_file(const char* path, size_t* out_size);

void working_dir_status(TreeNode* work_root, TreeNode* committed_root, const char* repo_root);

void working_dir_diff_file(const char* filepath, const char* committed_content, const char* disk_path);

#endif 
