// src/scheduler.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include "game_state.h"

// Forward declarations of functions in game_logic.c
extern void reset_game_round(GameState *gs);
extern void determine_winner(GameState *gs);

#define TURN_DURATION 20 // 20 seconds timeout

void handle_turn_timeout(GameState* gs, int player_id) {
    PlayerState *p = &gs->players[player_id];
    if (p->active && !p->standing) {
        printf("[SCHEDULER] Timeout for Player %d. Forcing STAND.\n", player_id);
        p->standing = true;
    }
}

// Helper to find next player
int find_next_active_player(GameState* gs, int current) {
    int next = current;
    int loops = 0;
    
    do {
        next = (next + 1) % MAX_PLAYERS;
        loops++;
        
        PlayerState *np = &gs->players[next];
        if (np->active && np->connected && !np->standing && np->points <= 21) {
            return next;
        }
    } while (loops < MAX_PLAYERS);
    
    return -1; // No active players found
}

void* scheduler_thread_func(void* arg) {
    GameState *gs = (GameState*)arg;
    printf("[SCHEDULER] Thread started. Waiting for game to begin...\n");

    while (1) {
        // Wait for game to be active
        if (!gs->game_active) {
            usleep(100000); // 100ms
            continue;
        }
        
        if (gs->game_over) {
             printf("[SCHEDULER] Game Over. Waiting for new round...\n");
             usleep(500000);
             continue;
        }

        // Lock to check state (using &gs->turn_sem as per Black_Jack-main struct)
        sem_wait(&gs->turn_sem);
        
        int current = gs->current_turn;
        PlayerState *p = &gs->players[current];
        
        bool need_pass_turn = false;
        
        // 1. Check for Timeout
        time_t now = time(NULL);
        if (p->last_active == 0) {
            p->last_active = now; // Initialize if fresh
        }
        
        if (now - p->last_active > TURN_DURATION) {
            handle_turn_timeout(gs, current);
            need_pass_turn = true;
        }

        // 2. Check status (Busted, Standing, Disconnected)
        if (!p->active || !p->connected) {
            need_pass_turn = true;
            printf("[SCHEDULER] Player %d inactive/disconnected. Passing turn.\n", current);
        }
        else if (p->standing) {
            need_pass_turn = true;
        }
        else if (p->points > 21) {
             need_pass_turn = true;
             p->standing = true;
             printf("[SCHEDULER] Player %d busted. Passing turn.\n", current);
        }
        
        // 3. Pass Turn if needed
        if (need_pass_turn) {
            int next = find_next_active_player(gs, current);
            
            if (next != -1) {
                gs->current_turn = next;
                gs->players[next].last_active = time(NULL); // Reset timer for new player
                printf("[SCHEDULER] Turn passed to Player %d\n", next);
            } else {
                // No one left to play
                 printf("[SCHEDULER] All players done. Determining winner.\n");
                 // Note: determine_winner should be defined or extracted from handle_client in game_logic.c
                 determine_winner(gs);
            }
        }
        
        sem_post(&gs->turn_sem);
        
        usleep(100000); // 100ms slice
    }
    return NULL;
}
