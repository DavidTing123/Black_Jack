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

    // Receive welcome message + player ID
    if (receive_message(sockfd, buffer, sizeof(buffer))) {
        printf("%s", buffer);
        // Assume server sends: "Welcome! You are Player X"
        sscanf(buffer, "Welcome! You are Player %d", &my_player_id);
    }

    while (1) {
        // Receive game state or prompt
        if (!receive_message(sockfd, buffer, sizeof(buffer))) {
            break;
        }

        printf("\n%s\n", buffer);

        // Simple parsing - server should send clear prompts
        if (strstr(buffer, "Your turn") != NULL) {
            printf("Your action (hit / stand): ");
            char command[32];
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0;  // remove newline

            if (strcmp(command, "hit") == 0 || strcmp(command, "stand") == 0) {
                char msg[64];
                snprintf(msg, sizeof(msg), "%s", command);
                send_message(sockfd, msg);
            } else {
                printf("Invalid command. Use 'hit' or 'stand'\n");
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
