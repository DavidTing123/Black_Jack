#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

// Game constants
#define MAX_PLAYERS 5
#define MAX_CARDS 10
#define DECK_SIZE 52

// IPC names (for MiniOS)
#define SHM_NAME "/blackjack_shm"
#define SEM_TURN "/blackjack_turn_sem"
#define SEM_DECK "/blackjack_deck_sem"
#define SEM_SCORE "/blackjack_score_sem"

// Player state
typedef struct {
    int player_id;
    int cards[MAX_CARDS];
    int card_count;
    int points;
    bool connected;
    bool active;
    bool standing;
    time_t last_active;
} PlayerState;

// Global game state (shared memory structure)
typedef struct {
    // Players
    PlayerState players[MAX_PLAYERS];
    int current_turn;
    int active_count;
    int connected_count;
    
    // Game status
    bool game_active;
    bool game_over;
    int winner;
    
    // Deck
    int deck[DECK_SIZE];
    int deck_idx;
    
    // Synchronization (process-shared)
    sem_t *turn_sem;
    sem_t *deck_sem;
    sem_t *score_sem;
} GameState;

// Shared memory functions
GameState* init_shared_memory();
void cleanup_shared_memory();
void print_game_state(GameState *gs);

#endif