#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "network.h"

int connect_to_server(const char *ip) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    return sock;
}

int receive_message(int sock, char *buf, int size) {
    int bytes = recv(sock, buf, size - 1, 0);
    if (bytes > 0) buf[bytes] = '\0';
    return bytes;
}

void send_message(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

void close_connection(int sock) {
    close(sock);
}