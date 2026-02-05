// src/network.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/network.h"

int connect_to_server(const char *server_ip) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    printf("Connected to server %s:%d\n", server_ip, SERVER_PORT);
    return sockfd;
}

bool send_message(int sockfd, const char *message) {
    if (send(sockfd, message, strlen(message), 0) < 0) {
        perror("Send failed");
        return false;
    }
    return true;
}

bool receive_message(int sockfd, char *buffer, size_t bufsize) {
    ssize_t bytes = recv(sockfd, buffer, bufsize - 1, 0);
    if (bytes <= 0) {
        if (bytes == 0) printf("Server disconnected\n");
        else perror("Receive failed");
        return false;
    }
    buffer[bytes] = '\0';
    return true;
}

void close_connection(int sockfd) {
    close(sockfd);
}
