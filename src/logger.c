#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stddef.h>
#include "game_state.h"

// Added these to satisfy the Linker
void init_logger() {
    printf("[SYS] Logger system initialized.\n");
}

void shutdown_logger() {
    printf("[SYS] Logger system shutting down... logs flushed.\n");
}

void log_event(const char* type, const char* details) {
    FILE *fp = fopen("game.log", "a");
    if (fp == NULL) return;

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[24] = '\0'; // Remove newline

    fprintf(fp, "[%s] %s: %s\n", timestamp, type, details);
    fclose(fp);
}

void log_player_connect(int id) { (void)id; log_event("CONN", "Player connected"); }
void log_player_disconnect(int id) { (void)id; log_event("DISC", "Player disconnected"); }
void log_card_dealt(int id, int val) { (void)id; (void)val; log_event("DEAL", "Card dealt"); }
void log_player_action(int id, const char* act, int pts) { (void)id; (void)pts; log_event("ACT", act); }
void log_game_start(int count) { (void)count; log_event("START", "Game on"); }
void log_game_end(int winner) { (void)winner; log_event("END", "Game over"); }