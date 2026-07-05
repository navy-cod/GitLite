#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repo.h"

int main(void) {
    Repository* repo = repo_init();

    /* ── Build a shared history on master ───────────────────────── */
    printf("=== Commit A on master ===\n");
    staging_area_add(repo->staging, "src/main.c");
    repo_commit(repo, "commit A: initial");
    printf("\n");

    printf("=== Commit B on master ===\n");
    staging_area_add(repo->staging, "src/blob.c");
    repo_commit(repo, "commit B: add blob");
    printf("\n");

    /* ── Create and switch to feature branch ────────────────────── */
    printf("=== Create and switch to 'feature' ===\n");
    repo_create_branch(repo, "feature");
    repo_switch_branch(repo, "feature");
    printf("\n");

    /* ── Make commits on feature ────────────────────────────────── */
    printf("=== Commit F1 on feature ===\n");
    staging_area_add(repo->staging, "src/hashmap.c");
    repo_commit(repo, "commit F1: add hashmap on feature");
    printf("\n");

    printf("=== Commit F2 on feature ===\n");
    staging_area_add(repo->staging, "src/tree.c");
    repo_commit(repo, "commit F2: add tree on feature");
    printf("\n");

    /* ── Switch back to master, make a diverging commit ─────────── */
    printf("=== Switch back to master ===\n");
    repo_switch_branch(repo, "master");
    printf("\n");

    printf("=== Commit C on master (diverging) ===\n");
    staging_area_add(repo->staging, "src/staging_area.c");
    repo_commit(repo, "commit C: add staging area on master");
    printf("\n");

    /* ── Find LCA ────────────────────────────────────────────────── */
    printf("=== Finding LCA of master and feature ===\n");
    char* master_hash  = (char*)hashmap_get(repo->branches, "master");
    char* feature_hash = (char*)hashmap_get(repo->branches, "feature");

    printf("master  tip: %.8s\n", master_hash  ? master_hash  : "none");
    printf("feature tip: %.8s\n", feature_hash ? feature_hash : "none");

    char* lca = repo_find_lca(repo, master_hash, feature_hash);
    if (lca) {
        printf("LCA found:   %.8s\n", lca);
        free(lca);
    } else {
        printf("No LCA found.\n");
    }
    printf("\n");

    /* ── Merge feature into master ───────────────────────────────── */
    printf("=== Merging 'feature' into 'master' ===\n");
    repo_merge(repo, "feature");
    printf("\n");

    /* ── Log master history ──────────────────────────────────────── */
    printf("=== git log (master) ===\n");
    repo_log(repo);

    /* ── Switch to feature and log it too ───────────────────────── */
    printf("=== git log (feature) ===\n");
    repo_switch_branch(repo, "feature");
    repo_log(repo);

    repo_free(repo);
    return 0;
}
