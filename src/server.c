// src/server.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <semaphore.h>

#include "../include/game_state.h"
#include "../include/network.h"

// External references
extern void init_game_state_struct(GameState *gs);
extern const char* get_card_name(int val);

GameState *gs = NULL;
int server_sock = -1;

void handle_signal(int sig) {
    printf("\nShutting down server...\n");
    if (gs) cleanup_shared_memory(gs);
    if (server_sock != -1) close(server_sock);
    exit(0);
}

// Helper to format and send game state
void send_game_state(int client_sock, int player_id, const char *msg_text) {
    char state_buf[256];
    char cards_str[64] = "";
    
    // Build cards string manually to be safe
    for (int i = 0; i < gs->players[player_id].card_count; i++) {
        const char *n = get_card_name(gs->players[player_id].cards[i]);
        strcat(cards_str, n);
        if (i < gs->players[player_id].card_count - 1) {
            strcat(cards_str, ",");
        }
    }

    snprintf(state_buf, sizeof(state_buf), 
        "STATE: turn=%d player_id=%d cards=%s points=%d standing=%s\n"
        "MESSAGE: %s",
        gs->current_turn,
        player_id,
        cards_str,
        gs->players[player_id].points,
        gs->players[player_id].standing ? "true" : "false",
        msg_text
    );
    send(client_sock, state_buf, strlen(state_buf), 0);
}

