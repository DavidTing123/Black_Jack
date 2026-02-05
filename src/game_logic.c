// src/game_logic.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/game_state.h"

void shuffle_deck(int *deck, int size) {
    for (int i = 0; i < size; i++) {
        int j = rand() % size;
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void init_deck(GameState *gs) {
    // 1 to 13 (A, 2-10, J, Q, K) * 4 suits
    int idx = 0;
    for (int s = 0; s < 4; s++) {
        for (int v = 1; v <= 13; v++) {
            gs->deck[idx++] = v;
        }
    }
    gs->deck_idx = 0;
    shuffle_deck(gs->deck, DECK_SIZE);
}

int draw_card(GameState *gs) {
    if (gs->deck_idx >= DECK_SIZE) {
        // Reshuffle if empty (simple rule)
        printf("Reshuffling deck...\n");
        init_deck(gs);
    }
    return gs->deck[gs->deck_idx++];
}

int calculate_points(const int *cards, int count) {
    int points = 0;
    int aces = 0;

    for (int i = 0; i < count; i++) {
        int val = cards[i];
        if (val == 1) { // Ace
            aces++;
            points += 11;
        } else if (val >= 10) { // J, Q, K
            points += 10;
        } else {
            points += val;
        }
    }

    while (points > 21 && aces > 0) {
        points -= 10;
        aces--;
    }
    return points;
}

void reset_player(PlayerState *p, int id) {
    p->player_id = id;
    p->card_count = 0;
    p->points = 0;
    p->connected = false;
    p->active = false;
    p->standing = false;
    memset(p->cards, 0, sizeof(p->cards));
}

void init_game_state_struct(GameState *gs) {
    gs->current_turn = 0;
    gs->active_count = 0;
    gs->connected_count = 0;
    gs->game_active = false;
    gs->game_over = false;
    gs->winner = -1;
    srand(time(NULL));
    init_deck(gs);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        reset_player(&gs->players[i], i);
    }
}

const char* get_card_name(int val) {
    static char buf[8];
    if (val == 1) return "A";
    if (val == 11) return "J";
    if (val == 12) return "Q";
    if (val == 13) return "K";
    sprintf(buf, "%d", val);
    return buf;
}
