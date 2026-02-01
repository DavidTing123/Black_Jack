#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "game_state.h"

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
        
        if (game_state && game_state->game_active) {
            sem_wait(game_state->turn_sem);
            printf("[Scheduler] Current turn: Player %d\n", 
                   game_state->current_turn + 1);
            // 这里Member 2会实现Round Robin逻辑
            sem_post(game_state->turn_sem);
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
        printf("[Child %d] Game starting!\n", getpid());
        // 游戏逻辑将由Member 2实现
        for (int i = 0; i < 3 && running; i++) {
            sleep(2);
            printf("[Child %d] Round %d\n", getpid(), i+1);
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
