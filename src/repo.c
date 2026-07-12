#include "repo.h"
#include "diff.h"
#include "working_dir.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

static int dir_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static int mkdir_safe(const char* path) {
    int ret = mkdir(path, 0755);
    if (ret == -1 && errno == EEXIST) return 0;
    return ret;
}

static int write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }

    char* buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static unsigned int fnv1a(const char* data, size_t len) {
    unsigned int hash = 2166136261u;
    const unsigned char* p = (const unsigned char*)data;
    for (size_t i = 0; i < len; i++) {
        hash ^= p[i];
        hash *= 16777619u;
    }
    return hash;
}

static void hash_content(const char* data, size_t len, char* out) {
    snprintf(out, 9, "%08x", fnv1a(data, len));
}

static void get_timestamp(char* buf, size_t size) {
    time_t     now = time(NULL);
    struct tm* tm  = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm);
}

static char* read_head_branch(void) {
    char* content = read_file(HEAD_FILE);
    if (!content) return strdup("master");

    const char* prefix = "ref: refs/heads/";
    char* branch;
    if (strncmp(content, prefix, strlen(prefix)) == 0) {
        branch = strdup(content + strlen(prefix));
    } else {
        branch = strdup(content);
    }
    free(content);

    size_t len = strlen(branch);
    if (len > 0 && branch[len - 1] == '\n') branch[len - 1] = '\0';

    return branch;
}

static void write_head_branch(const char* branch_name) {
    char buf[256];
    snprintf(buf, sizeof(buf), "ref: refs/heads/%s\n", branch_name);
    write_file(HEAD_FILE, buf);
}

static char* read_branch_hash(const char* branch_name) {
    char path[512];
    snprintf(path, sizeof(path), HEADS_DIR "/%s", branch_name);

    char* content = read_file(path);
    if (!content) return NULL;

    size_t len = strlen(content);
    if (len > 0 && content[len - 1] == '\n') content[len - 1] = '\0';

    if (strlen(content) == 0) { free(content); return NULL; }
    return content;
}

static void write_branch_hash(const char* branch_name, const char* hash) {
    char path[512];
    snprintf(path, sizeof(path), HEADS_DIR "/%s", branch_name);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s\n", hash ? hash : "");
    write_file(path, buf);
}

static int branch_exists(const char* branch_name) {
    char path[512];
    snprintf(path, sizeof(path), HEADS_DIR "/%s", branch_name);
    return file_exists(path);
}

static TreeNode* find_path_in_tree(TreeNode* root, const char* path) {
    char* path_copy = strdup(path);
    char* saveptr;
    TreeNode* current = root;
    TreeNode* result   = NULL;

    char* tok = strtok_r(path_copy, "/", &saveptr);
    while (tok != NULL) {
        TreeNode* next = tree_node_find_child(current, tok);
        if (!next) { result = NULL; break; }
        result  = next;
        current = next;
        tok = strtok_r(NULL, "/", &saveptr);
    }

    free(path_copy);
    return result;
}

Repository* repo_init(void) {
    Repository* repo = malloc(sizeof(Repository));
    if (!repo) { fprintf(stderr, "repo_init: malloc failed\n"); exit(1); }

    if (!dir_exists(GLITE_DIR)) {
        mkdir_safe(GLITE_DIR);
        mkdir_safe(OBJECTS_DIR);
        mkdir_safe(REFS_DIR);
        mkdir_safe(HEADS_DIR);
        write_head_branch("master");
        write_branch_hash("master", NULL);
        printf("Initialised empty GitLite repository in .glite/\n");
    }

    repo->store   = fs_store_init(GLITE_DIR);
    repo->staging = staging_area_init();
    repo->index   = btree_init();
    repo->HEAD    = read_head_branch();

    return repo;
}

void repo_add(Repository* repo, const char* path) {
    if (staging_area_add(repo->staging, path) != 0) return;

    size_t file_size;
    char* content = working_dir_read_file(path, &file_size);
    if (!content) return;

    char hash[9];
    hash_content(content, file_size, hash);

    fs_store_put(repo->store, hash, content, file_size);

    free(content);
    btree_insert(repo->index, path, hash);
}

