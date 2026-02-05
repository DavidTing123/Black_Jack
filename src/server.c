#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "game_state.h"
#include "game_logic.h"

volatile int running = 1;
GameState *game_state = NULL;

// 信号处理
void handle_sigint(int sig) {
    printf("\n[Server] Shutting down...\n");
    running = 0;
}

void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// 线程函数（框架）
void* scheduler_thread(void* arg) {
    printf("[Scheduler] Started (TID: %ld)\n", pthread_self());
    
    while (running) {
        sleep(1);
        
        if (game_state) {
            if (!game_state->game_active && game_state->connected_count >= 3) {
                sem_wait(game_state->turn_sem);
                game_state->game_active = true;
                printf("[Scheduler] Game Started! 3 players connected.\n");
                sem_post(game_state->turn_sem);
            }

            if (game_state->game_active) {
            sem_wait(game_state->turn_sem);
            printf("[Scheduler] Current turn: Player %d\n", 
                   game_state->current_turn + 1);
            // 这里Member 2会实现Round Robin逻辑
            // Round Robin: Find next active player
            int start_turn = game_state->current_turn;
            int next_turn = (start_turn + 1) % MAX_PLAYERS;
            
            // Loop until we find an active player or return to start
            while (next_turn != start_turn) {
                if (game_state->players[next_turn].active && game_state->players[next_turn].connected) {
                    game_state->current_turn = next_turn;
                    break;
                }
                next_turn = (next_turn + 1) % MAX_PLAYERS;
            }
            
            // If current player is no longer active, find someone else even if we did full loop (edge case)
            if (!game_state->players[game_state->current_turn].active || !game_state->players[game_state->current_turn].connected) {
                 for(int i=0; i<MAX_PLAYERS; i++) {
                     if (game_state->players[i].active && game_state->players[i].connected) {
                         game_state->current_turn = i;
                         break;
                     }
                 }
            }
            // 检查是否所有活跃玩家都停牌
            int active_players = 0;
            int standing_players = 0;
            
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (game_state->players[i].active && game_state->players[i].connected) {
                    active_players++;
                    if (game_state->players[i].standing) {
                        standing_players++;
                    }
                }
            }
            
            if (active_players > 0 && active_players == standing_players) {
                printf("[Scheduler] All players standing. Game Over!\n");
                game_state->game_active = false;
            }
            
            sem_post(game_state->turn_sem);
        }
        }
    }
    
    printf("[Scheduler] Exiting\n");
    return NULL;
}

void* logger_thread(void* arg) {
    printf("[Logger] Started (TID: %ld)\n", pthread_self());
    
    int count = 0;
    while (running) {
        sleep(2);
        printf("[Logger] Heartbeat %d\n", ++count);
        // 这里Member 4会实现文件日志
    }
    
    printf("[Logger] Exiting\n");
    return NULL;
}

// 客户端子进程
void client_process(int player_id) {
    printf("[Child %d] Player %d starting\n", getpid(), player_id);
    
    // 标记为已连接
    sem_wait(game_state->turn_sem);
    game_state->players[player_id-1].connected = true;
    game_state->players[player_id-1].active = true;
    game_state->connected_count++;
    sem_post(game_state->turn_sem);
    
    printf("[Child %d] Connected. Total: %d/3 needed\n", 
           getpid(), game_state->connected_count);
    
    // 等待最少3个玩家
    while (game_state->connected_count < 3 && running) {
        sleep(1);
    }
    
    if (running) {
        printf("[Child %d] Game starting! Waiting for scheduler...\n", getpid());
        
        while (!game_state->game_active && running) {
            usleep(100000); // 100ms
        }

        // 游戏逻辑将由Member 2实现
        while (running && game_state->game_active) {
            sem_wait(game_state->turn_sem);
            
            // Check if it's my turn
            if (game_state->current_turn == (player_id - 1)) {
                
                PlayerState *my_state = &game_state->players[player_id-1];
                
                if (my_state->standing) {
                     // Already standing, skip turn
                     sem_post(game_state->turn_sem);
                     usleep(100000); // Wait a bit
                     continue;
                }

                printf("[Child %d] It's my turn (P%d). Points: %d\n", getpid(), player_id, my_state->points);

                // Decision: Hit if points < 17
                if (my_state->points < 17) {
                    printf("[Child %d] Hitting...\n", getpid());
                    
                    sem_wait(game_state->deck_sem);
                    if (game_state->deck_idx < DECK_SIZE) {
                        int card = game_state->deck[game_state->deck_idx++];
                        sem_post(game_state->deck_sem);
                        
                        my_state->cards[my_state->card_count++] = card;
                        my_state->points = calculate_points(my_state->cards, my_state->card_count);
                        
                        printf("[Child %d] Drew card %d. New points: %d\n", getpid(), card, my_state->points);
                        
                        if (my_state->points > 21) {
                            printf("[Child %d] Busted!\n", getpid());
                            my_state->standing = true;
                        }
                    } else {
                        sem_post(game_state->deck_sem);
                        printf("[Child %d] Deck empty! Standing.\n", getpid());
                        my_state->standing = true;
                    }
                } else {
                    printf("[Child %d] Standing at %d.\n", getpid(), my_state->points);
                    my_state->standing = true;
                }
                
                // End turn (Scheduler handles shifting, but we can signal done-ness implicitly by not holding lock)
                // In this implementation, Scheduler runs separately and shifts turns. 
                // We just do our action and release semaphore.
                // NOTE: Real round robin shifting usually happens by scheduler or passing token.
                // The scheduler thread shifts turn every 1 sec. We just ensure we acted if it was our turn.
                
            }
            
            sem_post(game_state->turn_sem);
            usleep(200000); // Check again soon
        }
    }
    
    // 清理退出
    sem_wait(game_state->turn_sem);
    game_state->players[player_id-1].connected = false;
    game_state->connected_count--;
    sem_post(game_state->turn_sem);
    
    printf("[Child %d] Exiting\n", getpid());
    exit(0);
}

int main() {
    printf("=== Blackjack Server (MiniOS) ===\n");
    printf("PID: %d\n", getpid());
    
    // 信号处理
    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld);
    
    // 初始化共享内存
    game_state = init_shared_memory();
    if (!game_state) {
        fprintf(stderr, "Failed to init shared memory\n");
        return 1;
    }
    
    // 创建内部线程
    pthread_t sched_tid, log_tid;
    pthread_create(&sched_tid, NULL, scheduler_thread, NULL);
    pthread_create(&log_tid, NULL, logger_thread, NULL);
    
    printf("[Main] Threads created. Waiting for players...\n");
    
    // Fork子进程处理玩家（模拟3个玩家）
    for (int i = 1; i <= 3 && running; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            continue;
        }
        
        if (pid == 0) {
            // 子进程
            client_process(i);
        } else {
            printf("[Main] Forked player %d (PID: %d)\n", i, pid);
        }
        
        sleep(1); // 间隔启动
    }
    
    // 主循环
    while (running) {
        sleep(1);
        print_game_state(game_state);
    }
    
    // 清理
    printf("[Main] Cleaning up...\n");
    pthread_join(sched_tid, NULL);
    pthread_join(log_tid, NULL);
    cleanup_shared_memory();
    
    printf("[Main] Server stopped\n");
    return 0;
}
