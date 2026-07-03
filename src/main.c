#include <stdio.h>
#include <string.h>

#include "staging_area.h"

/* Forward Declarations */
static void cmd_add(StagingArea* sa, int argc, char* argv[]);
static void cmd_ls_files(StagingArea* sa);
static void print_usage(void);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    StagingArea* sa = staging_area_int();

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "add") == 0) {
        cmd_add(sa, argc, argv);
    } else if (strcmp(subcommand, "ls-files") == 0) {
        cmd_ls_files(sa);
    } else if (strcmp(subcommand, "help") == 0) {
        print_usage();
    } else {
        fprintf(stderr, "glite: unknown command '%s'\n", subcommand);
        fprintf(stderr, "Run 'glite help' for usage information.\n");
        staging_area_free(sa);
        return 1;
    }

    staging_area_free(sa);
    return 0;
}

static void cmd_add(StagingArea* sa, int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "glite add: no path specified\n");
        fprintf(stderr, "Usage: glite add <path> [<path>...]\n");
        return;
    }

    for (int i = 2; i < argc; i++) {
        staging_area_add(sa, argv[i]);
    }

    printf("\n");
    staging_area_list(sa);
}

static void cmd_ls_files(StagingArea* sa) {
    staging_area_list(sa);
}

static void print_usage(void) {
    printf("Usage: glite <command> [<args>]\n\n");
    printf("Commands:\n");
    printf("  add <path> [<path>...]   Add files to the staging area\n");
    printf("  ls-files                  List files in the staging area\n");
    printf("  help                      Show this help message\n");
}