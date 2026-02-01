#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/game_state.h"

// ==================== Helper Functions ====================
bool can_player_act(const PlayerState *player) {
    return (player != NULL && 
            player->active && 
            player->connected && 
            !player->standing &&
            player->points <= 21);
}

int find_next_player(GameState *gs, int current_index) {
    if (!gs) return -1;
    
    int checked = 0;
    int next_index = (current_index + 1) % MAX_PLAYERS;
    
    while (checked < MAX_PLAYERS) {
        if (can_player_act(&gs->players[next_index])) {
            return next_index;
        }
        
        next_index = (next_index + 1) % MAX_PLAYERS;
        checked++;
    }
    
    return -1;  // No player can act
}

bool is_game_complete(GameState *gs) {
    if (!gs) return true;
    
    int active_can_act = 0;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].active && can_player_act(&gs->players[i])) {
            active_can_act++;
        }
    }
    
    return (active_can_act == 0);
}

// ==================== Scheduler Thread ====================
void* scheduler_thread_func(void* arg) {
    GameState *gs = (GameState*)arg;
    
    if (!gs) {
        fprintf(stderr, "[SCHEDULER] Invalid game state\n");
        return NULL;
    }
    
    printf("[SCHEDULER] Thread started (TID: %ld)\n", (long)pthread_self());
    
    while (1) {
        // Wait for game to start
        if (!gs->game_active) {
            sleep(1);
            continue;
        }
        
        sem_wait(gs->turn_sem);
        
        // Check if game should end
        if (is_game_complete(gs)) {
            printf("[SCHEDULER] Game complete\n");
            gs->game_active = false;
            sem_post(gs->turn_sem);
            
            // Determine winner
            int winner = -1;
            int best_score = -1;
            
            sem_wait(gs->turn_sem);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (gs->players[i].active && gs->players[i].points <= 21) {
                    if (gs->players[i].points > best_score) {
                        best_score = gs->players[i].points;
                        winner = gs->players[i].player_id;
                    } else if (gs->players[i].points == best_score) {
                        winner = -1;  // Tie
                    }
                }
            }
            sem_post(gs->turn_sem);
            
            if (winner > 0) {
                printf("[SCHEDULER] Winner: Player %d (%d points)\n", winner, best_score);
            }
            
            sleep(2);
            continue;
        }
        
        int current = gs->current_turn;
        PlayerState *current_player = &gs->players[current];
        
        // Announce turn
        if (can_player_act(current_player)) {
            printf("[SCHEDULER] Turn: Player %d (Points: %d)\n",
                   current_player->player_id,
                   current_player->points);
            
            // Simulate waiting for player action
            printf("[SCHEDULER] Waiting for Player %d...\n",
                   current_player->player_id);
        }
        
        // Find next player
        int next_player = find_next_player(gs, current);
        
        if (next_player >= 0) {
            gs->current_turn = next_player;
            printf("[SCHEDULER] Next: Player %d\n", next_player + 1);
        }
        
        sem_post(gs->turn_sem);
        
        // Wait between turns
        sleep(2);
    }
    
    return NULL;
}
