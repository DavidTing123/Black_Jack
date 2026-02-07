#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stddef.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

// Game Constants
#define MAX_PLAYERS 5
#define MAX_CARDS 10
#define DECK_SIZE 52

// Player Score Structure
typedef struct {
    int player_id;
    int wins;
    int losses;
    int highest_score;
} PlayerScore;

// Player State Structure
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

// Global Game State (Shared Memory Structure)
typedef struct {
    PlayerState players[MAX_PLAYERS];
    PlayerScore scores[MAX_PLAYERS];
    int score_count;
    
    int current_turn;
    int active_count;
    int connected_count;
    bool game_active;
    bool game_over;
    int winner;
    int round_number; 
    
    int deck[DECK_SIZE];
    int deck_idx;

    // MEMBER 4: Synchronization primitives
    // These are placed directly in the struct to live in shared memory
    sem_t deck_mutex; 
    sem_t turn_sem;
    sem_t score_sem;
    sem_t log_sem;
} GameState;

// Function Prototypes
void init_game_state_struct(GameState *gs);

#endif