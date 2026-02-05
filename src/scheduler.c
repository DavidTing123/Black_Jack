#include "game_state.h"
#include <unistd.h>

void* scheduler_thread_func(void* arg) {
    GameState *gs = (GameState*)arg;
    
    while (1) {
        // 等待游戏开始
        if (!gs->game_active) {
            sleep(1);
            continue;
        }
        
        // 保护当前回合的访问
        sem_wait(gs->turn_sem);
        
        int current = gs->current_turn;
        PlayerState *current_player = &gs->players[current];
        
        // 检查当前玩家是否可以行动
        if (current_player->active && !current_player->standing) {
            printf("[SCHEDULER] Player %d's turn\n", current_player->player_id);
            // 这里可以通知客户端或设置标志
            
            // 给玩家时间行动（比如10秒）
            // 超时处理可以由Member 3实现
        }
        
        // 寻找下一个活跃且未停牌的玩家
        int next_turn = (current + 1) % MAX_PLAYERS;
        int checked = 0;
        
        while (checked < MAX_PLAYERS) {
            PlayerState *next = &gs->players[next_turn];
            
            if (next->active && !next->standing) {
                // 找到下一个可行动的玩家
                break;
            }
            
            next_turn = (next_turn + 1) % MAX_PLAYERS;
            checked++;
        }
        
        // 更新回合
        gs->current_turn = next_turn;
        
        // 检查是否所有玩家都停牌（游戏结束）
        int standing_count = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (gs->players[i].active && gs->players[i].standing) {
                standing_count++;
            }
        }
        
        if (standing_count == gs->active_players && gs->active_players > 0) {
            // 所有活跃玩家都停牌，游戏结束
            gs->game_active = false;
            // 触发胜负判定
        }
        
        sem_post(gs->turn_sem);
        
        // 等待一段时间再检查（模拟回合间隔）
        sleep(2);
    }
    
    return NULL;
}