void handle_client(int client_sock, int player_id) {
    printf("Player %d handler started (PID %d)\n", player_id, getpid());

    // 1. Initial State
    sem_wait(gs->score_sem);
    gs->players[player_id].connected = true;
    gs->players[player_id].active = true;
    gs->players[player_id].standing = false;
    gs->active_count++;
    sem_post(gs->score_sem);

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Connected to server 127.0.0.1:8888\nWelcome! Player %d\n", player_id);
    send(client_sock, buffer, strlen(buffer), 0);
    // Prevent TCP bundling with the first game state message
    usleep(100000);

    // Initial Deal (2 cards)
    sem_wait(gs->deck_sem);
    int c1 = gs->deck[gs->deck_idx++];
    int c2 = gs->deck[gs->deck_idx++];
    sem_post(gs->deck_sem);

    gs->players[player_id].cards[0] = c1;
    gs->players[player_id].cards[1] = c2;
    gs->players[player_id].card_count = 2;
    // Calculate points
    extern int calculate_points(const int *cards, int count);
    gs->players[player_id].points = calculate_points(gs->players[player_id].cards, 2);

    int last_known_turn = -1;

    while (1) {
        // CHECK FOR GAME OVER
        bool game_over = false;
        sem_wait(gs->turn_sem);
        if (gs->game_over) game_over = true;
        sem_post(gs->turn_sem);

        if (game_over) {
            char winner_msg[128];
            int winner = gs->winner;
            
            if (winner == -1) {
                 snprintf(winner_msg, sizeof(winner_msg), "No Winner (Everyone Busted!)\nPlayer %d finished with %d points.", player_id, gs->players[player_id].points);
            } else {
                int winner_pts = gs->players[winner].points;
                snprintf(winner_msg, sizeof(winner_msg), "Winner is Player %d with %d points\nPlayer %d finished with %d points.", 
                     winner, winner_pts, player_id, gs->players[player_id].points);
            }
            
            char final_buf[256];
            snprintf(final_buf, sizeof(final_buf), "STATE: Game Over\nMESSAGE: %s", winner_msg);
            send(client_sock, final_buf, strlen(final_buf), 0);
            break;
        }

        // TURN CHECK
        bool my_turn = false;
        int current_turn_val = -1;

        sem_wait(gs->turn_sem);
        current_turn_val = gs->current_turn;
        if (current_turn_val == player_id) {
            my_turn = true;
        }
        sem_post(gs->turn_sem);

        if (!my_turn) {
            if (current_turn_val != last_known_turn) {
                // New turn detected, send status update
                char wait_msg[64];
                snprintf(wait_msg, sizeof(wait_msg), "Not Player %d's turn. Waiting...", player_id);
                send_game_state(client_sock, player_id, wait_msg);
                last_known_turn = current_turn_val;
            }
            usleep(200000); // 200ms
            continue;
        }

        // IT IS MY TURN
        // Check if we are already done (bust/stand) but turn came back to us?
        // If the turn passing logic is correct, this shouldn't happen unless everyone is done?
        // But for safety, check status.
        int points = gs->players[player_id].points;
        if (gs->players[player_id].standing || points > 21) {
             // We should not have the turn if we are done. Pass it.
             sem_wait(gs->turn_sem);
             
             // Check if Game Over condition (everyone done)
             // Simple scan
             bool anyone_left = false;
             for(int i=0; i<MAX_PLAYERS; i++) {
                 PlayerState *p = &gs->players[i];
                 if (p->active && p->connected && !p->standing && p->points <= 21) {
                     anyone_left = true;
                     break;
                 }
             }

             if (!anyone_left) {
                 gs->game_over = true;
                 // Determine winner
                 int max_pts = 0;
                 int best_player = -1;
                 for(int i=0; i<MAX_PLAYERS; i++) {
                     PlayerState *p = &gs->players[i];
                     if (p->active && p->points <= 21) {
                         if (p->points > max_pts) {
                             max_pts = p->points;
                             best_player = i;
                         }
                     }
                 }
                 gs->winner = best_player;
                 if (best_player != -1)
                    printf("GAME OVER: Winner is Player %d with %d points.\n", best_player, max_pts);
                 else
                    printf("GAME OVER: No Winner (Everyone Busted).\n");
             } else {
                 // Pass to next eligible
                 int loops = 0;
                 do {
                    gs->current_turn = (gs->current_turn + 1) % MAX_PLAYERS;
                    PlayerState *p = &gs->players[gs->current_turn];
                    if (p->active && p->connected && !p->standing && p->points <= 21) {
                        break;
                    }
                    loops++;
                 } while (loops < MAX_PLAYERS);
             }
             sem_post(gs->turn_sem);
             continue; // Loop back to see game over msg
        }

        // Notify User it IS their turn
        char turn_msg[64];
        snprintf(turn_msg, sizeof(turn_msg), "Player %d's turn! hit or stand?", player_id);
        send_game_state(client_sock, player_id, turn_msg);
        
        // Wait for input
        int bytes = recv(client_sock, buffer, sizeof(buffer)-1, 0);
        if (bytes <= 0) break; // disconnected
        buffer[bytes] = 0;

        // Clean newline
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcmp(buffer, "hit") == 0) {
            printf("[SERVER Player %d] Processing hit...\n", player_id);
            sem_wait(gs->deck_sem);
            extern int draw_card(GameState*);
            int new_card = draw_card(gs);
            sem_post(gs->deck_sem);
            printf("[SERVER Player %d] Drew card: %d\n", player_id, new_card);

            gs->players[player_id].cards[gs->players[player_id].card_count++] = new_card;
            gs->players[player_id].points = calculate_points(gs->players[player_id].cards, gs->players[player_id].card_count);
            printf("[SERVER Player %d] New points: %d (card_count: %d)\n", player_id, gs->players[player_id].points, gs->players[player_id].card_count);
            
        } else if (strcmp(buffer, "stand") == 0) {
            printf("[SERVER Player %d] Processing stand...\n", player_id);
            gs->players[player_id].standing = true;
        }

        // Pass turn logic
        printf("[SERVER Player %d] Passing turn...\n", player_id);
        sem_wait(gs->turn_sem);
        
        if (gs->players[player_id].points > 21) {
             gs->players[player_id].standing = true; 
        }
        
        int prev_turn = gs->current_turn;
        int loops = 0;
        do {
            gs->current_turn = (gs->current_turn + 1) % MAX_PLAYERS;
            PlayerState *p = &gs->players[gs->current_turn];
            if (p->active && p->connected && !p->standing && p->points <= 21) {
                break;
            }
            loops++;
        } while (loops < MAX_PLAYERS);
        
        printf("[SERVER Player %d] Turn passed from %d to %d (loops: %d)\n", player_id, prev_turn, gs->current_turn, loops);

        if (loops >= MAX_PLAYERS) {
             printf("[SERVER] Detected GAME OVER (no eligible players left)\n");
             gs->game_over = true;
             int max_pts = 0;
             int best_player = -1;
             for(int i=0; i<MAX_PLAYERS; i++) {
                 PlayerState *p = &gs->players[i];
                 if (p->active && p->points <= 21) {
                     if (p->points > max_pts) {
                         max_pts = p->points;
                         best_player = i;
                     }
                 }
             }
             gs->winner = best_player;
        }

        sem_post(gs->turn_sem);
        printf("[SERVER Player %d] Turn pass complete. Re-entering loop.\n", player_id);
    }

    // Cleanup
    close(client_sock);
    sem_wait(gs->score_sem);
    gs->players[player_id].connected = false;
    gs->active_count--;
    sem_post(gs->score_sem);
    exit(0);
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // 1. Setup Shared Memory
    gs = init_shared_memory();
    if (!gs) {
        fprintf(stderr, "Failed to init shared memory\n");
        exit(1);
    }
    
    // Initialize Game Logic (shuffle deck, etc)
    init_game_state_struct(gs);

    // 2. Setup Network
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket failed");
        exit(1);
    }
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);

    if (bind(server_sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_sock, MAX_PLAYERS) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Blackjack Server running on port 8888...\n");

    // 3. Accept Loop
    int client_idx = 0;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int new_socket = accept(server_sock, (struct sockaddr *)&client_addr, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New connection: Player %d\n", client_idx);
        
        pid_t pid = fork();
        if (pid == 0) {
            // Child
            close(server_sock); // Child doesn't listen
            handle_client(new_socket, client_idx);
            exit(0);
        } else if (pid > 0) {
            // Parent
            close(new_socket); // Parent doesn't handle client
            client_idx = (client_idx + 1) % MAX_PLAYERS;
            // Wait for zombies? 
            // signal(SIGCHLD, SIG_IGN) is easiest way to avoid zombies
            signal(SIGCHLD, SIG_IGN);
        } else {
            perror("Fork failed");
        }
    }

    return 0;
}