void repo_commit(Repository* repo, const char* message) {
    char* tree_str  = tree_serialize(repo->staging->root);
    size_t tree_len = strlen(tree_str);

    char tree_hash[9];
    hash_content(tree_str, tree_len, tree_hash);

    fs_store_put(repo->store, tree_hash, tree_str, tree_len);
    free(tree_str);

    char* parent_hash = read_branch_hash(repo->HEAD);

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    Commit* c = commit_create(parent_hash, tree_hash, message, timestamp);
    free(parent_hash);

    char* commit_str  = commit_serialize(c);
    size_t commit_len = strlen(commit_str);
    fs_store_put(repo->store, c->hash, commit_str, commit_len);
    free(commit_str);

    write_branch_hash(repo->HEAD, c->hash);

    printf("[%s %.8s] %s\n", repo->HEAD, c->hash, message);

    staging_area_free(repo->staging);
    repo->staging = staging_area_init();

    btree_free(repo->index);
    repo->index = btree_init();

    commit_free(c);
}

void repo_log(Repository* repo) {
    char* hash = read_branch_hash(repo->HEAD);

    if (!hash) {
        printf("No commits yet on branch '%s'.\n", repo->HEAD);
        return;
    }

    char* current = hash;

    while (current) {
        size_t size;
        char* data = fs_store_get(repo->store, current, &size);
        if (!data) {
            fprintf(stderr, "repo_log: object '%s' not found\n", current);
            free(current);
            break;
        }

        Commit* c = commit_deserialize(data);
        free(data);
        if (!c) { free(current); break; }

        printf("commit %s\n", c->hash);
        printf("Date:   %s\n", c->timestamp);
        printf("        %s\n\n", c->message);

        char* next = c->parent_hash ? strdup(c->parent_hash) : NULL;
        commit_free(c);
        free(current);
        current = next;
    }
}

void repo_status(Repository* repo) {
    char* tip_hash = read_branch_hash(repo->HEAD);

    if (!tip_hash) {
        printf("On branch %s\n\nNo commits yet.\n", repo->HEAD);
        TreeNode* work = working_dir_scan(".");
        working_dir_status(work, NULL, ".");
        tree_node_free(work);
        return;
    }

    size_t size;
    char* commit_data = fs_store_get(repo->store, tip_hash, &size);
    free(tip_hash);
    if (!commit_data) {
        fprintf(stderr, "repo_status: cannot load tip commit\n");
        return;
    }

    Commit* c = commit_deserialize(commit_data);
    free(commit_data);
    if (!c) return;

    size_t tree_size;
    char* tree_data = fs_store_get(repo->store, c->tree_hash, &tree_size);
    commit_free(c);

    if (!tree_data) {
        fprintf(stderr, "repo_status: cannot load tree object\n");
        return;
    }

    const char* ptr = tree_data;
    TreeNode* committed = tree_deserialize(&ptr);
    free(tree_data);

    TreeNode* working = working_dir_scan(".");

    printf("On branch %s\n\n", repo->HEAD);
    working_dir_status(working, committed, ".");

    tree_node_free(working);
    if (committed) tree_node_free(committed);
}

void repo_diff(Repository* repo, const char* path) {
    char* tip_hash = read_branch_hash(repo->HEAD);
    if (!tip_hash) {
        printf("No commits yet — nothing to diff against.\n");
        return;
    }

    size_t commit_size;
    char* commit_data = fs_store_get(repo->store, tip_hash, &commit_size);
    free(tip_hash);
    if (!commit_data) {
        fprintf(stderr, "repo_diff: cannot load tip commit\n");
        return;
    }

    Commit* c = commit_deserialize(commit_data);
    free(commit_data);
    if (!c) return;

    size_t tree_size;
    char* tree_data = fs_store_get(repo->store, c->tree_hash, &tree_size);
    commit_free(c);
    if (!tree_data) {
        fprintf(stderr, "repo_diff: cannot load tree\n");
        return;
    }

    const char* ptr = tree_data;
    TreeNode* committed_root = tree_deserialize(&ptr);
    free(tree_data);

    char* committed_content = NULL;

    if (committed_root) {
        TreeNode* file_node = find_path_in_tree(committed_root, path);
        if (file_node && !file_node->is_dir) {
            size_t blob_size;
            committed_content = fs_store_get(repo->store, file_node->blob_hash, &blob_size);
        }
    }

    working_dir_diff_file(path, committed_content ? committed_content : "", path);

    free(committed_content);
    if (committed_root) tree_node_free(committed_root);
}

void repo_create_branch(Repository* repo, const char* name) {
    if (branch_exists(name)) {
        fprintf(stderr, "glite: branch '%s' already exists\n", name);
        return;
    }

    char* current_hash = read_branch_hash(repo->HEAD);
    write_branch_hash(name, current_hash);
    free(current_hash);

    printf("Branch '%s' created.\n", name);
}

void repo_switch_branch(Repository* repo, const char* name) {
    if (!branch_exists(name)) {
        fprintf(stderr, "glite: branch '%s' does not exist\n", name);
        return;
    }

    free(repo->HEAD);
    repo->HEAD = strdup(name);
    write_head_branch(name);

    printf("Switched to branch '%s'.\n", name);
}
typedef struct {
    char* hash;
    int   colour;
} BFSEntry;

