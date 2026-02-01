#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "game_state.h"

// Point calculation
int calculate_points(int cards[], int count);

// Deck management
void init_deck(GameState *gs);
void shuffle_deck(int deck[], int size);
int deal_card(GameState *gs, int player_id);

// Player actions
int player_hit(GameState *gs, int player_id);
void player_stand(GameState *gs, int player_id);

// Game control
void init_game_round(GameState *gs);
int determine_winner(GameState *gs);
void reset_player(PlayerState *player);

// Scheduler
void* scheduler_thread_func(void* arg);

#endif