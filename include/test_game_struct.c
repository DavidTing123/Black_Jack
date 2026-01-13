#include <stdio.h>
#include "game_state.h"

int main() {
    printf("GameState size: %lu bytes\n", sizeof(GameState));
    printf("PlayerState size: %lu bytes\n", sizeof(PlayerState));
    printf("MAX_PLAYERS: %d\n", MAX_PLAYERS);
    printf("MAX_CARDS_PER_PLAYER: %d\n", MAX_CARDS_PER_PLAYER);
    
    // 测试是否可以创建局部实例
    GameState local_gs;
    local_gs.current_turn = 0;
    printf("Local game state created, turn: %d\n", local_gs.current_turn);
    
    return 0;
}
