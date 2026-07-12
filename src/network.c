#include "network.h"

#include "queue.h"
#include "hashmap.h"
#include "tree.h"
#include "commit.h"
#include "fs_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>       
#include <sys/types.h>    
#include <sys/socket.h>  
#include <netinet/in.h>   
#include <arpa/inet.h>    
#include <netdb.h>   

static int send_all(int sockfd, const char* data, size_t len) {
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(sockfd, data + total_sent, len - total_sent, 0);
        if (sent <= 0) return -1;   
        total_sent += (size_t)sent;
    }
    return 0;
}

static int send_line(int sockfd, const char* line) {
    return send_all(sockfd, line, strlen(line));
}

static int recv_all(int sockfd, char* buf, size_t len) {
    size_t total_received = 0;
    while (total_received < len) {
        ssize_t received = recv(sockfd, buf + total_received, len - total_received, 0);
        if (received <= 0) return -1;
        total_received += (size_t)received;
    }
    return 0;
}

static char* recv_line(int sockfd) {
    size_t cap = 128;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) { fprintf(stderr, "recv_line: malloc failed\n"); exit(1); }

    while (1) {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);
        if (n <= 0) { free(buf); return NULL; }
        if (c == '\n') break;

        if (len + 1 >= cap) {
            cap *= 2;
            char* new_buf = realloc(buf, cap);
            if (!new_buf) { fprintf(stderr, "recv_line: realloc failed\n"); exit(1); }
            buf = new_buf;
        }
        buf[len++] = c;
    }

    buf[len] = '\0';
    return buf;
}

static int client_connect(const char* host, const char* port_str) {
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    struct addrinfo* rp = NULL;
    int sockfd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "network: getaddrinfo failed: %s\n", gai_strerror(err));
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd < 0) {
            continue;
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break; // Success
        }

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);

    if (sockfd < 0) {
        perror("network: connect failed");
    }
    return sockfd;
}

typedef enum { KIND_COMMIT, KIND_TREE, KIND_BLOB } ObjectKind;

typedef struct {
    char*      hash;
    ObjectKind kind;
} TransferEntry;

static void transfer_entry_free(TransferEntry* te) {
    if (!te) return;
    free(te->hash);
    free(te);
}

static void collect_blob_hashes(TreeNode* node, Queue* queue) {
    if (!node) return;

    if (node->is_dir) {
        TreeNode* child = node->children;
        while (child != NULL) {
            collect_blob_hashes(child, queue);
            child = child->next;
        }
    } else {
        TransferEntry* te = malloc(sizeof(TransferEntry));
        if (!te) { fprintf(stderr, "collect_blob_hashes: malloc failed\n"); exit(1); }
        te->hash = strdup(node->blob_hash);
        te->kind = KIND_BLOB;
        queue_enqueue(queue, te);
    }
}

