#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include "game_state.h"

// 初始化共享内存
GameState* init_shared_memory() {
    int fd;
    GameState *gs;
    
    // 1. 创建共享内存对象
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open");
        return NULL;
    }
    
    // 2. 设置大小
    if (ftruncate(fd, sizeof(GameState)) < 0) {
        perror("ftruncate");
        close(fd);
        return NULL;
    }
    
    // 3. 内存映射
    gs = mmap(NULL, sizeof(GameState), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (gs == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }
    close(fd);
    
    // 4. 首次使用时初始化
    static int first_time = 1;
    if (first_time) {
        memset(gs, 0, sizeof(GameState));
        
        // 初始化玩家
        for (int i = 0; i < MAX_PLAYERS; i++) {
            gs->players[i].player_id = i + 1;
            for (int j = 0; j < MAX_CARDS; j++) {
                gs->players[i].cards[j] = -1;
            }
        }
        
        // 初始化牌堆
        for (int i = 0; i < DECK_SIZE; i++) {
            gs->deck[i] = (i % 13) + 1;
        }
        
        first_time = 0;
        printf("[SHM] First-time initialization complete\n");
    }
    
    // 5. 创建信号量（进程共享）
    gs->turn_sem = sem_open(SEM_TURN, O_CREAT, 0666, 1);
    gs->deck_sem = sem_open(SEM_DECK, O_CREAT, 0666, 1);
    gs->score_sem = sem_open(SEM_SCORE, O_CREAT, 0666, 1);
    
    if (gs->turn_sem == SEM_FAILED || 
        gs->deck_sem == SEM_FAILED || 
        gs->score_sem == SEM_FAILED) {
        perror("sem_open");
        cleanup_shared_memory();
        return NULL;
    }
    
    printf("[SHM] Ready at %p\n", (void*)gs);
    return gs;
}

// 清理共享内存
void cleanup_shared_memory() {
    sem_unlink(SEM_TURN);
    sem_unlink(SEM_DECK);
    sem_unlink(SEM_SCORE);
    shm_unlink(SHM_NAME);
    printf("[SHM] Cleanup done\n");
}

// 调试函数
void print_game_state(GameState *gs) {
    if (!gs) return;
    
    printf("\n=== Game State ===\n");
    printf("Turn: P%d | Active: %d | Connected: %d\n",
           gs->current_turn + 1, gs->active_count, gs->connected_count);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        PlayerState *p = &gs->players[i];
        if (p->connected) {
            printf("P%d: Cards=%d Points=%d %s\n",
                   p->player_id, p->card_count, p->points,
                   p->standing ? "(STAND)" : "");
        }
    }
    printf("==================\n");
}
