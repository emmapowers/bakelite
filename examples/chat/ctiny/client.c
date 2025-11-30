#include "proto.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int sock_fd = -1;

static int read_byte(void) {
    uint8_t byte;
    ssize_t n = recv(sock_fd, &byte, 1, MSG_DONTWAIT);
    if (n == 1) return byte;
    return -1;
}

static size_t write_bytes(const uint8_t* data, size_t len) {
    return send(sock_fd, data, len, 0);
}

static int has_input(void) {
    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    return poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN);
}

int main(void) {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7032);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }
    printf("Connected to server.\n");

    Protocol proto;
    Protocol_init(&proto, read_byte, write_bytes);
    char my_name[32] = "client";
    char peer_name[32] = "server";

    printf("> ");
    fflush(stdout);

    while (1) {
        Protocol_Message msg = Protocol_poll(&proto);
        if (msg == Protocol_ChatMessage) {
            ChatMessage* chat = Protocol_message_ChatMessage(&proto);
            printf("\r%s > %s\n> ", chat->sender, chat->text);
            fflush(stdout);
        } else if (msg == Protocol_SetName) {
            SetName* setName = Protocol_message_SetName(&proto);
            printf("\r* %s is now known as %s\n> ", peer_name, setName->name);
            fflush(stdout);
            strncpy(peer_name, setName->name, sizeof(peer_name) - 1);
            peer_name[sizeof(peer_name) - 1] = '\0';
        }

        if (has_input()) {
            char line[300];
            if (!fgets(line, sizeof(line), stdin)) break;
            line[strcspn(line, "\n")] = '\0';

            if (strncmp(line, "/name ", 6) == 0) {
                strncpy(my_name, line + 6, sizeof(my_name) - 1);
                my_name[sizeof(my_name) - 1] = '\0';
                SetName* setName = Protocol_message_SetName(&proto);
                strncpy(setName->name, my_name, sizeof(setName->name));
                Protocol_send_zerocopy_SetName(&proto);
            } else if (strlen(line) > 0) {
                ChatMessage* chat = Protocol_message_ChatMessage(&proto);
                strncpy(chat->sender, my_name, sizeof(chat->sender));
                strncpy(chat->text, line, sizeof(chat->text));
                Protocol_send_zerocopy_ChatMessage(&proto);
            }

            printf("> ");
            fflush(stdout);
        }

        usleep(1000);
    }

    close(sock_fd);
    return 0;
}
