// ST5004CEM - Task 4: Network Programming and IPC
// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

// ---------- Send/Receive Helper ----------

static int send_and_recv(int sock, const char *msg) {
    if (send(sock, msg, strlen(msg), 0) < 0) {
        perror("send");
        return -1;
    }
    char buf[BUF_SIZE];
    ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        fprintf(stderr, "connection closed by server or recv error\n");
        return -1;
    }
    buf[n] = '\0';
    printf(">> sent: %s   << reply: %s", msg, buf);
    return 0;
}

// ---------- Main ----------

int main(int argc, char *argv[]) {
    const char *ip   = (argc > 1) ? argv[1] : "127.0.0.1";
    int         port = (argc > 2) ? atoi(argv[2]) : 5555;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "invalid address: %s\n", ip);
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }
    printf("Connected to %s:%d\n", ip, port);

    send_and_recv(sock, "AUTH student wrongpass\n");
    send_and_recv(sock, "MSG should_be_rejected\n");
    send_and_recv(sock, "AUTH student cw2026\n");
    send_and_recv(sock, "MSG Hello from the client!\n");
    send_and_recv(sock, "MSG Another message for testing\n");
    send_and_recv(sock, "QUIT\n");

    close(sock);
    return 0;
}
