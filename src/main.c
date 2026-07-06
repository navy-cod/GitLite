#include <stdio.h>
#include <string.h>

#include "repo.h"
#include "working_dir.h"
#include "diff.h"

static void print_usage(void) {
    printf("usage: glite <command> [<args>]\n\n");
    printf("Commands:\n");
    printf("  add <path> [<path> ...]   Stage one or more files\n");
    printf("  commit <message>          Commit the staged snapshot\n");
    printf("  log                       Show commit history\n");
    printf("  ls-files                  List staged files\n");
    printf("  status                    Show working directory changes\n");
    printf("  diff <path>               Diff a file against committed version\n");
    printf("  help                      Show this message\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) { print_usage(); return 1; }

    Repository* repo = repo_init();
    const char* cmd  = argv[1];

    if (strcmp(cmd, "add") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite add: no path specified\n");
            repo_free(repo); return 1;
        }
        for (int i = 2; i < argc; i++) {
            staging_area_add(repo->staging, argv[i]);
        }

    } else if (strcmp(cmd, "commit") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite commit: no message provided\n");
            repo_free(repo); return 1;
        }
        repo_commit(repo, argv[2]);

    } else if (strcmp(cmd, "log") == 0) {
        repo_log(repo);

    } else if (strcmp(cmd, "ls-files") == 0) {
        staging_area_list(repo->staging);

    } else if (strcmp(cmd, "status") == 0) {
        TreeNode* work_tree = working_dir_scan(".");
        working_dir_status(work_tree, repo->staging->root, ".");
        tree_node_free(work_tree);

    } else if (strcmp(cmd, "diff") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite diff: no path specified\n");
            repo_free(repo); return 1;
        }
        working_dir_diff_file(argv[2], "", argv[2]);

    } else if (strcmp(cmd, "help") == 0) {
        print_usage();

    } else {
        fprintf(stderr, "glite: unknown command '%s'\n", cmd);
        fprintf(stderr, "Run 'glite help' for usage.\n");
        repo_free(repo); return 1;
    }

    repo_free(repo);
    return 0;
}
