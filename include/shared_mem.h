#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "game_state.h"

// Memory management functions
GameState* setup_shared_memory();
void cleanup_shared_memory(GameState *gs);

// This is the declaration that fixes the "implicit declaration" error
void init_game_state_struct(GameState *gs);

#endif