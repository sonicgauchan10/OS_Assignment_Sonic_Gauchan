// ST5004CEM - Task 4: Network Programming and IPC
// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE     1024
#define VALID_USER   "student"
#define VALID_PASS   "cw2026"
#define MAX_LINE     512

typedef struct {
    int  sock;
    struct sockaddr_in addr;
} client_ctx_t;

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

// ---------- Logging ----------

static void server_log(const char *client_ip, int port, const char *msg) {
    pthread_mutex_lock(&log_lock);
    printf("[server] %s:%d - %s\n", client_ip, port, msg);
    fflush(stdout);
    pthread_mutex_unlock(&log_lock);
}

// ---------- Input Validation ----------

static int validate_line(const char *line, int len) {
    if (len <= 0 || len >= MAX_LINE) return 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)line[i];
        if (c < 32 && c != '\r' && c != '\n') return 0;
    }
    return 1;
}

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

// ---------- Client Handler Thread ----------

void *handle_client(void *arg) {
    client_ctx_t *ctx = (client_ctx_t *)arg;
    int sock = ctx->sock;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ctx->addr.sin_addr, ip, sizeof(ip));
    int port = ntohs(ctx->addr.sin_port);

    server_log(ip, port, "connected");

    char buf[BUF_SIZE];
    int authenticated = 0;

    while (1) {
        ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { server_log(ip, port, "disconnected"); break; }
        buf[n] = '\0';

        if (!validate_line(buf, (int)n)) {
            send(sock, "ERR invalid_input\n", 18, 0);
            continue;
        }
        trim_newline(buf);

        if (strncmp(buf, "AUTH ", 5) == 0) {
            char user[128] = {0}, pass[128] = {0};
            sscanf(buf + 5, "%127s %127s", user, pass);
            if (strcmp(user, VALID_USER) == 0 && strcmp(pass, VALID_PASS) == 0) {
                authenticated = 1;
                send(sock, "OK\n", 3, 0);
                server_log(ip, port, "AUTH success");
            } else {
                send(sock, "ERR badauth\n", 12, 0);
                server_log(ip, port, "AUTH failed");
            }
        } else if (strncmp(buf, "MSG ", 4) == 0) {
            if (!authenticated) {
                send(sock, "ERR not_authenticated\n", 22, 0);
                continue;
            }
            int msg_len = (int)strlen(buf + 4);
            char reply[64];
            int rn = snprintf(reply, sizeof(reply), "ACK %d\n", msg_len);
            send(sock, reply, rn, 0);
            server_log(ip, port, "MSG processed");
        } else if (strncmp(buf, "QUIT", 4) == 0) {
            send(sock, "BYE\n", 4, 0);
            break;
        } else {
            send(sock, "ERR unknown_command\n", 20, 0);
        }
    }

    close(sock);
    free(ctx);
    return NULL;
}

// ---------- Main ----------

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : 5555;

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); close(listen_sock); return 1;
    }
    if (listen(listen_sock, 16) < 0) {
        perror("listen"); close(listen_sock); return 1;
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addrlen);
        if (client_sock < 0) { perror("accept"); continue; }

        client_ctx_t *ctx = malloc(sizeof(client_ctx_t));
        ctx->sock = client_sock;
        ctx->addr = client_addr;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, ctx);
        pthread_detach(tid);
    }

    close(listen_sock);
    return 0;
}