static int transfer_send_objects(int sockfd, FsStore* store, const char* start_hash) {
    Queue*   queue   = queue_init();
    HashMap* visited = hashmap_init(64);
    int      ok      = 1;

    TransferEntry* seed = malloc(sizeof(TransferEntry));
    if (!seed) { fprintf(stderr, "transfer_send_objects: malloc failed\n"); exit(1); }
    seed->hash = strdup(start_hash);
    seed->kind = KIND_COMMIT;
    queue_enqueue(queue, seed);

    while (!queue_is_empty(queue)) {
        TransferEntry* entry = (TransferEntry*)queue_dequeue(queue);

        if (hashmap_get(visited, entry->hash)) {
            transfer_entry_free(entry);
            continue;
        }
        hashmap_put(visited, entry->hash, "1");

        char check_line[128];
        snprintf(check_line, sizeof(check_line), "CHECK %s\n", entry->hash);
        if (send_line(sockfd, check_line) != 0) {
            ok = 0; transfer_entry_free(entry); break;
        }

        char* response = recv_line(sockfd);
        if (!response) { ok = 0; transfer_entry_free(entry); break; }

        if (strcmp(response, "HAVE") == 0) {
            free(response);
            transfer_entry_free(entry);
            continue;  
        }
        free(response);  

        size_t size;
        char* data = fs_store_get(store, entry->hash, &size);
        if (!data) {
            fprintf(stderr, "transfer_send_objects: missing local object %s\n", entry->hash);
            transfer_entry_free(entry);
            continue;
        }

        char data_header[160];
        snprintf(data_header, sizeof(data_header), "DATA %s %zu\n",
                 entry->hash, size);

        if (send_line(sockfd, data_header) != 0 ||
            send_all(sockfd, data, size) != 0) {
            ok = 0;
            free(data);
            transfer_entry_free(entry);
            break;
        }

        char* ack = recv_line(sockfd);
        if (!ack) { ok = 0; free(data); transfer_entry_free(entry); break; }
        free(ack);   

        if (entry->kind == KIND_COMMIT) {
            Commit* c = commit_deserialize(data);
            if (c) {
                TransferEntry* tree_entry = malloc(sizeof(TransferEntry));
                if (!tree_entry) { fprintf(stderr, "malloc failed\n"); exit(1); }
                tree_entry->hash = strdup(c->tree_hash);
                tree_entry->kind = KIND_TREE;
                queue_enqueue(queue, tree_entry);

                if (c->parent_hash) {
                    TransferEntry* parent_entry = malloc(sizeof(TransferEntry));
                    if (!parent_entry) { fprintf(stderr, "malloc failed\n"); exit(1); }
                    parent_entry->hash = strdup(c->parent_hash);
                    parent_entry->kind = KIND_COMMIT;
                    queue_enqueue(queue, parent_entry);
                }
                commit_free(c);
            }
        } else if (entry->kind == KIND_TREE) {
            const char* ptr = data;
            TreeNode* tree = tree_deserialize(&ptr);
            if (tree) {
                collect_blob_hashes(tree, queue);
                tree_node_free(tree);
            }
        }

        free(data);
        transfer_entry_free(entry);
    }

    while (!queue_is_empty(queue)) {
        TransferEntry* leftover = (TransferEntry*)queue_dequeue(queue);
        transfer_entry_free(leftover);
    }
    queue_free(queue);
    hashmap_free(visited);

    if (!ok) return -1;

    if (send_line(sockfd, "DONE\n") != 0) return -1;
    char* final_ack = recv_line(sockfd);
    if (!final_ack) return -1;
    free(final_ack);

    return 0;
}

static int transfer_receive_objects(int sockfd, FsStore* store) {
    while (1) {
        char* line = recv_line(sockfd);
        if (!line) return -1;

        if (strcmp(line, "DONE") == 0) {
            free(line);
            if (send_line(sockfd, "OK\n") != 0) return -1;
            return 0;
        }

        char keyword[16];
        char hash[64];
        int matched = sscanf(line, "%15s %63s", keyword, hash);
        free(line);

        if (matched != 2 || strcmp(keyword, "CHECK") != 0) {
            fprintf(stderr, "transfer_receive_objects: protocol error\n");
            return -1;
        }

        if (fs_store_exists(store, hash)) {
            if (send_line(sockfd, "HAVE\n") != 0) return -1;
            continue;
        }

        if (send_line(sockfd, "WANT\n") != 0) return -1;

        char* data_line = recv_line(sockfd);
        if (!data_line) return -1;

        char data_keyword[16];
        char data_hash[64];
        size_t size;
        int data_matched = sscanf(data_line, "%15s %63s %zu", data_keyword, data_hash, &size);
        free(data_line);

        if (data_matched != 3 || strcmp(data_keyword, "DATA") != 0) {
            fprintf(stderr, "transfer_receive_objects: expected DATA line\n");
            return -1;
        }

        char* buf = malloc(size + 1);
        if (!buf) { fprintf(stderr, "transfer_receive_objects: malloc failed\n"); exit(1); }

        if (recv_all(sockfd, buf, size) != 0) {
            free(buf);
            return -1;
        }
        buf[size] = '\0';

        fs_store_put(store, data_hash, buf, size);
        free(buf);

        if (send_line(sockfd, "OK\n") != 0) return -1;
    }
}

