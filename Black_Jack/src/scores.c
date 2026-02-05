#include <stdio.h>
#include <stdlib.h>
#include "game_state.h"

void init_score_system() {
    printf("[SYS] Score system initialized.\n");
}

void shutdown_score_system() {
    printf("[SYS] Score system shutting down... scores saved.\n");
}

void update_score(int player_id, int score) {
    FILE *f = fopen("scores.txt", "a");
    if (f) {
        fprintf(f, "Player %d: %d\n", player_id, score);
        fclose(f);
    }
}