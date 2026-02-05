#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "game_state.h"

// Task 2.1: Point Calculation
int calculate_points(const int *cards, int count);

// Task 2.2: Deck Management
void init_deck(GameState *gs);
void shuffle_deck(int *deck, int size);
int deal_card(GameState *gs, int player_id);

// Task 2.3: Player Actions
void player_hit(GameState *gs, int player_id);
void player_stand(GameState *gs, int player_id);

// Task 2.5: Game Flow Control
void init_game_state_struct(GameState *gs);
void init_game_round(GameState *gs);
void determine_winner(GameState *gs);

// Helper for card names
const char* get_card_name(int val);

#endif
