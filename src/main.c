#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "repo.h"
#include "network.h"

static void print_usage(void) {
    printf("usage: glite <command> [<args>]\n\n");
    printf("Commands:\n");
    printf("  init                       Initialise a new repository\n");
    printf("  add <path> [<path>...]     Stage one or more files\n");
    printf("  commit <message>           Commit the staged snapshot\n");
    printf("  log                        Show commit history\n");
    printf("  status                     Show working directory status\n");
    printf("  diff <path>                Diff a file against its committed version\n");
    printf("  branch <name>              Create a new branch\n");
    printf("  switch <name>              Switch to a branch\n");
    printf("  merge <branch>             Merge a branch into HEAD\n");
    printf("  ls-files                   List staged files\n");
    printf("  daemon [port]              Run the GitLite network daemon\n");
    printf("  push <host> <port> [branch] Push a branch to a remote daemon\n");
    printf("  pull <host> <port> [branch] Pull a branch from a remote daemon\n");
    printf("  help                       Show this message\n");
}

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);

    if (argc < 2) { print_usage(); return 1; }

    const char* cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        Repository* repo = repo_init();
        repo_free(repo);
        return 0;
    }

    if (strcmp(cmd, "help") == 0) {
        print_usage();
        return 0;
    }

    Repository* repo = repo_init();

    if (strcmp(cmd, "add") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite add: no path specified\n");
            repo_free(repo); return 1;
        }
        for (int i = 2; i < argc; i++) repo_add(repo, argv[i]);

    } else if (strcmp(cmd, "commit") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite commit: no message provided\n");
            repo_free(repo); return 1;
        }
        repo_commit(repo, argv[2]);

    } else if (strcmp(cmd, "log") == 0) {
        repo_log(repo);

    } else if (strcmp(cmd, "status") == 0) {
        repo_status(repo);

    } else if (strcmp(cmd, "diff") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite diff: no path specified\n");
            repo_free(repo); return 1;
        }
        repo_diff(repo, argv[2]);

    } else if (strcmp(cmd, "branch") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite branch: no name provided\n");
            repo_free(repo); return 1;
        }
        repo_create_branch(repo, argv[2]);

    } else if (strcmp(cmd, "switch") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite switch: no branch name provided\n");
            repo_free(repo); return 1;
        }
        repo_switch_branch(repo, argv[2]);

    } else if (strcmp(cmd, "merge") == 0) {
        if (argc < 3) {
            fprintf(stderr, "glite merge: no branch name provided\n");
            repo_free(repo); return 1;
        }
        repo_merge(repo, argv[2]);

    } else if (strcmp(cmd, "ls-files") == 0) {
        staging_area_list(repo->staging);

    } else if (strcmp(cmd, "daemon") == 0) {
        int port = (argc >= 3) ? atoi(argv[2]) : 9418;
        network_serve(repo, port);   

    } else if (strcmp(cmd, "push") == 0) {
        if (argc < 4) {
            fprintf(stderr, "glite push: usage: glite push <host> <port> [branch]\n");
            repo_free(repo); return 1;
        }
        const char* branch = (argc >= 5) ? argv[4] : repo->HEAD;
        network_push(repo, argv[2], argv[3], branch);

    } else if (strcmp(cmd, "pull") == 0) {
        if (argc < 4) {
            fprintf(stderr, "glite pull: usage: glite pull <host> <port> [branch]\n");
            repo_free(repo); return 1;
        }
        const char* branch = (argc >= 5) ? argv[4] : repo->HEAD;
        network_pull(repo, argv[2], argv[3], branch);

    } else {
        fprintf(stderr, "glite: unknown command '%s'\n", cmd);
        fprintf(stderr, "Run 'glite help' for usage.\n");
        repo_free(repo); return 1;
    }

    repo_free(repo);
    return 0;
}
