// src/client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/network.h"
#include "../include/game_state.h"  // for PlayerState if needed

#define SERVER_IP "127.0.0.1"   // change to real server IP for multi-machine

int main() {
    int sockfd = connect_to_server(SERVER_IP);
    if (sockfd < 0) {
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    int my_player_id = -1;

    // Receive welcome message
    if (receive_message(sockfd, buffer, sizeof(buffer))) {
        printf("%s", buffer);
        // Updated parsing to match "Welcome! Player %d" or similar
        if (sscanf(buffer, "Welcome! Player %d", &my_player_id) != 1) {
            sscanf(buffer, "Welcome! You are Player %d", &my_player_id);
        }
    }

    while (1) {
        // Receive game state or prompt
        if (!receive_message(sockfd, buffer, sizeof(buffer))) {
            break;
        }

        printf("%s\n", buffer);

        // Check for action prompt
        if (strstr(buffer, "hit or stand?") != NULL) {
            printf("Your action (hit / stand): ");
            char command[32];
            fflush(stdout);
            if (fgets(command, sizeof(command), stdin) != NULL) {
                command[strcspn(command, "\n")] = 0;  // remove newline
                send_message(sockfd, command);
            }
        }

        // Game over message
        if (strstr(buffer, "Game Over") != NULL ||
            strstr(buffer, "Winner") != NULL) {
            printf("Game finished.\n");
            break;
        }
    }

    close_connection(sockfd);
    printf("Client exited.\n");
    return 0;
}
