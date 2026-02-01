#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../include/game_state.h"

// ==================== Point Calculation ====================
int calculate_points(int cards[], int count) {
    if (count == 0) return 0;
    
    int points = 0;
    int ace_count = 0;
    
    // First pass: count Aces as 11
    for (int i = 0; i < count; i++) {
        int card_value = cards[i];
        
        if (card_value == 1) {          // Ace
            ace_count++;
            points += 11;
        } else if (card_value >= 10) {  // 10, J, Q, K
            points += 10;
        } else {                        // 2-9
            points += card_value;
        }
    }
    
    // Convert Aces from 11 to 1 if busted
    while (points > 21 && ace_count > 0) {
        points -= 10;
        ace_count--;
    }
    
    return points;
}

// ==================== Deck Management ====================
void shuffle_deck(int deck[], int size) {
    srand(time(NULL) ^ getpid());  // Better random seed
    
    // Fisher-Yates shuffle
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void init_deck(GameState *gs) {
    if (!gs) return;
    
    // Fill deck: 1-13, 4 of each
    for (int i = 0; i < DECK_SIZE; i++) {
        gs->deck[i] = (i % 13) + 1;  // 1=Ace, 2-10, 11=J, 12=Q, 13=K
    }
    
    // Shuffle
    shuffle_deck(gs->deck, DECK_SIZE);
    gs->deck_idx = 0;
    
    printf("[DECK] Deck initialized and shuffled\n");
}

// ==================== Card Dealing ====================
int deal_card(GameState *gs, int player_id) {
    if (!gs || player_id < 1 || player_id > MAX_PLAYERS) {
        return -1;
    }
    
    PlayerState *player = &gs->players[player_id - 1];
    
    // Check if hand is full
    if (player->card_count >= MAX_CARDS) {
        printf("[ERROR] Player %d hand full!\n", player_id);
        return -1;
    }
    
    // Protect deck access
    sem_wait(gs->deck_sem);
    
    // Check if deck needs reshuffling
    if (gs->deck_idx >= DECK_SIZE) {
        printf("[DECK] Reshuffling...\n");
        init_deck(gs);
    }
    
    // Deal card
    int card = gs->deck[gs->deck_idx];
    gs->deck_idx++;
    
    // Update player's hand
    player->cards[player->card_count] = card;
    player->card_count++;
    
    // Recalculate points
    player->points = calculate_points(player->cards, player->card_count);
    
    sem_post(gs->deck_sem);
    
    printf("[DEAL] Player %d got: %d (Cards: %d, Points: %d)\n",
           player_id, card, player->card_count, player->points);
    
    return card;
}

// ==================== Player Actions ====================
int player_hit(GameState *gs, int player_id) {
    int card = deal_card(gs, player_id);
    
    // Check if busted
    PlayerState *player = &gs->players[player_id - 1];
    if (player->points > 21) {
        printf("[GAME] Player %d busted! (%d points)\n", 
               player_id, player->points);
        player->standing = true;  // Busted = auto stand
    }
    
    return card;
}

void player_stand(GameState *gs, int player_id) {
    PlayerState *player = &gs->players[player_id - 1];
    player->standing = true;
    printf("[GAME] Player %d stands with %d points\n", 
           player_id, player->points);
}

// ==================== Game Control ====================
void reset_player(PlayerState *player) {
    if (!player) return;
    
    player->card_count = 0;
    player->points = 0;
    player->standing = false;
    
    // Clear cards
    for (int j = 0; j < MAX_CARDS; j++) {
        player->cards[j] = -1;
    }
}

void init_game_round(GameState *gs) {
    if (!gs) return;
    
    printf("\n[GAME] ===== NEW GAME ROUND =====\n");
    
    sem_wait(gs->turn_sem);
    sem_wait(gs->deck_sem);
    
    // Reset deck
    init_deck(gs);
    
    // Reset active players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].active) {
            reset_player(&gs->players[i]);
        }
    }
    
    // Deal initial cards (2 each)
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].active) {
            deal_card(gs, i + 1);
            deal_card(gs, i + 1);
        }
    }
    
    // Set game state
    gs->game_active = true;
    gs->game_over = false;
    gs->winner = -1;
    
    // Find first active player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].active) {
            gs->current_turn = i;
            break;
        }
    }
    
    sem_post(gs->deck_sem);
    sem_post(gs->turn_sem);
    
    printf("[GAME] Game started! First turn: Player %d\n", 
           gs->current_turn + 1);
    printf("[GAME] =============================\n");
}

int determine_winner(GameState *gs) {
    if (!gs) return -1;
    
    int winner = -1;
    int best_score = -1;
    
    sem_wait(gs->turn_sem);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        PlayerState *p = &gs->players[i];
        
        if (p->active && p->points <= 21) {
            if (p->points > best_score) {
                best_score = p->points;
                winner = p->player_id;
            } else if (p->points == best_score && best_score > 0) {
                winner = -1;  // Tie
            }
        }
    }
    
    sem_post(gs->turn_sem);
    
    // Update game state
    if (winner > 0) {
        gs->winner = winner;
        gs->game_over = true;
        printf("[GAME] 🏆 Winner: Player %d (%d points)\n", winner, best_score);
    } else if (best_score > 0) {
        printf("[GAME] 🤝 Tie! Best: %d points\n", best_score);
    } else {
        printf("[GAME] ❌ All players busted\n");
    }
    
    return winner;
}
