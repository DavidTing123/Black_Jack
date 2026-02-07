#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include "game_state.h"

// Shared GameState pointer (defined in server.c)
extern GameState *gs;

#define LOG_FILE "game.log"

// Initialize logger
void init_logger() {
    FILE *fp = fopen(LOG_FILE, "w");
    if (fp) {
        fprintf(fp, "=== BLACK JACK GAME LOG ===\n");
        fprintf(fp, "Started: %s\n\n", ctime(&(time_t){time(NULL)}));
        fclose(fp);
    }
    printf("[LOGGER] Logger system initialized.\n");
}

void shutdown_logger() {
    if (gs == NULL) return;
    sem_wait(&gs->log_sem);
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        fprintf(fp, "\n=== GAME LOG ENDED ===\n");
        fclose(fp);
    }
    sem_post(&gs->log_sem);
    printf("[LOGGER] Logger system shutting down... logs flushed.\n");
}

// Core logging function - PROCESS SAFE
void log_event(const char* type, const char* details) {
    if (gs == NULL) return;
    sem_wait(&gs->log_sem);
    
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp == NULL) {
        sem_post(&gs->log_sem);
        return;
    }

    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(fp, "[%s] [%s] %s\n", timestamp, type, details);
    fflush(fp);
    fclose(fp);
    
    sem_post(&gs->log_sem);
}

// Logging functions for specific events
void log_player_connect(int id) {
    char buffer[128];
    sprintf(buffer, "Player %d connected", id);
    log_event("SYSTEM", buffer);
}

void log_player_disconnect(int id) {
    char buffer[128];
    sprintf(buffer, "Player %d disconnected", id);
    log_event("SYSTEM", buffer);
}

void log_card_dealt(int id, int val) {
    char buffer[128];
    const char* names[] = {"Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"};
    const char* card_name = (val >= 1 && val <= 13) ? names[val-1] : "Unknown";
    sprintf(buffer, "Player %d drew card: %s (%d)", id, card_name, val);
    log_event("CARD", buffer);
}

void log_player_action(int id, const char* act, int pts) {
    char buffer[128];
    sprintf(buffer, "Player %d action: %s (Total: %d points)", id, act, pts);
    log_event("ACTION", buffer);
}

void log_game_start(int count) {
    char buffer[128];
    sprintf(buffer, "=== ROUND STARTED === (%d players)", count);
    log_event("SYSTEM", buffer);
}

void log_game_end(int winner) {
    char buffer[128];
    sprintf(buffer, "=== ROUND ENDED === Winner: Player %d", winner);
    log_event("SYSTEM", buffer);
}

void log_game_round(int round_num) {
    char buffer[64];
    sprintf(buffer, "ROUND %d START", round_num);
    log_event("ROUND", buffer);
}

void log_player_bust(int id) {
    char buffer[64];
    sprintf(buffer, "Player %d BUST (over 21)", id);
    log_event("BUST", buffer);
}

void log_player_stand(int id, int points) {
    char buffer[64];
    sprintf(buffer, "Player %d STAND with %d points", id, points);
    log_event("STAND", buffer);
}