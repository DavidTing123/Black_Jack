#ifndef NETWORK_H
#define NETWORK_H

// These are just declarations (blueprints)
int connect_to_server(const char *ip);
int receive_message(int sock, char *buf, int size);
void send_message(int sock, const char *msg);
void close_connection(int sock);

#endif