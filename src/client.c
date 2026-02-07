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

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    printf("[CLIENT] Connected to server.\n");

    // CONTINUOUS LOOP TO MATCH SERVER
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int valread = recv(sock, buffer, 2048, 0);
        if (valread <= 0) {
            printf("[CLIENT] Server disconnected or error occurred.\n");
            break;
        }

        printf("%s", buffer);

        // Check what type of input is being asked for
        if (strstr(buffer, "Your action:")) {
            // Game action: hit or stand
            printf("> ");
            fflush(stdout);  // Ensure prompt appears
            
            // Clear input buffer
            memset(input, 0, sizeof(input));
            fgets(input, sizeof(input), stdin);
            
            // Remove newline
            input[strcspn(input, "\n")] = 0;
            
            // Add newline for server
            strcat(input, "\n");
            
            send(sock, input, strlen(input), 0);
        }
        else if (strstr(buffer, "another round?")) {
            // Continue playing prompt
            printf("> ");
            fflush(stdout);  // Ensure prompt appears
            
            // Clear input buffer
            memset(input, 0, sizeof(input));
            fgets(input, sizeof(input), stdin);
            
            // Remove newline
            input[strcspn(input, "\n")] = 0;
            
            // Validate input
            if (strcasecmp(input, "yes") != 0 && strcasecmp(input, "no") != 0) {
                printf("Please enter 'yes' or 'no': ");
                fflush(stdout);
                
                memset(input, 0, sizeof(input));
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = 0;
            }
            
            // Add newline for server
            strcat(input, "\n");
            
            send(sock, input, strlen(input), 0);
            
            // If user said no, prepare to exit
            if (strcasecmp(input, "no\n") == 0) {
                printf("[CLIENT] Ending game session...\n");
            }
        }
        // Handle any other prompts that end with colon
        else if (strchr(buffer, ':') && (strstr(buffer, "MESSAGE:") == NULL)) {
            // Look for the last colon in the buffer
            char *last_colon = strrchr(buffer, ':');
            if (last_colon != NULL) {
                // Check if it's asking for input (colon at end of line)
                char *newline_after_colon = strchr(last_colon, '\n');
                if (newline_after_colon != NULL && (newline_after_colon - last_colon < 10)) {
                    printf("> ");
                    fflush(stdout);
                    
                    memset(input, 0, sizeof(input));
                    fgets(input, sizeof(input), stdin);
                    
                    // Remove newline and add one back for server
                    input[strcspn(input, "\n")] = 0;
                    strcat(input, "\n");
                    
                    send(sock, input, strlen(input), 0);
                }
            }
        }
    }

    close(sock);
    printf("[CLIENT] Connection closed.\n");
    return 0;
}