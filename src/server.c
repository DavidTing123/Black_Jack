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
#include <pthread.h> // Added for scheduler

#include "../include/game_state.h"
#include "../include/network.h"
#include "../include/game_logic.h" // Added game logic

// External references
extern void* scheduler_thread_func(void* arg); // From scheduler.c

GameState *gs = NULL;
int server_sock = -1;
pthread_t sched_tid;

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
    
    // Check if we can start game (e.g. 3 players)
    if (gs->active_count >= 3 && !gs->game_active) {
         printf("[SERVER] 3 players connected. Starting game!\n");
         // Only one process should trigger this. 
         // Since we are inside semaphore, it's safe to check/set but init_game_round is unsafe 
         // if another process is also checking. But 'active_count' increment is protected.
         // However, init_game_round modifies Deck which needs deck_sem protection ideally, 
         // OR we assume init happens before main loop logic.
         // Let's rely on init_game_round protecting itself or being called here.
         // We should release score_sem before heavy logic? No, to avoid races on start.
         // But init_game_round uses rand() etc.
         // Let's do it here.
         init_game_round(gs);
    }
    sem_post(gs->score_sem);

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Connected to server 127.0.0.1:8888\nWelcome! Player %d\n", player_id);
    send(client_sock, buffer, strlen(buffer), 0);
    usleep(100000);

    // Main Game Loop for Client
    int last_known_turn = -1;
    bool sent_game_over = false;

    while (1) {
        // CHECK FOR GAME OVER
        bool game_over = false;
        int winner = -1;
        
        sem_wait(gs->turn_sem); // Protect read of game state
        if (gs->game_over) {
            game_over = true;
            winner = gs->winner;
        }
        sem_post(gs->turn_sem);

        if (game_over) {
            if (!sent_game_over) {
                 char winner_msg[128];
                 
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
                 sent_game_over = true;
            }
            // Wait for reset or disconnect
             usleep(500000); 
             // If game restarts?
             if (!gs->game_over) sent_game_over = false; // logic to support restart
             continue; // Keep connection open or break? Prompt implies game ends.
        }

        if (!gs->game_active) {
            // Waiting for players
             usleep(500000);
             continue;
        }

        // TURN CHECK
        bool my_turn = false;
        int current_turn_val = -1;

        sem_wait(gs->turn_sem);
        current_turn_val = gs->current_turn;
        if (current_turn_val == player_id && !gs->players[player_id].standing && gs->players[player_id].points <= 21) {
            my_turn = true;
        }
        sem_post(gs->turn_sem);

        if (!my_turn) {
            if (current_turn_val != last_known_turn) {
                // New turn detected, send status update
                char wait_msg[64];
                snprintf(wait_msg, sizeof(wait_msg), "Player %d's Turn. Waiting...", current_turn_val);
                send_game_state(client_sock, player_id, wait_msg);
                last_known_turn = current_turn_val;
            }
            usleep(200000); // 200ms poll
            continue;
        }

        // IT IS MY TURN
        
        // Notify User it IS their turn
        char turn_msg[64];
        snprintf(turn_msg, sizeof(turn_msg), "Player %d's turn! hit or stand?", player_id);
        send_game_state(client_sock, player_id, turn_msg);
        
        // Wait for input
        int bytes = recv(client_sock, buffer, sizeof(buffer)-1, 0);
        if (bytes <= 0) break; // disconnected
        buffer[bytes] = 0;
        buffer[strcspn(buffer, "\r\n")] = 0; // Clean newline

        // Process Action
        if (strcmp(buffer, "hit") == 0) {
            sem_wait(gs->deck_sem);
            player_hit(gs, player_id);
            sem_post(gs->deck_sem);
        } else if (strcmp(buffer, "stand") == 0) {
            sem_wait(gs->turn_sem); // Protecting state change
            player_stand(gs, player_id);
            sem_post(gs->turn_sem);
        }

        // We don't manually pass turn here. The scheduler thread will see we are standing/busted or that we took an action?
        // Actually, if we hit, we might still be active. Round Robin usually implies 1 move per turn or "Hit until Stand"?
        // Prompt says: "Manage player turn order (0->1->2->0...)".
        // In Blackjack, typically you play until you stand/bust.
        // If we want "One card per turn" round robin, we would pass turn after hit.
        // If we want "Play until done", we keep turn.
        // Spec 2.4: "Manage player turn order... Skip inactive...".
        // If the scheduler is running, and we just hit and are still <= 21, do we keep turn?
        // The scheduler loop just checks "current". It doesn't auto-rotate unless we are invalid?
        // Wait, my scheduler code rotates if "need_pass_turn" (inactive/standing/busted). 
        // If I am active and <= 21, scheduler does NOT rotate.
        // So I will stay in my turn loop! 
        // This effectively implements "Play until done" (standard Blackjack). 
        // BUT, if I want to yield turn after HITTING (like some variants or to let others see?), no, standard is play till stand.
        // So I will loop back.
        
        // However, if I busted, `player_hit` called `player_stand` or I should set myself as having finished turn?
        // `player_hit` checks bust and calls `player_stand`.
        // So scheduler will see Busted/Standing and rotate. Perfect.
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
    
    // Initialize basic struct pointers/vals
    init_game_state_struct(gs);

    // 2. Start Scheduler Thread
    if (pthread_create(&sched_tid, NULL, scheduler_thread_func, (void*)gs) != 0) {
        perror("Failed to create scheduler thread");
        handle_signal(0);
    }

    // 3. Setup Network
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
    printf("Waiting for 3 players to start game...\n");

    // 4. Accept Loop
    int client_idx = 0;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int new_socket = accept(server_sock, (struct sockaddr *)&client_addr, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Find a free slot if client_idx is taken? 
        // Simple round robin assignment for now as per Member 1 code
        // But better to find first inactive slot?
        // Preserving Member 1 logic: client_idx = (client_idx + 1) % MAX;
        
        printf("New connection assigned to Player %d\n", client_idx);
        
        pid_t pid = fork();
        if (pid == 0) {
            // Child
            close(server_sock); 
            handle_client(new_socket, client_idx);
            exit(0);
        } else if (pid > 0) {
            // Parent
            close(new_socket); 
            client_idx = (client_idx + 1) % MAX_PLAYERS;
            signal(SIGCHLD, SIG_IGN);
        } else {
            perror("Fork failed");
        }
    }

    return 0;
}
