// include/network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

// Function prototypes
int connect_to_server(const char *server_ip);
bool send_message(int sockfd, const char *message);
bool receive_message(int sockfd, char *buffer, size_t bufsize);
void close_connection(int sockfd);

#endif
