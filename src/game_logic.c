// src/game_logic.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/game_state.h"
#include "../include/game_logic.h"

// Task 2.1: Point Calculation
int calculate_points(const int *cards, int count) {
    int points = 0;
    int aces = 0;

    for (int i = 0; i < count; i++) {
        int val = cards[i];
        if (val == 1) { // Ace
            aces++;
            points += 11;
        } else if (val >= 10 && val <= 13) { // J, Q, K
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

// Task 2.2: Deck Management
void shuffle_deck(int *deck, int size) {
    // Fisher-Yates shuffle
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
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

int deal_card(GameState *gs, int player_id) {
    // Check if deck needs reshuffling
    if (gs->deck_idx >= DECK_SIZE) {
        printf("[GAME] Reshuffling deck...\n");
        init_deck(gs);
    }
    
    int card_val = gs->deck[gs->deck_idx++];
    
    // Add to player hand
    PlayerState *p = &gs->players[player_id];
    if (p->card_count < MAX_CARDS) {
        p->cards[p->card_count++] = card_val;
        p->points = calculate_points(p->cards, p->card_count);
    }
    
    return card_val;
}

// Task 2.3: Player Actions
void player_hit(GameState *gs, int player_id) {
    printf("[GAME] Player %d Hits\n", player_id);
    deal_card(gs, player_id);
    
    PlayerState *p = &gs->players[player_id];
    if (p->points > 21) {
        printf("[GAME] Player %d Busted with %d points!\n", player_id, p->points);
        player_stand(gs, player_id); // Auto stand on bust
    }
}

void player_stand(GameState *gs, int player_id) {
    printf("[GAME] Player %d Stands with %d points\n", player_id, gs->players[player_id].points);
    gs->players[player_id].standing = true;
}

// Task 2.5: Game Flow Control
void init_game_round(GameState *gs) {
    printf("[GAME] Initializing new game round...\n");
    
    // Initialize deck
    init_deck(gs);
    
    // Reset players and deal initial cards
    for (int i = 0; i < MAX_PLAYERS; i++) {
        PlayerState *p = &gs->players[i];
        if (p->connected && p->active) {
            p->card_count = 0;
            p->points = 0;
            p->standing = false;
            memset(p->cards, 0, sizeof(p->cards));
            
            // Deal 2 cards
            deal_card(gs, i);
            deal_card(gs, i);
            
            printf("[GAME] Player %d dealt initial hand. Points: %d\n", i, p->points);
        }
    }
    
    gs->game_active = true;
    gs->game_over = false;
    gs->winner = -1;
    
    // Set first active player as current turn
    // Find the first active player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].active && gs->players[i].connected) {
            gs->current_turn = i;
            break;
        }
    }
    printf("[GAME] Game Round Started. First turn: Player %d\n", gs->current_turn);
}

void determine_winner(GameState *gs) {
    int max_score = -1;
    int winner_id = -1;
    bool all_busted = true;
    
    // Find highest score <= 21
    for (int i = 0; i < MAX_PLAYERS; i++) {
        PlayerState *p = &gs->players[i];
        if (p->active && p->connected) {
            if (p->points <= 21) {
                all_busted = false;
                if (p->points > max_score) {
                    max_score = p->points;
                    winner_id = i;
                } else if (p->points == max_score) {
                    // Handle tie - for now, first player keeps it, or return -1?
                    // Spec says: "Handle ties (return -1 for tie)"
                    winner_id = -1; 
                }
            }
        }
    }
    
    if (all_busted) {
        gs->winner = -1; // Everyone lost
    } else {
        gs->winner = winner_id;
    }
    
    gs->game_over = true;
    gs->game_active = false; // Stop the round
    printf("[GAME] Game Ended. Winner: Player %d (Points: %d)\n", gs->winner, max_score);
}

void init_game_state_struct(GameState *gs) {
    gs->current_turn = 0;
    gs->active_count = 0;
    gs->connected_count = 0;
    gs->game_active = false;
    gs->game_over = false;
    gs->winner = -1;
    srand(time(NULL));
    
    // Initialize deck
    init_deck(gs);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        gs->players[i].active = false;
        gs->players[i].connected = false;
        gs->players[i].standing = false;
        gs->players[i].card_count = 0;
        gs->players[i].points = 0;
    }
}

const char* get_card_name(int val) {
    // static char buf[8]; // Removed unused
    if (val == 1) return "A";
    if (val == 11) return "J";
    if (val == 12) return "Q";
    if (val == 13) return "K";
    static char num_bufs[14][4]; // Pre-computed buffers for 1-13
    if (val >= 2 && val <= 10) {
        sprintf(num_bufs[val], "%d", val);
        return num_bufs[val];
    }
    return "?";
}
