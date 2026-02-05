// src/scheduler.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include "../include/game_state.h"
#include "../include/game_logic.h"

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
             usleep(500000); 
             continue; // Wait for server to reset or handle end
        }

        sem_wait(gs->turn_sem);
        
        int current = gs->current_turn;
        PlayerState *p = &gs->players[current];
        
        // Check if current player should lose turn or if game is over
        bool need_pass_turn = false;
        
        // If player disconnected or inactive
        if (!p->active || !p->connected) {
            need_pass_turn = true;
            printf("[SCHEDULER] Player %d inactive/disconnected. Passing turn.\n", current);
        }
        // If player is standing
        else if (p->standing) {
            need_pass_turn = true;
            // printf("[SCHEDULER] Player %d is standing. Passing turn.\n", current); // Verbose
        }
        // If player busted
        else if (p->points > 21) {
             need_pass_turn = true;
             p->standing = true; // Ensure they are marked standing
             printf("[SCHEDULER] Player %d busted. Passing turn.\n", current);
        }
        
        if (need_pass_turn) {
            // Check if ANYONE is valid to play
            bool anyone_can_play = false;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (gs->players[i].active && gs->players[i].connected && !gs->players[i].standing && gs->players[i].points <= 21) {
                    anyone_can_play = true;
                    break;
                }
            }
            
            if (!anyone_can_play) {
                // Game Over
                printf("[SCHEDULER] No active players left. Determining winner.\n");
                determine_winner(gs);
            } else {
                // Find next player
                int loops = 0;
                int next = current;
                do {
                    next = (next + 1) % MAX_PLAYERS;
                    PlayerState *np = &gs->players[next];
                    if (np->active && np->connected && !np->standing && np->points <= 21) {
                        gs->current_turn = next;
                        printf("[SCHEDULER] Turn passed to Player %d\n", next);
                        break;
                    }
                    loops++;
                } while (loops < MAX_PLAYERS);
                
                // If we looped back to current and current is invalid, that means we missed the anyone_can_play check?
                // Or concurrent update. If loops >= MAX, it's game over.
                 if (loops >= MAX_PLAYERS) {
                     printf("[SCHEDULER] All players done (loop check). Determining winner.\n");
                     determine_winner(gs);
                 }
            }
        }
        
        sem_post(gs->turn_sem);
        
        // Sleep specifically to allow clients to act
        usleep(100000); // 100ms slice
    }
    return NULL;
}
