#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include "game_state.h"
#include "shared_mem.h"

// Global pointer for the signal handler to access
GameState *gs = NULL;

// Forward declaration of client handler
void handle_client(int sock, int id, GameState *gs);

void handle_signal(int sig) {
    (void)sig;
    printf("\n[SERVER] Shutting down...\n");
    if (gs != NULL) cleanup_shared_memory(gs);
    exit(0);
}

int main() {
    int server_sock, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    signal(SIGINT, handle_signal);

    // Initialize Shared Memory
    gs = setup_shared_memory();
    if (!gs) exit(1);

    // Initialize game structure once in parent
    init_game_state_struct(gs);
    gs->connected_count = 0; // Ensure counter starts at zero
    gs->game_over = false;

    // Start Scheduler Thread
    pthread_t sched_tid;
    extern void* scheduler_thread_func(void* arg);
    if (pthread_create(&sched_tid, NULL, scheduler_thread_func, (void*)gs) != 0) {
        perror("[ERROR] Failed to create scheduler thread");
    }
    pthread_detach(sched_tid);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);

    if (bind(server_sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    listen(server_sock, 5);
    printf("Blackjack Server ready for PvP on port 8888...\n");

    while (1) {
        new_socket = accept(server_sock, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) continue;

        // MEMBER 4: Locking the count update
        sem_wait(&gs->score_sem);
        int my_id = gs->connected_count;
        if (my_id < MAX_PLAYERS) {
            gs->connected_count++;
            // Initialize player state in parent to avoid race condition with scheduler
            gs->players[my_id].player_id = my_id;
            gs->players[my_id].connected = true;
            gs->players[my_id].active = true;
            gs->players[my_id].last_active = time(NULL);
        }
        sem_post(&gs->score_sem);

        printf("[SERVER] Player %d connected. Total: %d\n", my_id, gs->connected_count);

        if (fork() == 0) { // Child Process
            close(server_sock);
            handle_client(new_socket, my_id, gs);
            exit(0);
        }
        
        close(new_socket);
        
        // Optional: Reset count if you want to start a new game after 2 players
        if (gs->connected_count >= 2) {
             // Logic to handle more players or stop accepting can go here
        }
    }
    return 0;
}