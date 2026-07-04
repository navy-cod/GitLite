#include <stdio.h>
#include <string.h>
#include "repo.h"

int main(void) {
    Repository* repo = repo_init();

    /* ── Commit 1 ─────────────────────────────────────────────────── */
    printf("=== Staging files for commit 1 ===\n");
    staging_area_add(repo->staging, "src/main.c");
    staging_area_add(repo->staging, "src/blob.c");
    printf("\n");

    printf("=== Committing ===\n");
    repo_commit(repo, "initial commit");
    printf("\n");

    /* ── Commit 2 ─────────────────────────────────────────────────── */
    printf("=== Staging files for commit 2 ===\n");
    staging_area_add(repo->staging, "src/hashmap.c");
    printf("\n");

    printf("=== Committing ===\n");
    repo_commit(repo, "add hashmap implementation");
    printf("\n");

    /* ── Commit 3 ─────────────────────────────────────────────────── */
    printf("=== Staging files for commit 3 ===\n");
    staging_area_add(repo->staging, "src/tree.c");
    staging_area_add(repo->staging, "src/staging_area.c");
    printf("\n");

    printf("=== Committing ===\n");
    repo_commit(repo, "add tree and staging area");
    printf("\n");

    /* ── Log ──────────────────────────────────────────────────────── */
    printf("=== glite log ===\n");
    repo_log(repo);

    repo_free(repo);
    return 0;
}
