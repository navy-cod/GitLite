#include <stdio.h>
#include <string.h>
#include "diff.h"
#include "working_dir.h"
#include "repo.h"

/* ── Test 1: Pure diff engine ─────────────────────────────────────── */
static void test_diff_engine(void) {
    printf("========================================\n");
    printf("Test 1: LCS diff engine\n");
    printf("========================================\n\n");

    const char* old_text =
        "int main(void) {\n"
        "    printf(\"hello\\n\");\n"
        "    return 0;\n"
        "}\n";

    const char* new_text =
        "#include <stdio.h>\n"
        "\n"
        "int main(void) {\n"
        "    printf(\"GitLite Toolchain Ready.\\n\");\n"
        "    return 0;\n"
        "}\n";

    DiffResult* result = diff_compute(old_text, new_text);
    diff_print(result, "main.c (old)", "main.c (new)");
    diff_result_free(result);
    printf("\n");
}

/* ── Test 2: Identical files ──────────────────────────────────────── */
static void test_no_changes(void) {
    printf("========================================\n");
    printf("Test 2: Identical files\n");
    printf("========================================\n\n");

    const char* text = "line one\nline two\nline three\n";
    DiffResult* result = diff_compute(text, text);
    diff_print(result, "file.c (old)", "file.c (new)");
    printf("has_changes = %d (expected 0)\n", result->has_changes);
    diff_result_free(result);
    printf("\n");
}

/* ── Test 3: Empty old file (all lines added) ─────────────────────── */
static void test_all_added(void) {
    printf("========================================\n");
    printf("Test 3: All lines added\n");
    printf("========================================\n\n");

    const char* new_text = "line one\nline two\nline three\n";
    DiffResult* result   = diff_compute("", new_text);
    diff_print(result, "file.c (old)", "file.c (new)");
    diff_result_free(result);
    printf("\n");
}

/* ── Test 4: Empty new file (all lines removed) ───────────────────── */
static void test_all_removed(void) {
    printf("========================================\n");
    printf("Test 4: All lines removed\n");
    printf("========================================\n\n");

    const char* old_text = "line one\nline two\nline three\n";
    DiffResult* result   = diff_compute(old_text, "");
    diff_print(result, "file.c (old)", "file.c (new)");
    diff_result_free(result);
    printf("\n");
}

/* ── Test 5: Working directory scan ──────────────────────────────── */
static void test_working_dir_scan(void) {
    printf("========================================\n");
    printf("Test 5: Working directory scan\n");
    printf("========================================\n\n");

    TreeNode* root = working_dir_scan("src");
    if (root) {
        printf("Scanned 'src/':\n");
        tree_node_print(root, 0);
        tree_node_free(root);
    } else {
        printf("(scan returned NULL)\n");
    }
    printf("\n");
}

/* ── Test 6: Full repo add → commit → status cycle ───────────────── */
static void test_repo_status(void) {
    printf("========================================\n");
    printf("Test 6: Repo add/commit/status\n");
    printf("========================================\n\n");

    Repository* repo = repo_init();

    /* Stage two files and commit */
    staging_area_add(repo->staging, "src/main.c");
    staging_area_add(repo->staging, "src/blob.c");
    repo_commit(repo, "initial commit");
    printf("\n");

    /*
     * Simulate a status check. The "committed tree" in this module is
     * the staging area root at the moment of commit. We scan the working
     * directory for comparison.
     *
     * Note: committed_root is the staging area root AFTER the commit
     * reset it — it will be empty. This demonstrates the mechanism;
     * full persistence of the committed tree arrives in Module 6.
     * For now we scan src/ against itself to show the scan works.
     */
    TreeNode* work_tree = working_dir_scan("src");
    working_dir_status(work_tree, work_tree, ".");
    tree_node_free(work_tree);

    printf("\n");
    repo_free(repo);
}

int main(void) {
    test_diff_engine();
    test_no_changes();
    test_all_added();
    test_all_removed();
    test_working_dir_scan();
    test_repo_status();

    printf("All diff tests complete.\n");
    return 0;
}
