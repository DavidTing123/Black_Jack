#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include "../include/game_state.h"

extern void* scheduler_thread_func(void* arg);

volatile sig_atomic_t server_running = 1;
GameState *game_state = NULL;

void* simple_logger(void* arg) {
    (void)arg;
    printf("[LOGGER] Started\n");
    while (server_running) {
        sleep(3);
        printf("[LOGGER] Alive\n");
    }
    return NULL;
}

void handle_sigint() {
    printf("\nShutting down...\n");
    server_running = 0;
}

int main() {
    printf("=== Test Server ===\n");
    
    signal(SIGINT, (void (*)(int))handle_sigint);
    
    game_state = init_shared_memory();
    if (!game_state) {
        printf("Failed to init shared memory\n");
        return 1;
    }
    
    pthread_t sched, logger;
    pthread_create(&sched, NULL, scheduler_thread_func, (void*)game_state);
    pthread_create(&logger, NULL, simple_logger, NULL);
    
    printf("Threads created. Testing for 10 seconds...\n");
    
    sleep(10);
    
    server_running = 0;
    pthread_join(sched, NULL);
    pthread_join(logger, NULL);
    cleanup_shared_memory();
    
    printf("Test complete\n");
    return 0;
}
