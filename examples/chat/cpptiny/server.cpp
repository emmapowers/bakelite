#include "proto.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

static int conn_fd = -1;

static int read_byte() {
    uint8_t byte;
    ssize_t n = recv(conn_fd, &byte, 1, MSG_DONTWAIT);
    if (n == 1) return byte;
    return -1;
}

static size_t write_bytes(const char* data, size_t len) {
    return send(conn_fd, data, len, 0);
}

static bool has_input() {
    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    return poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN);
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(7032);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, 1) < 0) {
        perror("listen");
        return 1;
    }

    printf("Listening on port 7032...\n");
    conn_fd = accept(listen_fd, nullptr, nullptr);
    if (conn_fd < 0) {
        perror("accept");
        return 1;
    }
    printf("Client connected.\n");

    Protocol proto(read_byte, write_bytes);
    char my_name[32] = "server";
    char peer_name[32] = "client";

    printf("> ");
    fflush(stdout);

    while (true) {
        auto msg = proto.poll();
        if (msg == Protocol::Message::ChatMessage) {
            auto& chat = proto.message<ChatMessage>();
            printf("\r%s > %s\n> ", chat.sender, chat.text);
            fflush(stdout);
        } else if (msg == Protocol::Message::SetName) {
            auto& setName = proto.message<SetName>();
            printf("\r* %s is now known as %s\n> ", peer_name, setName.name);
            fflush(stdout);
            strncpy(peer_name, setName.name, sizeof(peer_name) - 1);
            peer_name[sizeof(peer_name) - 1] = '\0';
        }

        if (has_input()) {
            char line[300];
            if (!fgets(line, sizeof(line), stdin)) break;
            line[strcspn(line, "\n")] = '\0';

            if (strncmp(line, "/name ", 6) == 0) {
                strncpy(my_name, line + 6, sizeof(my_name) - 1);
                my_name[sizeof(my_name) - 1] = '\0';
                auto& setName = proto.message<SetName>();
                strncpy(setName.name, my_name, sizeof(setName.name));
                proto.send<SetName>();
            } else if (strlen(line) > 0) {
                auto& chat = proto.message<ChatMessage>();
                strncpy(chat.sender, my_name, sizeof(chat.sender));
                strncpy(chat.text, line, sizeof(chat.text));
                proto.send<ChatMessage>();
            }

            printf("> ");
            fflush(stdout);
        }

        usleep(1000);
    }

    close(conn_fd);
    close(listen_fd);
    return 0;
}