static void handle_client(int client_fd, Repository* repo) {
    char* line = recv_line(client_fd);
    if (!line) { return; }

    char keyword[16];
    char branch[128];
    char hash[64];

    int push_matched = sscanf(line, "%15s %127s %63s", keyword, branch, hash);

    if (push_matched == 3 && strcmp(keyword, "PUSH") == 0) {
        free(line);
        printf("Received push request for branch '%s' (tip %.8s)\n", branch, hash);

        if (transfer_receive_objects(client_fd, repo->store) != 0) {
            fprintf(stderr, "handle_client: push transfer failed\n");
            return;
        }

        char* ref_line = recv_line(client_fd);
        if (ref_line) {
            char ref_keyword[16];
            char ref_branch[128];
            char ref_hash[64];
            if (sscanf(ref_line, "%15s %127s %63s", ref_keyword, ref_branch, ref_hash) == 3 &&
                strcmp(ref_keyword, "UPDATE-REF") == 0) {

                repo_update_branch_ref(repo, ref_branch, ref_hash);
                send_line(client_fd, "OK\n");
                printf("Updated branch '%s' to %.8s\n", ref_branch, ref_hash);
            }
            free(ref_line);
        }
        return;
    }

    char pull_keyword[16];
    char pull_branch[128];
    int pull_matched = sscanf(line, "%15s %127s", pull_keyword, pull_branch);
    free(line);

    if (pull_matched == 2 && strcmp(pull_keyword, "PULL") == 0) {
        printf("Received pull request for branch '%s'\n", pull_branch);

        char* tip_hash = repo_read_branch_ref(repo, pull_branch);

        char head_line[160];
        if (tip_hash) {
            snprintf(head_line, sizeof(head_line), "HEAD %s\n", tip_hash);
        } else {
            snprintf(head_line, sizeof(head_line), "HEAD NONE\n");
        }
        send_line(client_fd, head_line);

        if (tip_hash) {
            transfer_send_objects(client_fd, repo->store, tip_hash);
            free(tip_hash);
        }
        return;
    }

    fprintf(stderr, "handle_client: unrecognised request\n");
}

int network_serve(Repository* repo, int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("network_serve: socket"); return -1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("network_serve: bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("network_serve: listen");
        close(server_fd);
        return -1;
    }

    printf("glite daemon listening on port %d (Ctrl+C to stop)\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) { perror("network_serve: accept"); continue; }

        printf("Connection from %s\n", inet_ntoa(client_addr.sin_addr));
        handle_client(client_fd, repo);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

int network_push(Repository* repo, const char* host, const char* port_str, const char* branch) {
    char* local_hash = repo_read_branch_ref(repo, branch);
    if (!local_hash) {
        fprintf(stderr, "glite push: no commits on branch '%s'\n", branch);
        return -1;
    }

    int sockfd = client_connect(host, port_str);
    if (sockfd < 0) { free(local_hash); return -1; }

    char push_line[300];
    snprintf(push_line, sizeof(push_line), "PUSH %s %s\n", branch, local_hash);
    if (send_line(sockfd, push_line) != 0) {
        fprintf(stderr, "glite push: failed to send request\n");
        close(sockfd); free(local_hash); return -1;
    }

    if (transfer_send_objects(sockfd, repo->store, local_hash) != 0) {
        fprintf(stderr, "glite push: object transfer failed\n");
        close(sockfd); free(local_hash); return -1;
    }

    char update_line[300];
    snprintf(update_line, sizeof(update_line), "UPDATE-REF %s %s\n", branch, local_hash);
    send_line(sockfd, update_line);

    char* ack = recv_line(sockfd);
    if (ack) {
        printf("Push complete: %s -> %.8s\n", branch, local_hash);
        free(ack);
    }

    close(sockfd);
    free(local_hash);
    return 0;
}

int network_pull(Repository* repo, const char* host, const char* port_str, const char* branch) {
    int sockfd = client_connect(host, port_str);
    if (sockfd < 0) return -1;

    char pull_line[200];
    snprintf(pull_line, sizeof(pull_line), "PULL %s\n", branch);
    if (send_line(sockfd, pull_line) != 0) {
        fprintf(stderr, "glite pull: failed to send request\n");
        close(sockfd); return -1;
    }

    char* head_line = recv_line(sockfd);
    if (!head_line) {
        fprintf(stderr, "glite pull: no response from server\n");
        close(sockfd); return -1;
    }

    char keyword[16];
    char hash[64];
    int matched = sscanf(head_line, "%15s %63s", keyword, hash);
    free(head_line);

    if (matched != 2 || strcmp(keyword, "HEAD") != 0) {
        fprintf(stderr, "glite pull: protocol error\n");
        close(sockfd); return -1;
    }

    if (strcmp(hash, "NONE") == 0) {
        printf("glite pull: remote branch '%s' has no commits\n", branch);
        close(sockfd); return 0;
    }

    if (transfer_receive_objects(sockfd, repo->store) != 0) {
        fprintf(stderr, "glite pull: object transfer failed\n");
        close(sockfd); return -1;
    }

    repo_update_branch_ref(repo, branch, hash);
    printf("Pull complete: %s -> %.8s\n", branch, hash);

    close(sockfd);
    return 0;
}