char* repo_find_lca(Repository* repo, const char* hash1, const char* hash2) {
    if (!hash1 || !hash2) return NULL;

    HashMap* visited = hashmap_init(64);
    Queue*   queue   = queue_init();

    BFSEntry* e1 = malloc(sizeof(BFSEntry));
    BFSEntry* e2 = malloc(sizeof(BFSEntry));
    if (!e1 || !e2) { fprintf(stderr, "repo_find_lca: malloc failed\n"); exit(1); }

    e1->hash = strdup(hash1); e1->colour = 0;
    e2->hash = strdup(hash2); e2->colour = 1;

    queue_enqueue(queue, e1);
    queue_enqueue(queue, e2);

    char* lca = NULL;

    while (!queue_is_empty(queue) && !lca) {
        BFSEntry* entry  = (BFSEntry*)queue_dequeue(queue);
        char*     hash   = entry->hash;
        int       colour = entry->colour;
        free(entry);

        char* seen = (char*)hashmap_get(visited, hash);
        if (seen) {
            if (seen[0] != (char)('0' + colour)) lca = strdup(hash);
            free(hash);
            continue;
        }

        hashmap_put(visited, hash, colour == 0 ? "0" : "1");

        size_t sz;
        char* data = fs_store_get(repo->store, hash, &sz);
        if (data) {
            Commit* c = commit_deserialize(data);
            free(data);
            if (c && c->parent_hash) {
                BFSEntry* pe = malloc(sizeof(BFSEntry));
                if (!pe) { fprintf(stderr, "repo_find_lca: malloc failed\n"); exit(1); }
                pe->hash   = strdup(c->parent_hash);
                pe->colour = colour;
                queue_enqueue(queue, pe);
            }
            commit_free(c);
        }

        free(hash);
    }

    while (!queue_is_empty(queue)) {
        BFSEntry* leftover = (BFSEntry*)queue_dequeue(queue);
        free(leftover->hash);
        free(leftover);
    }

    queue_free(queue);
    hashmap_free(visited);
    return lca;
}

void repo_merge(Repository* repo, const char* source_branch) {
    if (!branch_exists(source_branch)) {
        fprintf(stderr, "glite merge: branch '%s' does not exist\n",
                source_branch);
        return;
    }
    if (strcmp(repo->HEAD, source_branch) == 0) {
        fprintf(stderr, "glite merge: cannot merge a branch into itself\n");
        return;
    }

    char* current_hash = read_branch_hash(repo->HEAD);
    char* source_hash  = read_branch_hash(source_branch);

    if (!current_hash) {
        if (source_hash) write_branch_hash(repo->HEAD, source_hash);
        printf("Fast-forward: '%s' is now at %.8s\n",
               repo->HEAD, source_hash ? source_hash : "empty");
        free(source_hash);
        return;
    }

    char* lca = repo_find_lca(repo, current_hash, source_hash);
    if (lca) {
        printf("Merge base (LCA): %.8s\n", lca);
        free(lca);
    }

    TreeNode* tree_b = NULL;
    if (source_hash) {
        size_t sz;
        char* cd = fs_store_get(repo->store, source_hash, &sz);
        if (cd) {
            Commit* sc = commit_deserialize(cd);
            free(cd);
            if (sc) {
                size_t tsz;
                char* td = fs_store_get(repo->store, sc->tree_hash, &tsz);
                if (td) {
                    const char* ptr = td;
                    tree_b = tree_deserialize(&ptr);
                    free(td);
                }
                commit_free(sc);
            }
        }
    }

    TreeNode* merged = tree_merge_union(repo->staging->root, tree_b ? tree_b : repo->staging->root);
    if (tree_b) tree_node_free(tree_b);

    tree_node_free(repo->staging->root);
    repo->staging->root = merged;

    char msg[256];
    snprintf(msg, sizeof(msg),
             "Merge branch '%s' into '%s'", source_branch, repo->HEAD);
    repo_commit(repo, msg);

    free(current_hash);
    free(source_hash);
}


char* repo_read_branch_ref(Repository* repo, const char* branch_name) {
    (void)repo;   
    return read_branch_hash(branch_name);
}

void repo_update_branch_ref(Repository* repo, const char* branch_name, const char* hash) {
    (void)repo;
    write_branch_hash(branch_name, hash);
}

void repo_free(Repository* repo) {
    if (!repo) return;
    fs_store_free(repo->store);
    staging_area_free(repo->staging);
    btree_free(repo->index);
    free(repo->HEAD);
    free(repo);
}
