#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[2048] = {0};
    char input[1024] = {0};

    if (argc < 2) {
        printf("Usage: ./client <IP_ADDRESS>\n");
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) return -1;
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return -1;

    printf("[CLIENT] Connected to server.\n");

    // CONTINUOUS LOOP TO MATCH SERVER
    while (1) {
    memset(buffer, 0, sizeof(buffer));
    int valread = recv(sock, buffer, 2048, 0);
    if (valread <= 0) break;

    printf("%s", buffer);

    // If the server is asking for input (Your action:)
    if (strstr(buffer, "action:") || strstr(buffer, "(y/n):")) {
        fgets(input, 1024, stdin);
        send(sock, input, strlen(input), 0);
    }
}

    close(sock);
    return 0;
}