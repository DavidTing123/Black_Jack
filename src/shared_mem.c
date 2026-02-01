#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "../include/game_state.h"

// Initialize shared memory
GameState* init_shared_memory() {
    int shm_fd;
    GameState *game_state = NULL;
    
    printf("[SHM] Initializing shared memory...\n");
    
    // 1. Create/open shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("[ERROR] shm_open failed");
        return NULL;
    }
    
    // 2. Set size
    if (ftruncate(shm_fd, sizeof(GameState)) == -1) {
        perror("[ERROR] ftruncate failed");
        close(shm_fd);
        return NULL;
    }
    
    // 3. Map to process address space
    game_state = (GameState*)mmap(NULL, sizeof(GameState),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED, shm_fd, 0);
    if (game_state == MAP_FAILED) {
        perror("[ERROR] mmap failed");
        close(shm_fd);
        return NULL;
    }
    
    close(shm_fd);  // File descriptor no longer needed
    
    // 4. First-time initialization
    static int first_time = 1;
    if (first_time) {
        memset(game_state, 0, sizeof(GameState));
        
        // Initialize players
        for (int i = 0; i < MAX_PLAYERS; i++) {
            game_state->players[i].player_id = i + 1;
            game_state->players[i].card_count = 0;
            game_state->players[i].points = 0;
            game_state->players[i].connected = false;
            game_state->players[i].active = false;
            game_state->players[i].standing = false;
            game_state->players[i].last_active = time(NULL);
            
            // Initialize cards to -1 (empty)
            for (int j = 0; j < MAX_CARDS; j++) {
                game_state->players[i].cards[j] = -1;
            }
        }
        
        // Initialize deck
        for (int i = 0; i < DECK_SIZE; i++) {
            game_state->deck[i] = (i % 13) + 1;
        }
        game_state->deck_idx = 0;
        
        // Initialize game state
        game_state->current_turn = 0;
        game_state->active_count = 0;
        game_state->connected_count = 0;
        game_state->game_active = false;
        game_state->game_over = false;
        game_state->winner = -1;
        
        first_time = 0;
        printf("[SHM] First-time initialization complete\n");
    }
    
    // 5. Create/open process-shared semaphores
    game_state->turn_sem = sem_open(SEM_TURN, O_CREAT, 0666, 1);
    game_state->deck_sem = sem_open(SEM_DECK, O_CREAT, 0666, 1);
    game_state->score_sem = sem_open(SEM_SCORE, O_CREAT, 0666, 1);
    
    if (game_state->turn_sem == SEM_FAILED ||
        game_state->deck_sem == SEM_FAILED ||
        game_state->score_sem == SEM_FAILED) {
        perror("[ERROR] sem_open failed");
        cleanup_shared_memory();
        return NULL;
    }
    
    printf("[SHM] Ready at %p\n", (void*)game_state);
    return game_state;
}

// Clean up shared memory
void cleanup_shared_memory() {
    printf("[SHM] Cleaning up shared memory...\n");
    
    // Close and unlink semaphores
    sem_unlink(SEM_TURN);
    sem_unlink(SEM_DECK);
    sem_unlink(SEM_SCORE);
    
    // Unlink shared memory object
    if (shm_unlink(SHM_NAME) == -1) {
        if (errno != ENOENT) {
            perror("[WARNING] shm_unlink failed");
        }
    }
    
    printf("[SHM] Cleanup complete\n");
}

// Print game state (debug)
void print_game_state(GameState *gs) {
    if (!gs) {
        printf("[DEBUG] GameState is NULL\n");
        return;
    }
    
    printf("\n=== Game State (PID: %d) ===\n", getpid());
    printf("Turn: Player %d\n", gs->current_turn + 1);
    printf("Active: %d | Connected: %d\n", 
           gs->active_count, gs->connected_count);
    printf("Game active: %s | Game over: %s\n",
           gs->game_active ? "Yes" : "No",
           gs->game_over ? "Yes" : "No");
    
    if (gs->game_over && gs->winner != -1) {
        printf("Winner: Player %d\n", gs->winner);
    }
    
    printf("\nPlayers:\n");
    for (int i = 0; i < MAX_PLAYERS; i++) {
        PlayerState *p = &gs->players[i];
        if (p->connected || p->active) {
            printf("  P%d: Cards=%d Points=%d Active=%s Stand=%s\n",
                   p->player_id, p->card_count, p->points,
                   p->active ? "Y" : "N",
                   p->standing ? "Y" : "N");
            
            if (p->card_count > 0) {
                printf("      Cards: ");
                for (int j = 0; j < p->card_count; j++) {
                    if (p->cards[j] != -1) {
                        printf("%d ", p->cards[j]);
                    }
                }
                printf("\n");
            }
        }
    }
    printf("======================\n\n");
}