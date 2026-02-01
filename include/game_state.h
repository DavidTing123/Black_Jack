#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

// 游戏常量
#define MAX_PLAYERS 5
#define MAX_CARDS 10
#define DECK_SIZE 52

// IPC名称（MiniOS中可能需要调整）
#define SHM_NAME "/bj_shm"
#define SEM_TURN "/bj_turn"
#define SEM_DECK "/bj_deck"
#define SEM_SCORE "/bj_score"

// 玩家状态
typedef struct {
    int player_id;
    int cards[MAX_CARDS];
    int card_count;
    int points;
    bool connected;
    bool active;
    bool standing;
    time_t last_active;
} PlayerState;

// 游戏全局状态（共享内存结构）
typedef struct {
    PlayerState players[MAX_PLAYERS];
    int current_turn;
    int active_count;
    int connected_count;
    bool game_active;
    bool game_over;
    int winner;
    int deck[DECK_SIZE];
    int deck_idx;
    sem_t *turn_sem;
    sem_t *deck_sem;
    sem_t *score_sem;
} GameState;

// 函数声明
GameState* init_shared_memory();
void cleanup_shared_memory();
void print_game_state(GameState *gs);

#endif
