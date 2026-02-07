#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "game_state.h"

#define SCORES_FILE "scores.txt"

// Shared GameState pointer (defined in server.c)
extern GameState *gs;

// Helper: Write all scores to file
void write_scores_to_file() {
    FILE *f = fopen(SCORES_FILE, "w");
    if (f) {
        for (int i = 0; i < gs->score_count; i++) {
            fprintf(f, "Player %d: Wins=%d, Losses=%d, Highest=%d\n",
                    gs->scores[i].player_id,
                    gs->scores[i].wins,
                    gs->scores[i].losses,
                    gs->scores[i].highest_score);
        }
        fflush(f);
        fclose(f);
        printf("[SCORES] File written: %d players\n", gs->score_count);
    } else {
        printf("[SCORES] ERROR: Could not write to %s\n", SCORES_FILE);
    }
}

// Initialize score system
void init_score_system() {
    if (gs == NULL) return;
    
    sem_wait(&gs->score_sem);
    
    // Try to load existing scores
    FILE *f = fopen(SCORES_FILE, "r");
    if (f) {
        gs->score_count = 0;
        PlayerScore temp;
        while (gs->score_count < MAX_PLAYERS && 
               fscanf(f, "Player %d: Wins=%d, Losses=%d, Highest=%d\n", 
                      &temp.player_id, &temp.wins, &temp.losses, &temp.highest_score) == 4) {
            gs->scores[gs->score_count++] = temp;
        }
        fclose(f);
        printf("[SCORES] Loaded %d existing player scores\n", gs->score_count);
    } else {
        // Initialize fresh scores for players 0-4
        gs->score_count = 5;
        for (int i = 0; i < 5; i++) {
            gs->scores[i].player_id = i;
            gs->scores[i].wins = 0;
            gs->scores[i].losses = 0;
            gs->scores[i].highest_score = 0;
        }
        printf("[SCORES] Initialized fresh scores for 5 players\n");
        write_scores_to_file();
    }
    
    sem_post(&gs->score_sem);
    printf("[SCORES] Score system initialized.\n");
}

void shutdown_score_system() {
    if (gs == NULL) return;
    sem_wait(&gs->score_sem);
    write_scores_to_file();
    sem_post(&gs->score_sem);
    printf("[SCORES] Score system shutting down... scores saved.\n");
}

// Update score for a player (winner)
void update_score(int player_id, int score) {
    if (gs == NULL) return;
    sem_wait(&gs->score_sem);
    
    // Find player entry
    int idx = -1;
    for (int i = 0; i < gs->score_count; i++) {
        if (gs->scores[i].player_id == player_id) {
            idx = i;
            break;
        }
    }
    
    if (idx != -1) {
        gs->scores[idx].wins++;
        if (score > gs->scores[idx].highest_score) {
            gs->scores[idx].highest_score = score;
        }
        printf("[SCORES] Player %d wins! (Total: %d wins)\n", player_id, gs->scores[idx].wins);
        write_scores_to_file();
    }
    
    sem_post(&gs->score_sem);
}

// Record a loss for a player
void record_loss(int player_id) {
    if (gs == NULL) return;
    sem_wait(&gs->score_sem);
    
    int idx = -1;
    for (int i = 0; i < gs->score_count; i++) {
        if (gs->scores[i].player_id == player_id) {
            idx = i;
            break;
        }
    }
    
    if (idx != -1) {
        gs->scores[idx].losses++;
        printf("[SCORES] Player %d loss recorded! (Total: %d losses)\n", player_id, gs->scores[idx].losses);
        write_scores_to_file();
    }
    
    sem_post(&gs->score_sem);
}
